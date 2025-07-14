/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: TcpConnector.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-10
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

#include <boost/asio.hpp>
#include <boost/core/ignore_unused.hpp>

#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/ConnectorUtils.h"
#include "iqrf/connector/tcp/TcpConfig.h"
#include "iqrf/log/Logging.h"

namespace iqrf::connector::tcp {

namespace log = ::iqrf::log;

/**
 * IQRF TCP connector
 */
class TcpConnector : public IConnector {
 public:
    /**
     * Constructs the IQRF TCP connector
     * @param config TCP connector configuration
     */
    explicit TcpConnector(TcpConfig config);

    /**
     * Destructs the IQRF TCP connector
     */
    ~TcpConnector() override;

    // Basic state

    /**
     * Get the current state of the connector.
     */
    State getState() const override;

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
    void resetTr() override {
        throw std::runtime_error("Not implemented");
    }

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
    void upload(const ProgrammingTarget target, const std::vector<uint8_t> &data) override {
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
    std::vector<uint8_t> download(const ProgrammingTarget target, const uint16_t address) override {
        boost::ignore_unused(target, address);
        throw std::runtime_error("Not implemented");
    }

 protected:
    /**
     * Connects to the TCP server.
     * @throws std::runtime_error if connection fails
     */
    void connect();

    /**
     * Disconnects from the TCP server.
     */
    void disconnect();

    /**
     * Reconnects to the TCP server.
     * This method will attempt to reconnect if the connection is lost.
     */
    void reconnect();

 private:
    /// TCP configuration
    TcpConfig config;
    /// Boost ASIO IO context
    boost::asio::io_context ioContext;
    /// TCP socket for communication
    boost::asio::ip::tcp::socket socket;
    /// TCP resolver for resolving hostnames
    boost::asio::ip::tcp::resolver resolver;
    /// Timer for asynchronous operations
    boost::asio::steady_timer timer;
    /// Flag indicating whether the connector is connecting
    std::atomic_bool connecting = false;
};

}  // namespace iqrf::connector::tcp
