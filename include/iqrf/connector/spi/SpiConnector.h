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

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#if defined(__linux__)
#include <linux/spi/spidev.h>
#elif defined(__FreeBSD__)
#include <dev/spibus/spi.h>
#endif

#include <cerrno>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <boost/core/ignore_unused.hpp>

#include "iqrf/connector/BusSwitcher.h"
#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/spi/SpiConfig.h"

namespace iqrf::connector::spi {

/**
 * SPI transport modes.
 */
enum class SpiTransportMode {
    /// Low speed SPI mode
    LowSpeed,
    /// High speed SPI mode
    HighSpeed,
};

/**
 * SPI Connector provides an access to TR module connected via SPI.
 */
class SpiConnector : public IConnector {
 public:
    /**
     * Constructs the SPI connector and initializes the SPI lines.
     */
    explicit SpiConnector(SpiConfig config, SpiTransportMode transportMode = SpiTransportMode::LowSpeed);

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
    void resetTr() override {
        this->resetTr(true);
    }

    /**
     * Reset the TR module.
     * @param enableBus true to enable the bus after reset, false to keep it disabled
     */
    void resetTr(bool enableBus = true);

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

    /**
     * Initializes the SPI transport in a null mode.
     */
    void initNullTransport();

    /**
     * Returns the SPI transport mode.
     * @return the SPI transport mode
     */
    SpiTransportMode getTransportMode() const {
        return this->transportMode;
    }

    /**
     * Sets the SPI transport mode.
     * @param mode the SPI transport mode to set
     */
    void setTransportMode(SpiTransportMode mode) {
        this->transportMode = mode;
        this->initNullTransport();
    }

 protected:
    /**
     * Initializes the GPIO pins used for the connector
     */
    void initGpio();

 private:
    /// Device's word size for writing.
    static constexpr uint8_t BITS_PER_WORD_WRITE = 8;
    /// Device's word size for reading.
    static constexpr uint8_t BITS_PER_WORD_READ = -1;
    /// Time delay T1 between CS and SCLK falling edge for high speed communication.
    static constexpr uint16_t DELAY_AFTER_CS_HS_US = 5;
    /// Time delay T1 between CS and SCLK falling edge for low speed communication.
    static constexpr uint16_t DELAY_AFTER_CS_LS_US = 10;
    /// Device's maximum speed in Hz for writing.
    static constexpr uint32_t SPI_MAX_SPEED_WRITE = 250000;
    /// Device's maximum speed in Hz for reading.
    static constexpr uint32_t SPI_MAX_SPEED_READ = -1;
    /// Device's mode for writing.
    static constexpr uint8_t SPI_MODE_WRITE = SPI_MODE_0;
    /// Device's mode for reading.
    static constexpr uint8_t SPI_MODE_READ = -1;
    // Slave select behavior - deselect device after transfer
    static constexpr uint8_t SPI_CS_DESELECT_DEVICE = 0;
    /// Slave select behavior - keep device selected after transfer
    static constexpr uint8_t SPI_CS_DEVICE_STAY_SELECTED = 1;

    /// Bus switcher
    iqrf::connector::BusSwitcher busSwitcher;
    /// SPI configuration
    SpiConfig config;
    /// SPI transport mode
    SpiTransportMode transportMode = SpiTransportMode::LowSpeed;
    /// File descriptor for the SPI bus
    mutable int fd = -1;
    /// Null transport structure for SPI transfers
    std::optional<struct spi_ioc_transfer> nullTransport;
};

}  // namespace iqrf::connector::spi
