/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: BusSwitcherTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-16
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <map>
#include <string>
#include <vector>
#include <thread>

#include "iqrf/connector/BusSwitcher.h"

namespace iqrf::connector {

/**
 * GPIO value change event type.
 */
typedef std::tuple<std::chrono::milliseconds, std::string, bool, bool> GpioValueChangeEvent;

class BusSwitcherTest : public ::testing::Test {
 protected:
    /**
     * Sets up the test fixture by clearing the history of GPIO changes.
     */
    void SetUp() override {
        this->history.clear();
    }

    /**
     * Creates a mock GPIO for testing purposes.
     * @param config Configuration for the GPIO pin.
     * @return A mock Gpio instance.
     */
    iqrf::gpio::Gpio createGpio(iqrf::gpio::GpioConfig config) {
        config.use_mock = true;
        iqrf::gpio::Gpio gpio(config);
        gpio.registerValueCallback([this, config](bool oldValue, bool newValue) {
            this->history.emplace_back(this->epochTime(), config.consumer_name, oldValue, newValue);
        });
        return gpio;
    }

    /**
     * Returns epoch time in milliseconds.
     * @return Current time in milliseconds since epoch.
     */
    std::chrono::milliseconds epochTime() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
    }

    void expectGpioChange(
        GpioValueChangeEvent currentEvent,
        const std::string& gpioName,
        bool oldValue,
        bool newValue,
        std::optional<GpioValueChangeEvent> previousEvent = std::nullopt,
        std::optional<std::chrono::milliseconds> timeDifference = std::nullopt,
        int timeTolerance = 5
    ) {
        EXPECT_EQ(std::get<1>(currentEvent), gpioName);
        EXPECT_EQ(std::get<2>(currentEvent), oldValue);
        EXPECT_EQ(std::get<3>(currentEvent), newValue);
        if (previousEvent.has_value() && timeDifference.has_value()) {
            EXPECT_NEAR(
                std::get<0>(currentEvent).count() - std::get<0>(previousEvent.value()).count(),
                timeDifference.value().count(),
                timeTolerance
            );
        }
    }

    /// History of GPIO value changes
    std::vector<GpioValueChangeEvent> history;
};

TEST_F(BusSwitcherTest, busEnableGpioOnly) {
    iqrf::gpio::GpioConfig busEnableGpioConfig("gpiochip0", 1, "busEnable");
    iqrf::gpio::Gpio busEnableGpio = this->createGpio(busEnableGpioConfig);
    BusSwitcherConfig config(busEnableGpio, std::nullopt, std::nullopt, std::nullopt);
    BusSwitcher busSwitcher(config);

    busSwitcher.init();
    EXPECT_EQ(busEnableGpio.getDirection(), iqrf::gpio::GpioDirection::Output);
    EXPECT_FALSE(busEnableGpio.getValue());

    busSwitcher.toggleUart(true);
    EXPECT_TRUE(busEnableGpio.getValue());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    busSwitcher.toggleSpi(false);
    EXPECT_FALSE(busEnableGpio.getValue());

    EXPECT_EQ(history.size(), 2);
    this->expectGpioChange(history[0], "busEnable", false, true);
    this->expectGpioChange(history[1], "busEnable", true, false, history[0], std::chrono::milliseconds(50));
}

TEST_F(BusSwitcherTest, enableGpios) {
    iqrf::gpio::GpioConfig i2cEnableConfig("gpiochip0", 2, "i2cEnable");
    iqrf::gpio::Gpio i2cEnableGpio = this->createGpio(i2cEnableConfig);

    iqrf::gpio::GpioConfig spiEnableConfig("gpiochip0", 3, "spiEnable");
    iqrf::gpio::Gpio spiEnableGpio = this->createGpio(spiEnableConfig);

    iqrf::gpio::GpioConfig uartEnableConfig("gpiochip0", 4, "uartEnable");
    iqrf::gpio::Gpio uartEnableGpio = this->createGpio(uartEnableConfig);

    BusSwitcherConfig config(std::nullopt, i2cEnableGpio, spiEnableGpio, uartEnableGpio);
    BusSwitcher busSwitcher(config);
    busSwitcher.init();

    EXPECT_EQ(i2cEnableGpio.getDirection(), iqrf::gpio::GpioDirection::Output);
    EXPECT_EQ(spiEnableGpio.getDirection(), iqrf::gpio::GpioDirection::Output);
    EXPECT_EQ(uartEnableGpio.getDirection(), iqrf::gpio::GpioDirection::Output);

    EXPECT_FALSE(i2cEnableGpio.getValue());
    EXPECT_FALSE(spiEnableGpio.getValue());
    EXPECT_FALSE(uartEnableGpio.getValue());

    busSwitcher.toggleI2c(true);
    EXPECT_TRUE(i2cEnableGpio.getValue());
    EXPECT_FALSE(spiEnableGpio.getValue());
    EXPECT_FALSE(uartEnableGpio.getValue());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    busSwitcher.toggleSpi(true);
    EXPECT_FALSE(i2cEnableGpio.getValue());
    EXPECT_TRUE(spiEnableGpio.getValue());
    EXPECT_FALSE(uartEnableGpio.getValue());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    busSwitcher.toggleUart(true);
    EXPECT_FALSE(i2cEnableGpio.getValue());
    EXPECT_FALSE(spiEnableGpio.getValue());
    EXPECT_TRUE(uartEnableGpio.getValue());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    busSwitcher.toggleUart(false);
    EXPECT_FALSE(i2cEnableGpio.getValue());
    EXPECT_FALSE(spiEnableGpio.getValue());
    EXPECT_FALSE(uartEnableGpio.getValue());

    EXPECT_EQ(history.size(), 6);

    this->expectGpioChange(history[0], "i2cEnable", false, true, std::nullopt, std::nullopt);
    this->expectGpioChange(history[1], "i2cEnable", true, false, history[0], std::chrono::milliseconds(50));
    this->expectGpioChange(history[2], "spiEnable", false, true, history[0], std::chrono::milliseconds(50));
    this->expectGpioChange(history[3], "spiEnable", true, false, history[2], std::chrono::milliseconds(50));
    this->expectGpioChange(history[4], "uartEnable", false, true, history[2], std::chrono::milliseconds(50));
    this->expectGpioChange(history[5], "uartEnable", true, false, history[4], std::chrono::milliseconds(50));
}

}  // namespace iqrf::connector
