/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: HdlcFrame.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-10
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "iqrf/connector/uart/HdlcFrame.h"

#include <optional>
#include <utility>
#include <vector>

#include "iqrf/connector/uart/HdlcDecoder.h"

namespace iqrf::connector::uart {

HdlcFrame::HdlcFrame(std::vector<uint8_t> data): data(std::move(data)) {
    if (this->data.empty()) {
        throw std::invalid_argument("Data is empty");
    }
}

HdlcFrame HdlcFrame::decode(const std::vector<uint8_t> &data) {
    HdlcDecoder decoder;
    for (const auto byte : data) {
        std::optional<HdlcFrame> frame = decoder.decodeByte(byte);
        if (frame.has_value()) {
            return std::move(frame.value());
        }
    }
    throw HdlcFrameError("Incomplete frame");
}

std::vector<uint8_t> HdlcFrame::encode() const {
    std::vector<uint8_t> encoded;
    // Flags, CRC and possible escaping of a few bytes
    encoded.reserve(this->data.size() + 8);
    encoded.push_back(HDLC_FLAG);
    for (const auto byte : this->data) {
        HdlcFrame::encodeInsertByte(encoded, byte);
    }
    HdlcFrame::encodeInsertByte(encoded, HdlcFrame::calculateCrc(this->data));
    encoded.push_back(HDLC_FLAG);
    return encoded;
}

void HdlcFrame::encodeInsertByte(std::vector<uint8_t> &encoded, uint8_t byte) {
    if (byte == HDLC_FLAG || byte == HDLC_ESCAPE) {
        encoded.push_back(HDLC_ESCAPE);
        encoded.push_back(byte ^ HDLC_ESCAPE_BIT);
    } else {
        encoded.push_back(byte);
    }
}

const std::vector<uint8_t> &HdlcFrame::getData() const {
    return this->data;
}

uint8_t HdlcFrame::calculateCrc(const std::vector<uint8_t> &data) {
    uint8_t crc = 0xFF;
    for (const uint8_t byte : data) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x01) {
                crc = (crc >> 1) ^ 0x8C;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

}  // namespace iqrf::connector::uart
