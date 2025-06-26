/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: SpiConnector.cpp
 * Authors: Ondřej Hujňák <ondrej.hujnak@iqrf.org>
 * Date: 2025-05-09
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "iqrf/connector/spi/SpiConnector.h"

#include <cstring>
#include <sstream>

#include "iqrf/log/Logging.h"

namespace iqrf::connector::spi {

SpiConnector::SpiConnector(
    SpiConfig config,
    SpiTransportMode transportMode
): IConnector(), busSwitcher(config.busSwitch()), config(std::move(config)), transportMode(transportMode) {
    this->initGpio();

    if (this->config.trModuleReset) {
        this->resetTr(true);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    this->fd = open(this->config.interface.c_str(), O_RDWR);
    if (this->fd < 0) {
        std::stringstream errorMsg;
        errorMsg << "Failed to open SPI device: "
                 << this->config.interface
                 << " - " << strerror(errno);
        throw std::runtime_error(errorMsg.str());
    }

    /// Set SPI R/W modes
    if (ioctl(this->fd, SPI_IOC_WR_MODE, &SpiConnector::SPI_MODE_WRITE) < 0) {
        std::stringstream errorMsg;
        errorMsg << "Failed to set SPI mode for device: "
                 << this->config.interface
                 << " - " << strerror(errno);
        throw std::runtime_error(errorMsg.str());
    }

    if (ioctl(this->fd, SPI_IOC_RD_MODE, &SpiConnector::SPI_MODE_READ) < 0) {
        std::stringstream errorMsg;
        errorMsg << "Failed to get SPI mode for device: "
                 << this->config.interface
                 << " - " << strerror(errno);
        throw std::runtime_error(errorMsg.str());
    }

    // Set SPI bits per word
    if (ioctl(this->fd, SPI_IOC_WR_BITS_PER_WORD, &SpiConnector::BITS_PER_WORD_WRITE) < 0) {
        std::stringstream errorMsg;
        errorMsg << "Failed to set SPI bits per word for device: "
                 << this->config.interface
                 << " - " << strerror(errno);
        throw std::runtime_error(errorMsg.str());
    }

    if (ioctl(this->fd, SPI_IOC_RD_BITS_PER_WORD, &SpiConnector::BITS_PER_WORD_READ) < 0) {
        std::stringstream errorMsg;
        errorMsg << "Failed to get SPI bits per word for device: "
                 << this->config.interface
                 << " - " << strerror(errno);
        throw std::runtime_error(errorMsg.str());
    }

    // Set SPI max speed
    if (ioctl(this->fd, SPI_IOC_WR_MAX_SPEED_HZ, &SpiConnector::SPI_MAX_SPEED_WRITE) < 0) {
        std::stringstream errorMsg;
        errorMsg << "Failed to set SPI max speed for device: "
                 << this->config.interface
                 << " - " << strerror(errno);
        throw std::runtime_error(errorMsg.str());
    }

    if (ioctl(this->fd, SPI_IOC_RD_MAX_SPEED_HZ, &SpiConnector::SPI_MAX_SPEED_READ) < 0) {
        std::stringstream errorMsg;
        errorMsg << "Failed to get SPI max speed for device: "
                 << this->config.interface
                 << " - " << strerror(errno);
        throw std::runtime_error(errorMsg.str());
    }

    this->initNullTransport();
}

SpiConnector::~SpiConnector() {
    if (this->config.powerEnableGpio) {
        this->config.powerEnableGpio->setValue(false);
    }

    this->busSwitcher.toggleSpi(false);

    if (this->config.pgmSwitchGpio) {
        this->config.pgmSwitchGpio->setValue(false);
    }

    if (this->fd >= 0) {
        close(this->fd);
    }
}

void SpiConnector::initGpio() {
    if (this->config.pgmSwitchGpio) {
        this->config.pgmSwitchGpio->initOutput(false);
    }
    if (this->config.powerEnableGpio) {
        this->config.powerEnableGpio->initOutput(true);
    }
    this->busSwitcher.init();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (this->config.trModuleReset) {
        this->resetTr(true);
    }

    this->busSwitcher.toggleSpi(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void SpiConnector::resetTr(bool enableBus) {
    this->busSwitcher.toggleSpi(false);

    if (!this->config.powerEnableGpio.has_value()) {
        return;
    }
    this->config.powerEnableGpio->setValue(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    this->config.powerEnableGpio->setValue(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    this->busSwitcher.toggleSpi(enableBus);
}

void SpiConnector::initNullTransport() {
    nullTransport->tx_buf = 0;
    nullTransport->rx_buf = 0;

    nullTransport->len = 0;
    nullTransport->speed_hz = 250000;

    nullTransport->delay_usecs = (this->transportMode == SpiTransportMode::HighSpeed) ? DELAY_AFTER_CS_HS_US : DELAY_AFTER_CS_LS_US;
    nullTransport->bits_per_word = BITS_PER_WORD_WRITE;
    nullTransport->cs_change = SPI_CS_DESELECT_DEVICE;
    nullTransport->tx_nbits = 0;
    nullTransport->rx_nbits = 0;
    nullTransport->word_delay_usecs = 0;
    nullTransport->pad = 0;
}

void SpiConnector::awaitProgrammingMode() {
    std::chrono::milliseconds startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
    // Wait for the TR module to enter programming mode until 1000 ms have passed
    while (true) {
        std::chrono::milliseconds currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
        if (currentTime - startTime >= std::chrono::milliseconds(1000)) {
            throw std::runtime_error("Timeout while waiting for TR module to enter programming mode");
        }
        // TODO: Check SPI status
    }
}

void SpiConnector::enterProgrammingMode() {
    // TODO: Check SPI status

    this->busSwitcher.toggleSpi(false);
    if (this->config.pgmSwitchGpio) {
        this->config.pgmSwitchGpio->setValue(true);
    } else {
        throw std::runtime_error("PGM switch GPIO is not configured");
    }
    this->resetTr(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    this->busSwitcher.toggleSpi(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void SpiConnector::exitProgrammingMode() {
    // TODO: Check SPI status and

    this->awaitProgrammingMode();

    this->resetTr(true);
}

State SpiConnector::getState() const { return State::NotReady; }

}  // namespace iqrf::connector::spi
