/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: IConnector.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-10
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <libserialport.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "iqrf/gpio/Gpio.h"

namespace iqrf::connector::uart {

using iqrf::gpio::Gpio;

/**
 * IQRF UART connector configuration
 */
class UartConfig {
 public:
    /// UART device name
    std::string device;
    /// UART baud rate
    uint32_t baudRate = 115200;
    /// GPIO to enable power supply to TR module
    std::optional<Gpio> powerEnableGpio;
    /// GPIO to enable function of buses
    std::optional<Gpio> busEnableGpio;
    /// GPIO to switch TR module to PGM mode
    std::optional<Gpio> pgmSwitchGpio;
    /// GPIO to enable function of SPI bus
    std::optional<Gpio> spiEnableGpio;
    /// GPIO to enable function of UART bus
    std::optional<Gpio> uartEnableGpio;
    /// GPIO to enable function of I2C bus
    std::optional<Gpio> i2cEnableGpio;
    /// Enable TR module reset during library initialization
    bool trModuleReset = true;

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
        const bool trModuleReset
    ) :
        device(std::move(device)),
        baudRate(baudRate),
        powerEnableGpio(powerEnableGpio),
        busEnableGpio(busEnableGpio),
        pgmSwitchGpio(pgmSwitchGpio),
        spiEnableGpio(spiEnableGpio),
        uartEnableGpio(uartEnableGpio),
        i2cEnableGpio(i2cEnableGpio),
        trModuleReset(trModuleReset) {}
};

}  // namespace iqrf::connector::uart
