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
    if ((level) > ::iqrf::log::Logger::logLevel)  \
        ;                          \
    else                           \
        iqrf::log::Logger().stream(level) << IQRF_LOG_HEADER(level)

/**
 * Trace logging shorthand.
 *
 * Includes file, line and function where it was called.
 */
#define IQRF_TRACE() \
    IQRF_LOG(::iqrf::log::Level::Trace) << __FILENAME__ << ":" << __LINE__ << " - " << __func__ << "(): "

/**
 * Log header is prepended to every log message.
 */
#ifndef IQRF_LOG_HEADER
#define IQRF_LOG_HEADER(level) "[" << ::iqrf::log::LevelNames.at(level) << "] "
#endif

/**
 * Strip the path from __FILE__ and keep only file name.
 */
#ifndef __FILENAME__
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

namespace iqrf::log {

/**
 * Enumeration for severity level used for setting the level of logging.
 */
enum class Level {
    #define X(name, value) name = (value),
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

/**
 * Log interface.
 *
 * Represents an output where logs are stored.
 */
class ILog {
 public:
    /**
     * Virtual destructor to prevent possible inheritance leaks.
     */
    virtual ~ILog() = default;

    /**
     * Add the message `msg` to the log.
     *
     * @param msg is a message which shall be appended.
     */
    virtual void append(const std::string& msg) = 0;

    /**
     * Add the message `msg` to the log with severity level `severity`.
     *
     * @param msg is a message which shall be appended.
     * @param severity is the severity level of the message.
     */
    virtual void append(const std::string& msg, const Level& severity);
};

/**
 * The main logging class.
 *
 * Accumulates the log messages and then appends it to a log output.
 */
class Logger {
 public:
    /**
     * Reporting level which is shared amongst all loggers.
     *
     * Defaults to Level::Error.
     */
    static Level logLevel;
    /**
     * Output log shared amongst all loggers.
     *
     * Defaults to logging to stderr.
     */
    static std::unique_ptr<ILog> log;

    /**
     * Constructor
     *
     * Creates a new empty Logger instance. The default message severity level is Level::Info.
     */
    Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * Writes the contents of accumulated log message and destructs the Logger.
     */
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
    /**
     * String stream where the log message is accumulated.
     */
    std::ostringstream buffer;

 private:
    /**
     * The severity level of this message.
     */
    Level messageLevel;
};

/**
 * Implementation of the Log interface for logging into stderr.
 */
class StderrLog : public ILog {
 public:
    /**
     * Appends the message to stderr.
     */
    void append(const std::string& msg) override;
};

}  // namespace iqrf::log
