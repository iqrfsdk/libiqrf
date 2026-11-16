/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: TcpSocketTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-11-16
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#if defined(__unix__) || defined(__APPLE__)

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#include "iqrf/connector/tcp/ITcpSocket.h"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace iqrf::connector::tcp {

namespace {

using namespace std::chrono_literals;

/**
 * Listening TCP server on the loopback interface (IPv4) with an ephemeral port
 */
class LoopbackServer {
 public:
    LoopbackServer() {
        this->listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (this->listenFd == -1) {
            throw std::runtime_error("Cannot create server socket");
        }
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        socklen_t length = sizeof(address);
        if (::bind(this->listenFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1
            || ::listen(this->listenFd, 1) == -1
            || ::getsockname(this->listenFd, reinterpret_cast<sockaddr *>(&address), &length) == -1) {
            ::close(this->listenFd);
            throw std::runtime_error("Cannot start server");
        }
        this->port = ntohs(address.sin_port);
    }

    ~LoopbackServer() {
        this->closeClient();
        ::close(this->listenFd);
    }

    LoopbackServer(const LoopbackServer&) = delete;
    LoopbackServer& operator=(const LoopbackServer&) = delete;

    void accept() {
        this->clientFd = ::accept(this->listenFd, nullptr, nullptr);
        if (this->clientFd == -1) {
            throw std::runtime_error("Cannot accept client");
        }
    }

    void send(const std::vector<uint8_t> &data) const {
        ASSERT_EQ(::send(this->clientFd, data.data(), data.size(), 0), static_cast<ssize_t>(data.size()));
    }

    std::vector<uint8_t> receive() const {
        std::vector<uint8_t> buffer(64);
        const ssize_t length = ::recv(this->clientFd, buffer.data(), buffer.size(), 0);
        buffer.resize(length > 0 ? static_cast<std::size_t>(length) : 0);
        return buffer;
    }

    void closeClient() {
        if (this->clientFd != -1) {
            ::close(this->clientFd);
            this->clientFd = -1;
        }
    }

    uint16_t port = 0;

 private:
    int listenFd = -1;
    int clientFd = -1;
};

}  // namespace

TEST(TcpSocketTest, exchangesDataWithServer) {
    LoopbackServer server;
    const auto socket = createTcpSocket("127.0.0.1", server.port, 1s);
    server.accept();

    socket->write({0x01, 0x02, 0x03}, 1s);
    EXPECT_THAT(server.receive(), ElementsAre(0x01, 0x02, 0x03));

    server.send({0x04, 0x05});
    EXPECT_THAT(socket->read(1024, 1s), ElementsAre(0x04, 0x05));
}

TEST(TcpSocketTest, readTimesOutWithoutData) {
    LoopbackServer server;
    const auto socket = createTcpSocket("127.0.0.1", server.port, 1s);
    server.accept();

    EXPECT_THAT(socket->read(1024, 10ms), IsEmpty());
}

TEST(TcpSocketTest, readFailsIfPeerClosesConnection) {
    LoopbackServer server;
    const auto socket = createTcpSocket("127.0.0.1", server.port, 1s);
    server.accept();
    server.closeClient();

    EXPECT_THROW(socket->read(1024, 1s), std::runtime_error);
}

TEST(TcpSocketTest, writeFailsIfPeerClosesConnection) {
    LoopbackServer server;
    const auto socket = createTcpSocket("127.0.0.1", server.port, 1s);
    server.accept();
    server.closeClient();

    // The first write may succeed before the reset from the peer arrives, SIGPIPE must not be raised
    EXPECT_THROW({
        for (int i = 0; i < 100; ++i) {
            socket->write({0x01}, 1s);
            std::this_thread::sleep_for(1ms);
        }
    }, std::runtime_error);
}

TEST(TcpSocketTest, connectFailsIfServerIsNotListening) {
    uint16_t port = 0;
    {
        LoopbackServer server;
        port = server.port;
    }

    EXPECT_THROW(createTcpSocket("127.0.0.1", port, 1s), std::runtime_error);
}

TEST(TcpSocketTest, connectFailsForUnresolvableHost) {
    EXPECT_THROW(createTcpSocket("nonexistent.invalid", 10000, 1s), std::runtime_error);
}

}  // namespace iqrf::connector::tcp

#endif
