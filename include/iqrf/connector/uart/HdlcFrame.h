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

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace iqrf::connector::uart {

/**
 * Error of a received HDLC frame (e.g. CRC mismatch, invalid escape sequence)
 */
class HdlcFrameError : public std::runtime_error {
 public:
    using std::runtime_error::runtime_error;
};

/**
 * HDLC-like (High-Level Data Link Control) frame
 *
 * Frame format: flag (0x7E), escaped data, escaped 1-Wire CRC8 of data, flag (0x7E).
 * Flag and escape bytes inside the frame are escaped by the escape byte (0x7D) followed by the byte XORed with 0x20.
 */
class HdlcFrame {
 public:
    /// HDLC frame start/end flag
    static constexpr uint8_t HDLC_FLAG = 0x7E;
    /// HDLC escape character
    static constexpr uint8_t HDLC_ESCAPE = 0x7D;
    /// HDLC escape bit to XOR with the byte
    static constexpr uint8_t HDLC_ESCAPE_BIT = 0x20;

    /**
     * Constructs an HDLC frame with the given data
     * @param data Data to be set in the frame
     * @throws std::invalid_argument if the data is empty
     */
    explicit HdlcFrame(std::vector<uint8_t> data);

    /**
     * Decodes the first HDLC frame from encoded data
     * @param data Encoded HDLC frame data
     * @return Decoded HDLC frame
     * @throws HdlcFrameError if the frame is invalid or the data does not contain a complete frame
     */
    static HdlcFrame decode(const std::vector<uint8_t> &data);

    /**
     * Encodes the HDLC frame
     * @return Encoded HDLC frame
     */
    [[nodiscard]] std::vector<uint8_t> encode() const;

    /**
     * Returns the data of the HDLC frame
     * @return Data of the HDLC frame
     */
    [[nodiscard]] const std::vector<uint8_t> &getData() const;

    /**
     * Calculate 1-Wire CRC8 checksum for the given data.
     * @param data Data to calculate the checksum for
     * @return 1-Wire CRC8 checksum
     */
    static uint8_t calculateCrc(const std::vector<uint8_t> &data);

 private:
    /**
     * Encodes a byte for HDLC frame and inserts it into data
     * @param encoded Vector of encoded data
     * @param byte Byte to encode and insert
     */
    static void encodeInsertByte(std::vector<uint8_t> &encoded, uint8_t byte);

    /// Data
    std::vector<uint8_t> data;
};

}  // namespace iqrf::connector::uart
