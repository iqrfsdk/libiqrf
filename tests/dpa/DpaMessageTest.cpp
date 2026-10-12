/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: DpaMessageTest.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "iqrf/dpa/DpaMessage.h"

namespace iqrf::dpa {

TEST(DpaMessageTest, requestFields) {
    const DpaMessage request(0x0102, 0x06, 0x00, 0xFFFF, {0x01, 0x02});
    EXPECT_EQ(request.direction(), DpaMessage::Direction::Request);
    EXPECT_EQ(request.nodeAddress(), 0x0102);
    EXPECT_EQ(request.peripheralType(), 0x06);
    EXPECT_EQ(request.peripheralCommand(), 0x00);
    EXPECT_EQ(request.bytes(), (std::vector<uint8_t>{0x02, 0x01, 0x06, 0x00, 0xFF, 0xFF, 0x01, 0x02}));
}

TEST(DpaMessageTest, responseDirection) {
    const DpaMessage response(std::vector<uint8_t>{0x00, 0x00, 0x06, 0x80, 0xFF, 0xFF, 0x00, 0x00, 0xAB});
    EXPECT_EQ(response.direction(), DpaMessage::Direction::Response);
    EXPECT_EQ(response.responseCode(), 0x00);
}

TEST(DpaMessageTest, confirmationDirection) {
    const DpaMessage confirmation(
        std::vector<uint8_t>{0x01, 0x00, 0x06, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x02, 0x00});
    EXPECT_EQ(confirmation.direction(), DpaMessage::Direction::Confirmation);
}

TEST(DpaMessageTest, emptyMessageIsRequest) {
    const DpaMessage empty;
    EXPECT_EQ(empty.size(), 0U);
    EXPECT_EQ(empty.direction(), DpaMessage::Direction::Request);
}

TEST(DpaMessageTest, tooLongDataThrows) {
    EXPECT_THROW(DpaMessage(std::vector<uint8_t>(DpaMessage::MAX_SIZE + 1, 0)), std::length_error);
    EXPECT_NO_THROW(DpaMessage(std::vector<uint8_t>(DpaMessage::MAX_SIZE, 0)));
}

}  // namespace iqrf::dpa
