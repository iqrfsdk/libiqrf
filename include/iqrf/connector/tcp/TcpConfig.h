/**
 * Copyright 2023-2025 MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: TcpConfig.h
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2025-07-14
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace iqrf::connector::tcp {

/**
 * IQRF TCP connector configuration
 */
class TcpConfig {
 public:
    /// TCP host address (hostname or IP address)
    std::string host;
    /// TCP port number
    uint16_t port = 10000;

    /**
     * Constructs the TCP connector configuration
     */
    explicit TcpConfig(
        std::string host,
        const uint16_t port = 10000
    ): host(std::move(host)), port(port) {}

};

}  // namespace iqrf::connector::tcp
