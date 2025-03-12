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
        auto orig_impl = orig.impl;

        // Move the Gpio instance
        auto other = std::move(orig);
        
        EXPECT_NE(&other, &orig);
        EXPECT_EQ(orig_impl, other.impl);
        EXPECT_EQ(orig.impl, nullptr);
    }

    TEST_F(GpioTest, VerifyAssignmentOperator_GPIO) {
        // Create Gpio instance
        const GpioConfig config("gpiochip0", 1);
        auto orig = Gpio(config);

        // Assign the Gpio instance
        auto other = orig;

        EXPECT_EQ(orig.impl, other.impl);
    }
}