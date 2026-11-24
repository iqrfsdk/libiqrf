/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: HdlcFuzzer.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-11-24
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <fuzzer/FuzzedDataProvider.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <vector>

#include "iqrf/connector/uart/HdlcDecoder.h"
#include "iqrf/connector/uart/HdlcFrame.h"

using iqrf::connector::uart::HdlcDecoder;
using iqrf::connector::uart::HdlcFrame;
using iqrf::connector::uart::HdlcFrameError;

/**
 * Fuzzes the HDLC stream decoder and the HDLC frame decoder with bytes received from the UART
 *
 * Every successfully decoded frame must survive the encode/decode round trip.
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, std::size_t size) {
    const std::vector<uint8_t> input(data, data + size);

    // Stream decoder, the input is a stream of bytes received from the UART in chunks, the decoder may be reset
    // between the chunks (e.g. when the UART connector discards the received data)
    FuzzedDataProvider provider(data, size);
    HdlcDecoder decoder;
    std::size_t flagsSinceReset = 0;
    while (provider.remaining_bytes() > 0) {
        const std::size_t chunkSize = provider.ConsumeIntegralInRange<std::size_t>(1, 64);
        for (const uint8_t byte : provider.ConsumeBytes<uint8_t>(chunkSize)) {
            if (byte == HdlcFrame::HDLC_FLAG) {
                ++flagsSinceReset;
            }
            try {
                const std::optional<HdlcFrame> frame = decoder.decodeByte(byte);
                if (!frame.has_value()) {
                    continue;
                }
                // A frame requires both the start and the end flag received after the reset
                if (flagsSinceReset < 2) {
                    std::abort();
                }
                if (HdlcFrame::decode(frame->encode()).getData() != frame->getData()) {
                    std::abort();
                }
            } catch (const HdlcFrameError &) {
                // Invalid frame is discarded, the decoder continues with the next one
            }
        }
        if (provider.ConsumeBool()) {
            decoder.reset();
            flagsSinceReset = 0;
        }
    }

    // Frame decoder, the input is a single encoded frame
    try {
        const HdlcFrame frame = HdlcFrame::decode(input);
        if (HdlcFrame::decode(frame.encode()).getData() != frame.getData()) {
            std::abort();
        }
    } catch (const HdlcFrameError &) {
        // Invalid frame
    }

    // Encoder, the input is frame data, the encoded frame must be decoded by both decoders
    if (!input.empty()) {
        const std::vector<uint8_t> encoded = HdlcFrame(input).encode();
        if (HdlcFrame::decode(encoded).getData() != input) {
            std::abort();
        }
        HdlcDecoder roundTripDecoder;
        std::optional<HdlcFrame> decoded;
        for (const uint8_t byte : encoded) {
            if (decoded.has_value()) {
                // The frame was completed before the end flag
                std::abort();
            }
            decoded = roundTripDecoder.decodeByte(byte);
        }
        if (!decoded.has_value() || decoded->getData() != input) {
            std::abort();
        }
    }
    return 0;
}
