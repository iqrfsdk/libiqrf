/**
* Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: UartConnectorTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-16
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <vector>

#include "iqrf/connector/uart/UartConnector.h"

namespace iqrf::connector::uart {

class UartConnectorTest : public ::testing::Test {
};

TEST_F(UartConnectorTest, calculateCrc) {
    const std::map<uint8_t, std::vector<uint8_t>> values = {
        {
            0x4e,
            {
                0x00, 0x00, 0xff, 0x3f, 0x00, 0x00, 0x80, 0x00, 0x17, 0x04,
                0x00, 0xfd, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
            },
        },
        {
            0xa4,
            { 0x00, 0x00, 0x06, 0x80, 0x00, 0x00, 0x00, 0x00 },
        },
        {
            0x69,
            { 0x00, 0x00, 0x06, 0x81, 0x00, 0x00, 0x00, 0x00 },
        },
    };
    for (const auto& [expectedCrc, bytes] : values) {
        EXPECT_EQ(expectedCrc, UartConnector::calculateCrc(bytes));
    }
}

}  // namespace iqrf::connector::uart
