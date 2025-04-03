/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: main.cpp
 * Authors: Ondřej Hujňák <ondrej.hujnak@iqrf.org>
 * Date: 2025-04-03
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */
#include <iostream>
#include <boost/core/null_deleter.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/trivial.hpp>

// Override the log header so it does not duplicate the severity level
#define IQRF_LOG_HEADER(level) ""

#include "iqrf/log/Logging.h"

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;

namespace iqrf_log = ::iqrf::log;

namespace iqrf::log {

/**
 * Implementation of the Log interface for logging into boost::log.
 */
class BoostLogTrivial : public ILog {
 public:
  /**
   * Appends the message to Boost logger with default severity.
   *
   * @param msg is a message which shall be appended.
   */
  void append(const std::string& msg) override {
    this->append(msg, Level::Info);
  }

  /**
   * Appends the message to Boost logger with severity `severity`.
   *
   * @param msg is a message which shall be appended.
   * @param severity is the severity level of the message.
   * 
   * @details Boost Trivial Logging uses lvl from BOOST_LOG_TRIVIAL(lvl) to expand
   * ::boost::log::trivial::lvl, so unfortunatelly no simpler mapping is possible.
   */
  void append(const std::string& msg, const Level& severity) override {
    switch (severity) {
    case Level::Fatal:
        BOOST_LOG_TRIVIAL(fatal) << msg;
        break;
    case Level::Error:
        BOOST_LOG_TRIVIAL(error) << msg;
        break;
    case Level::Warning:
        BOOST_LOG_TRIVIAL(warning) << msg;
        break;
    case Level::Info:
        BOOST_LOG_TRIVIAL(info) << msg;
        break;
    case Level::Debug:
        BOOST_LOG_TRIVIAL(debug) << msg;
        break;
    case Level::Trace:
        BOOST_LOG_TRIVIAL(trace) << msg;
        break;
    default:
        BOOST_LOG_TRIVIAL(info) << msg;
        break;
    }
  }
};

}  // namespace iqrf::log

/**
 * Initialize Boost logging to STDOUT
 */
void init_logging() {
    // Create the STDOUT backend
    boost::shared_ptr<sinks::text_ostream_backend> backend =
        boost::make_shared<sinks::text_ostream_backend>();
    backend->add_stream(
        boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));
    backend->auto_flush(true);

    // Wrap it into the frontend
    typedef sinks::synchronous_sink<sinks::text_ostream_backend> sink_t;
    boost::shared_ptr<sink_t> sink(new sink_t(backend));

    // Add information that this was logged by Boost
    sink->set_formatter(expr::stream << "(Boost) [" << logging::trivial::severity << "] " << expr::smessage);

    // Register the sink in the core
    logging::core::get()->add_sink(sink);
}

int main() {
    // Initialize logging with boost::log
    init_logging();

    // Initialize logging with libiqrf
    iqrf_log::Logger::logLevel = iqrf_log::Level::Trace;
    iqrf_log::Logger::log = std::make_unique<iqrf_log::BoostLogTrivial>();

    // Write several log messages
    IQRF_LOG(iqrf_log::Level::Trace) << "This is a trace message.";
    IQRF_LOG(iqrf_log::Level::Debug) << "This is a debug message.";
    IQRF_LOG(iqrf_log::Level::Info) << "This is an informational message.";
    IQRF_LOG(iqrf_log::Level::Warning) << "This is a warning message.";
    IQRF_LOG(iqrf_log::Level::Error) << "This is an error message.";
    IQRF_LOG(iqrf_log::Level::Fatal) << "This is a fatal error message.";

    return 0;
}
