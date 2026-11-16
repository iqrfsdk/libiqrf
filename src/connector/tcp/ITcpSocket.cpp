/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: ITcpSocket.cpp
 * Authors: Roman Ondráček <roman.ondracek@iqrf.com>
 * Date: 2026-11-16
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "iqrf/connector/tcp/ITcpSocket.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

#if defined(_WIN32)
#include "iqrf/connector/tcp/WinsockTcpSocket.h"
#elif defined(__unix__) || defined(__APPLE__)
#include "iqrf/connector/tcp/PosixTcpSocket.h"
#endif

namespace iqrf::connector::tcp {

std::unique_ptr<ITcpSocket> createTcpSocket(
    const std::string &host,
    const uint16_t port,
    const std::chrono::milliseconds timeout
) {
#if defined(_WIN32)
    return std::make_unique<WinsockTcpSocket>(host, port, timeout);
#elif defined(__unix__) || defined(__APPLE__)
    return std::make_unique<PosixTcpSocket>(host, port, timeout);
#else
    (void) host;
    (void) port;
    (void) timeout;
    throw std::runtime_error("TCP connector is not supported on this platform");
#endif
}

}  // namespace iqrf::connector::tcp
