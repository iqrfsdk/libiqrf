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

#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "iqrf/gpio/Config.h"
#include "iqrf/gpio/Gpio.h"
#include "iqrf/gpio/GpioMock.h"

namespace iqrf::gpio {

class GpioMockTest : public ::testing::Test {
 protected:
    /// GPIO mock
    std::shared_ptr<GpioMock> mock = std::make_shared<GpioMock>(GpioConfig("gpiochip0", 1, "test:mock"));
    /// GPIO using the mock
    Gpio gpio = Gpio(mock);
};

TEST_F(GpioMockTest, nullDriver) {
    EXPECT_THROW(Gpio(std::shared_ptr<Base>()), std::invalid_argument);
}

TEST_F(GpioMockTest, config) {
    EXPECT_EQ(this->mock->getConfig().chip, "gpiochip0");
    EXPECT_EQ(this->mock->getConfig().line, 1);
    EXPECT_EQ(this->mock->getConfig().consumer_name, "test:mock");
    EXPECT_EQ(GpioMock().getConfig().chip, "");
}

TEST_F(GpioMockTest, copySharesDriver) {
    const Gpio gpioCopy(this->gpio);
    gpioCopy.initOutput(false);
    this->gpio.setValue(true);
    EXPECT_TRUE(gpioCopy.getValue());
    EXPECT_EQ(this->mock->getState(), GpioMockState::Initialized);
}

TEST_F(GpioMockTest, moveKeepsDriver) {
    const Gpio moved(std::move(this->gpio));
    moved.initOutput(true);
    EXPECT_TRUE(this->mock->getValue());
}

TEST_F(GpioMockTest, assignmentSharesDriver) {
    Gpio other(std::make_shared<GpioMock>());
    other = this->gpio;
    other.initOutput(true);
    EXPECT_TRUE(this->mock->getValue());
}

TEST_F(GpioMockTest, swap) {
    auto mock2 = std::make_shared<GpioMock>();
    Gpio gpio2(mock2);
    this->gpio.initOutput(true);
    gpio2.initOutput(false);

    swap(this->gpio, gpio2);

    EXPECT_FALSE(this->gpio.getValue());
    EXPECT_TRUE(gpio2.getValue());
}

TEST_F(GpioMockTest, uninitializedGpio) {
    EXPECT_EQ(this->mock->getState(), GpioMockState::Uninitialized);
    EXPECT_THROW(this->gpio.setValue(true), std::runtime_error);
    EXPECT_THROW(static_cast<void>(this->gpio.getValue()), std::runtime_error);
    EXPECT_THROW(static_cast<void>(this->gpio.getDirection()), std::runtime_error);
    EXPECT_THROW(this->gpio.setDirection(iqrf::gpio::GpioDirection::Input), std::runtime_error);
}

TEST_F(GpioMockTest, initInput) {
    this->mock->setInputValue(true);
    EXPECT_NO_THROW(this->gpio.initInput());
    EXPECT_EQ(this->gpio.getDirection(), GpioDirection::Input);
    EXPECT_TRUE(this->gpio.getValue());
    EXPECT_THROW(this->gpio.setValue(false), std::runtime_error);
}

TEST_F(GpioMockTest, initOutput) {
    EXPECT_NO_THROW(this->gpio.initOutput(true));
    EXPECT_EQ(this->gpio.getDirection(), GpioDirection::Output);
    EXPECT_TRUE(this->gpio.getValue());

    this->gpio.setValue(false);
    EXPECT_FALSE(this->gpio.getValue());
    EXPECT_THROW(this->mock->setInputValue(true), std::runtime_error);
}

TEST_F(GpioMockTest, setDirection) {
    this->gpio.initOutput(true);
    EXPECT_NO_THROW(this->gpio.setDirection(GpioDirection::Input));
    EXPECT_EQ(this->gpio.getDirection(), GpioDirection::Input);
    EXPECT_NO_THROW(this->gpio.setDirection(GpioDirection::Output));
    EXPECT_EQ(this->gpio.getDirection(), GpioDirection::Output);
}

TEST_F(GpioMockTest, registerDirectionCallback) {
    std::optional<GpioDirection> previousDirection;
    std::optional<GpioDirection> newDirection;
    this->mock->registerDirectionCallback([&previousDirection, &newDirection](GpioDirection prev, GpioDirection next) {
        previousDirection = prev;
        newDirection = next;
    });

    this->gpio.initInput();
    EXPECT_FALSE(newDirection.has_value());

    this->gpio.setDirection(GpioDirection::Output);
    EXPECT_EQ(previousDirection, GpioDirection::Input);
    EXPECT_EQ(newDirection, GpioDirection::Output);
}

TEST_F(GpioMockTest, registerValueCallback) {
    std::optional<bool> previousValue;
    std::optional<bool> newValue;
    this->mock->registerValueCallback([&previousValue, &newValue](bool prev, bool next) {
        previousValue = prev;
        newValue = next;
    });

    this->gpio.initOutput(true);
    EXPECT_FALSE(newValue.has_value());

    this->gpio.setValue(false);
    EXPECT_EQ(previousValue, true);
    EXPECT_EQ(newValue, false);
}

TEST_F(GpioMockTest, registerWriteCallback) {
    std::vector<bool> writes;
    std::vector<bool> changes;
    this->mock->registerWriteCallback([&writes](bool value) {
        writes.push_back(value);
    });
    this->mock->registerValueCallback([&changes](bool, bool next) {
        changes.push_back(next);
    });

    // Initial value is not reported
    this->gpio.initOutput(false);
    EXPECT_TRUE(writes.empty());

    // Every write is reported, including writes of the current value
    this->gpio.setValue(false);
    this->gpio.setValue(true);
    this->gpio.setValue(true);
    this->gpio.setValue(false);
    EXPECT_EQ(writes, std::vector<bool>({false, true, true, false}));
    EXPECT_EQ(changes, std::vector<bool>({true, false}));

    // Rejected write is not reported
    this->gpio.setDirection(GpioDirection::Input);
    EXPECT_THROW(this->gpio.setValue(true), std::runtime_error);
    EXPECT_EQ(writes.size(), 4);
}

TEST_F(GpioMockTest, callbackCanAccessMock) {
    // Callbacks are called outside of the internal lock, so they can query the mock
    std::optional<bool> valueInCallback;
    this->mock->registerValueCallback([this, &valueInCallback](bool, bool) {
        valueInCallback = this->mock->getValue();
    });
    this->gpio.initOutput(false);
    this->gpio.setValue(true);
    EXPECT_EQ(valueInCallback, true);
}

TEST_F(GpioMockTest, concurrentAccess) {
    this->gpio.initOutput(false);
    std::vector<std::thread> threads;
    threads.reserve(4);
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, i] {
            for (int j = 0; j < 1000; ++j) {
                this->gpio.setValue(((i + j) % 2) == 0);
                static_cast<void>(this->gpio.getValue());
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_EQ(this->mock->getState(), GpioMockState::Initialized);
}

}  // namespace iqrf::gpio
