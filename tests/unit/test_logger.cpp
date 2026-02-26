/**
 * @file test_logger.cpp
 * @brief Logger unit tests
 */

#include <gtest/gtest.h>
#include <VoxelForge/core/Logger.hpp>

using namespace VoxelForge;

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Logger already initialized in test_main
    }
};

TEST_F(LoggerTest, LoggerExists) {
    auto& logger = Logger::get();
    ASSERT_NE(logger, nullptr);
}

TEST_F(LoggerTest, ClientLoggerExists) {
    auto& logger = Logger::getClientLogger();
    ASSERT_NE(logger, nullptr);
}

TEST_F(LoggerTest, CanLogTrace) {
    EXPECT_NO_THROW(VF_CORE_TRACE("Test trace message"));
    EXPECT_NO_THROW(VF_TRACE("Test client trace message"));
}

TEST_F(LoggerTest, CanLogInfo) {
    EXPECT_NO_THROW(VF_CORE_INFO("Test info message"));
    EXPECT_NO_THROW(VF_INFO("Test client info message"));
}

TEST_F(LoggerTest, CanLogWarning) {
    EXPECT_NO_THROW(VF_CORE_WARN("Test warning message"));
    EXPECT_NO_THROW(VF_WARN("Test client warning message"));
}

TEST_F(LoggerTest, CanLogError) {
    EXPECT_NO_THROW(VF_CORE_ERROR("Test error message"));
    EXPECT_NO_THROW(VF_ERROR("Test client error message"));
}

TEST_F(LoggerTest, CanLogFormatted) {
    EXPECT_NO_THROW(VF_CORE_INFO("Formatted: {} {}", 42, "test"));
    EXPECT_NO_THROW(VF_INFO("Float: {:.2f}", 3.14159));
}

TEST_F(LoggerTest, CanLogMultipleArgs) {
    EXPECT_NO_THROW(VF_CORE_INFO("Multiple: {} {} {} {} {}", 1, 2, 3, 4, 5));
}
