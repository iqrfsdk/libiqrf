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

#include <memory>
#include <utility>

#include "iqrf/gpio/Base.h"
#include "iqrf/gpio/Common.h"
#include "iqrf/gpio/Config.h"

namespace iqrf::gpio {

/**
 * GPIO pin
 *
 * Copies of the object share the same GPIO driver instance (and thus the same GPIO line).
 */
// The by-value assignment operator (copy-and-swap) handles both copy and move assignment
class Gpio {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
    /**
     * Constructs GPIO pin using the GPIO driver for the current platform
     * @param config GPIO pin configuration
     * @throws std::runtime_error if the platform is not supported
     */
    explicit Gpio(const GpioConfig& config);

    /**
     * Constructs GPIO pin using the specified GPIO driver (e.g. GpioMock for testing)
     * @param impl GPIO driver
     * @throws std::invalid_argument if the GPIO driver is null
     */
    explicit Gpio(std::shared_ptr<iqrf::gpio::Base> impl);

    /**
     * Copy Constructor
     * @param other Gpio object to copy
     */
    Gpio(const Gpio& other) noexcept = default;

    /**
     * Move Constructor
     * @param other Gpio object to move
     */
    Gpio(Gpio&& other) noexcept = default;

    /**
     * Destructor
     */
    ~Gpio() = default;

    /**
     * Assignment Operator
     */
    Gpio& operator=(Gpio other) noexcept;

    /**
     * Initializes GPIO pin as an input
     */
    void initInput() const;

    /**
     * Initializes GPIO pin as an output
     * @param initialValue Initial output value
     */
    void initOutput(bool initialValue) const;

    /**
     * Sets GPIO pin direction
     * @param direction GPIO pin direction
     */
    void setDirection(iqrf::gpio::GpioDirection direction) const;

    /**
     * Retrieves GPIO pin direction
     * @return GPIO pin direction
     */
    [[nodiscard]] iqrf::gpio::GpioDirection getDirection() const;

    /**
     * Sets GPIO pin output value
     * @param value GPIO pin output value
     */
    void setValue(bool value) const;

    /**
     * Retrieves GPIO line input value
     * @return GPIO line input value
     */
    [[nodiscard]] bool getValue() const;

    /**
     * Swap function
     */
    friend void swap(Gpio& first, Gpio& second) noexcept;

 private:
    /// GPIO driver instance
    std::shared_ptr<iqrf::gpio::Base> impl;
};

}  // namespace iqrf::gpio
