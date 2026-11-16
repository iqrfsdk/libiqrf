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

#include <chrono>
#include <cstdint>
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
    /// Maximum time to establish the connection to a single resolved address
    std::chrono::milliseconds connectTimeout{5000};
    /// Delay before the first reconnection attempt, doubled after each failed attempt
    std::chrono::milliseconds reconnectDelay{1000};
    /// Maximum delay between reconnection attempts
    std::chrono::milliseconds maxReconnectDelay{64000};

    /**
     * Constructs the TCP connector configuration
     */
    explicit TcpConfig(
        std::string host,
        const uint16_t port = 10000
    ): host(std::move(host)), port(port) {}
};

}  // namespace iqrf::connector::tcp
