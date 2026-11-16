/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: PosixTcpSocket.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-11-16
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "iqrf/connector/tcp/PosixTcpSocket.h"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
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

namespace {

using Clock = std::chrono::steady_clock;

#ifdef MSG_NOSIGNAL
/// Flags of send(), do not raise SIGPIPE if the peer has closed the connection
constexpr int SEND_FLAGS = MSG_NOSIGNAL;
#else
/// Flags of send(), SIGPIPE is suppressed by the SO_NOSIGPIPE socket option
constexpr int SEND_FLAGS = 0;
#endif

/**
 * Creates the system error from errno
 * @param message Description of the failed operation
 */
std::system_error systemError(const std::string &message) {
    return {errno, std::generic_category(), message};
}

/**
 * Closes the file descriptor, retries are not allowed by POSIX on EINTR
 */
void closeFd(const int fd) noexcept {
    if (fd >= 0) {
        ::close(fd);
    }
}

/**
 * Waits for the events on the socket
 * @param fd Socket file descriptor
 * @param events Events to wait for (POLLIN, POLLOUT)
 * @param deadline Time point to stop waiting
 * @return true if the socket is ready (or an error is pending on it), false on timeout
 * @throws std::system_error on poll failure
 */
bool waitFor(const int fd, const short events, const Clock::time_point deadline) {
    while (true) {
        const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline - Clock::now());
        pollfd pfd{fd, events, 0};
        const int result = ::poll(&pfd, 1, static_cast<int>(std::max<std::chrono::milliseconds::rep>(remaining.count(), 0)));
        if (result > 0) {
            return true;
        }
        if (result == 0) {
            return false;
        }
        if (errno != EINTR) {
            throw systemError("Failed to poll TCP socket");
        }
    }
}

/**
 * Configures the socket: non-blocking mode, close-on-exec, no SIGPIPE, no Nagle's algorithm
 * @param fd Socket file descriptor
 * @throws std::system_error on failure
 */
void configureSocket(const int fd) {
    const int fdFlags = ::fcntl(fd, F_GETFD);
    if (fdFlags == -1 || ::fcntl(fd, F_SETFD, fdFlags | FD_CLOEXEC) == -1) {
        throw systemError("Failed to set close-on-exec flag");
    }
    const int statusFlags = ::fcntl(fd, F_GETFL);
    if (statusFlags == -1 || ::fcntl(fd, F_SETFL, statusFlags | O_NONBLOCK) == -1) {
        throw systemError("Failed to set non-blocking mode");
    }
#ifdef SO_NOSIGPIPE
    const int noSigPipe = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &noSigPipe, sizeof(noSigPipe)) == -1) {
        throw systemError("Failed to disable SIGPIPE");
    }
#endif
    // DPA messages are small and latency sensitive
    const int noDelay = 1;
    if (::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &noDelay, sizeof(noDelay)) == -1) {
        throw systemError("Failed to disable Nagle's algorithm");
    }
}

/**
 * Connects the socket to the address within the timeout
 * @param address Resolved address
 * @param timeout Maximum time to establish the connection
 * @return Connected socket file descriptor
 * @throws std::system_error on failure or timeout
 */
int connectTo(const addrinfo &address, const std::chrono::milliseconds timeout) {
    const int fd = ::socket(address.ai_family, address.ai_socktype, address.ai_protocol);
    if (fd == -1) {
        throw systemError("Failed to create TCP socket");
    }
    try {
        configureSocket(fd);
        if (::connect(fd, address.ai_addr, address.ai_addrlen) == -1) {
            if (errno != EINPROGRESS && errno != EINTR) {
                throw systemError("Failed to connect");
            }
            if (!waitFor(fd, POLLOUT, Clock::now() + timeout)) {
                throw std::system_error(std::make_error_code(std::errc::timed_out), "Failed to connect");
            }
            int error = 0;
            socklen_t length = sizeof(error);
            if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &length) == -1) {
                throw systemError("Failed to get connection result");
            }
            if (error != 0) {
                throw std::system_error(error, std::generic_category(), "Failed to connect");
            }
        }
        return fd;
    } catch (...) {
        closeFd(fd);
        throw;
    }
}

}  // namespace

PosixTcpSocket::PosixTcpSocket(const std::string &host, const uint16_t port, const std::chrono::milliseconds timeout) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_NUMERICSERV;
    addrinfo *result = nullptr;
    const int gaiError = ::getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);
    if (gaiError != 0) {
        const std::string reason = gaiError == EAI_SYSTEM ? std::generic_category().message(errno) : ::gai_strerror(gaiError);
        throw std::runtime_error("Cannot resolve TCP host " + host + ": " + reason);
    }
    const std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)> addresses(result, &::freeaddrinfo);

    std::string lastError = "no address found";
    for (const addrinfo *address = addresses.get(); address != nullptr; address = address->ai_next) {
        try {
            this->fd = connectTo(*address, timeout);
            return;
        } catch (const std::exception &e) {
            lastError = e.what();
            IQRF_LOG(log::Level::Debug) << "Cannot connect to an address of " << host << ":" << port << ": " << lastError;
        }
    }
    throw std::runtime_error("Cannot connect to " + host + ":" + std::to_string(port) + ": " + lastError);
}

PosixTcpSocket::~PosixTcpSocket() {
    closeFd(this->fd);
}

std::vector<uint8_t> PosixTcpSocket::read(const std::size_t maxSize, const std::chrono::milliseconds timeout) {
    if (!waitFor(this->fd, POLLIN, Clock::now() + timeout)) {
        return {};
    }
    std::vector<uint8_t> buffer(maxSize);
    while (true) {
        const ssize_t length = ::recv(this->fd, buffer.data(), buffer.size(), 0);
        if (length > 0) {
            buffer.resize(static_cast<std::size_t>(length));
            return buffer;
        }
        if (length == 0) {
            throw std::runtime_error("TCP connection has been closed by the peer");
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return {};
        }
        if (errno != EINTR) {
            throw systemError("Failed to read from TCP socket");
        }
    }
}

void PosixTcpSocket::write(const std::vector<uint8_t> &data, const std::chrono::milliseconds timeout) {
    const auto deadline = Clock::now() + timeout;
    std::size_t written = 0;
    while (written < data.size()) {
        const ssize_t length = ::send(this->fd, data.data() + written, data.size() - written, SEND_FLAGS);
        if (length >= 0) {
            written += static_cast<std::size_t>(length);
            continue;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            throw systemError("Failed to write to TCP socket");
        }
        if (!waitFor(this->fd, POLLOUT, deadline)) {
            throw std::system_error(std::make_error_code(std::errc::timed_out), "Failed to write to TCP socket");
        }
    }
}

}  // namespace iqrf::connector::tcp
