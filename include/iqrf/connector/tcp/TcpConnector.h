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

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <stdexcept>
#include <vector>

#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/tcp/ITcpSocket.h"
#include "iqrf/connector/tcp/TcpConfig.h"

namespace iqrf::connector::tcp {

/**
 * IQRF TCP connector
 *
 * Transfers raw data (e.g. DPA messages) to and from the TCP server. The connection is (re)established
 * on demand by send() and receive() with an exponential backoff between failed attempts.
 */
class TcpConnector : public IConnector {
 public:
    /// Creates the connected TCP socket according to the configuration
    using SocketFactory = std::function<std::unique_ptr<ITcpSocket>(const TcpConfig &config)>;

    /**
     * Constructs the IQRF TCP connector and attempts to connect to the TCP server
     *
     * The constructor does not fail if the server is not reachable, the connection is retried later.
     *
     * @param config TCP connector configuration
     * @param socketFactory Creates the TCP socket (e.g. for testing), the platform socket is used if null
     */
    explicit TcpConnector(TcpConfig config, SocketFactory socketFactory = nullptr);

    /**
     * Destructs the IQRF TCP connector
     */
    ~TcpConnector() override;

    // Disable copying and moving
    TcpConnector(const TcpConnector&) = delete;
    TcpConnector& operator=(const TcpConnector&) = delete;
    TcpConnector(TcpConnector&&) = delete;
    TcpConnector& operator=(TcpConnector&&) = delete;

    // Basic state

    /**
     * Get the current state of the connector.
     * @return Ready if connected to the TCP server, NotReady otherwise
     */
    State getState() const override;

    // Basic communication

    /**
     * Send the data message via the connector.
     * @throws std::runtime_error if the connector is not connected or the data cannot be sent
     */
    void send(const std::vector<uint8_t> &data) override;

    /**
     * Read the data synchronously from the connector.
     *
     * Waits up to the receive timeout for the data. The connection is reestablished if it has been lost.
     *
     * @return Received data, empty if no data has been received within the receive timeout
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
    void upload(
        [[maybe_unused]] const ProgrammingTarget target,
        [[maybe_unused]] const std::vector<uint8_t> &data
    ) override {
        throw std::runtime_error("Not implemented");
    }

    /**
     * Downloads data from the TR module in programming mode.
     *
     * @param target specifies which data shall be downloaded.
     */
    std::vector<uint8_t> download([[maybe_unused]] const ProgrammingTarget target) override {
        throw std::runtime_error("Not implemented");
    }

    /**
     * Downloads data from the TR module memory in programming mode.
     *
     * @param target specifies which data shall be downloaded.
     * @param address specifies the Flash or EEPROM address from which the data will be downloaded.
     */
    std::vector<uint8_t> download(
        [[maybe_unused]] const ProgrammingTarget target,
        [[maybe_unused]] const uint16_t address
    ) override {
        throw std::runtime_error("Not implemented");
    }

 private:
    /**
     * Connects to the TCP server if not connected and the reconnection delay has elapsed
     * @return true if connected
     */
    bool ensureConnected();

    /**
     * Closes the broken connection and schedules the reconnection
     * @param generation Generation of the broken connection, nothing is done if it has already been replaced
     * @param reason Reason of the disconnection
     */
    void disconnect(uint64_t generation, const char *reason);

    /// Maximum time to wait for data in receive()
    static constexpr std::chrono::milliseconds RECEIVE_TIMEOUT{100};
    /// Maximum time to write the data
    static constexpr std::chrono::milliseconds WRITE_TIMEOUT{1000};
    /// Maximum number of bytes read from the socket at once
    static constexpr std::size_t READ_SIZE = 1024;

    /// TCP configuration
    TcpConfig config;
    /// Creates the TCP socket
    SocketFactory socketFactory;
    /// Socket of the current connection, null if disconnected
    std::unique_ptr<ITcpSocket> socket;
    /// Guards the socket: shared for reading and writing, exclusive for (re)connecting and disconnecting
    std::shared_mutex socketMutex;
    /// Generation of the current connection, incremented on each established connection
    uint64_t connectionGeneration = 0;
    /// Whether the connector is connected (readable without locking)
    std::atomic_bool connected = false;
    /// Time point of the next connection attempt
    std::chrono::steady_clock::time_point nextConnectAttempt;
    /// Delay after the next failed connection attempt
    std::chrono::milliseconds reconnectDelay;
};

}  // namespace iqrf::connector::tcp
