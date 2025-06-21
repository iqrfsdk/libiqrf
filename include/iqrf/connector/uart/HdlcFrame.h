/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: HdlcFrame.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-18
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#if BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <vector>

namespace iqrf::connector::uart {

/**
 * HDLC-like (High-Level Data Link Control) frame
 */
class HdlcFrame {
 public:
    /**
     * Constructs an empty HDLC frame
     */
    HdlcFrame() = default;

    /**
     * Constructs an HDLC frame with the given data
     * @param data Data to be set in the frame
     */
    explicit HdlcFrame(const std::vector<uint8_t> &data): data(data) {
        if (this->data.empty()) {
            throw std::logic_error("Data is empty");
        }
        this->crc = HdlcFrame::calculateCrc(this->data);
    }

    /**
     * Constructs an HDLC frame object from encoded data
     * @param data Encoded HDLC frame data
     */
    static HdlcFrame decode(const std::vector<uint8_t> &data);

    /**
     * Decodes the HDLC frame by one byte
     * @param data Data to be decoded
     * @return Decoded HDLC frame
     */
    void decodeByte(uint8_t byte);

    /**
     * Encodes the HDLC frame
     * @return Encoded HDLC frame
     */
    std::vector<uint8_t> encode();

    /**
     * Returns the data of the HDLC frame
     * @return Data of the HDLC frame
     */
    const std::vector<uint8_t> &getData() const;

 private:
#if BUILD_TESTS
    FRIEND_TEST(HdlcFrameTest, calculateCrc);
    FRIEND_TEST(HdlcFrameTest, getData);
#endif
    /**
     * Encodes a byte for HDLC frame
     * @param byte Byte to be encoded
     * @return Encoded byte
     */
    static uint8_t encodeByte(uint8_t byte);

    /**
     * Calculate 1-Wire CRC8 checksum for the given data.
     * @param data Data to calculate the checksum for
     * @return 1-Wire CRC8 checksum
     */
    static uint8_t calculateCrc(const std::vector<uint8_t> &data);

    /// Data
    std::vector<uint8_t> data;
    /// HDLC frame decoding in progress
    bool decoding = false;
    /// HDLC escape flag has been seen
    bool escape = false;
    /// HDLC frame 1-Wire CRC8
    int16_t crc = -1;
    /// HDLC frame start/end flag
    static constexpr uint8_t HDLC_FLAG = 0x7E;
    /// HDLC escape character
    static constexpr uint8_t HDLC_ESCAPE = 0x7D;
    /// HDLC escape bit to XOR with the byte
    static constexpr uint8_t HDLC_ESCAPE_BIT = 0x20;
};

}  // namespace iqrf::connector::uart
