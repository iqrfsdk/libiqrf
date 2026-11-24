/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: CdcProtocolFuzzer.cpp
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
#include <stdexcept>
#include <vector>

#include "iqrf/connector/cdc/CdcProtocol.h"

using iqrf::connector::cdc::CdcFrame;
using iqrf::connector::cdc::CdcFrameError;
namespace protocol = iqrf::connector::cdc::protocol;

namespace {

/**
 * Extracts all complete frames from the buffer, the same way as CdcConnector::readFrame() does
 * @param buffer Received bytes
 * @param downloadLength Length of the downloaded data, nullopt if no download is in progress
 */
void extractFrames(std::vector<uint8_t> &buffer, const std::optional<std::size_t> downloadLength) {
    while (true) {
        const std::size_t sizeBefore = buffer.size();
        std::optional<CdcFrame> frame;
        try {
            frame = downloadLength.has_value()
                ? protocol::extractDownloadFrame(buffer, *downloadLength)
                : protocol::extractFrame(buffer);
        } catch (const CdcFrameError &) {
            buffer.clear();
            return;
        }
        if (!frame.has_value()) {
            return;
        }
        // Every extracted frame must consume at least one byte, otherwise the connector would loop forever
        if (buffer.size() >= sizeBefore) {
            std::abort();
        }
        (void)protocol::statusOf(*frame);
        if (frame->header == "I") {
            try {
                (void)protocol::parseUsbDeviceInfo(frame->body);
            } catch (const std::runtime_error &) {
                // Invalid USB device information
            }
        }
    }
}

}  // namespace

/**
 * Fuzzes the extraction of the frames received from the USB CDC device
 *
 * The input is split into chunks to simulate bytes arriving in multiple reads from the port.
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, std::size_t size) {
    FuzzedDataProvider provider(data, size);
    std::optional<std::size_t> downloadLength;
    if (provider.ConsumeBool()) {
        downloadLength = provider.ConsumeIntegralInRange<std::size_t>(0, 64);
    }
    std::vector<uint8_t> buffer;
    while (provider.remaining_bytes() > 0) {
        const std::size_t chunkSize = provider.ConsumeIntegralInRange<std::size_t>(1, 64);
        const std::vector<uint8_t> chunk = provider.ConsumeBytes<uint8_t>(chunkSize);
        buffer.insert(buffer.end(), chunk.begin(), chunk.end());
        extractFrames(buffer, downloadLength);
    }
    return 0;
}
