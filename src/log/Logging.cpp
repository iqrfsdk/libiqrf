/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: Logger.cpp
 * Authors: Ondřej Hujňák <ondrej.hujnak@iqrf.org>
 * Date: 2025-03-24
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include "iqrf/log/Logging.h"

namespace iqrf::log {

std::unique_ptr<ILog> Logger::log = std::make_unique<StderrLog>();

// Default Log Level is Error
Level Logger::logLevel = Level::Error;

// Default Message Level is Info
Logger::Logger(): messageLevel(Level::Info) {}

// cppcheck-suppress unusedFunction
std::ostringstream& Logger::stream(Level level) {
    messageLevel = level;

    // Include a header with log level
    buffer << "[" << LevelNames.at(level) << "] ";

    return buffer;
}

Logger::~Logger() {
    buffer << std::endl;
    log->append(buffer.str());
}

void StderrLog::append(const std::string& msg) {
    fprintf(stderr, "%s", msg.c_str());
    fflush(stderr);
}

}  // namespace iqrf::log
