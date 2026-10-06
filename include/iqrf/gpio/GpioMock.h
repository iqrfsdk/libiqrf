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

#include <cstdint>
#include <functional>
#include <mutex>

#include "iqrf/gpio/Base.h"
#include "iqrf/gpio/Common.h"
#include "iqrf/gpio/Config.h"

namespace iqrf::gpio {

/**
 * Callback for GPIO direction change
 * @param old Old GPIO direction
 * @param new New GPIO direction
 */
using GpioDirectionCallback = std::function<void(iqrf::gpio::GpioDirection, iqrf::gpio::GpioDirection)>;

/**
 * Callback for GPIO value change
 * @param old Old GPIO value
 * @param new New GPIO value
 */
using GpioValueCallback = std::function<void(bool, bool)>;

/**
 * Callback for GPIO value write
 * @param value Written GPIO value
 */
using GpioWriteCallback = std::function<void(bool)>;

/**
 * GPIO mock state
 */
enum class GpioMockState : uint8_t {
    /// GPIO line is not initialized
    Uninitialized,
    /// GPIO line is initialized
    Initialized,
};

/**
 * GPIO driver - mock for testing purposes
 *
 * The mock is provided by the iqrf_gpio_mock library (built with BUILD_TESTING_SUPPORT option). It can be injected
 * into the code under test via Gpio constructor, the test keeps a pointer to the mock to control and observe it:
 *
 * @code
 * auto mock = std::make_shared<iqrf::gpio::GpioMock>(iqrf::gpio::GpioConfig("gpiochip0", 1, "power"));
 * mock->registerValueCallback([](bool oldValue, bool newValue) { ... });
 * iqrf::gpio::Gpio gpio(mock);
 * @endcode
 *
 * All methods are thread-safe, callbacks are called outside of the internal lock.
 */
class GpioMock: public Base {
 public:
    /**
     * Constructor
     */
    GpioMock() = default;

    /**
     * Constructor
     * @param config GPIO configuration (used only for identification of the mock)
     */
    explicit GpioMock(GpioConfig config);

    /**
     * Destructor
     */
    ~GpioMock() override = default;

    // The mock represents a single GPIO line, disable copying and moving
    GpioMock(const GpioMock&) = delete;
    GpioMock& operator=(const GpioMock&) = delete;
    GpioMock(GpioMock&&) = delete;
    GpioMock& operator=(GpioMock&&) = delete;

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
     * @param newDirection GPIO line direction
     * @throws std::runtime_error if the GPIO line is not initialized
     */
    void setDirection(iqrf::gpio::GpioDirection newDirection) override;

    /**
     * Retrieves GPIO line direction
     * @return GPIO line direction
     * @throws std::runtime_error if the GPIO line is not initialized
     */
    iqrf::gpio::GpioDirection getDirection() override;

    /**
     * Sets GPIO line output value
     * @param newValue GPIO line output value
     * @throws std::runtime_error if the GPIO line is not initialized or is not an output
     */
    void setValue(bool newValue) override;

    /**
     * Retrieves GPIO line value
     * @return GPIO line value
     * @throws std::runtime_error if the GPIO line is not initialized
     */
    bool getValue() override;

    /**
     * Simulates GPIO line input value
     * @param newValue GPIO line input value
     * @throws std::runtime_error if the GPIO line is not an input
     */
    void setInputValue(bool newValue);

    /**
     * Returns GPIO mock state
     * @return GPIO mock state
     */
    [[nodiscard]] GpioMockState getState() const;

    /**
     * Returns GPIO configuration the mock was created with
     * @return GPIO configuration
     */
    [[nodiscard]] const GpioConfig& getConfig() const;

    /**
     * Registers a callback for GPIO direction change
     * @param callback Callback function to be called when the GPIO direction changes
     */
    void registerDirectionCallback(const GpioDirectionCallback& callback);

    /**
     * Registers a callback for GPIO value change
     * @param callback Callback function to be called when the GPIO output value changes
     */
    void registerValueCallback(const GpioValueCallback& callback);

    /**
     * Registers a callback for every GPIO value write
     *
     * Unlike the value change callback, the callback is called for every setValue() call, including writes of
     * the current value, which is needed to verify bit-banged protocols. The initial value set by initOutput()
     * is not reported.
     *
     * @param callback Callback function to be called when the GPIO output value is written
     */
    void registerWriteCallback(const GpioWriteCallback& callback);

 private:
    /**
     * Checks that the GPIO line is initialized
     * @throws std::runtime_error if the GPIO line is not initialized
     */
    void checkInitialized() const;

    /// Guards the mock state
    mutable std::mutex mutex;
    /// GPIO configuration
    const GpioConfig config;
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
    /// Callback for GPIO value write
    GpioWriteCallback writeCallback = nullptr;
};

}  // namespace iqrf::gpio
