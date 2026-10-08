/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: SpiConnector.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-05
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "iqrf/connector/BusSwitcher.h"
#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/spi/ISpiDevice.h"
#include "iqrf/connector/spi/SpiConfig.h"
#include "iqrf/connector/spi/SpiProtocol.h"

namespace iqrf::connector::spi {

/**
 * IQRF SPI connector
 *
 * Implements IQRF SPI protocol on top of a platform-specific SPI device (see ISpiDevice).
 */
class SpiConnector : public IConnector {
 public:
    /// Timeout for waiting on the expected SPI status of TR module
    static constexpr std::chrono::milliseconds STATUS_TIMEOUT{1000};
    /// Interval between two SPI status checks while waiting on the expected SPI status
    static constexpr std::chrono::milliseconds STATUS_POLL_INTERVAL{10};

    /**
     * Constructs the IQRF SPI connector using SPI device for the current platform
     * @param config SPI connector configuration
     */
    explicit SpiConnector(SpiConfig config);

    /**
     * Constructs the IQRF SPI connector using the specified SPI device
     *
     * Useful for testing or for platforms with a custom SPI device implementation.
     *
     * @param config SPI connector configuration
     * @param device SPI device, if nullptr, SPI device for the current platform is created
     */
    SpiConnector(SpiConfig config, std::unique_ptr<ISpiDevice> device);

    /**
     * Destructs the IQRF SPI connector
     */
    ~SpiConnector() override;

    // Disable copying and moving
    SpiConnector(const SpiConnector&) = delete;
    SpiConnector& operator=(const SpiConnector&) = delete;
    SpiConnector(SpiConnector&&) = delete;
    SpiConnector& operator=(SpiConnector&&) = delete;

    // Basic state

    /**
     * Get the current state of the connector.
     */
    State getState() const override;

    /**
     * Reads SPI status of TR module
     * @return SPI status
     * @throws std::runtime_error if the SPI status cannot be read or is invalid
     */
    SpiStatus getSpiStatus() const;

    /**
     * Returns SPI communication mode
     * @return SPI communication mode
     */
    SpiCommunicationMode getCommunicationMode() const;

    /**
     * Sets SPI communication mode
     * @param mode SPI communication mode
     */
    void setCommunicationMode(SpiCommunicationMode mode);

    // Basic communication

    /**
     * Send the data message via the connector.
     */
    void send(const std::vector<uint8_t> &data) override;

    using IConnector::send;

    /**
     * Read the data synchronously from the connector.
     *
     * Returns empty vector if TR module has no data ready.
     */
    std::vector<uint8_t> receive() override;

    // Transceiver operations

    /**
     * Retrieve basic information about the TR module.
     *
     * IBK is read as well for IQRF OS v4.03 or higher.
     */
    TrInfo readTrInfo() override;

    /**
     * Reset the TR module.
     */
    void resetTr() override;

    // Programming mode

    /**
     * Switch the connected TR to programming mode.
     */
    void enterProgrammingMode() override;

    /**
     * Wait for TR to enter programming mode.
     */
    void awaitProgrammingMode() override;

    /**
     * Switch the connected TR back from programming mode.
     */
    void exitProgrammingMode() override;

    /**
     * Uploads the data to the TR module in programming mode.
     *
     * @param target specifies what the uploaded data contain.
     * @param data is the actual data to be uploaded.
     */
    void upload(
        const ProgrammingTarget target,
        const std::vector<uint8_t> &data
    ) override;

    using IConnector::upload;

    /**
     * Downloads data from the TR module in programming mode.
     *
     * Only RFPGM and RF band targets can be downloaded without address.
     *
     * @param target specifies which data shall be downloaded.
     */
    std::vector<uint8_t> download(const ProgrammingTarget target) override;

    /**
     * Downloads data from the TR module memory in programming mode.
     *
     * @param target specifies which data shall be downloaded.
     * @param address specifies the Flash or EEPROM address from which the data will be downloaded.
     */
    std::vector<uint8_t> download(
        const ProgrammingTarget target,
        const uint16_t address
    ) override;

 protected:
    /**
     * Initializes the GPIO pins used for the connector
     */
    void initGpio();

    /**
     * Restores GPIO pins to the inactive state (disables TR power if configured, disables SPI bus, releases
     * PGM switch), failures are logged
     */
    void shutdownGpio() noexcept;

 private:
    /**
     * Returns SPI transfer timing for the current communication mode
     * @return SPI transfer timing
     */
    SpiTransferTiming timing() const;

    /**
     * Transfers raw data via the SPI device
     * @param tx Data to transmit
     * @return Received data
     */
    std::vector<uint8_t> transfer(const std::vector<uint8_t> &tx) const;

    /**
     * Reads data from TR module
     * @param length Length of data
     * @return Data, std::nullopt on CRCS mismatch
     */
    std::optional<std::vector<uint8_t>> readData(std::size_t length);

    /**
     * Reads Module Info block from TR module
     * @param length Length of Module Info block (16 or 32 B)
     * @return Module Info block
     * @throws std::runtime_error on CRCS mismatch
     */
    std::vector<uint8_t> readModuleInfo(std::size_t length);

    /**
     * Waits until the SPI status satisfies the predicate
     * @param predicate SPI status predicate
     * @param description Description of the expected status for the error message
     * @return SPI status satisfying the predicate
     * @throws std::runtime_error on timeout
     */
    SpiStatus awaitStatus(const std::function<bool(const SpiStatus&)> &predicate, const char *description) const;

    /**
     * Resets the TR module by power cycling, SPI is disconnected from TR module during the power cycle
     *
     * Power enable GPIO must be configured.
     *
     * @param enableBus Enable SPI bus after reset
     */
    void powerCycleTr(bool enableBus);

    /// Bus switcher
    iqrf::connector::BusSwitcher busSwitcher;
    /// SPI configuration
    SpiConfig config;
    /// SPI device
    std::unique_ptr<ISpiDevice> device;
    /// Messages received while waiting for TR module to be ready to receive data
    std::deque<std::vector<uint8_t>> pendingMessages;
    /// Guards SPI bus transactions
    mutable std::recursive_mutex spiMutex;
};

}  // namespace iqrf::connector::spi
