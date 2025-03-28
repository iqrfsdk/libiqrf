/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: Logger.h
 * Authors: Ondřej Hujňák <ondrej.hujnak@iqrf.org>
 * Date: 2025-03-24
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <string>

/**
 * IQRF severity levels.
 *
 * Severity levels are compatible with Syslog.
 */
#define _IQRF_LOG_LEVELS \
X(Fatal, 2)              \
X(Error, 3)              \
X(Warning, 4)            \
X(Info, 6)               \
X(Debug, 7)              \
X(Trace, 10)

/**
 * Main logging macro.
 */
#define IQRF_LOG(level)            \
    if (level > Logger::logLevel)  \
        ;                          \
    else                           \
        Logger().stream(level)

/**
 * Trace logging shorthand.
 * 
 * Includes file, line and function where it was called.
 */
#define IQRF_TRACE() \
    IQRF_LOG(iqrf::log::Level::Trace) << __FILE__ << ":" << __LINE__ << " - " << __func__ << "(): "


namespace iqrf::log {

/**
 * Enumeration for severity level used for setting the level of logging.
 */
enum class Level {
    #define X(name, value) name = value,
    _IQRF_LOG_LEVELS
    #undef X
};

/**
 * Map of severity level names.
 */
const std::unordered_map<Level, std::string> LevelNames = {
    #define X(name, value) {Level::name, #name},
    _IQRF_LOG_LEVELS
    #undef X
};

class ILog {
 public:
    virtual ~ILog() = default;
    virtual void append(const std::string& msg) = 0;
};

class Logger {
 public:
    static Level logLevel;
    static std::unique_ptr<ILog> log;

    Logger();
    virtual ~Logger();

    /**
     * Get the string stream to construct the log message.
     *
     * @param level is the severity level of the log message.
     *
     * @return String stream for the log message.
     *
     * @details Preferred use is via the IQRF_LOG macro.
     */
    std::ostringstream& stream(Level level = Level::Info);

 protected:
    std::ostringstream buffer;

 private:
    Level messageLevel;

    Logger(const Logger&) = default;
    Logger& operator=(const Logger&) = default;
};

class StderrLog : public ILog {
 public:
    void append(const std::string& msg) override;
};

}  // namespace iqrf::log
