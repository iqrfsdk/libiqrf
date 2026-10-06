/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: HdlcDecoder.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-06
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "iqrf/connector/uart/HdlcDecoder.h"

#include <optional>
#include <utility>
#include <vector>

namespace iqrf::connector::uart {

std::optional<HdlcFrame> HdlcDecoder::decodeByte(uint8_t byte) {
    switch (this->state) {
        case State::Idle:
            if (byte == HdlcFrame::HDLC_FLAG) {
                this->buffer.clear();
                this->state = State::Receiving;
            }
            return std::nullopt;
        case State::Escape:
            if (byte == HdlcFrame::HDLC_FLAG) {
                this->fail("Received abort sequence", State::Idle);
            }
            byte ^= HdlcFrame::HDLC_ESCAPE_BIT;
            if (byte != HdlcFrame::HDLC_ESCAPE && byte != HdlcFrame::HDLC_FLAG) {
                this->fail("Invalid escape sequence", State::Idle);
            }
            this->buffer.push_back(byte);
            this->state = State::Receiving;
            return std::nullopt;
        case State::Receiving:
            break;
    }

    if (byte == HdlcFrame::HDLC_ESCAPE) {
        this->state = State::Escape;
        return std::nullopt;
    }
    if (byte != HdlcFrame::HDLC_FLAG) {
        this->buffer.push_back(byte);
        return std::nullopt;
    }
    // End flag, it can be also the start flag of the next frame
    if (this->buffer.empty()) {
        return std::nullopt;
    }
    if (this->buffer.size() < 2) {
        this->fail("Received too short frame", State::Receiving);
    }
    const uint8_t crc = this->buffer.back();
    this->buffer.pop_back();
    if (crc != HdlcFrame::calculateCrc(this->buffer)) {
        this->fail("CRC check failed", State::Receiving);
    }
    std::vector<uint8_t> data;
    data.swap(this->buffer);
    return HdlcFrame(std::move(data));
}

void HdlcDecoder::reset() {
    this->buffer.clear();
    this->state = State::Idle;
}

void HdlcDecoder::fail(const char *message, const State state) {
    this->buffer.clear();
    this->state = state;
    throw HdlcFrameError(message);
}

}  // namespace iqrf::connector::uart
