/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: HdlcFrameTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-16
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>

#include "iqrf/connector/uart/HdlcFrame.h"

using ::testing::ElementsAre;

namespace iqrf::connector::uart {

class HdlcFrameTest : public ::testing::Test {
 protected:
    /// Raw data to HDLC frame conversion test data
    std::map<std::vector<uint8_t>, std::vector<uint8_t>> testData = {
        {
            {
                0x00, 0x00, 0xff, 0x3f, 0x00, 0x00, 0x80, 0x00, 0x17, 0x04,
                0x00, 0xfd, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
            },
            {
                0x7e, 0x00, 0x00, 0xff, 0x3f, 0x00, 0x00, 0x80, 0x00, 0x17, 0x04,
                0x00, 0xfd, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x4e, 0x7e,
            },
        },
        {
            { 0x00, 0x00, 0x06, 0x80, 0x00, 0x00, 0x00, 0x00 },
            { 0x7e, 0x00, 0x00, 0x06, 0x80, 0x00, 0x00, 0x00, 0x00, 0xa4, 0x7e },
        },
        {
            { 0x00, 0x00, 0x03, 0x00, 0xff, 0xff, 0x00, 0x7e, 0x7d, 0x7e },
            {
                0x7e, 0x00, 0x00, 0x03, 0x00, 0xff, 0xff, 0x00, 0x7d, 0x5e,
                0x7d, 0x5d, 0x7d, 0x5e, 0x48, 0x7e,
            },
        },
        {
            { 0x00, 0x00, 0x00, 0x0b, 0xff, 0xff, 0x00 },
            { 0x7e, 0x00, 0x00, 0x00, 0x0b, 0xff, 0xff, 0x00, 0xc1, 0x7e }
        },
        {
            { 0x00, 0x00, 0x00, 0x0b, 0xff, 0xff, 0x02 },
            { 0x7e, 0x00, 0x00, 0x00, 0x0b, 0xff, 0xff, 0x02, 0x7d, 0x5d, 0x7e }
        }
    };
};

TEST_F(HdlcFrameTest, constructor) {
    for (const auto& [rawData, encodedData] : testData) {
        const HdlcFrame frame(rawData);
        EXPECT_EQ(rawData, frame.getData());
        EXPECT_EQ(encodedData, frame.encode());
    }
    // Empty frame
    EXPECT_THROW(HdlcFrame(std::vector<uint8_t>()), std::invalid_argument);
}

TEST_F(HdlcFrameTest, decode) {
    for (const auto& [rawData, encodedData] : testData) {
        const HdlcFrame frame = HdlcFrame::decode(encodedData);
        EXPECT_EQ(rawData, frame.getData());
    }
    // Invalid CRC
    const std::vector<uint8_t> invalidCrc = {0x7e, 0x00, 0x00, 0x06, 0x80, 0x00, 0x00, 0x00, 0x00, 0xa5, 0x7e};
    EXPECT_THROW(HdlcFrame::decode(invalidCrc), HdlcFrameError);
    // Empty frame
    const std::vector<uint8_t> emptyFrame = {0x7e, 0x7e};
    EXPECT_THROW(HdlcFrame::decode(emptyFrame), HdlcFrameError);
    // Short frame
    const std::vector<uint8_t> shortFrame = {0x7e, 0x00, 0x7e};
    EXPECT_THROW(HdlcFrame::decode(shortFrame), HdlcFrameError);
    // Abort sequence
    const std::vector<uint8_t> abortSequence = {0x7e, 0x01, 0x02, 0x7d, 0x7e, 0x40, 0x7e};
    EXPECT_THROW(HdlcFrame::decode(abortSequence), HdlcFrameError);
    // Invalid escape sequence
    const std::vector<uint8_t> invalidEscape = {0x7e, 0x01, 0x7d, 0x4e, 0x7d};
    EXPECT_THROW(HdlcFrame::decode(invalidEscape), HdlcFrameError);
    // Incomplete frame
    const std::vector<uint8_t> incompleteFrame = {0x7e, 0x00, 0x00, 0x06, 0x80};
    EXPECT_THROW(HdlcFrame::decode(incompleteFrame), HdlcFrameError);
}

TEST_F(HdlcFrameTest, encodeEscapedBytes) {
    // Flag and escape bytes in data must be escaped
    const HdlcFrame frame({0x01, HdlcFrame::HDLC_FLAG, HdlcFrame::HDLC_ESCAPE});
    const std::vector<uint8_t> encoded = frame.encode();
    ASSERT_GE(encoded.size(), 8);
    EXPECT_THAT(
        std::vector(encoded.begin(), encoded.begin() + 6),
        ElementsAre(HdlcFrame::HDLC_FLAG, 0x01, 0x7D, 0x5E, 0x7D, 0x5D)
    );
    EXPECT_EQ(encoded.back(), HdlcFrame::HDLC_FLAG);
}

TEST_F(HdlcFrameTest, calculateCrc) {
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
        EXPECT_EQ(expectedCrc, HdlcFrame::calculateCrc(bytes));
    }
}

}  // namespace iqrf::connector::uart
