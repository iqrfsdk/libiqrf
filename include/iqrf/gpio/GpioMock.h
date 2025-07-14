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

#include <cstdlib>
#include <cstdint>
#include <exception>
#include <functional>
#include <stdexcept>
#include <string>

#include "iqrf/gpio/Common.h"
#include "iqrf/gpio/Base.h"
#include "iqrf/gpio/Config.h"

namespace iqrf::gpio {

/**
 * Callback for GPIO direction change
 * @param old Old GPIO direction
 * @param new New GPIO direction
 * @internal
 */
typedef std::function<void(iqrf::gpio::GpioDirection, iqrf::gpio::GpioDirection)> GpioDirectionCallback;

/**
 * Callback for GPIO value change
 * @param old Old GPIO value
 * @param new New GPIO value
 * @internal
 */
typedef std::function<void(bool, bool)> GpioValueCallback;

/**
 * GPIO mock state
 * @internal
 */
enum class GpioMockState {
    /// GPIO line is not initialized
    Uninitialized,
    /// GPIO line is initialized
    Initialized,
};

/**
 * GPIO driver - mock for testing purposes
 * @internal
 */
class GpioMock: public Base {
 public:
    /**
     * Constructor
     * @param config GPIO configuration
     * @throws std::system_error for invalid chip name or busy GPIO line
     * @throws std::out_of_range for line offset out of bands
     */
    explicit GpioMock(GpioConfig  config);

    /**
     * Destructor
     */
    ~GpioMock() override = default;

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
     * Sets GPIO line input value for testing purposes
     * @param value GPIO line input value
     */
    void setInputValue(bool value);

    /**
     * Retrieves GPIO line input value
     * @return GPIO line input value
     */
    bool getValue() override;

    /**
     * Registers a callback for GPIO direction change
     * @param callback Callback function to be called when the GPIO direction changes
     */
    void registerDirectionCallback(const GpioDirectionCallback& callback) {
        this->directionCallback = callback;
    }

    /**
     * Registers a callback for GPIO value change
     * @param callback Callback function to be called when the GPIO value changes
     */
    void registerValueCallback(const GpioValueCallback& callback) {
        this->valueCallback = callback;
    }

 private:
    /// GPIO configuration
    GpioConfig config;
    /// GPIO line direction
    iqrf::gpio::GpioDirection direction = iqrf::gpio::GpioDirection::Input;
    /// GPIO line state
    GpioMockState state = GpioMockState::Uninitialized;
    /// GPIO line value
    bool value = false;

    /// Callback for GPIO direction change
    GpioDirectionCallback directionCallback = nullptr;
    /// Callback for GPIO value change
    GpioValueCallback valueCallback = nullptr;
};
}  // namespace iqrf::gpio
