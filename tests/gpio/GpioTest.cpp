/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: GpioTest.cpp
 * Authors: Ondřej Hujňák <ondrej.hujnak@iqrf.com>, Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-06-21
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>

#ifdef __linux__
#include <gpiod.hpp>

#include "iqrf/gpio/libgpiodVersion.h"
#endif

#include "iqrf/gpio/Gpio.h"
#include "iqrf/gpio/Config.h"

namespace iqrf::gpio {

class GpioTest : public ::testing::Test {
 protected:
    void SetUp() override {
#ifdef __linux__
        if (!std::filesystem::exists("/dev/gpiochip0")) {
            GTEST_SKIP() << "GPIO chip /dev/gpiochip0 is not available";
        }
#elif defined(__FreeBSD__)
        if (!std::filesystem::exists("/dev/gpioc0")) {
            GTEST_SKIP() << "GPIO chip /dev/gpioc0 is not available";
        }
#endif
    }
};

TEST_F(GpioTest, TestInput_GPIO) {
    const iqrf::gpio::GpioConfig config("gpiochip0", 2, "libiqrf:test:input");
    const auto gpio = std::make_unique<Gpio>(config);
    gpio->initInput();

#ifdef __linux__
#if libgpiod_VERSION_MAJOR == 1
    auto chip = std::make_unique<::gpiod::chip>("gpiochip0");
    const auto line = chip->get_line(2);

    ASSERT_TRUE(line.is_used());
    EXPECT_EQ(::gpiod::line::DIRECTION_INPUT, line.direction());
    EXPECT_STREQ("libiqrf:test:input", line.consumer().c_str());
#else
    const auto chip = std::make_unique<::gpiod::chip>(::std::filesystem::path("/dev/gpiochip0"));
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

#ifdef __linux__
#if libgpiod_VERSION_MAJOR == 1
    auto chip = std::make_unique<::gpiod::chip>("gpiochip0");
    const auto line = chip->get_line(0);

    ASSERT_TRUE(line.is_used());
    EXPECT_EQ(::gpiod::line::DIRECTION_OUTPUT, line.direction());
    EXPECT_STREQ("libiqrf:test:output", line.consumer().c_str());
#else
    const auto chip = std::make_unique<::gpiod::chip>(::std::filesystem::path("/dev/gpiochip0"));
    const auto line_info = chip->get_line_info(0);

    ASSERT_TRUE(line_info.used());
    EXPECT_EQ(::gpiod::line::direction::OUTPUT, line_info.direction());
    EXPECT_STREQ("libiqrf:test:output", line_info.consumer().c_str());
#endif
#endif
}

}  // namespace iqrf::gpio
