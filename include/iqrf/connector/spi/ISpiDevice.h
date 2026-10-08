/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: ISpiDevice.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-05
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace iqrf::connector::spi {

/**
 * SPI device configuration
 */
struct SpiDeviceConfig {
    /// SPI device path (e.g. /dev/spidev0.0 on Linux)
    std::string device;
    /// SPI clock speed [Hz]
    uint32_t speed = 250000;
};

/**
 * Timing of a SPI transfer
 *
 * IQRF TR modules require delays between chip select and clock and between the transferred bytes,
 * so the platform implementation has to transfer the data byte by byte.
 */
struct SpiTransferTiming {
    /// Delay between chip select assertion and the first clock edge (T1)
    std::chrono::microseconds csSetupDelay;
    /// Delay between two consecutive bytes (T2)
    std::chrono::microseconds interByteDelay;
    /// Keep chip select asserted during the whole transfer, otherwise deassert it after each byte
    bool keepChipSelect;
};

/**
 * Platform-independent interface of a SPI master device
 *
 * Implement this interface to add support for a new platform and register the implementation
 * in createSpiDevice().
 */
class ISpiDevice {
 public:
    /**
     * Destructor
     */
    virtual ~ISpiDevice() = default;

    ISpiDevice() = default;
    ISpiDevice(const ISpiDevice&) = delete;
    ISpiDevice& operator=(const ISpiDevice&) = delete;
    ISpiDevice(ISpiDevice&&) = delete;
    ISpiDevice& operator=(ISpiDevice&&) = delete;

    /**
     * Transfers data in full-duplex mode (SPI mode 0, 8 bits per word, MSB first)
     * @param tx Data to transmit
     * @param timing Transfer timing
     * @return Received data, same length as transmitted data
     * @throws std::system_error on transfer failure
     */
    virtual std::vector<uint8_t> transfer(const std::vector<uint8_t> &tx, const SpiTransferTiming &timing) = 0;
};

/**
 * Creates SPI device for the current platform
 * @param config SPI device configuration
 * @return SPI device
 * @throws std::runtime_error if the platform is not supported
 * @throws std::system_error if the device cannot be opened or configured
 */
std::unique_ptr<ISpiDevice> createSpiDevice(const SpiDeviceConfig &config);

}  // namespace iqrf::connector::spi
