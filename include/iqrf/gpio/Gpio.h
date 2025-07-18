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

#if defined(__linux__)
#include <gpiod.hpp>
#endif
#if BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

#include "iqrf/gpio/Base.h"
#include "iqrf/gpio/Common.h"
#include "iqrf/gpio/Config.h"

#if IQRF_TESTING_SUPPORT
#include "iqrf/gpio/GpioMock.h"
#endif

#if defined(__linux__)
#if libgpiod_VERSION_MAJOR == 1
#include "iqrf/gpio/GpiodV1.h"
#else
#include "iqrf/gpio/GpiodV2.h"
#endif
#elif defined(__FreeBSD__)
#include "iqrf/gpio/GpioFreeBsd.h"
#endif

namespace iqrf::gpio {

/**
 * GPIO pin
 */
class Gpio {
 public:
    /**
     * Constructor
     * @param config GPIO pin configuration
     */
    explicit Gpio(const GpioConfig& config);

    /**
     * Copy Constructor
     * @param other Gpio object to copy
     */
    Gpio(const Gpio& other) noexcept;

    /**
     * Move Constructor
     * @param other Gpio object to move
     */
    Gpio(Gpio&& other) noexcept;

    /**
     * Destructor
     */
    ~Gpio();

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

#if IQRF_TESTING_SUPPORT
    /**
     * Sets GPIO line input value for testing purposes
     * @param value GPIO line input value
     */
    void setInputValue(bool value);

    /**
     * Registers a callback for GPIO direction change
     * @param callback Callback function to be called when the GPIO direction changes
     */
    void registerDirectionCallback(const GpioDirectionCallback& callback);

    /**
     * Registers a callback for GPIO value change
     * @param callback Callback function to be called when the GPIO value changes
     */
    void registerValueCallback(const GpioValueCallback& callback);
#endif

 private:
    /// GPIO driver instance
    std::shared_ptr<iqrf::gpio::Base> impl;
    /// Mock flag
    bool isMock = false;

#if BUILD_TESTS
    /// Testing friends
    FRIEND_TEST(GpioTest, VerifyCopyConstructor_GPIO);
    FRIEND_TEST(GpioTest, VerifyMoveConstructor_GPIO);
    FRIEND_TEST(GpioTest, VerifyAssignmentOperator_GPIO);
    FRIEND_TEST(GpioMockTest, copyConstructor);
    FRIEND_TEST(GpioMockTest, moveConstructor);
    FRIEND_TEST(GpioMockTest, assignmentOperator);
    FRIEND_TEST(GpioMockTest, swap);
#endif
};

}  // namespace iqrf::gpio
