/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: UartConnector.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-10
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <libserialport.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

#include <boost/core/ignore_unused.hpp>

#include "iqrf/connector/BusSwitcher.h"
#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/ConnectorUtils.h"
#include "iqrf/connector/uart/HdlcFrame.h"
#include "iqrf/connector/uart/UartConfig.h"
#include "iqrf/log/Logging.h"

namespace iqrf::connector::uart {

namespace log = ::iqrf::log;

/**
 * IQRF UART connector
 */
class UartConnector : public IConnector {
 public:
    /**
     * Constructs the IQRF UART connector
     * @param config UART connector configuration
     */
    explicit UartConnector(UartConfig config);

    /**
     * Destructs the IQRF UART connector
     */
    ~UartConnector() override;

    // Basic state

    /**
     * Get the current state of the connector.
     */
    State getState() const override { return State::Ready; }

    // Basic communication

    /**
     * Send the data message via the connector.
     */
    void send(const std::vector<uint8_t> &data) override;

    /**
     * Read the data synchronously from the connector.
     */
    std::vector<uint8_t> receive() override;

    // Transceiver operations

    /**
     * Retrieve basic information about the TR module.
     */
    TrInfo readTrInfo() override {
        throw std::runtime_error("Not implemented");
    }

    /**
     * Reset the TR module.
     */
    void resetTr() override;

    // Programming mode

    /**
     * Switch the connected TR to programming mode.
     */
    void enterProgrammingMode() override {
        throw std::runtime_error("Not implemented");
    }

    /**
     * Wait for TR to enter programming mode.
     */
    void awaitProgrammingMode() override {
        throw std::runtime_error("Not implemented");
    }

    /**
     * Switch the connected TR back from programming mode.
     */
    void exitProgrammingMode() override {
        throw std::runtime_error("Not implemented");
    }

    /**
     * Uploads the data to the TR module in programming mode.
     *
     * @param target specifies what the uploaded data contain.
     * @param data is the actual data to be uploaded.
     */
    void upload(
        const ProgrammingTarget target,
        const std::vector<uint8_t> &data
    ) override {
        boost::ignore_unused(target, data);
        throw std::runtime_error("Not implemented");
    }

    /**
     * Downloads data from the TR module in programming mode.
     *
     * @param target specifies which data shall be downloaded.
     */
    std::vector<uint8_t> download(const ProgrammingTarget target) override {
        boost::ignore_unused(target);
        throw std::runtime_error("Not implemented");
    }

    /**
     * Downloads data from the TR module memory in programming mode.
     *
     * @param target specifies which data shall be downloaded.
     * @param address specifies the Flash or EEPROM address from which the data will be downloaded.
     */
    std::vector<uint8_t> download(
        const ProgrammingTarget target,
        const uint16_t address
    ) override {
        boost::ignore_unused(target, address);
        throw std::runtime_error("Not implemented");
    }

 protected:
    /**
     * Initializes the GPIO pins used for the connector
     */
    void initGpio();

 private:
    /**
     * Check the result of the libserialport functions and throw an exception on error.
     * @param result libserialport return code
     * @return libserialport return code
     */
    static int checkSerialResult(sp_return result);

    /// Bus switcher
    iqrf::connector::BusSwitcher busSwitcher;
    /// UART configuration
    UartConfig config;
    /// UART port
    sp_port *port = nullptr;
};

}  // namespace iqrf::connector::uart
