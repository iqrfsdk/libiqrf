/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: HdlcDecoderTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-06
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <vector>

#include "iqrf/connector/uart/HdlcDecoder.h"
#include "iqrf/connector/uart/HdlcFrame.h"

using ::testing::ElementsAre;

namespace iqrf::connector::uart {

class HdlcDecoderTest : public ::testing::Test {
 protected:
    /**
     * Decodes the stream and records the decoded frames and errors
     * @param stream Received bytes
     */
    void decode(const std::vector<uint8_t> &stream) {
        for (const uint8_t byte : stream) {
            try {
                const std::optional<HdlcFrame> frame = this->decoder.decodeByte(byte);
                if (frame.has_value()) {
                    this->frames.push_back(frame->getData());
                }
            } catch (const HdlcFrameError &) {
                ++this->errors;
            }
        }
    }

    /**
     * Concatenates byte vectors
     * @param parts Byte vectors
     * @return Concatenated bytes
     */
    static std::vector<uint8_t> concat(const std::initializer_list<std::vector<uint8_t>> parts) {
        std::vector<uint8_t> result;
        for (const auto &part : parts) {
            result.insert(result.end(), part.begin(), part.end());
        }
        return result;
    }

    /// First frame data
    const std::vector<uint8_t> dataA = {0x00, 0x00, 0x06, 0x80, 0x00, 0x00, 0x00, 0x00};
    /// Second frame data (contains bytes to escape)
    const std::vector<uint8_t> dataB = {0x00, 0x00, 0x03, 0x00, 0xff, 0xff, 0x00, 0x7e, 0x7d, 0x7e};
    /// Encoded first frame
    const std::vector<uint8_t> frameA = HdlcFrame(dataA).encode();
    /// Encoded second frame
    const std::vector<uint8_t> frameB = HdlcFrame(dataB).encode();
    /// HDLC decoder
    HdlcDecoder decoder;
    /// Decoded frames
    std::vector<std::vector<uint8_t>> frames;
    /// Number of decoding errors
    int errors = 0;
};

TEST_F(HdlcDecoderTest, multipleFrames) {
    this->decode(concat({this->frameA, this->frameB}));
    EXPECT_THAT(this->frames, ElementsAre(this->dataA, this->dataB));
    EXPECT_EQ(this->errors, 0);
}

TEST_F(HdlcDecoderTest, sharedFlag) {
    // End flag of the first frame is the start flag of the second frame
    const std::vector<uint8_t> frameAWithoutEndFlag(this->frameA.begin(), this->frameA.end() - 1);
    this->decode(concat({frameAWithoutEndFlag, this->frameB}));
    EXPECT_THAT(this->frames, ElementsAre(this->dataA, this->dataB));
    EXPECT_EQ(this->errors, 0);
}

TEST_F(HdlcDecoderTest, consecutiveFlags) {
    this->decode(concat({{0x7e, 0x7e, 0x7e}, this->frameA, {0x7e, 0x7e}}));
    EXPECT_THAT(this->frames, ElementsAre(this->dataA));
    EXPECT_EQ(this->errors, 0);
}

TEST_F(HdlcDecoderTest, bytesBeforeFirstFlag) {
    this->decode(concat({{0x01, 0x02, 0x7d}, this->frameA}));
    EXPECT_THAT(this->frames, ElementsAre(this->dataA));
    EXPECT_EQ(this->errors, 0);
}

TEST_F(HdlcDecoderTest, startInTheMiddleOfFrame) {
    // End of a frame, which started before the decoder, is taken as the start of a frame
    this->decode(concat({{0x55, 0x66, 0x7e}, this->frameA}));
    EXPECT_THAT(this->frames, ElementsAre(this->dataA));
    EXPECT_EQ(this->errors, 0);
}

TEST_F(HdlcDecoderTest, frameSplitIntoParts) {
    const auto middle = this->frameA.begin() + static_cast<std::ptrdiff_t>(this->frameA.size() / 2);
    this->decode(std::vector<uint8_t>(this->frameA.begin(), middle));
    EXPECT_TRUE(this->frames.empty());
    this->decode(std::vector<uint8_t>(middle, this->frameA.end()));
    EXPECT_THAT(this->frames, ElementsAre(this->dataA));
}

TEST_F(HdlcDecoderTest, recoveryAfterCrcError) {
    const std::vector<uint8_t> invalidCrc = {0x7e, 0x00, 0x00, 0x06, 0x80, 0x00, 0x00, 0x00, 0x00, 0xa5, 0x7e};
    this->decode(concat({invalidCrc, this->frameB}));
    EXPECT_THAT(this->frames, ElementsAre(this->dataB));
    EXPECT_EQ(this->errors, 1);
}

TEST_F(HdlcDecoderTest, recoveryAfterShortFrame) {
    this->decode(concat({{0x7e, 0x00, 0x7e}, this->frameB}));
    EXPECT_THAT(this->frames, ElementsAre(this->dataB));
    EXPECT_EQ(this->errors, 1);
}

TEST_F(HdlcDecoderTest, recoveryAfterInvalidEscape) {
    this->decode(concat({{0x7e, 0x01, 0x7d, 0x11, 0x02, 0x03}, this->frameB}));
    EXPECT_THAT(this->frames, ElementsAre(this->dataB));
    EXPECT_EQ(this->errors, 1);
}

TEST_F(HdlcDecoderTest, recoveryAfterAbort) {
    this->decode(concat({{0x7e, 0x01, 0x02, 0x7d, 0x7e}, this->frameB}));
    EXPECT_THAT(this->frames, ElementsAre(this->dataB));
    EXPECT_EQ(this->errors, 1);
}

TEST_F(HdlcDecoderTest, reset) {
    // Partially received frame is discarded, the decoder waits for the next start flag
    this->decode(std::vector<uint8_t>(this->frameA.begin(), this->frameA.end() - 1));
    this->decoder.reset();
    this->decode(concat({{0x7e}, this->frameB}));
    EXPECT_THAT(this->frames, ElementsAre(this->dataB));
    EXPECT_EQ(this->errors, 0);
}

}  // namespace iqrf::connector::uart
