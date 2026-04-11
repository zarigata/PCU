/**
 * @file test_fluid.cpp
 * @brief Unit tests for FluidSystem
 */

#include <gtest/gtest.h>
#include <VoxelForge/world/FluidSystem.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/world/Block.hpp>

using namespace VoxelForge;

// ============================================
// Basic FluidState tests
// ============================================

TEST(FluidState, DefaultConstruction) {
    FluidState state;
    EXPECT_EQ(state.type, FluidType::Empty);
    EXPECT_EQ(state.level, 0);
    EXPECT_FALSE(state.falling);
    EXPECT_TRUE(state.isEmpty());
    EXPECT_TRUE(state.isSource());
}

TEST(FluidState, WaterSource) {
    FluidState state;
    state.type = FluidType::Water;
    state.level = 0;
    state.falling = false;

    EXPECT_FALSE(state.isEmpty());
    EXPECT_TRUE(state.isSource());
    EXPECT_EQ(state.getFlowDistance(), 0);
}

TEST(FluidState, FlowingWater) {
    FluidState state;
    state.type = FluidType::Water;
    state.level = 3;
    state.falling = false;

    EXPECT_FALSE(state.isEmpty());
    EXPECT_FALSE(state.isSource());
    EXPECT_EQ(state.getFlowDistance(), 3);
}

TEST(FluidState, FallingLava) {
    FluidState state;
    state.type = FluidType::Lava;
    state.level = 0;
    state.falling = true;

    EXPECT_FALSE(state.isEmpty());
    EXPECT_TRUE(state.isSource());
    EXPECT_TRUE(state.falling);
}

// ============================================
// FluidSystem registration tests
// ============================================

TEST(FluidSystem, RegisterDefaultFluids) {
    FluidSystem system;

    // Water should be registered
    FluidProperties water = system.getProperties(FluidType::Water);
    EXPECT_EQ(water.flowDistance, FluidSystem::MAX_FLOW_DISTANCE_WATER);
    EXPECT_EQ(water.flowSpeed, FluidSystem::WATER_FLOW_SPEED);
    EXPECT_FALSE(water.canSetFire);
    EXPECT_TRUE(water.canExtinguish);

    // Lava should be registered
    FluidProperties lava = system.getProperties(FluidType::Lava);
    EXPECT_EQ(lava.flowDistance, FluidSystem::MAX_FLOW_DISTANCE_LAVA_OVERWORLD);
    EXPECT_EQ(lava.flowSpeed, FluidSystem::LAVA_FLOW_SPEED);
    EXPECT_TRUE(lava.canSetFire);
    EXPECT_FALSE(lava.canExtinguish);
    EXPECT_TRUE(lava.harmsEntities);
    EXPECT_FLOAT_EQ(lava.brightness, 15.0f);
}

TEST(FluidSystem, RegisterCustomFluid) {
    FluidSystem system;

    FluidSystem::FluidProperties custom;
    custom.flowDistance = 5;
    custom.flowSpeed = 10;
    custom.canSetFire = true;
    custom.canExtinguish = false;
    custom.harmsEntities = false;
    custom.brightness = 5.0f;

    system.registerFluidType(FluidType::Custom1, custom);

    FluidProperties retrieved = system.getProperties(FluidType::Custom1);
    EXPECT_EQ(retrieved.flowDistance, 5);
    EXPECT_EQ(retrieved.flowSpeed, 10);
    EXPECT_TRUE(retrieved.canSetFire);
}

// ============================================
// Block state helpers tests
// ============================================

TEST(FluidSystem, GetSourceBlockState) {
    FluidSystem system;

    BlockState water = system.getSourceBlockState(FluidType::Water);
    EXPECT_TRUE(water.isValid());

    BlockState lava = system.getSourceBlockState(FluidType::Lava);
    EXPECT_TRUE(lava.isValid());

    BlockState empty = system.getSourceBlockState(FluidType::Empty);
    // Empty should return air block
}

TEST(FluidSystem, GetFlowingBlockState) {
    FluidSystem system;

    BlockState flowingWater = system.getFlowingBlockState(FluidType::Water, 2, false);
    EXPECT_TRUE(flowingWater.isValid());

    BlockState fallingWater = system.getFlowingBlockState(FluidType::Water, 0, true);
    EXPECT_TRUE(fallingWater.isValid());

    BlockState flowingLava = system.getFlowingBlockState(FluidType::Lava, 3, false);
    EXPECT_TRUE(flowingLava.isValid());

    BlockState fallingLava = system.getFlowingBlockState(FluidType::Lava, 1, true);
    EXPECT_TRUE(fallingLava.isValid());
}

// ============================================
// Scheduling tests
// ============================================

TEST(FluidSystem, ScheduleUpdate) {
    FluidSystem system;

    BlockPos pos(0, 0, 0);

    EXPECT_EQ(system.getPendingUpdateCount(), 0);
    EXPECT_EQ(system.getActiveFluidCount(), 0);

    system.scheduleUpdate(pos, 0);

    EXPECT_EQ(system.getPendingUpdateCount(), 1);
    EXPECT_EQ(system.getActiveFluidCount(), 1);

    // Duplicate scheduling should be ignored
    system.scheduleUpdate(pos, 0);

    EXPECT_EQ(system.getPendingUpdateCount(), 1);
    EXPECT_EQ(system.getActiveFluidCount(), 1);
}

TEST(FluidSystem, ScheduleMultipleUpdates) {
    FluidSystem system;

    BlockPos pos1(0, 0, 0);
    BlockPos pos2(1, 0, 0);
    BlockPos pos3(2, 0, 0);

    system.scheduleUpdate(pos1, 5);
    system.scheduleUpdate(pos2, 10);
    system.scheduleUpdate(pos3, 0);

    EXPECT_EQ(system.getPendingUpdateCount(), 3);
    EXPECT_EQ(system.getActiveFluidCount(), 3);
}

// ============================================
// Flow distance limits tests
// ============================================

TEST(FluidSystem, WaterFlowDistance) {
    FluidSystem system;

    FluidProperties water = system.getProperties(FluidType::Water);
    EXPECT_EQ(water.flowDistance, FluidSystem::MAX_FLOW_DISTANCE_WATER);
}

TEST(FluidSystem, LavaFlowDistance) {
    FluidSystem system;

    FluidProperties lava = system.getProperties(FluidType::Lava);
    EXPECT_EQ(lava.flowDistance, FluidSystem::MAX_FLOW_DISTANCE_LAVA_OVERWORLD);
}

// ============================================
// Main entry point
// ============================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
