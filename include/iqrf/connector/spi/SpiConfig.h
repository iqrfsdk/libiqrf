/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: SpiConfig.h
 * Authors: Ondřej Hujňák <ondrej.hujnak@iqrf.org>
 * Date: 2025-05-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#pragma once

#include <optional>
#include <string>
#include <utility>

#include "iqrf/connector/BusSwitcher.h"
#include "iqrf/gpio/Gpio.h"

namespace iqrf::connector::spi {

using iqrf::gpio::Gpio;

/**
 * IQRF SPI connector configuration
 */
class SpiConfig : public iqrf::connector::BusSwitcherConfig {
 public:
    /// System path to SPI device
    std::string interface;

    /// TR module reset during initialization (true by default)
    bool trModuleReset;

    /// GPIO to enable power supply to TR module
    std::optional<Gpio> powerEnableGpio;
    /// GPIO to switch TR module to PGM mode
    std::optional<Gpio> pgmSwitchGpio;

    /**
     * Constructs the minimal SPI connector configuration
     * @param interface is system path to an SPI device
     * @param trModuleReset whether the TR module should be reset during initialization
     */
    explicit SpiConfig(std::string interface, const bool trModuleReset = true)
            : interface(std::move(interface)), trModuleReset(trModuleReset) {}

    /**
     * Constructs the full SPI connector configuration
     * @param interface is system path to an SPI device
     * @param powerEnableGpio GPIO to enable power supply to TR module
     * @param spiEnableGpio GPIO to enable function of SPI bus
     * @param pgmSwitchGpio GPIO to switch TR module to PGM mode
     * @param uartEnableGpio GPIO to enable function of UART bus (UART deactivated if set)
     * @param i2cEnableGpio GPIO to enable function of I2C bus (I2C deactivated if set)
     * @param trModuleReset whether the TR module should be reset during initialization
     */
    SpiConfig(
        std::string interface,
        const std::optional<Gpio> &powerEnableGpio,
        const std::optional<Gpio> &busEnableGpio,
        const std::optional<Gpio> &spiEnableGpio,
        const std::optional<Gpio> &pgmSwitchGpio,
        const std::optional<Gpio> &uartEnableGpio,
        const std::optional<Gpio> &i2cEnableGpio,
        const bool trModuleReset = true
    ) :
        BusSwitcherConfig(
            busEnableGpio,
            i2cEnableGpio,
            spiEnableGpio,
            uartEnableGpio
        ),
        interface(std::move(interface)),
        trModuleReset(trModuleReset),
        powerEnableGpio(powerEnableGpio),
        pgmSwitchGpio(pgmSwitchGpio) {}
};

}  // namespace iqrf::connector::spi
