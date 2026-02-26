/**
 * @file test_main.cpp
 * @brief Test entry point
 */

#include <gtest/gtest.h>
#include <VoxelForge/core/Logger.hpp>

class TestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        VoxelForge::Logger::init();
    }
    
    void TearDown() override {
        VoxelForge::Logger::shutdown();
    }
};

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new TestEnvironment());
    return RUN_ALL_TESTS();
}
