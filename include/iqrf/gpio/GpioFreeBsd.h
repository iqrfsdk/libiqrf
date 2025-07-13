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

#include <fcntl.h>
#include <unistd.h>
#include <sys/gpio.h>

#include <cstdlib>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>

#include "iqrf/gpio/Common.h"
#include "iqrf/gpio/Base.h"
#include "iqrf/gpio/Config.h"

namespace iqrf::gpio {

/**
 * GPIO driver - FreeBSD
 */
class GpioFreeBsd: public Base {
 public:
    /**
     * Constructor
     * @param config GPIO configuration
     * @throws std::system_error for invalid chip name or busy GPIO line
     * @throws std::out_of_range for line offset out of bands
     */
    explicit GpioFreeBsd(const GpioConfig& config);

    /**
     * Destructor
     */
    ~GpioFreeBsd() override;

    /**
     * Initializes GPIO line as an input
     */
    void initInput() override;

    /**
     * Initializes GPIO line as an output
     * @param initialValue Initial output value
     */
    void initOutput(bool initialValue) override;

    /**
     * Sets GPIO line direction
     * @param direction GPIO line direction
     */
    void setDirection(iqrf::gpio::GpioDirection direction) override;

    /**
     * Retrieves GPIO line direction
     * @return GPIO line direction
     */
    iqrf::gpio::GpioDirection getDirection() override;

    /**
     * Sets GPIO line output value
     * @param value GPIO line output value
     */
    void setValue(bool value) override;

    /**
     * Retrieves GPIO line input value
     * @return GPIO line input value
     */
    bool getValue() override;

 private:
    /**
     * Retrieves the GPIO pin configuration
     * @return GPIO pin configuration
     */
    struct gpio_pin getPinConfig() const;

    /**
     * Sets the consumer name for the GPIO line
     * @param name Consumer name
     * @throws std::system_error if setting the consumer name fails
     */
    void setConsumerName(const std::string &name) const;

    /// File descriptor for the GPIO chip
    mutable int fd = -1;
    /// GPIO line
    uint32_t line;
};
}  // namespace iqrf::gpio
