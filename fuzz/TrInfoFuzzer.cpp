/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: TrInfoFuzzer.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-11-24
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "iqrf/connector/TrInfo.h"

using iqrf::connector::TrInfo;

/**
 * Fuzzes the parsing and formatting of the Module Info block received from the TR module
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, std::size_t size) {
    try {
        const TrInfo info = TrInfo::parse(std::vector<uint8_t>(data, data + size));
        if (info.ibk.has_value() && (size < TrInfo::EXTENDED_LENGTH || !info.supportsIbk())) {
            std::abort();
        }
        std::ostringstream ss;
        ss << info;
        if (ss.str().empty()) {
            std::abort();
        }
    } catch (const std::invalid_argument &) {
        // Too short Module Info block
    }
    return 0;
}
