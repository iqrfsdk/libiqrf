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

namespace iqrf::connector::uart {

HdlcFrame HdlcFrame::decode(const std::vector<uint8_t> &data) {
    HdlcFrame frame;
    for (const auto byte : data) {
        frame.decodeByte(byte);
    }
    return frame;
}

void HdlcFrame::decodeByte(uint8_t byte) {
    if (!this->decoding && byte == HDLC_FLAG) {
        if (!this->data.empty()) {
            this->data.clear();
        }
        this->crc = -1;
        this->decoding = true;
        return;
    }
    if (!this->decoding) {
        return;
    }

    if (!this->escape && byte == HDLC_FLAG) {
        this->decoding = false;
        this->escape = false;
        if (this->data.empty()) {
            throw std::logic_error("Received empty frame");
        }
        if (this->data.size() < 2) {
            throw std::logic_error("Received too short frame");
        }
        this->crc = this->data.back();
        this->data.pop_back();
        if (this->crc != calculateCrc(this->data)) {
            throw std::logic_error("CRC check failed");
        }
        return;
    }
    if (!this->escape && byte == HDLC_ESCAPE) {
        this->escape = true;
        return;
    }
    if (this->escape) {
        if (byte == HDLC_FLAG) {
            this->decoding = false;
            this->escape = false;
            throw std::logic_error("Received abort sequence");
        }
        byte ^= HDLC_ESCAPE_BIT;
        if (byte != HDLC_ESCAPE && byte != HDLC_FLAG) {
            this->escape = false;
            throw std::logic_error("Invalid escape sequence");
        }
        this->escape = false;
    }
    this->data.push_back(byte);
}

std::vector<uint8_t> HdlcFrame::encode() {
    if (this->decoding) {
        throw std::logic_error("Frame is not fully decoded");
    }
    if (this->data.empty()) {
        throw std::logic_error("No data to encode");
    }
    std::vector<uint8_t> encoded;
    encoded.reserve(this->data.size() + 2);
    encoded.push_back(HDLC_FLAG);
    for (const auto byte : this->data) {
        if (byte == HDLC_FLAG || byte == HDLC_ESCAPE) {
            encoded.push_back(HDLC_ESCAPE);
            encoded.push_back(HdlcFrame::encodeByte(byte));
        } else {
            encoded.push_back(byte);
        }
    }
    if (this->crc == -1) {
        this->crc = HdlcFrame::calculateCrc(this->data);
    }
    encoded.push_back(iqrf::connector::uart::HdlcFrame::encodeByte(this->crc));
    encoded.push_back(HDLC_FLAG);
    return encoded;
}

uint8_t HdlcFrame::encodeByte(const uint8_t byte) {
    if (byte == HDLC_FLAG || byte == HDLC_ESCAPE) {
        return byte ^ HDLC_ESCAPE_BIT;
    }
    return byte;
}

const std::vector<uint8_t> &HdlcFrame::getData() const {
    if (this->decoding) {
        throw std::logic_error("Frame is not fully decoded");
    }
    if (this->data.empty()) {
        return this->data;
    }
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
