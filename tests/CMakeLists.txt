cmake_minimum_required(VERSION 3.26)

# Test sources (main test executable)
set(TEST_SOURCES
    unit/test_main.cpp
    unit/test_logger.cpp
    unit/test_timer.cpp
    unit/test_memory.cpp
    unit/test_ecs.cpp
    unit/test_tree_generation.cpp
    unit/test_noise.cpp
    unit/test_random.cpp
)

# NOTE: These test files exist but cannot be wired yet:
#   unit/test_daynight.cpp — compiles but linker errors (DayNightCycle methods undefined)
#   unit/test_vulkan_buffer.cpp — compiles but likely linker errors (Vulkan Buffer class)
#   unit/test_chunk.cpp — compile error (ChunkPos constructor mismatch)
#   unit/test_chunk_mesher.cpp — compile error (ChunkMesher API mismatch)
#   unit/test_renderer.cpp — compile error (Camera constructor mismatch)
#   test_fluid.cpp — compile errors (FluidProperties API mismatch), has own main()

# Create main test executable
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
