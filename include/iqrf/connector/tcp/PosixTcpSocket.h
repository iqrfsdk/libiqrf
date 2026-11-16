/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: PosixTcpSocket.h
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
 * TCP client socket - POSIX sockets implementation
 */
class PosixTcpSocket : public ITcpSocket {
 public:
    /**
     * Connects to the TCP server
     * @param host Host name or IP address of the server
     * @param port Port number of the server
     * @param timeout Maximum time to establish the connection to a single address
     * @throws std::runtime_error if the host cannot be resolved or the connection cannot be established
     */
    PosixTcpSocket(const std::string &host, uint16_t port, std::chrono::milliseconds timeout);

    /**
     * Closes the connection
     */
    ~PosixTcpSocket() override;

    // Disable copying and moving
    PosixTcpSocket(const PosixTcpSocket&) = delete;
    PosixTcpSocket& operator=(const PosixTcpSocket&) = delete;
    PosixTcpSocket(PosixTcpSocket&&) = delete;
    PosixTcpSocket& operator=(PosixTcpSocket&&) = delete;

    std::vector<uint8_t> read(std::size_t maxSize, std::chrono::milliseconds timeout) override;
    void write(const std::vector<uint8_t> &data, std::chrono::milliseconds timeout) override;

 private:
    /// Socket file descriptor
    int fd = -1;
};

}  // namespace iqrf::connector::tcp
