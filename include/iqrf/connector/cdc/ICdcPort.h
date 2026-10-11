/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: ICdcPort.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
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

namespace iqrf::connector::cdc {

/**
 * Interface of a serial port of the USB CDC device (8 data bits, no parity, 1 stop bit, no flow control)
 */
class ICdcPort {
 public:
    /**
     * Destructor
     */
    virtual ~ICdcPort() = default;

    ICdcPort() = default;
    ICdcPort(const ICdcPort&) = delete;
    ICdcPort& operator=(const ICdcPort&) = delete;
    ICdcPort(ICdcPort&&) = delete;
    ICdcPort& operator=(ICdcPort&&) = delete;

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
 * Opens the serial port of the USB CDC device
 * @param device Name of the device (e.g. /dev/ttyACM0 on Linux)
 * @return CDC port
 * @throws std::runtime_error if the port cannot be opened or configured
 */
std::unique_ptr<ICdcPort> createCdcPort(const std::string &device);

}  // namespace iqrf::connector::cdc
