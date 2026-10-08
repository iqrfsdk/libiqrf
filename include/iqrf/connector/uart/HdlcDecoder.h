/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: HdlcDecoder.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-06
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "iqrf/connector/uart/HdlcFrame.h"

namespace iqrf::connector::uart {

/**
 * Stream decoder of HDLC frames
 *
 * Bytes received from the stream are passed one by one, the decoder returns a frame once it is complete.
 * The decoder synchronizes to the stream by the flags, so it can start in the middle of a frame:
 * - bytes before the first flag are ignored,
 * - consecutive flags (empty frames) are ignored, so the end flag of a frame can be the start flag of the next one.
 *
 * After an invalid frame is detected, the decoder discards it and continues with the next frame.
 */
class HdlcDecoder {
 public:
    /**
     * Decodes one byte of the stream
     * @param byte Received byte
     * @return Decoded frame if the byte completed a frame, std::nullopt otherwise
     * @throws HdlcFrameError if the byte completed an invalid frame, the frame is discarded
     */
    std::optional<HdlcFrame> decodeByte(uint8_t byte);

    /**
     * Discards the partially received frame and waits for the next start flag
     */
    void reset();

 private:
    /**
     * Decoder state
     */
    enum class State {
        /// Waiting for the start flag
        Idle,
        /// Receiving frame data
        Receiving,
        /// Receiving frame data, escape byte has been received
        Escape,
    };

    /**
     * Discards the frame and throws an error
     * @param message Error message
     * @param state Decoder state to continue with
     * @throws HdlcFrameError always
     */
    [[noreturn]] void fail(const char *message, State state);

    /// Decoder state
    State state = State::Idle;
    /// Received frame data including CRC
    std::vector<uint8_t> buffer;
};

}  // namespace iqrf::connector::uart
