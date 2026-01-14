/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: TcpConnector.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-10
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

// This class should implement a TCP connector for IQRF communication which transfers only raw data and should using boost::asio.

#include "iqrf/connector/tcp/TcpConnector.h"

namespace iqrf::connector::tcp {

TcpConnector::TcpConnector(TcpConfig config): config(std::move(config)), socket(ioContext), resolver(ioContext), timer(ioContext) {
    this->connect();
}

TcpConnector::~TcpConnector() {
    this->stopListen();
    this->disconnect();
}

State TcpConnector::getState() const {
    return this->socket.is_open() ? State::Ready : State::NotReady;
}

std::vector<uint8_t> TcpConnector::receive() {
    if (!this->socket.is_open()) {
        throw std::runtime_error("Socket is not open");
    }

    std::vector<uint8_t> buffer(1024);
    boost::system::error_code ec;
    std::size_t bytesRead = 0;
    bool timeout = false;

    boost::asio::steady_timer deadline(this->ioContext);
    deadline.expires_after(std::chrono::seconds(1));

    deadline.async_wait([&](const boost::system::error_code& error) {
        if (!error) {
            timeout = true;
            this->socket.cancel();
        }
    });

    this->socket.async_read_some(
        boost::asio::buffer(buffer),
        [&](const boost::system::error_code& error, const std::size_t length) {
            ec = error;
            bytesRead = length;
            deadline.cancel();
        }
    );

    this->ioContext.restart();
    this->ioContext.run();

    if (timeout) {
        return {};  // timeout
    }

    if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset) {
        IQRF_LOG(log::Level::Warning) << "Connection closed by the peer, retrying...";
        this->reconnect();
        return {};
    }

    if (ec) {
        IQRF_LOG(log::Level::Error) << "Error receiving data: " << ec.message();
        this->reconnect();
        return {};
    }

    buffer.resize(bytesRead);
    return buffer;
}

void TcpConnector::send(const std::vector<uint8_t> &data) {
    if (this->getState() != State::Ready) {
        throw std::runtime_error("TCP connector is not ready");
    }
    if (data.empty()) {
        throw std::runtime_error("No data to send");
    }
    try {
        boost::asio::write(this->socket, boost::asio::buffer(data), boost::asio::transfer_all());
    } catch (const boost::system::system_error& e) {
        IQRF_LOG(log::Level::Error) << "Failed to send data: " << e.what();
        this->reconnect();
    }
}

void TcpConnector::connect() {
    if (this->connecting) {
        return;
    }
    IQRF_LOG(log::Level::Debug) << "Opening TCP connection to: " << this->config.host << ":" << this->config.port;

    std::chrono::seconds currentTimeout(2);
    const std::chrono::seconds maxTimeout(64);

    while (true) {
        try {
            this->connecting = true;
            auto endpoints = this->resolver.resolve(config.host, std::to_string(config.port));

            boost::system::error_code ec;
            bool connected = false;
            bool timedOut = false;

            this->timer.expires_after(currentTimeout);
            this->timer.async_wait([&](const boost::system::error_code &ec) {
                if (!ec && !connected) {
                    timedOut = true;
                    this->socket.close();
                }
            });

            boost::asio::async_connect(
                this->socket,
                endpoints,
                [&](const boost::system::error_code &error, const boost::asio::ip::tcp::endpoint &) {
                    ec = error;
                    connected = !error;
                    this->timer.cancel();
                }
            );

            this->ioContext.restart();
            this->ioContext.run();

            if (timedOut) {
                throw std::runtime_error("Connection timed out");
            }
            if (ec) {
                throw std::runtime_error("Connection failed: " + ec.message());
            }

            IQRF_LOG(log::Level::Info) << "TCP connection established to: "
                << this->config.host << ":" << this->config.port;
            this->connecting = false;
            return;
        } catch (const std::exception &e) {
            IQRF_LOG(log::Level::Warning) << "Connection attempt failed: " << e.what()
                << ". Retrying in " << currentTimeout.count() << " seconds.";

            boost::asio::steady_timer delay(ioContext);
            delay.expires_after(currentTimeout);
            delay.wait();

            // exponential backoff
            if (currentTimeout < maxTimeout) {
                currentTimeout *= 2;
            } else {
                currentTimeout = maxTimeout;
            }
        }
    }
}

void TcpConnector::disconnect() {
    if (this->socket.is_open()) {
        IQRF_LOG(log::Level::Debug) << "Closing TCP connection to: "
            << this->config.host << ":" << this->config.port;
        boost::system::error_code ec;
        this->socket.close(ec);
        if (ec) {
            IQRF_LOG(log::Level::Error) << "Failed to close TCP socket: " << ec.message();
        } else {
            IQRF_LOG(log::Level::Debug) << "TCP connection closed successfully.";
        }
    }
}

void TcpConnector::reconnect() {
    this->disconnect();
    this->connect();
}

}  // namespace iqrf::connector::tcp
