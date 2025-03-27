#include <regex>

#include <gtest/gtest.h>

#include "iqrf/log/Logging.h"

namespace iqrf::log {

class LoggingTest : public ::testing::Test {
};

TEST_F(LoggingTest, CheckDefaultLogLevel) {
    ASSERT_EQ(Level::Error, Logger::logLevel);
}

TEST_F(LoggingTest, DefaultLoggingToStderr) {
    std::string log;

    ::testing::internal::CaptureStderr();
    Logger::logLevel = Level::Info;
    Logger().stream(Level::Info) << "Basic information logging.";
    log = ::testing::internal::GetCapturedStderr();
    ASSERT_STREQ(log.c_str(), "[Info] Basic information logging.\n");

    ::testing::internal::CaptureStderr();
    IQRF_LOG(Level::Error) << "An error occured.";
    log = ::testing::internal::GetCapturedStderr();
    ASSERT_STREQ(log.c_str(), "[Error] An error occured.\n");
}

TEST_F(LoggingTest, TestTraceInfo) {
    Logger::logLevel = Level::Trace;
    const std::regex trace_regex(
        "\\[Trace\\] .*/tests/LoggingTest.cpp:[0-9]+ - TestBody\\(\\): Tracing information.\n"
    );

    ::testing::internal::CaptureStderr();
    IQRF_TRACE() << "Tracing information.";
    std::string log = ::testing::internal::GetCapturedStderr();
    // The predicate assertion below is not working, unknown reference to ASSERT_THAT
    // ASSERT_THAT(log.c_str(), MatchesRegex(".*"));
    ASSERT_TRUE(std::regex_match(log, trace_regex));
}

}  // namespace iqrf::log
