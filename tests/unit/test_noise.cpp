/**
 * @file test_noise.cpp
 * @brief Noise generation tests
 */

#include <gtest/gtest.h>
#include <VoxelForge/utils/Noise.hpp>
#include <cmath>

using namespace VoxelForge;

class NoiseTest : public ::testing::Test {
protected:
    PerlinNoise perlin{12345};
    SimplexNoise simplex{12345};
    VoronoiNoise voronoi{12345};
};

TEST_F(NoiseTest, PerlinNoiseReturnsValidRange) {
    for (int i = 0; i < 100; i++) {
        float x = static_cast<float>(i) * 0.1f;
        float z = static_cast<float>(i) * 0.1f;
        float noise = perlin.noise(x, z);
        
        EXPECT_GE(noise, -1.0f);
        EXPECT_LE(noise, 1.0f);
    }
}

TEST_F(NoiseTest, PerlinNoiseIsDeterministic) {
    PerlinNoise noise1{42};
    PerlinNoise noise2{42};
    
    for (int i = 0; i < 50; i++) {
        float x = static_cast<float>(i) * 0.5f;
        float z = static_cast<float>(i * 2) * 0.3f;
        
        EXPECT_FLOAT_EQ(noise1.noise(x, z), noise2.noise(x, z));
    }
}

TEST_F(NoiseTest, PerlinNoiseDifferentSeeds) {
    PerlinNoise noise1{1};
    PerlinNoise noise2{2};
    
    bool allDifferent = true;
    for (int i = 0; i < 10; i++) {
        float x = static_cast<float>(i);
        float z = static_cast<float>(i * 2);
        
        if (std::abs(noise1.noise(x, z) - noise2.noise(x, z)) < 0.01f) {
            allDifferent = false;
            break;
        }
    }
    EXPECT_TRUE(allDifferent);
}

TEST_F(NoiseTest, OctaveNoiseReturnsValidRange) {
    float noise = perlin.octaveNoise(10.0f, 20.0f, 4, 0.5f);
    EXPECT_GE(noise, -1.0f);
    EXPECT_LE(noise, 1.0f);
}

TEST_F(NoiseTest, Noise01InRange) {
    for (int i = 0; i < 50; i++) {
        float noise = perlin.noise01(static_cast<float>(i), static_cast<float>(i * 2));
        EXPECT_GE(noise, 0.0f);
        EXPECT_LE(noise, 1.0f);
    }
}

TEST_F(NoiseTest, SimplexNoise2DReturnsValidRange) {
    for (int i = 0; i < 50; i++) {
        float x = static_cast<float>(i) * 0.1f;
        float y = static_cast<float>(i) * 0.1f;
        float noise = simplex.noise2D(x, y);
        
        EXPECT_GE(noise, -1.0f);
        EXPECT_LE(noise, 1.0f);
    }
}

TEST_F(NoiseTest, VoronoiDistancePositive) {
    for (int i = 0; i < 20; i++) {
        float x = static_cast<float>(i) * 0.5f;
        float y = static_cast<float>(i) * 0.3f;
        float dist = voronoi.distance2D(x, y);
        
        EXPECT_GE(dist, 0.0f);
    }
}

TEST_F(NoiseTest, VoronoiCellCoordinates) {
    auto cell = voronoi.noise2D(5.5f, 7.3f);
    
    // Cell coordinates should be integers
    EXPECT_EQ(cell.cellX, static_cast<int>(cell.cellX));
    EXPECT_EQ(cell.cellY, static_cast<int>(cell.cellY));
}

TEST_F(NoiseTest, NoiseUtilsSmoothstep) {
    EXPECT_FLOAT_EQ(NoiseUtils::smoothstep(0.0f, 1.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(NoiseUtils::smoothstep(0.0f, 1.0f, 1.0f), 1.0f);
    EXPECT_FLOAT_EQ(NoiseUtils::smoothstep(0.0f, 1.0f, 0.5f), 0.5f);
}

TEST_F(NoiseTest, NoiseUtilsRemap) {
    float result = NoiseUtils::remap(0.5f, 0.0f, 1.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(result, 50.0f);
    
    result = NoiseUtils::remap(0.0f, -1.0f, 1.0f, 0.0f, 255.0f);
    EXPECT_FLOAT_EQ(result, 127.5f);
}

TEST_F(NoiseTest, NoiseUtilsClamp) {
    EXPECT_FLOAT_EQ(NoiseUtils::clamp(5.0f, 0.0f, 10.0f), 5.0f);
    EXPECT_FLOAT_EQ(NoiseUtils::clamp(-5.0f, 0.0f, 10.0f), 0.0f);
    EXPECT_FLOAT_EQ(NoiseUtils::clamp(15.0f, 0.0f, 10.0f), 10.0f);
}

TEST_F(NoiseTest, Perlin3DReturnsValidRange) {
    for (int i = 0; i < 20; i++) {
        float x = static_cast<float>(i) * 0.1f;
        float y = static_cast<float>(i) * 0.2f;
        float z = static_cast<float>(i) * 0.3f;
        float noise = perlin.noise(x, y, z);
        
        EXPECT_GE(noise, -1.0f);
        EXPECT_LE(noise, 1.0f);
    }
}
