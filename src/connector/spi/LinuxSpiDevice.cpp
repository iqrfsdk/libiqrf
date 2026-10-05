/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: LinuxSpiDevice.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-05
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "iqrf/connector/spi/LinuxSpiDevice.h"

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include "iqrf/log/Logging.h"

namespace iqrf::connector::spi {

namespace log = ::iqrf::log;

namespace {

/// SPI mode required by IQRF TR modules (idle clock low, data sampled on rising edge)
constexpr uint8_t SPI_MODE = SPI_MODE_0;
/// SPI word size
constexpr uint8_t BITS_PER_WORD = 8;

/**
 * Converts delay to spidev delay field
 * @param delay Delay
 * @return Delay in microseconds saturated to the spidev field range
 */
uint16_t toDelayUsecs(const std::chrono::microseconds delay) {
    return static_cast<uint16_t>(std::clamp<std::chrono::microseconds::rep>(delay.count(), 0, UINT16_MAX));
}

/**
 * Throws std::system_error with the current errno
 * @param message Error message
 */
[[noreturn]] void throwErrno(const std::string &message) {
    throw std::system_error(errno, std::generic_category(), message);
}

/**
 * Creates empty spidev transfer used to apply delay T1 after chip select assertion
 * @param speed SPI clock speed [Hz]
 * @param delay Delay after chip select assertion
 * @return spidev transfer
 */
spi_ioc_transfer setupTransfer(const uint32_t speed, const std::chrono::microseconds delay) {
    spi_ioc_transfer transfer{};
    transfer.len = 0;
    transfer.speed_hz = speed;
    transfer.bits_per_word = BITS_PER_WORD;
    transfer.delay_usecs = toDelayUsecs(delay);
    transfer.cs_change = 0;
    return transfer;
}

/**
 * Creates single byte spidev transfer
 * @param tx Byte to transmit
 * @param rx Received byte
 * @param speed SPI clock speed [Hz]
 * @param delay Delay after the byte transfer
 * @param csChange Chip select change (see spidev documentation)
 * @return spidev transfer
 */
spi_ioc_transfer byteTransfer(
    const uint8_t *tx,
    uint8_t *rx,  // NOLINT(readability-non-const-parameter) - written by the kernel via rx_buf
    const uint32_t speed,
    const std::chrono::microseconds delay,
    const bool csChange
) {
    spi_ioc_transfer transfer{};
    // spidev ABI passes buffers as 64-bit integers
    transfer.tx_buf = reinterpret_cast<uintptr_t>(tx);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    transfer.rx_buf = reinterpret_cast<uintptr_t>(rx);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    transfer.len = 1;
    transfer.speed_hz = speed;
    transfer.bits_per_word = BITS_PER_WORD;
    transfer.delay_usecs = toDelayUsecs(delay);
    transfer.cs_change = csChange ? 1 : 0;
    return transfer;
}

}  // namespace

LinuxSpiDevice::LinuxSpiDevice(SpiDeviceConfig config): config(std::move(config)) {
    IQRF_LOG(log::Level::Debug) << "Opening SPI device: " << this->config.device;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
    this->fd = ::open(this->config.device.c_str(), O_RDWR | O_CLOEXEC);
    if (this->fd < 0) {
        throwErrno("Unable to open SPI device " + this->config.device);
    }
    try {
        this->ioctl(SPI_IOC_WR_MODE, &SPI_MODE, "Unable to set SPI mode");
        this->ioctl(SPI_IOC_WR_BITS_PER_WORD, &BITS_PER_WORD, "Unable to set SPI bits per word");
        this->ioctl(SPI_IOC_WR_MAX_SPEED_HZ, &this->config.speed, "Unable to set SPI max speed");
    } catch (...) {
        ::close(this->fd);
        this->fd = -1;
        throw;
    }
}

LinuxSpiDevice::~LinuxSpiDevice() {
    if (this->fd >= 0) {
        ::close(this->fd);
    }
}

void LinuxSpiDevice::ioctl(const uint32_t request, const void *argument, const char *errorMessage) const {
    if (::ioctl(this->fd, request, argument) < 0) {  // NOLINT(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
        throwErrno(errorMessage);
    }
}

std::vector<uint8_t> LinuxSpiDevice::transfer(const std::vector<uint8_t> &tx, const SpiTransferTiming &timing) {
    std::vector<uint8_t> rx(tx.size(), 0);
    if (tx.empty()) {
        return rx;
    }
    if (timing.keepChipSelect) {
        this->transferKeepChipSelect(tx, rx, timing);
    } else {
        this->transferToggleChipSelect(tx, rx, timing);
    }
    return rx;
}

void LinuxSpiDevice::transferKeepChipSelect(
    const std::vector<uint8_t> &tx,
    std::vector<uint8_t> &rx,
    const SpiTransferTiming &timing
) const {
    const uint32_t speed = this->config.speed;
    for (std::size_t i = 0; i < tx.size(); ++i) {
        const bool first = i == 0;
        const bool last = i == tx.size() - 1;
        // cs_change on the last transfer of a message keeps chip select asserted until the next message,
        // the last byte of a multi-byte packet is followed by the shorter delay T1 (as in clibspi)
        const std::chrono::microseconds delay = last && !first ? timing.csSetupDelay : timing.interByteDelay;
        const spi_ioc_transfer byte = byteTransfer(&tx[i], &rx[i], speed, delay, !last);
        if (first) {
            const std::array message = {setupTransfer(speed, timing.csSetupDelay), byte};
            this->ioctl(SPI_IOC_MESSAGE(2), message.data(), "SPI transfer failed");
        } else {
            this->ioctl(SPI_IOC_MESSAGE(1), &byte, "SPI transfer failed");
        }
    }
}

void LinuxSpiDevice::transferToggleChipSelect(
    const std::vector<uint8_t> &tx,
    std::vector<uint8_t> &rx,
    const SpiTransferTiming &timing
) const {
    const uint32_t speed = this->config.speed;
    for (std::size_t i = 0; i < tx.size(); ++i) {
        // Chip select is deasserted after each byte
        const std::array message = {
            setupTransfer(speed, timing.csSetupDelay),
            byteTransfer(&tx[i], &rx[i], speed, timing.csSetupDelay, false),
        };
        this->ioctl(SPI_IOC_MESSAGE(2), message.data(), "SPI transfer failed");
        std::this_thread::sleep_for(timing.interByteDelay);
    }
}

}  // namespace iqrf::connector::spi
