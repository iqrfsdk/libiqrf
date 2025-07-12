/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: ConnectorUtilsTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-16
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "iqrf/connector/ConnectorUtils.h"

namespace iqrf::connector {

class ConnectorUtilsTest : public ::testing::Test {
 protected:
    /// Raw bytes as std::vector to human-readable string conversion test data
    std::map<std::vector<uint8_t>, std::string> testData = {
        {
            {
                0x00, 0x00, 0xff, 0x3f, 0x00, 0x00, 0x80, 0x00, 0x17, 0x04,
                0x00, 0xfd, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
            },
            "00.00.ff.3f.00.00.80.00.17.04.00.fd.26.00.00.00.00.00.00.05",
        },
        {
            { 0x00, 0x00, 0x06, 0x80, 0x00, 0x00, 0x00, 0x00 },
            "00.00.06.80.00.00.00.00"
        },
        {
            { 0x01 },
            "01",
        },
        {
            {},
            "",
        },
    };
};

TEST_F(ConnectorUtilsTest, vectorToHexString) {
    for (const auto& [data, expected] : testData) {
        EXPECT_EQ(expected, ConnectorUtils::vectorToHexString(data));
    }
}

}  // namespace iqrf::connector
