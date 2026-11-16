/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: WinsockTcpSocket.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-11-16
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "iqrf/connector/tcp/WinsockTcpSocket.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include "iqrf/log/Logging.h"

namespace iqrf::connector::tcp {

namespace log = ::iqrf::log;

static_assert(sizeof(SOCKET) <= sizeof(std::uintptr_t), "SOCKET does not fit into std::uintptr_t");

namespace {

using Clock = std::chrono::steady_clock;

/**
 * Creates the system error from the last Winsock error
 * @param message Description of the failed operation
 */
std::system_error winsockError(const std::string &message) {
    return {::WSAGetLastError(), std::system_category(), message};
}

/**
 * Closes the socket
 */
void closeSocket(const SOCKET socket) noexcept {
    if (socket != INVALID_SOCKET) {
        ::closesocket(socket);
    }
}

/**
 * Waits for the socket to become readable or writable
 *
 * select() is used instead of WSAPoll(), which does not report failed connection attempts on older Windows versions.
 *
 * @param socket Socket handle
 * @param write Wait for writability (and connection result) instead of readability
 * @param deadline Time point to stop waiting
 * @return true if the socket is ready (or an error is pending on it), false on timeout
 * @throws std::system_error on select failure
 */
bool waitFor(const SOCKET socket, const bool write, const Clock::time_point deadline) {
    const auto remaining = std::max(
        std::chrono::ceil<std::chrono::microseconds>(deadline - Clock::now()),
        std::chrono::microseconds::zero()
    );
    timeval tv{};
    tv.tv_sec = static_cast<long>(remaining.count() / 1000000);
    tv.tv_usec = static_cast<long>(remaining.count() % 1000000);
    fd_set readSet;
    fd_set writeSet;
    fd_set errorSet;
    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);
    FD_ZERO(&errorSet);
    FD_SET(socket, write ? &writeSet : &readSet);
    // Failed connection attempts are reported in the except set
    FD_SET(socket, &errorSet);
    // The first parameter is ignored by Winsock
    const int result = ::select(0, &readSet, &writeSet, &errorSet, &tv);
    if (result == SOCKET_ERROR) {
        throw winsockError("Failed to wait for TCP socket");
    }
    return result > 0;
}

/**
 * Configures the socket: non-blocking mode, no Nagle's algorithm
 * @param socket Socket handle
 * @throws std::system_error on failure
 */
void configureSocket(const SOCKET socket) {
    u_long nonBlocking = 1;
    if (::ioctlsocket(socket, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        throw winsockError("Failed to set non-blocking mode");
    }
    // DPA messages are small and latency sensitive
    const BOOL noDelay = TRUE;
    if (::setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&noDelay), sizeof(noDelay))
        == SOCKET_ERROR) {
        throw winsockError("Failed to disable Nagle's algorithm");
    }
}

/**
 * Connects the socket to the address within the timeout
 * @param address Resolved address
 * @param timeout Maximum time to establish the connection
 * @return Connected socket handle
 * @throws std::system_error on failure or timeout
 */
SOCKET connectTo(const addrinfo &address, const std::chrono::milliseconds timeout) {
    const SOCKET socket = ::WSASocketW(
        address.ai_family,
        address.ai_socktype,
        address.ai_protocol,
        nullptr,
        0,
        WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT
    );
    if (socket == INVALID_SOCKET) {
        throw winsockError("Failed to create TCP socket");
    }
    try {
        configureSocket(socket);
        if (::connect(socket, address.ai_addr, static_cast<int>(address.ai_addrlen)) == SOCKET_ERROR) {
            if (::WSAGetLastError() != WSAEWOULDBLOCK) {
                throw winsockError("Failed to connect");
            }
            if (!waitFor(socket, true, Clock::now() + timeout)) {
                throw std::system_error(WSAETIMEDOUT, std::system_category(), "Failed to connect");
            }
            int error = 0;
            int length = sizeof(error);
            if (::getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&error), &length)
                == SOCKET_ERROR) {
                throw winsockError("Failed to get connection result");
            }
            if (error != 0) {
                throw std::system_error(error, std::system_category(), "Failed to connect");
            }
        }
        return socket;
    } catch (...) {
        closeSocket(socket);
        throw;
    }
}

/**
 * Connects to the first reachable address of the host
 * @return Connected socket handle
 * @throws std::runtime_error on failure
 */
SOCKET connectToHost(const std::string &host, const uint16_t port, const std::chrono::milliseconds timeout) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_NUMERICSERV;
    addrinfo *result = nullptr;
    // getaddrinfo() returns the Winsock error code, gai_strerror() is not thread-safe on Windows
    const int gaiError = ::getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);
    if (gaiError != 0) {
        throw std::runtime_error("Cannot resolve TCP host " + host + ": " + std::system_category().message(gaiError));
    }
    const std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)> addresses(result, &::freeaddrinfo);

    std::string lastError = "no address found";
    for (const addrinfo *address = addresses.get(); address != nullptr; address = address->ai_next) {
        try {
            return connectTo(*address, timeout);
        } catch (const std::exception &e) {
            lastError = e.what();
            IQRF_LOG(log::Level::Debug) << "Cannot connect to an address of " << host << ":" << port << ": " << lastError;
        }
    }
    throw std::runtime_error("Cannot connect to " + host + ":" + std::to_string(port) + ": " + lastError);
}

}  // namespace

WinsockTcpSocket::WinsockTcpSocket(const std::string &host, const uint16_t port, const std::chrono::milliseconds timeout):
    handle(static_cast<std::uintptr_t>(INVALID_SOCKET)) {
    // WSAStartup() and WSACleanup() are reference counted, each socket holds its own reference
    WSADATA wsaData;
    const int startupError = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (startupError != 0) {
        throw std::system_error(startupError, std::system_category(), "Failed to initialize Winsock");
    }
    try {
        this->handle = static_cast<std::uintptr_t>(connectToHost(host, port, timeout));
    } catch (...) {
        // Destructor is not called if the constructor throws, so release Winsock here
        ::WSACleanup();
        throw;
    }
}

WinsockTcpSocket::~WinsockTcpSocket() {
    closeSocket(static_cast<SOCKET>(this->handle));
    ::WSACleanup();
}

std::vector<uint8_t> WinsockTcpSocket::read(const std::size_t maxSize, const std::chrono::milliseconds timeout) {
    const auto socket = static_cast<SOCKET>(this->handle);
    if (!waitFor(socket, false, Clock::now() + timeout)) {
        return {};
    }
    std::vector<uint8_t> buffer(std::min<std::size_t>(maxSize, INT_MAX));
    const int length = ::recv(socket, reinterpret_cast<char *>(buffer.data()), static_cast<int>(buffer.size()), 0);
    if (length > 0) {
        buffer.resize(static_cast<std::size_t>(length));
        return buffer;
    }
    if (length == 0) {
        throw std::runtime_error("TCP connection has been closed by the peer");
    }
    if (::WSAGetLastError() == WSAEWOULDBLOCK) {
        return {};
    }
    throw winsockError("Failed to read from TCP socket");
}

void WinsockTcpSocket::write(const std::vector<uint8_t> &data, const std::chrono::milliseconds timeout) {
    const auto socket = static_cast<SOCKET>(this->handle);
    const auto deadline = Clock::now() + timeout;
    std::size_t written = 0;
    while (written < data.size()) {
        const int chunk = static_cast<int>(std::min<std::size_t>(data.size() - written, INT_MAX));
        const int length = ::send(socket, reinterpret_cast<const char *>(data.data() + written), chunk, 0);
        if (length != SOCKET_ERROR) {
            written += static_cast<std::size_t>(length);
            continue;
        }
        if (::WSAGetLastError() != WSAEWOULDBLOCK) {
            throw winsockError("Failed to write to TCP socket");
        }
        if (!waitFor(socket, true, deadline)) {
            throw std::system_error(WSAETIMEDOUT, std::system_category(), "Failed to write to TCP socket");
        }
    }
}

}  // namespace iqrf::connector::tcp
