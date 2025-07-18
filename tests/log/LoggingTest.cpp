/**
 * Copyright MICRORISC s.r.o.
 * SPDX-License-Identifier: Apache-2.0
 * File: LoggingTest.cpp
 * Authors: Ondřej Hujňák <ondrej.hujnak@iqrf.com>
 * Date: 2025-03-28
 *
 * This file is a part of the LIBIQRF. For the full license information, see the
 * LICENSE file in the project root.
 */

#include <gtest/gtest.h>

#include <regex>

#include "iqrf/log/Logging.h"

namespace iqrf::log {

class LoggingTest : public ::testing::Test {
};

class StdoutLog : public ILog {
 public:
    void append(const std::string& msg) override {
        fprintf(stdout, "%s", msg.c_str());
        fflush(stdout);
    };
};

TEST_F(LoggingTest, CheckDefaultLogLevel) {
    ASSERT_EQ(Level::Error, Logger::logLevel);
}

TEST_F(LoggingTest, DefaultLogging) {
    std::string message;

    Logger::logLevel = Level::Info;

    ::testing::internal::CaptureStderr();
    Logger().stream(Level::Info) << "Basic information logging.";
    message = ::testing::internal::GetCapturedStderr();
    ASSERT_STREQ(message.c_str(), "Basic information logging.\n");

    ::testing::internal::CaptureStderr();
    IQRF_LOG(Level::Error) << "An error occurred.";
    message = ::testing::internal::GetCapturedStderr();
    ASSERT_STREQ(message.c_str(), "[Error] An error occurred.\n");
}

TEST_F(LoggingTest, SeverityLevels) {
    std::string message;

    Logger::logLevel = Level::Warning;

    ::testing::internal::CaptureStderr();
    IQRF_LOG(Level::Fatal) << "Fatal error.";
    message = ::testing::internal::GetCapturedStderr();
    EXPECT_STREQ(message.c_str(), "[Fatal] Fatal error.\n");

    ::testing::internal::CaptureStderr();
    IQRF_LOG(Level::Error) << "Error.";
    message = ::testing::internal::GetCapturedStderr();
    EXPECT_STREQ(message.c_str(), "[Error] Error.\n");

    ::testing::internal::CaptureStderr();
    IQRF_LOG(Level::Warning) << "Warning.";
    message = ::testing::internal::GetCapturedStderr();
    EXPECT_STREQ(message.c_str(), "[Warning] Warning.\n");

    ::testing::internal::CaptureStderr();
    IQRF_LOG(Level::Info) << "Info.";
    message = ::testing::internal::GetCapturedStderr();
    EXPECT_STREQ(message.c_str(), "");

    Logger::logLevel = Level::Info;

    ::testing::internal::CaptureStderr();
    IQRF_LOG(Level::Info) << "Info.";
    message = ::testing::internal::GetCapturedStderr();
    EXPECT_STREQ(message.c_str(), "[Info] Info.\n");
}

TEST_F(LoggingTest, VerifyTraceInfo) {
    Logger::logLevel = Level::Trace;
    const std::regex trace_regex(
        "\\[Trace\\] LoggingTest.cpp:[0-9]+ - TestBody\\(\\): Tracing information.\n");

    ::testing::internal::CaptureStderr();

    IQRF_TRACE() << "Tracing information.";

    const std::string message = ::testing::internal::GetCapturedStderr();
    // The predicate assertion below is not working, unknown reference to ASSERT_THAT
    // ASSERT_THAT(log.c_str(), MatchesRegex(".*"));
    ASSERT_TRUE(std::regex_match(message, trace_regex));
}

TEST_F(LoggingTest, AlternativeLog) {
    std::string message;

    Logger::log = std::make_unique<StdoutLog>();
    Logger::logLevel = Level::Trace;

    ::testing::internal::CaptureStdout();
    ::testing::internal::CaptureStderr();

    IQRF_LOG(Level::Debug) << "Standard output.";

    message = ::testing::internal::GetCapturedStderr();
    EXPECT_STREQ(message.c_str(), "");

    message = ::testing::internal::GetCapturedStdout();
    ASSERT_STREQ(message.c_str(), "[Debug] Standard output.\n");
}

}  // namespace iqrf::log
