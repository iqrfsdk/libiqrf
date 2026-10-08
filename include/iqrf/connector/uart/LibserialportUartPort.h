/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: LibserialportUartPort.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-06
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <libserialport.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "iqrf/connector/uart/IUartPort.h"

namespace iqrf::connector::uart {

/**
 * UART port - libserialport implementation
 */
class LibserialportUartPort : public IUartPort {
 public:
    /**
     * Opens and configures UART port
     * @param config UART port configuration
     * @throws std::runtime_error if the port cannot be opened or configured
     */
    explicit LibserialportUartPort(const UartPortConfig &config);

    /**
     * Closes UART port
     */
    ~LibserialportUartPort() override;

    // Disable copying and moving
    LibserialportUartPort(const LibserialportUartPort&) = delete;
    LibserialportUartPort& operator=(const LibserialportUartPort&) = delete;
    LibserialportUartPort(LibserialportUartPort&&) = delete;
    LibserialportUartPort& operator=(LibserialportUartPort&&) = delete;

    std::vector<uint8_t> read(std::size_t maxSize, std::chrono::milliseconds timeout) override;

    void write(const std::vector<uint8_t> &data, std::chrono::milliseconds timeout) override;

    void flushInput() override;

 private:
    /**
     * Opens and configures the UART port
     * @param config UART port configuration
     */
    void open(const UartPortConfig &config);

    /**
     * Closes and frees the UART port if it was created
     */
    void close() noexcept;

    /**
     * Logs information about the UART port
     */
    void logPortInfo() const;

    /**
     * Check the result of the libserialport functions and throw an exception on error.
     * @param result libserialport return code
     * @return libserialport return code
     */
    static int checkResult(sp_return result);

    /// UART port
    sp_port *port = nullptr;
};

}  // namespace iqrf::connector::uart
