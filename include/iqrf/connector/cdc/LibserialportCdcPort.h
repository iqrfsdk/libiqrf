/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: LibserialportCdcPort.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#pragma once

#include <libserialport.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "iqrf/connector/cdc/ICdcPort.h"

namespace iqrf::connector::cdc {

/**
 * CDC port - libserialport implementation
 */
class LibserialportCdcPort : public ICdcPort {
 public:
    /**
     * Opens and configures the CDC port
     * @param device Name of the device (e.g. /dev/ttyACM0 on Linux)
     * @throws std::runtime_error if the port cannot be opened or configured
     */
    explicit LibserialportCdcPort(const std::string &device);

    /**
     * Closes the CDC port
     */
    ~LibserialportCdcPort() override;

    // Disable copying and moving
    LibserialportCdcPort(const LibserialportCdcPort&) = delete;
    LibserialportCdcPort& operator=(const LibserialportCdcPort&) = delete;
    LibserialportCdcPort(LibserialportCdcPort&&) = delete;
    LibserialportCdcPort& operator=(LibserialportCdcPort&&) = delete;

    std::vector<uint8_t> read(std::size_t maxSize, std::chrono::milliseconds timeout) override;
    void write(const std::vector<uint8_t> &data, std::chrono::milliseconds timeout) override;
    void flushInput() override;

 private:
    /**
     * Opens and configures the CDC port, waits for the device to settle and discards its input
     * @param device Name of the device
     */
    void open(const std::string &device);

    /**
     * Closes and frees the CDC port if it was created
     */
    void close() noexcept;

    /**
     * Logs information about the CDC port
     */
    void logPortInfo() const;

    /**
     * Check the result of the libserialport functions and throw an exception on error.
     * @param result libserialport return code
     * @return libserialport return code
     */
    static int checkResult(sp_return result);

    /// CDC port
    sp_port *port = nullptr;
};

}  // namespace iqrf::connector::cdc
