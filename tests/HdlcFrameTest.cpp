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

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <vector>

#include "iqrf/connector/uart/HdlcFrame.h"

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
    };
};

TEST_F(HdlcFrameTest, constructor) {
    for (const auto& [rawData, encodedData] : testData) {
        HdlcFrame frame(rawData);
        EXPECT_EQ(rawData, frame.getData());
        EXPECT_EQ(encodedData, frame.encode());
    }
    // Empty frame
    HdlcFrame frame;
    EXPECT_TRUE(frame.getData().empty());
    EXPECT_THROW(frame.encode(), std::logic_error);
    // Empty encoded frame
    EXPECT_THROW(HdlcFrame(std::vector<uint8_t>()), std::logic_error);
}

TEST_F(HdlcFrameTest, decode) {
    for (const auto& [rawData, encodedData] : testData) {
        HdlcFrame frame = HdlcFrame::decode(encodedData);
        EXPECT_EQ(rawData, frame.getData());
    }
    // Invalid CRC
    {
        HdlcFrame frame;
        std::vector<uint8_t> invalidCrc = {0x7e, 0x00, 0x00, 0x06, 0x80, 0x00, 0x00, 0x00, 0x00, 0xa5, 0x7e};
        EXPECT_THROW(frame.decode(invalidCrc), std::logic_error);
    }
    // Empty frame
    {
        HdlcFrame frame;
        std::vector<uint8_t> emptyFrame = {0x7e, 0x7e};
        EXPECT_THROW(frame.decode(emptyFrame), std::logic_error);
    }
    // Abort sequence
    {
        HdlcFrame frame;
        std::vector<uint8_t> abortSequence = {0x7e, 0x01, 0x7d, 0x7e, 0x40};
        EXPECT_THROW(frame.decode(abortSequence), std::logic_error);
    }
}

TEST_F(HdlcFrameTest, encode) {
    for (const auto& [rawData, encodedData] : testData) {
        HdlcFrame frame(rawData);
        EXPECT_EQ(encodedData, frame.encode());
    }

    HdlcFrame frame;
    EXPECT_NO_THROW(frame.decodeByte(0x40));
    EXPECT_NO_THROW(frame.decodeByte(0x7e));
    EXPECT_THROW(frame.encode(), std::logic_error);
    EXPECT_NO_THROW(frame.decodeByte(0x7d));
    EXPECT_THROW(frame.encode(), std::logic_error);
}

TEST_F(HdlcFrameTest, getData) {
    HdlcFrame frame;
    EXPECT_NO_THROW(frame.decodeByte(0x40));
    EXPECT_NO_THROW(frame.decodeByte(HdlcFrame::HDLC_FLAG));
    EXPECT_THROW(frame.getData(), std::logic_error);
    EXPECT_NO_THROW(frame.decodeByte(HdlcFrame::HDLC_ESCAPE));
    EXPECT_THROW(frame.getData(), std::logic_error);
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
