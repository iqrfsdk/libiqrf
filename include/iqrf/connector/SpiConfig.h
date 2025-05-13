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

#include "iqrf/gpio/Gpio.h"

namespace iqrf::connector::spi {

using iqrf::gpio::Gpio;

/**
 * IQRF SPI connector configuration
 */
class SpiConfig {
 public:
    /// GPIO to enable power supply to TR module
    Gpio powerEnableGpio;
    /// GPIO to enable function of SPI bus
    Gpio spiEnableGpio;
    /// GPIO to switch TR module to PGM mode
    std::optional<Gpio> pgmSwitchGpio;
    /// GPIO to enable function of UART bus (UART deactivated if set)
    std::optional<Gpio> uartEnableGpio;
    /// GPIO to enable function of I2C bus (I2C deactivated if set)
    std::optional<Gpio> i2cEnableGpioPin;

    /**
     * Constructs the minimal SPI connector configuration
     * @param powerEnableGpio GPIO to enable power supply to TR module
     * @param spiEnableGpio GPIO to enable function of SPI bus
     */
    SpiConfig(
        const Gpio &powerEnableGpio,
        const Gpio &spiEnableGpio
    ) :
        powerEnableGpio(powerEnableGpio),
        spiEnableGpio(spiEnableGpio) {}

    /**
     * Constructs the SPI connector configuration with programming mode
     * @param powerEnableGpio GPIO to enable power supply to TR module
     * @param spiEnableGpio GPIO to enable function of SPI bus
     * @param pgmSwitchGpio GPIO to switch TR module to PGM mode
     */
    SpiConfig(
        const Gpio &powerEnableGpio,
        const Gpio &spiEnableGpio,
        const Gpio &pgmSwitchGpio,
    ) :
        powerEnableGpio(powerEnableGpio),
        spiEnableGpio(spiEnableGpio),
        pgmSwitchGpio(pgmSwitchGpio) {}

    /**
     * Constructs the full SPI connector configuration
     * @param powerEnableGpio GPIO to enable power supply to TR module
     * @param spiEnableGpio GPIO to enable function of SPI bus
     * @param pgmSwitchGpio GPIO to switch TR module to PGM mode
     * @param uartEnableGpio GPIO to enable function of UART bus (UART deactivated if set)
     * @param i2cEnableGpioPin GPIO to enable function of I2C bus (I2C deactivated if set)
     */
    SpiConfig(
        const Gpio &powerEnableGpio,
        const Gpio &spiEnableGpio,
        const std::optional<Gpio> &pgmSwitchGpio,
        const std::optional<Gpio> &uartEnableGpio,
        const std::optional<Gpio> &i2cEnableGpioPin
    ) :
        powerEnableGpio(powerEnableGpio),
        spiEnableGpio(spiEnableGpio),
        pgmSwitchGpio(pgmSwitchGpio),
        uartEnableGpio(uartEnableGpio),
        i2cEnableGpioPin(i2cEnableGpioPin) {}
}

}
