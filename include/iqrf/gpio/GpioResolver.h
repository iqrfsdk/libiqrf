/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: GpioResolver.h
 * Authors: Karel Hanák <karel.hanak@iqrf.com>
 * Date: 2024-09-29
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <iomanip>
#include <mutex>
#include <string>

#include "iqrf/gpio/Config.h"
#include "iqrf/gpio/GpioMap.h"

namespace iqrf::gpio {

class GpioResolver {
 private:
    /// GPIO resolver instance carrier
    static GpioResolver* gpioResolverInstance;
    /// GPIO resolver mutex
    static std::mutex gpioResolverMtx;
    /// Map of GPIO pin numbers and chip names / line offsets
    GpioMap gpioMap;

    /**
     * Constructor
     */
    GpioResolver();

    /**
     * Constructor with custom map
     */
    explicit GpioResolver(const GpioMap& map);

    /**
     * Destructor
     */
    ~GpioResolver();

 public:
    /**
     * Delete copy constructor
     */
    GpioResolver(const GpioResolver&) = delete;

    /**
     * Delete assignment operator
     */
    void operator=(const GpioResolver&) = delete;

    /**
     * Get resolver instance
     * @return GPIO resolver instance
     */
    static GpioResolver* GetResolver();

    /**
     * Get resolver instance (with manually passed GPIO map)
     * @param map Map of GPIO pins and chip names / line offsets
     * @return GPIO resolver instance
     */
    static GpioResolver* GetResolver(const GpioMap& map);

    /**
     * Resolves GPIO pin number to gpio chip name and line offset
     * @param pin GPIO pin number
     * @param chip GPIO chip name (contains resolved chip name)
     * @param line Line offset (contains resolved line offset)
     */
    void resolveGpioPin(int64_t pin, ::std::string& chip, ::std::size_t& line);

    /**
     * Dumps map of pin numbers and gpio chips / line numbers
     */
    void dump() const;
};

}  // namespace iqrf::gpio
