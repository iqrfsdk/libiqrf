/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: IUartPort.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-06
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

namespace iqrf::connector::uart {

/**
 * UART port configuration
 */
struct UartPortConfig {
    /// UART device name (e.g. /dev/ttyS0 on Linux)
    std::string device;
    /// UART baud rate
    uint32_t baudRate = 115200;
};

/**
 * Interface of a UART port (8 data bits, no parity, 1 stop bit, no flow control)
 */
class IUartPort {
 public:
    /**
     * Destructor
     */
    virtual ~IUartPort() = default;

    IUartPort() = default;
    IUartPort(const IUartPort&) = delete;
    IUartPort& operator=(const IUartPort&) = delete;
    IUartPort(IUartPort&&) = delete;
    IUartPort& operator=(IUartPort&&) = delete;

    /**
     * Reads received bytes, returns as soon as any data is available
     * @param maxSize Maximum number of bytes to read
     * @param timeout Maximum time to wait for data
     * @return Received bytes, empty on timeout
     * @throws std::runtime_error on read failure
     */
    virtual std::vector<uint8_t> read(std::size_t maxSize, std::chrono::milliseconds timeout) = 0;

    /**
     * Writes all bytes
     * @param data Data to write
     * @param timeout Maximum time to write the data
     * @throws std::runtime_error on write failure or if the data is not written within the timeout
     */
    virtual void write(const std::vector<uint8_t> &data, std::chrono::milliseconds timeout) = 0;

    /**
     * Discards received bytes which have not been read yet
     * @throws std::runtime_error on failure
     */
    virtual void flushInput() = 0;
};

/**
 * Opens UART port
 * @param config UART port configuration
 * @return UART port
 * @throws std::runtime_error if the port cannot be opened or configured
 */
std::unique_ptr<IUartPort> createUartPort(const UartPortConfig &config);

}  // namespace iqrf::connector::uart
