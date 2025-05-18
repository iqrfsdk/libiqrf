/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: ConnectorUtils.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-05-10
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <cstdint>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>

namespace iqrf::connector {

/**
 * Utility class for IQRF connectors
 */
class ConnectorUtils {
 public:
    /**
     * Converts a vector of bytes to a hex string representation.
     * @param input Vector of bytes to convert
     * @return Hex string representation of the input vector
     */
    static std::string vectorToHexString(const std::vector<uint8_t>& input) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        for (size_t i = 0; i < input.size(); ++i) {
            ss << std::setw(2) << static_cast<int>(input[i]);
            if (i + 1 < input.size()) {
                ss << ".";
            }
        }

        return ss.str();
    }
};

}  // namespace iqrf::connector
