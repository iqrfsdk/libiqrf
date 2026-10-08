/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: LinuxSpiDevice.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-05
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <cstdint>
#include <vector>

#include "iqrf/connector/spi/ISpiDevice.h"

namespace iqrf::connector::spi {

/**
 * SPI device - Linux spidev implementation
 */
class LinuxSpiDevice : public ISpiDevice {
 public:
    /**
     * Opens and configures spidev device
     * @param config SPI device configuration
     * @throws std::system_error if the device cannot be opened or configured
     */
    explicit LinuxSpiDevice(SpiDeviceConfig config);

    /**
     * Closes spidev device
     */
    ~LinuxSpiDevice() override;

    // Disable copying and moving
    LinuxSpiDevice(const LinuxSpiDevice&) = delete;
    LinuxSpiDevice& operator=(const LinuxSpiDevice&) = delete;
    LinuxSpiDevice(LinuxSpiDevice&&) = delete;
    LinuxSpiDevice& operator=(LinuxSpiDevice&&) = delete;

    /**
     * Transfers data in full-duplex mode
     * @param tx Data to transmit
     * @param timing Transfer timing
     * @return Received data
     */
    std::vector<uint8_t> transfer(const std::vector<uint8_t> &tx, const SpiTransferTiming &timing) override;

 private:
    /**
     * Calls ioctl on the spidev device
     * @param request ioctl request
     * @param argument ioctl argument
     * @param errorMessage Error message
     * @throws std::system_error on failure
     */
    void ioctl(uint32_t request, const void *argument, const char *errorMessage) const;

    /**
     * Transfers data byte by byte, chip select stays asserted during the whole transfer (high speed mode)
     * @param tx Data to transmit
     * @param rx Received data
     * @param timing Transfer timing
     */
    void transferKeepChipSelect(
        const std::vector<uint8_t> &tx,
        std::vector<uint8_t> &rx,
        const SpiTransferTiming &timing
    ) const;

    /**
     * Transfers data byte by byte, chip select is deasserted after each byte (low speed mode)
     * @param tx Data to transmit
     * @param rx Received data
     * @param timing Transfer timing
     */
    void transferToggleChipSelect(
        const std::vector<uint8_t> &tx,
        std::vector<uint8_t> &rx,
        const SpiTransferTiming &timing
    ) const;

    /// SPI device configuration
    SpiDeviceConfig config;
    /// spidev file descriptor
    int fd = -1;
};

}  // namespace iqrf::connector::spi
