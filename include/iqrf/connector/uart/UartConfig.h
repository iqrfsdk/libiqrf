/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: UartConfig.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-10
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

namespace iqrf::connector::uart {

using iqrf::gpio::Gpio;

/**
 * IQRF UART connector configuration
 */
class UartConfig: public iqrf::connector::BusSwitcherConfig {
 public:
    /// UART device name
    std::string device;
    /// UART baud rate
    uint32_t baudRate = 115200;
    /// GPIO to enable power supply to TR module
    std::optional<Gpio> powerEnableGpio;
    /// GPIO to switch TR module to PGM mode
    std::optional<Gpio> pgmSwitchGpio;
    /// Enable TR module reset during library initialization
    bool trModuleReset = true;
    /// Disable TR module power during connector destruction
    bool disablePowerOnShutdown = true;

    /**
     * Constructs the minimal UART connector configuration
     * @param device UART device name
     * @param baudRate UART baud rate
     */
    explicit UartConfig(
        std::string device,
        const uint32_t baudRate = 115200
    ) : device(std::move(device)), baudRate(baudRate) {}

    /**
     * Constructs the full UART connector configuration
     * @param device UART device name
     * @param baudRate UART baud rate
     * @param powerEnableGpio GPIO to enable power supply to TR module
     * @param busEnableGpio GPIO to enable function of buses
     * @param pgmSwitchGpio GPIO to switch TR module to PGM mode
     * @param spiEnableGpio GPIO to enable function of SPI bus
     * @param uartEnableGpio GPIO to enable function of UART bus
     * @param i2cEnableGpio GPIO to enable function of I2C bus
     * @param trModuleReset Enable TR module reset during library initialization
     * @param disablePowerOnShutdown Disable TR power on shutdown
     */
    UartConfig(
        std::string device,
        const uint32_t baudRate,
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
        baudRate(baudRate),
        powerEnableGpio(powerEnableGpio),
        pgmSwitchGpio(pgmSwitchGpio),
        trModuleReset(trModuleReset),
        disablePowerOnShutdown(disablePowerOnShutdown)
    {
        std::cout << "UART CONFIG CONSTRUCTOR" << std::endl;
    }
};

}  // namespace iqrf::connector::uart
