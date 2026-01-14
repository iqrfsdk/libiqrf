/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: GpioMockTest.cpp
 * Authors: Ondřej Hujňák <ondrej.hujnak@iqrf.com>, Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-06-21
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gtest/gtest.h>

#include "iqrf/gpio/Gpio.h"
#include "iqrf/gpio/Config.h"

namespace iqrf::gpio {

#if IQRF_TESTING_SUPPORT
class GpioMockTest : public ::testing::Test {
 protected:
    /**
     * Set up the test environment.
     */
    void SetUp() override {
        this->config.use_mock = true;
    }

    // Create Gpio instance
    GpioConfig config = GpioConfig("gpiochip0", 1, "test:mock");
};

TEST_F(GpioMockTest, copyConstructor) {
    // Create Gpio instance
    auto orig = Gpio(this->config);

    // Create copy of the Gpio instance
    auto copy = Gpio(orig);

    EXPECT_NE(&copy, &orig);
    EXPECT_EQ(orig.impl, copy.impl);
}

TEST_F(GpioMockTest, moveConstructor) {
    // Create Gpio instance
    auto orig = Gpio(config);

    // Store the original pointer for later checking
    const auto orig_impl = orig.impl;

    // Move the Gpio instance
    auto other = std::move(orig);

    EXPECT_NE(&other, &orig);
    EXPECT_EQ(orig_impl, other.impl);
    EXPECT_EQ(orig.impl, nullptr);
}

TEST_F(GpioMockTest, assignmentOperator) {
    // Create Gpio instance
    const auto orig = Gpio(config);

    // Assign the Gpio instance
    const auto &other = orig;

    EXPECT_EQ(orig.impl, other.impl);
}

TEST_F(GpioMockTest, swap) {
    // Create Gpio instances
    auto gpio1 = Gpio(config);
    auto config2 = GpioConfig("gpiochip0", 2, "test:mock2");
    config2.use_mock = true;
    auto gpio2 = Gpio(config2);
    const auto expected1 = gpio2.impl;
    const auto expected2 = gpio1.impl;

    // Swap the instances
    swap(gpio1, gpio2);

    EXPECT_EQ(gpio1.impl, expected1);
    EXPECT_EQ(gpio2.impl, expected2);
}

TEST_F(GpioMockTest, uninitializedGpio) {
    Gpio gpio(this->config);
    EXPECT_THROW(gpio.setValue(true), std::runtime_error);
    EXPECT_THROW(gpio.getValue(), std::runtime_error);
    EXPECT_THROW(gpio.getDirection(), std::runtime_error);
    EXPECT_THROW(gpio.setDirection(iqrf::gpio::GpioDirection::Input), std::runtime_error);
}

TEST_F(GpioMockTest, initInput) {
    Gpio gpio(this->config);
    gpio.setInputValue(true);
    EXPECT_NO_THROW(gpio.initInput());
    EXPECT_EQ(gpio.getDirection(), GpioDirection::Input);
    EXPECT_TRUE(gpio.getValue());
}

TEST_F(GpioMockTest, initOutput) {
    const Gpio gpio(this->config);
    EXPECT_NO_THROW(gpio.initOutput(true));
    EXPECT_EQ(gpio.getDirection(), GpioDirection::Output);
    EXPECT_TRUE(gpio.getValue());

    gpio.setValue(false);
    EXPECT_FALSE(gpio.getValue());
}

TEST_F(GpioMockTest, setDirection) {
    const Gpio gpio(this->config);
    gpio.initOutput(true);
    EXPECT_NO_THROW(gpio.setDirection(GpioDirection::Input));
    EXPECT_EQ(gpio.getDirection(), GpioDirection::Input);
    EXPECT_NO_THROW(gpio.setDirection(GpioDirection::Output));
    EXPECT_EQ(gpio.getDirection(), GpioDirection::Output);
}

TEST_F(GpioMockTest, registerDirectionCallback) {
    Gpio gpio(this->config);
    bool directionChanged = false;
    std::optional<GpioDirection> previousDirection;
    std::optional<GpioDirection> newDirection;

    // Register a callback that sets the flag when the direction changes
    gpio.registerDirectionCallback([&directionChanged, &previousDirection, &newDirection](GpioDirection prev, GpioDirection next) {
        directionChanged = true;
        previousDirection = prev;
        newDirection = next;
    });

    gpio.initInput();
    EXPECT_FALSE(directionChanged);

    gpio.setDirection(GpioDirection::Output);
    EXPECT_TRUE(directionChanged);
    EXPECT_EQ(previousDirection, GpioDirection::Input);
    EXPECT_EQ(newDirection, GpioDirection::Output);
}
#endif

TEST_F(GpioMockTest, registerValueCallback) {
    Gpio gpio(this->config);
    bool valueChanged = false;
    std::optional<bool> previousValue;
    std::optional<bool> newValue;

    // Register a callback that sets the flag when the value changes
    gpio.registerValueCallback([&valueChanged, &previousValue, &newValue](bool prev, bool next) {
        valueChanged = true;
        previousValue = prev;
        newValue = next;
    });

    gpio.initOutput(true);
    EXPECT_FALSE(valueChanged);

    gpio.setValue(false);
    EXPECT_TRUE(valueChanged);
    EXPECT_EQ(previousValue, true);
    EXPECT_EQ(newValue, false);
}

}  // namespace iqrf::gpio
