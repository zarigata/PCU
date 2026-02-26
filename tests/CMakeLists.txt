cmake_minimum_required(VERSION 3.26)

# Test sources
set(TEST_SOURCES
    unit/test_main.cpp
    unit/test_logger.cpp
    unit/test_timer.cpp
    unit/test_memory.cpp
    unit/test_ecs.cpp
)

# Create test executable
add_executable(VoxelForge_Tests ${TEST_SOURCES})

target_link_libraries(VoxelForge_Tests
    PRIVATE
        VoxelForge_Engine
        gtest
        gtest_main
)

# Include directories
target_include_directories(VoxelForge_Tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Add tests
include(GoogleTest)
gtest_discover_tests(VoxelForge_Tests)
