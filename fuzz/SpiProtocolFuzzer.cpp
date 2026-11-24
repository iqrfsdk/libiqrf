/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: SpiProtocolFuzzer.cpp
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

#include "iqrf/connector/IConnector.h"
#include "iqrf/connector/spi/SpiProtocol.h"

using iqrf::connector::ProgrammingTarget;
using iqrf::connector::spi::SpiProtocol;
using iqrf::connector::spi::SpiStatus;

/**
 * Fuzzes the parsing of the bytes received from the TR module over SPI
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, std::size_t size) {
    FuzzedDataProvider provider(data, size);

    // SPI status
    const std::optional<SpiStatus> status = SpiStatus::parse(provider.ConsumeIntegral<uint8_t>());
    if (status.has_value() && status->getDataReadyLength() > SpiProtocol::MAX_DATA_LENGTH) {
        std::abort();
    }

    // Response to the read packet
    const std::size_t length = provider.ConsumeIntegralInRange<std::size_t>(0, SpiProtocol::MAX_DATA_LENGTH);
    const auto target = static_cast<ProgrammingTarget>(
        provider.ConsumeIntegralInRange<int>(0, static_cast<int>(ProgrammingTarget::Special))
    );
    const std::vector<uint8_t> response = provider.ConsumeRemainingBytes<uint8_t>();
    const std::optional<std::vector<uint8_t>> read = SpiProtocol::parseReadResponse(length, response);
    if (read.has_value() && read->size() != length) {
        std::abort();
    }
    (void)SpiProtocol::isCrcmConfirmed(response);

    // Downloaded data block
    try {
        (void)SpiProtocol::parseDownloadResponse(target, response);
    } catch (const std::out_of_range &) {
        // Too short data block
    }
    return 0;
}
