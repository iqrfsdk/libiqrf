/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: IConnector.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-10
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <libserialport.h>

#include "iqrf/connector/IConnector.h"
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
    explicit UartConnector(const UartConfig& config);

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
    void send(const std::vector<uint8_t> &data) override {;
    }

    /**
     * Read the data synchronously from the connector.
     */
    std::vector<uint8_t> receive() override;

    /**
     * Register the responseHandler for received messages.
     */
    void registerResponseHandler(ResponseHandler responseHandler, AccessType access) override {}

    /**
     * Start the listening loop in a separate thread.
     */
    void listen() override {}

    // Exclusive access

    /**
     * Check whether the connector is in exclusive access mode.
     */
    virtual bool hasExclusiveAccess() const override { return false; }

    // Transceiver operations

    /**
     * Retrieve basic information about the TR module.
     */
    TrInfo readTrInfo() override {
        throw new std::runtime_error("Not implemented");
    }

    /**
     * Reset the TR module.
     *
     * TODO: Used in Daemon(IqrfCdc), clibspi, clibcdc
     */
    void resetTr() override {};

    // Programming mode

    /**
     * Switch the connected TR to programming mode.
     */
    void enterProgrammingMode() override {
        throw new std::runtime_error("Not implemented");
    }

    /**
     * Wait for TR to enter programming mode.
     */
    void awaitProgrammingMode() override {
        throw new std::runtime_error("Not implemented");
    }

    /**
     * Switch the connected TR back from programming mode.
     */
    void exitProgrammingMode() override {
        throw new std::runtime_error("Not implemented");
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
        throw new std::runtime_error("Not implemented");
    }

    /**
     * Uploads the data to the TR module memory.
     *
     * Just an utility function which prepends the address to the data so it is properly packed for the upload.
     *
     * @param target specifies what the uploaded data contain.
     * @param data is the actual data to be uploaded.
     * @param address specifies the Flash or EEPROM address at which the data will be stored.
     */
    void upload(
        const ProgrammingTarget target,
        const std::vector<uint8_t> &data,
        const uint16_t address
    ) override {
        throw new std::runtime_error("Not implemented");
    }

    /**
     * Downloads data from the TR module in programming mode.
     *
     * @param target specifies which data shall be downloaded.
     */
    std::vector<uint8_t> download(const ProgrammingTarget target) override {
        throw new std::runtime_error("Not implemented");
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
        throw new std::runtime_error("Not implemented");
    }

protected:
    /**
     * Listen to the connector and asynchronously call responseHandler for
     * received messages.
     */
    void listeningLoop() override {
        throw new std::runtime_error("Not implemented");
    }

 private:

    static int checkSerialResult(enum sp_return result);

    /// UART configuration
    UartConfig config;
    /// UART port
    struct sp_port *port = nullptr;
};

}
