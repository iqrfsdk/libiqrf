/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: GpioResolverTest.cpp
 * Authors: Karel Hanák <karel.hanak@iqrf.com>
 * Date: 2024-10-29
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gtest/gtest.h>

#include <stdexcept>

#include "iqrf/gpio/GpioResolver.h"

namespace iqrf::gpio {

class GpioResolverTest : public ::testing::Test {
 protected:
    void SetUp() override {
        GpioMap map;
        std::shared_ptr<std::string> chip_name = std::make_shared<std::string>(chip0_name);
        int64_t pin_num = 0;
        for (size_t line_num = 0; line_num < chip0_num_lines; ++line_num) {
            map.insert_or_assign(pin_num++, std::make_pair(chip_name, line_num));
        }
#if defined(__linux__)
        chip_name = std::make_shared<std::string>(chip1_name);
        for (size_t line_num = 0; line_num < chip1_num_lines; ++line_num) {
            map.insert_or_assign(pin_num++, std::make_pair(chip_name, line_num));
        }
#endif
        this->resolver = GpioResolver::GetResolver(map);
    }

    /// GPIO resolver instance
    GpioResolver *resolver = nullptr;
#if defined(__linux__)
    /// Chip 0 name
    const std::string chip0_name = "gpiochip0";
    /// Chip 0 line count
    const size_t chip0_num_lines = 28;
    /// Chip 1 name
    const std::string chip1_name = "gpiochip1";
    /// Chip 1 line count
    const size_t chip1_num_lines = 12;
#elif defined(__FreeBSD__)
    /// Chip 0 name
    const std::string chip0_name = "gpioc0";
    /// Chip 0 line count
    const size_t chip0_num_lines = 53;
#endif
};

TEST_F(GpioResolverTest, ResolveExistingPins) {
    std::string chip_name;
    size_t line_offset = 0;

    // GPIO pin 0
    this->resolver->resolveGpioPin(0, chip_name, line_offset);
    ASSERT_STREQ(chip0_name.c_str(), chip_name.c_str());
    ASSERT_EQ(0, line_offset);

    // GPIO pin 11
    this->resolver->resolveGpioPin(11, chip_name, line_offset);
    ASSERT_STREQ(chip0_name.c_str(), chip_name.c_str());
    ASSERT_EQ(11, line_offset);

#if defined(__linux__)
    // GPIO pin 28
    this->resolver->resolveGpioPin(28, chip_name, line_offset);
    ASSERT_STREQ(chip1_name.c_str(), chip_name.c_str());
    ASSERT_EQ(0, line_offset);

    // GPIO pin 35
    this->resolver->resolveGpioPin(35, chip_name, line_offset);
    ASSERT_STREQ(chip1_name.c_str(), chip_name.c_str());
    ASSERT_EQ(7, line_offset);

    // GPIO pin 39
    this->resolver->resolveGpioPin(39, chip_name, line_offset);
    ASSERT_STREQ(chip1_name.c_str(), chip_name.c_str());
    ASSERT_EQ(11, line_offset);
#endif
}

TEST_F(GpioResolverTest, ResolveNonexistentPins) {
    std::string chip_name;
    size_t line_offset = 0;

    ASSERT_THROW(this->resolver->resolveGpioPin(500, chip_name, line_offset), std::runtime_error);
}

}  // namespace iqrf::gpio
