#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "iqrf/gpio/Gpio.h"
#include "iqrf/gpio/Config.h"

// Mock `gpiod::chip` class
class MockChip : public ::gpiod::chip {
public:
    MOCK_METHOD(::gpiod::line, get_line, (int line_num), ());
};

// Mock `gpiod::line` class
class MockLine : public ::gpiod::line {
public:
    MOCK_METHOD(void, is_requested, (), ());
    MOCK_METHOD(int, release, (), ());
};

namespace iqrf::gpio {

    class GpioTest : public ::testing::Test {
    };

    TEST_F(GpioTest, VerifyCopyConstructor) {
        // Create Gpio instance
        const GpioConfig config("gpiochip0", 1);
        // auto *orig = new Gpio(config);

        // Create copy of the Gpio instance
        // auto *copy = new Gpio(orig);
        
        // ASSERT_EQ(orig.impl, copy.impl);
    }

    TEST_F(GpioTest, VerifyAssignmentOperator) {
        // Create Gpio instance
        const GpioConfig config("gpiochip0", 1);
        // auto *orig = new Gpio(config);

        // Assign the Gpio instance
        // auto *other = orig;

        // ASSERT_EQ(orig.impl, other.impl)
    }
}