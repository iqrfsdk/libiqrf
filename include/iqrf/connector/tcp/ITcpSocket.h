/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: ITcpSocket.h
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
#include <memory>
#include <string>
#include <vector>

namespace iqrf::connector::tcp {

/**
 * Interface of a connected TCP client socket
 *
 * The socket represents a single connection, a new socket has to be created to reconnect.
 * One thread may read while another thread writes, other concurrent use is not allowed.
 */
class ITcpSocket {
 public:
    /**
     * Destructor, closes the connection
     */
    virtual ~ITcpSocket() = default;

    ITcpSocket() = default;
    ITcpSocket(const ITcpSocket&) = delete;
    ITcpSocket& operator=(const ITcpSocket&) = delete;
    ITcpSocket(ITcpSocket&&) = delete;
    ITcpSocket& operator=(ITcpSocket&&) = delete;

    /**
     * Reads received bytes, returns as soon as any data is available
     * @param maxSize Maximum number of bytes to read
     * @param timeout Maximum time to wait for data
     * @return Received bytes, empty on timeout
     * @throws std::runtime_error on read failure or if the connection has been closed by the peer
     */
    virtual std::vector<uint8_t> read(std::size_t maxSize, std::chrono::milliseconds timeout) = 0;

    /**
     * Writes all bytes
     * @param data Data to write
     * @param timeout Maximum time to write the data
     * @throws std::runtime_error on write failure or if the data is not written within the timeout
     */
    virtual void write(const std::vector<uint8_t> &data, std::chrono::milliseconds timeout) = 0;
};

/**
 * Connects to the TCP server using the platform socket implementation
 *
 * All addresses the host name resolves to are tried in order until the connection is established.
 *
 * @param host Host name or IP address of the server
 * @param port Port number of the server
 * @param timeout Maximum time to establish the connection to a single address
 * @return Connected TCP socket
 * @throws std::runtime_error if the host cannot be resolved or the connection cannot be established
 */
std::unique_ptr<ITcpSocket> createTcpSocket(const std::string &host, uint16_t port, std::chrono::milliseconds timeout);

}  // namespace iqrf::connector::tcp
