/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: DpaConstants.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-10-12
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include "iqrf/dpa/DPA.h"

namespace iqrf::dpa {

/// Length of the DPA interface header (NADR, PNUM, PCMD, HWPID)
constexpr std::size_t DPA_HEADER_LENGTH = 6;
/// Offset of the response code in the response
constexpr std::size_t RESPONSE_CODE_INDEX = 6;
/// Offset of the DPA message (payload) in the response
constexpr std::size_t RESPONSE_PAYLOAD_INDEX = 8;

/**
 * Response codes (status) of the DPA messages
 */
enum class DpaStatus : uint8_t {
    NoError = 0,
    Fail = 1,
    Pcmd = 2,
    Pnum = 3,
    Addr = 4,
    DataLen = 5,
    Data = 6,
    Hwpid = 7,
    Nadr = 8,
    IfaceCustomHandler = 9,
    MissingCustomDpaHandler = 10,
    UserFrom = 0x20,
    UserTo = 0x3F,
    Reserved = 0x40,
    AsyncResponse = 0x80,
    Confirmation = 0xFF
};

}  // namespace iqrf::dpa
