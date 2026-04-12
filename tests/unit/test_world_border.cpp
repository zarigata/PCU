/**
 * @file test_world_border.cpp
 * @brief Unit tests for WorldBorder
 */

#include <gtest/gtest.h>
#include <VoxelForge/world/WorldBorder.hpp>
#include <cmath>

using namespace VoxelForge;

TEST(WorldBorderTest, DefaultBorderIsInside) {
    WorldBorder border;
    // Default size is 60M — everything should be inside
    EXPECT_TRUE(border.isInside(0.0, 0.0));
    EXPECT_TRUE(border.isInside(1000000.0, 1000000.0));
    EXPECT_TRUE(border.isInside(-1000000.0, -1000000.0));
}

TEST(WorldBorderTest, SetSizeClampToMinimum) {
    WorldBorder border;
    border.setSize(-50.0);
    EXPECT_GE(border.getSize(), 1.0);
}

TEST(WorldBorderTest, SmallBorderChecks) {
    WorldBorder border;
    border.setSize(100.0);
    border.setCenter(0.0, 0.0);

    EXPECT_TRUE(border.isInside(0.0, 0.0));
    EXPECT_TRUE(border.isInside(49.0, 49.0));
    EXPECT_FALSE(border.isInside(51.0, 0.0));
    EXPECT_FALSE(border.isInside(0.0, 51.0));
}

TEST(WorldBorderTest, DistanceToBorderInside) {
    WorldBorder border;
    border.setSize(100.0);
    border.setCenter(0.0, 0.0);

    double dist = border.distanceToBorder(0.0, 0.0);
    EXPECT_DOUBLE_EQ(dist, 50.0);
}

TEST(WorldBorderTest, DistanceToBorderOutside) {
    WorldBorder border;
    border.setSize(100.0);
    border.setCenter(0.0, 0.0);

    double dist = border.distanceToBorder(60.0, 0.0);
    EXPECT_DOUBLE_EQ(dist, -10.0);
}

TEST(WorldBorderTest, CenterOffset) {
    WorldBorder border;
    border.setSize(100.0);
    border.setCenter(100.0, 200.0);

    EXPECT_TRUE(border.isInside(100.0, 200.0));
    EXPECT_TRUE(border.isInside(149.0, 249.0));
    EXPECT_FALSE(border.isInside(151.0, 200.0));
    EXPECT_FALSE(border.isInside(100.0, 151.0));

    EXPECT_DOUBLE_EQ(border.getMinX(), 50.0);
    EXPECT_DOUBLE_EQ(border.getMaxX(), 150.0);
    EXPECT_DOUBLE_EQ(border.getMinZ(), 150.0);
    EXPECT_DOUBLE_EQ(border.getMaxZ(), 250.0);
}

TEST(WorldBorderTest, TransitionAnimation) {
    WorldBorder border;
    border.setSize(100.0);
    border.setCenter(0.0, 0.0);

    // Animate from 100 to 200 over 1000ms
    border.interpolateSize(200.0, 1000);
    EXPECT_TRUE(border.isTransitioning());
    EXPECT_DOUBLE_EQ(border.getTargetSize(), 200.0);

    // After 500ms, should be halfway
    border.tick(500);
    EXPECT_NEAR(border.getSize(), 150.0, 0.01);
    EXPECT_TRUE(border.isTransitioning());

    // After remaining 500ms, should reach target
    border.tick(500);
    EXPECT_DOUBLE_EQ(border.getSize(), 200.0);
    EXPECT_FALSE(border.isTransitioning());
}

TEST(WorldBorderTest, TransitionZeroDuration) {
    WorldBorder border;
    border.setSize(100.0);
    border.interpolateSize(200.0, 0);
    EXPECT_DOUBLE_EQ(border.getSize(), 200.0);
    EXPECT_FALSE(border.isTransitioning());
}

TEST(WorldBorderTest, WarningZone) {
    WorldBorder border;
    border.setSize(100.0);
    border.setCenter(0.0, 0.0);
    border.setWarningDistance(10);

    // Center is far from edge — not in warning zone
    EXPECT_FALSE(border.isInWarningZone(0.0, 0.0));

    // Just inside the edge — in warning zone
    EXPECT_TRUE(border.isInWarningZone(46.0, 0.0));
    EXPECT_TRUE(border.isInWarningZone(0.0, 46.0));

    // Outside border — not in warning zone (it's past it)
    EXPECT_FALSE(border.isInWarningZone(60.0, 0.0));
}

TEST(WorldBorderTest, SnapshotRoundTrip) {
    WorldBorder border;
    border.setSize(500.0);
    border.setCenter(10.0, 20.0);
    border.setDamagePerSecond(3.0f);
    border.setSafeZoneBlocks(7);
    border.setWarningDistance(15);
    border.setWarningTimeSeconds(30);

    auto snap = border.takeSnapshot();
    EXPECT_DOUBLE_EQ(snap.centerX, 10.0);
    EXPECT_DOUBLE_EQ(snap.centerZ, 20.0);
    EXPECT_DOUBLE_EQ(snap.size, 500.0);
    EXPECT_FLOAT_EQ(snap.damagePerSecond, 3.0f);
    EXPECT_EQ(snap.safeZoneBlocks, 7);
    EXPECT_EQ(snap.warningDistance, 15);
    EXPECT_EQ(snap.warningTimeSeconds, 30);

    // Apply to a fresh border
    WorldBorder other;
    other.applySnapshot(snap);
    EXPECT_DOUBLE_EQ(other.getSize(), 500.0);
    EXPECT_DOUBLE_EQ(other.getCenterX(), 10.0);
    EXPECT_EQ(other.getSafeZoneBlocks(), 7);
}

TEST(WorldBorderTest, RemainingTransitionTime) {
    WorldBorder border;
    border.setSize(100.0);
    border.interpolateSize(200.0, 1000);

    EXPECT_EQ(border.getRemainingTransitionMs(), 1000);

    border.tick(300);
    EXPECT_EQ(border.getRemainingTransitionMs(), 700);

    border.tick(700);
    EXPECT_EQ(border.getRemainingTransitionMs(), 0);
    EXPECT_FALSE(border.isTransitioning());
}

TEST(WorldBorderTest, Vec3Overloads) {
    WorldBorder border;
    border.setSize(100.0);
    border.setCenter(0.0, 0.0);

    EXPECT_TRUE(border.isInside(glm::vec3(0.0f, 50.0f, 0.0f)));
    EXPECT_TRUE(border.isInside(glm::dvec3(0.0, 50.0, 0.0)));
    EXPECT_FALSE(border.isInside(glm::vec3(60.0f, 0.0f, 0.0f)));
}
