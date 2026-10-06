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

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>

#include "iqrf/gpio/GpioMap.h"

namespace iqrf::gpio {

/**
 * Resolves GPIO pin numbers (sysfs compatibility) to GPIO chip names and line offsets
 *
 * The resolver is a process-wide singleton. The map of the system GPIO chips is loaded lazily on the first use,
 * a custom map can be set instead (e.g. for testing).
 */
class GpioResolver {
 public:
    /**
     * Delete copy constructor
     */
    GpioResolver(const GpioResolver&) = delete;

    /**
     * Delete assignment operator
     */
    void operator=(const GpioResolver&) = delete;

    // The resolver is a singleton, disable moving
    GpioResolver(GpioResolver&&) = delete;
    GpioResolver& operator=(GpioResolver&&) = delete;

    /**
     * Destructor
     */
    ~GpioResolver() = default;

    /**
     * Get resolver instance
     * @return GPIO resolver instance
     */
    static GpioResolver* GetResolver();

    /**
     * Get resolver instance and replace its GPIO map
     *
     * The map is used by all subsequent pin resolutions, including GpioConfig constructed from pin number.
     *
     * @param map Map of GPIO pins and chip names / line offsets
     * @return GPIO resolver instance
     */
    static GpioResolver* GetResolver(const GpioMap& map);

    /**
     * Replaces the GPIO map
     * @param map Map of GPIO pins and chip names / line offsets
     */
    void setGpioMap(GpioMap map);

    /**
     * Resolves GPIO pin number to gpio chip name and line offset
     * @param pin GPIO pin number
     * @param chip GPIO chip name (contains resolved chip name)
     * @param line Line offset (contains resolved line offset)
     * @throws std::runtime_error if the pin is not found
     */
    void resolveGpioPin(int64_t pin, ::std::string& chip, ::std::size_t& line);  // NOLINT(runtime/references)

    /**
     * Dumps map of pin numbers and gpio chips / line numbers
     */
    void dump() const;

 private:
    /**
     * Constructor
     */
    GpioResolver() = default;

    /**
     * Returns the GPIO map, loads the map of the system GPIO chips if no map is set
     * @return GPIO map
     */
    const GpioMap& getMap() const;

    /// Guards the GPIO map
    mutable std::mutex mutex;
    /// Map of GPIO pin numbers and chip names / line offsets
    mutable std::optional<GpioMap> gpioMap;
};

}  // namespace iqrf::gpio
