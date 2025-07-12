/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: GpioConfigTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-07-13
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gtest/gtest.h>

#include <stdexcept>

#include "iqrf/gpio/Config.h"

namespace iqrf::gpio {

class GpioConfigTest : public ::testing::Test {
 protected:
    /// GPIO chip name
    const std::string chip = "gpiochip0";
    /// GPIO line offset
    const size_t line = 11;
    /// Consumer name
    const std::string consumer_name = "test_consumer";
};

TEST_F(GpioConfigTest, to_string_LineOffset) {
    GpioConfig config(chip, line, consumer_name);
    EXPECT_EQ(config.to_string(), "GPIO configuration: chip=gpiochip0, line=11, consumer_name=test_consumer");
    config = GpioConfig(chip, line);
    EXPECT_EQ(config.to_string(), "GPIO configuration: chip=gpiochip0, line=11, consumer_name=libiqrf");
}

TEST_F(GpioConfigTest, to_string_LineName) {
    GpioConfig config(chip, "gpio-line-name", consumer_name);
    EXPECT_EQ(config.to_string(), "GPIO configuration: chip=gpiochip0, line=0, consumer_name=test_consumer");
    config = GpioConfig(chip, "gpio-line-name");
    EXPECT_EQ(config.to_string(), "GPIO configuration: chip=gpiochip0, line=0, consumer_name=libiqrf");
}

}  // namespace iqrf::gpio
