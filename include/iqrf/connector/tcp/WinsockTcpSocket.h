/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: WinsockTcpSocket.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-11-16
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "iqrf/connector/tcp/ITcpSocket.h"

namespace iqrf::connector::tcp {

/**
 * TCP client socket - Windows Sockets (Winsock 2) implementation
 */
class WinsockTcpSocket : public ITcpSocket {
 public:
    /**
     * Initializes Winsock and connects to the TCP server
     * @param host Host name or IP address of the server
     * @param port Port number of the server
     * @param timeout Maximum time to establish the connection to a single address
     * @throws std::runtime_error if the host cannot be resolved or the connection cannot be established
     */
    WinsockTcpSocket(const std::string &host, uint16_t port, std::chrono::milliseconds timeout);

    /**
     * Closes the connection and releases Winsock
     */
    ~WinsockTcpSocket() override;

    // Disable copying and moving
    WinsockTcpSocket(const WinsockTcpSocket&) = delete;
    WinsockTcpSocket& operator=(const WinsockTcpSocket&) = delete;
    WinsockTcpSocket(WinsockTcpSocket&&) = delete;
    WinsockTcpSocket& operator=(WinsockTcpSocket&&) = delete;

    std::vector<uint8_t> read(std::size_t maxSize, std::chrono::milliseconds timeout) override;
    void write(const std::vector<uint8_t> &data, std::chrono::milliseconds timeout) override;

 private:
    /// Socket handle (SOCKET), stored as an integer to keep Winsock headers out of the public header
    std::uintptr_t handle;
};

}  // namespace iqrf::connector::tcp
