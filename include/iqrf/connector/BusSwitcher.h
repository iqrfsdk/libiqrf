/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: BusSwitcher.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-07-13
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <optional>
#include <utility>

#include "iqrf/gpio/Gpio.h"

namespace iqrf::connector {

using iqrf::gpio::Gpio;

/**
 * Bus switch configuration for enabling/disabling buses.
 *
 * This class allows to configure GPIOs for enabling I2C, SPI, and UART buses,
 * as well as a general bus enable GPIO.
 */
class BusSwitcherConfig {
 public:
    /**
     * Default constructor for empty bus switch configuration.
     */
    BusSwitcherConfig() = default;

    /**
     * Constructs the bus switch configuration with optional GPIOs for enabling buses.
     * @param busEnableGpio GPIO to enable function of buses
     * @param i2cEnableGpio GPIO to enable function of I2C bus
     * @param spiEnableGpio GPIO to enable function of SPI bus
     * @param uartEnableGpio GPIO to enable function of UART bus
     */
    BusSwitcherConfig(
        const std::optional<Gpio> &busEnableGpio,
        const std::optional<Gpio> &i2cEnableGpio,
        const std::optional<Gpio> &spiEnableGpio,
        const std::optional<Gpio> &uartEnableGpio
    ): busEnableGpio(busEnableGpio),
       i2cEnableGpio(i2cEnableGpio),
       spiEnableGpio(spiEnableGpio),
       uartEnableGpio(uartEnableGpio) {}

    /**
     * Returns a copy of the bus switch configuration to avoid slicing.
     * @return A copy of the bus switch configuration
     */
    [[nodiscard]] BusSwitcherConfig busSwitch() const {
        return *this;
    }

    /// GPIO to enable function of buses
    std::optional<Gpio> busEnableGpio;
    /// GPIO to enable function of I2C bus
    std::optional<Gpio> i2cEnableGpio;
    /// GPIO to enable function of SPI bus
    std::optional<Gpio> spiEnableGpio;
    /// GPIO to enable function of UART bus
    std::optional<Gpio> uartEnableGpio;
};

class BusSwitcher {
 public:
    /**
     * Constructor for the bus switcher.
     * @param config Bus switch configuration
     */
    explicit BusSwitcher(BusSwitcherConfig config): config(std::move(config)) {
    }

    /**
     * Destructor for the bus switcher.
     */
    ~BusSwitcher() {
        this->toggle(false, false, false);
    }

    /**
     * Initializes the bus switch GPIOs to their default state.
     */
    void init() {
        if (this->config.busEnableGpio) {
            this->config.busEnableGpio->initOutput(false);
            return;
        }
        if (this->config.i2cEnableGpio) {
            this->config.i2cEnableGpio->initOutput(false);
        }
        if (this->config.spiEnableGpio) {
            this->config.spiEnableGpio->initOutput(false);
        }
        if (this->config.uartEnableGpio) {
            this->config.uartEnableGpio->initOutput(false);
        }
    }

    /**
     * Toggles the bus switch GPIOs based on the provided parameters.
     * @param i2c I2C bus enable state
     * @param spi SPI bus enable state
     * @param uart UART bus enable state
     */
    void toggle(bool i2c, bool spi, bool uart) {
        if (this->config.busEnableGpio) {
            this->config.busEnableGpio->setValue(i2c || spi || uart);
        } else {
            if (this->config.i2cEnableGpio) {
                this->config.i2cEnableGpio->setValue(i2c);
            }
            if (this->config.spiEnableGpio) {
                this->config.spiEnableGpio->setValue(spi);
            }
            if (this->config.uartEnableGpio) {
                this->config.uartEnableGpio->setValue(uart);
            }
        }
    }

    /**
     * Toggles the I2C bus switch GPIO.
     * @param enable true to enable I2C bus, false to disable it
     */
    void toggleI2c(bool enable) {
        this->toggle(enable, false, false);
    }

    /**
     * Toggles the SPI bus switch GPIO.
     * @param enable true to enable SPI bus, false to disable it
     */
    void toggleSpi(bool enable) {
        this->toggle(false, enable, false);
    }

    /**
     * Toggles the UART bus switch GPIO.
     * @param enable true to enable UART bus, false to disable it
     */
    void toggleUart(bool enable) {
        this->toggle(false, false, enable);
    }

 private:
    /// Bus switch configuration
    BusSwitcherConfig config;
};

}  // namespace iqrf::connector
