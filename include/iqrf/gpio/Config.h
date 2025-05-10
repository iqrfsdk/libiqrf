/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <iostream>
#include <sstream>
#include <string>

#include "GpioResolver.h"

namespace iqrf::gpio {

inline const std::string default_consumer_name = "libiqrf";

/**
 * GPIO chip config
 */
class GpioConfig {
 public:
    /// GPIO chip name
    ::std::string chip;
    /// GPIO line offset
    ::std::size_t line;
    /// GPIO line name
    ::std::string line_name;
    /// GPIO pin number (sysfs compatibility)
    int64_t pin;

    /// Consumer name for better identification
    ::std::string consumer_name;

    /**
     * Empty constructor for filling up the data later
     */
    GpioConfig();

    /**
     * Constructor
     * @param chip GPIO chip name
     * @param line GPIO line offset
     * @param consumer_name Name of the GPIO line consumer
     */
    GpioConfig(const ::std::string& chip,
               ::std::size_t line,
               const ::std::string& consumer_name = default_consumer_name);

    /**
     * Constructor with named line
     * @param chip GPIO chip name
     * @param line_name GPIO line name
     * @param consumer_name Name of the GPIO line consumer
     */
    GpioConfig(const ::std::string& chip,
               const ::std::string& line_name,
               const ::std::string& consumer_name = default_consumer_name);

    /**
     * Compatibility constructor for sysfs
     * @param pin GPIO pin number
     * @param consumer_name Name of the GPIO pin consumer
     */
    explicit GpioConfig(int64_t pin, const ::std::string& consumer_name = default_consumer_name);

    /**
     * Returns textual representation of the configuration.
     */
    ::std::string to_string() const;
};

}  // namespace iqrf::gpio
