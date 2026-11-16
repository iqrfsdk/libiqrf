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

#include "iqrf/connector/tcp/TcpConnector.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/tcp/ITcpSocket.h"
#include "iqrf/connector/tcp/TcpConfig.h"
#include "iqrf/log/Logging.h"

namespace iqrf::connector::tcp {

namespace log = ::iqrf::log;

using Clock = std::chrono::steady_clock;

TcpConnector::TcpConnector(TcpConfig config, SocketFactory socketFactory):
    config(std::move(config)),
    socketFactory(std::move(socketFactory)),
    nextConnectAttempt(Clock::now()),
    reconnectDelay(this->config.reconnectDelay) {
    if (!this->socketFactory) {
        this->socketFactory = [](const TcpConfig &cfg) {
            return createTcpSocket(cfg.host, cfg.port, cfg.connectTimeout);
        };
    }
    this->ensureConnected();
}

TcpConnector::~TcpConnector() {
    this->stopListen();
}

State TcpConnector::getState() const {
    return this->connected ? State::Ready : State::NotReady;
}

void TcpConnector::send(const std::vector<uint8_t> &data) {
    if (data.empty()) {
        throw std::runtime_error("No data to send");
    }
    if (!this->ensureConnected()) {
        throw std::runtime_error("TCP connector is not connected to " + this->config.host + ":"
            + std::to_string(this->config.port));
    }
    std::shared_lock lock(this->socketMutex);
    if (!this->socket) {
        throw std::runtime_error("TCP connection has been lost");
    }
    const uint64_t generation = this->connectionGeneration;
    try {
        this->socket->write(data, WRITE_TIMEOUT);
    } catch (const std::exception &e) {
        lock.unlock();
        this->disconnect(generation, e.what());
        throw std::runtime_error(std::string("Failed to send data: ") + e.what());
    }
}

std::vector<uint8_t> TcpConnector::receive() {
    if (!this->ensureConnected()) {
        std::this_thread::sleep_for(RECEIVE_TIMEOUT);
        return {};
    }
    std::shared_lock lock(this->socketMutex);
    if (!this->socket) {
        return {};
    }
    const uint64_t generation = this->connectionGeneration;
    try {
        return this->socket->read(READ_SIZE, RECEIVE_TIMEOUT);
    } catch (const std::exception &e) {
        lock.unlock();
        this->disconnect(generation, e.what());
        return {};
    }
}

bool TcpConnector::ensureConnected() {
    std::unique_lock lock(this->socketMutex);
    if (this->socket) {
        return true;
    }
    if (Clock::now() < this->nextConnectAttempt) {
        return false;
    }
    IQRF_LOG(log::Level::Debug) << "Opening TCP connection to: " << this->config.host << ":" << this->config.port;
    try {
        this->socket = this->socketFactory(this->config);
    } catch (const std::exception &e) {
        IQRF_LOG(log::Level::Warning) << "Connection attempt failed: " << e.what() << ". Retrying in "
            << this->reconnectDelay.count() << " ms.";
        this->nextConnectAttempt = Clock::now() + this->reconnectDelay;
        this->reconnectDelay = std::min(this->reconnectDelay * 2, this->config.maxReconnectDelay);
        return false;
    }
    if (!this->socket) {
        throw std::logic_error("TCP socket factory returned null socket");
    }
    IQRF_LOG(log::Level::Info) << "TCP connection established to: " << this->config.host << ":" << this->config.port;
    this->reconnectDelay = this->config.reconnectDelay;
    ++this->connectionGeneration;
    this->connected = true;
    return true;
}

void TcpConnector::disconnect(const uint64_t generation, const char *reason) {
    std::unique_lock lock(this->socketMutex);
    if (!this->socket || this->connectionGeneration != generation) {
        // Already handled by another thread
        return;
    }
    IQRF_LOG(log::Level::Warning) << "TCP connection to " << this->config.host << ":" << this->config.port
        << " lost: " << reason << ". Reconnecting in " << this->reconnectDelay.count() << " ms.";
    this->socket.reset();
    this->connected = false;
    this->nextConnectAttempt = Clock::now() + this->reconnectDelay;
}

}  // namespace iqrf::connector::tcp
