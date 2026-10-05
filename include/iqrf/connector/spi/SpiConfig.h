/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: SpiConfig.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-05
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "iqrf/connector/BusSwitcher.h"
#include "iqrf/gpio/Gpio.h"

namespace iqrf::connector::spi {

using iqrf::gpio::Gpio;

/**
 * IQRF SPI communication mode
 */
enum class SpiCommunicationMode : uint8_t {
    /// Low speed mode - chip select is deasserted after each byte, longer delays
    LowSpeed,
    /// High speed mode - chip select is asserted during the whole packet
    HighSpeed,
};

/**
 * IQRF SPI connector configuration
 */
class SpiConfig: public iqrf::connector::BusSwitcherConfig {
 public:
    /// Maximum SPI clock speed supported by IQRF TR modules [Hz]
    static constexpr uint32_t MAX_SPEED = 250000;

    /// SPI device name (e.g. /dev/spidev0.0)
    std::string device;
    /// SPI clock speed [Hz]
    uint32_t speed = 250000;
    /// SPI communication mode
    SpiCommunicationMode communicationMode = SpiCommunicationMode::HighSpeed;
    /// GPIO to enable power supply to TR module
    std::optional<Gpio> powerEnableGpio;
    /// GPIO to switch TR module to PGM mode
    std::optional<Gpio> pgmSwitchGpio;
    /// Enable TR module reset during library initialization
    bool trModuleReset = true;
    /// Disable TR module power during connector destruction
    bool disablePowerOnShutdown = true;

    /**
     * Constructs the minimal SPI connector configuration
     * @param device SPI device name
     */
    explicit SpiConfig(std::string device) : device(std::move(device)) {}

    /**
     * Constructs the full SPI connector configuration
     * @param device SPI device name
     * @param powerEnableGpio GPIO to enable power supply to TR module
     * @param busEnableGpio GPIO to enable function of buses
     * @param pgmSwitchGpio GPIO to switch TR module to PGM mode
     * @param spiEnableGpio GPIO to enable function of SPI bus
     * @param uartEnableGpio GPIO to enable function of UART bus
     * @param i2cEnableGpio GPIO to enable function of I2C bus
     * @param trModuleReset Enable TR module reset during library initialization
     * @param disablePowerOnShutdown Disable TR power on shutdown
     */
    SpiConfig(
        std::string device,
        const std::optional<Gpio> &powerEnableGpio,
        const std::optional<Gpio> &busEnableGpio,
        const std::optional<Gpio> &pgmSwitchGpio,
        const std::optional<Gpio> &spiEnableGpio,
        const std::optional<Gpio> &uartEnableGpio,
        const std::optional<Gpio> &i2cEnableGpio,
        const bool trModuleReset,
        const bool disablePowerOnShutdown
    ) :
        BusSwitcherConfig(
            busEnableGpio,
            i2cEnableGpio,
            spiEnableGpio,
            uartEnableGpio
        ),
        device(std::move(device)),
        powerEnableGpio(powerEnableGpio),
        pgmSwitchGpio(pgmSwitchGpio),
        trModuleReset(trModuleReset),
        disablePowerOnShutdown(disablePowerOnShutdown) {
    }
};

}  // namespace iqrf::connector::spi
