/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: SpiConnector.h
 * Authors: Ondřej Hujňák <ondrej.hujnak@iqrf.org>, Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-05
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

#include <boost/core/ignore_unused.hpp>

#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/spi/SpiConfig.h"

namespace iqrf::connector::spi {

/**
 * SPI Connector provides an access to TR module connected via SPI.
 */
class SpiConnector : public IConnector {
 public:
    /**
     * Constructs the SPI connector and initializes the SPI lines.
     */
    explicit SpiConnector(SpiConfig config);

    /**
     * Release the SPI lines.
     */
    ~SpiConnector() override;

    // Basic state

    /**
     * Get the current state of the connector.
     *
     * NOTE: clibspi - very different states with more detail
     */
    State getState() const override;

    // Basic communication

    /**
     * Send the data message via the connector.
     */
    void send(const std::vector<uint8_t> &data) override {
        throw std::runtime_error("Not implemented yet");
    }

    /**
     * Read the data synchronously from the connector.
     */
    std::vector<uint8_t> receive() override {
        throw std::runtime_error("Not implemented yet");
    }

    // Transceiver operations

    /**
     * Retrieve basic information about the TR module.
     */
    TrInfo readTrInfo() override {
        throw std::runtime_error("Not implemented yet");
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
        throw std::runtime_error("Not implemented yet");
    }

    /**
     * Wait for TR to enter programming mode.
     */
    void awaitProgrammingMode() override {
        throw std::runtime_error("Not implemented yet");
    }

    /**
     * Switch the connected TR back from programming mode.
     */
    void exitProgrammingMode() override {
        throw std::runtime_error("Not implemented yet");
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
        throw std::runtime_error("Not implemented yet");
    }

    /**
     * Downloads data from the TR module in programming mode.
     *
     * @param target specifies which data shall be downloaded.
     */
    std::vector<uint8_t> download(const ProgrammingTarget target) override {
        boost::ignore_unused(target);
        throw std::runtime_error("Not implemented yet");
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
        throw std::runtime_error("Not implemented yet");
    }

protected:
    /**
     * Initializes the GPIO pins used for the connector
     */
    void initGpio();

    /**
     * Enables or disables the bus
     * @param enable true to enable the bus, false to disable it
     */
    void toggleBus(bool enable);

private:
    /// SPI configuration
    SpiConfig config;
};

}  // namespace iqrf::connector
