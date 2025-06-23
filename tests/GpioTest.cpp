/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: ConnectorUtilsTest.cpp
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

class GpioTest : public ::testing::Test {
};

TEST_F(GpioTest, VerifyCopyConstructor_GPIO) {
    // Create Gpio instance
    const GpioConfig config("gpiochip0", 1);
    auto orig = Gpio(config);

    // Create copy of the Gpio instance
    auto copy = Gpio(orig);

    EXPECT_NE(&copy, &orig);
    EXPECT_EQ(orig.impl, copy.impl);
}

TEST_F(GpioTest, VerifyMoveConstructor_GPIO) {
    // Create Gpio instance
    const GpioConfig config("gpiochip0", 1);
    auto orig = Gpio(config);

    // Store the original pointer for later checking
    const auto orig_impl = orig.impl;

    // Move the Gpio instance
    auto other = std::move(orig);

    EXPECT_NE(&other, &orig);
    EXPECT_EQ(orig_impl, other.impl);
    EXPECT_EQ(orig.impl, nullptr);
}

TEST_F(GpioTest, VerifyAssignmentOperator_GPIO) {
    // Create Gpio instance
    const GpioConfig config("gpiochip0", 1);
    const auto orig = Gpio(config);

    // Assign the Gpio instance
    const auto other = orig;

    EXPECT_EQ(orig.impl, other.impl);
}

TEST_F(GpioTest, TestInput_GPIO) {
    const iqrf::gpio::GpioConfig config("gpiochip0", 2, "libiqrf:test:input");
    const auto gpio = std::make_unique<Gpio>(config);
    gpio->initInput();

#if defined(__linux__)
#if libgpiod_VERSION_MAJOR == 1
    auto chip = std::make_unique<::gpiod::chip>("gpiochip0");
    const auto line = chip->get_line(2);

    ASSERT_TRUE(line.is_used());
    EXPECT_EQ(::gpiod::line::DIRECTION_INPUT, line.direction());
    EXPECT_STREQ("libiqrf:test:input", line.consumer().c_str());
#else
    auto chip = std::make_unique<::gpiod::chip>(::std::filesystem::path("/dev/gpiochip0"));
    const auto line_info = chip->get_line_info(2);

    ASSERT_TRUE(line_info.used());
    EXPECT_EQ(::gpiod::line::direction::INPUT, line_info.direction());
    EXPECT_STREQ("libiqrf:test:input", line_info.consumer().c_str());
#endif
#endif
}

TEST_F(GpioTest, TestOutput_GPIO) {
    const iqrf::gpio::GpioConfig config("gpiochip0", 0, "libiqrf:test:output");
    const auto gpio = std::make_unique<Gpio>(config);
    gpio->initOutput(true);

#if defined(__linux__)
#if libgpiod_VERSION_MAJOR == 1
    auto chip = std::make_unique<::gpiod::chip>("gpiochip0");
    const auto line = chip->get_line(0);

    ASSERT_TRUE(line.is_used());
    EXPECT_EQ(::gpiod::line::DIRECTION_OUTPUT, line.direction());
    EXPECT_STREQ("libiqrf:test:output", line.consumer().c_str());
#else
    auto chip = std::make_unique<::gpiod::chip>(::std::filesystem::path("/dev/gpiochip0"));
    const auto line_info = chip->get_line_info(0);

    ASSERT_TRUE(line_info.used());
    EXPECT_EQ(::gpiod::line::direction::OUTPUT, line_info.direction());
    EXPECT_STREQ("libiqrf:test:output", line_info.consumer().c_str());
#endif
#endif
}

}  // namespace iqrf::gpio
