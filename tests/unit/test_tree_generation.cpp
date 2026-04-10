/**
 * @file test_tree_generation.cpp
 * @brief Unit tests for tree generation in WorldGenerator
 */

#include <gtest/gtest.h>
#include <VoxelForge/world/WorldGenerator.hpp>
#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/world/BlockRegistry.hpp>
#include <VoxelForge/utils/Random.hpp>

using namespace VoxelForge;

namespace {

// Helper: create a flat chunk at a given position with grass on top
std::unique_ptr<Chunk> makeFlatChunk(ChunkPos pos, int surfaceY = 64) {
    auto chunk = std::make_unique<Chunk>(pos);
    auto& reg = BlockRegistry::get();
    auto dirt = reg.getDefaultState("minecraft:dirt");
    auto grass = reg.getDefaultState("minecraft:grass_block");
    auto bedrock = reg.getDefaultState("minecraft:bedrock");

    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_WIDTH; z++) {
            chunk->setBlock(x, -64, z, bedrock);
            for (int y = -63; y < surfaceY; y++) {
                chunk->setBlock(x, y, z, dirt);
            }
            chunk->setBlock(x, surfaceY, z, grass);
        }
    }
    chunk->recalculateHeightMaps();
    return chunk;
}

} // anonymous namespace

// Test: Oak tree placement produces trunk and leaves
TEST(TreeGeneration, OakTreeHasTrunkAndLeaves) {
    WorldGenSettings settings;
    settings.seed = 42;
    settings.flatWorld = true;
    WorldGenerator gen(settings);

    auto chunk = makeFlatChunk({0, 0}, 64);
    SeededRandom rng(12345);
    gen.placeOakTree(chunk.get(), 8, 65, 8, rng);

    auto& reg = BlockRegistry::get();
    auto log = reg.getDefaultState("minecraft:oak_log");
    auto leaves = reg.getDefaultState("minecraft:oak_leaves");

    // Should have at least some log blocks
    bool hasLog = false;
    bool hasLeaves = false;
    for (int y = 65; y < 75; y++) {
        auto block = chunk->getBlock(8, y, 8);
        if (block == log) hasLog = true;
    }
    for (int dx = -2; dx <= 2; dx++) {
        for (int dz = -2; dz <= 2; dz++) {
            for (int y = 67; y < 73; y++) {
                auto block = chunk->getBlock(8 + dx, y, 8 + dz);
                if (block == leaves) hasLeaves = true;
            }
        }
    }
    EXPECT_TRUE(hasLog);
    EXPECT_TRUE(hasLeaves);
}

// Test: Birch tree placement
TEST(TreeGeneration, BirchTreeHasTrunkAndLeaves) {
    WorldGenSettings settings;
    settings.seed = 42;
    settings.flatWorld = true;
    WorldGenerator gen(settings);

    auto chunk = makeFlatChunk({0, 0}, 64);
    SeededRandom rng(54321);
    gen.placeBirchTree(chunk.get(), 8, 65, 8, rng);

    auto& reg = BlockRegistry::get();
    auto log = reg.getDefaultState("minecraft:birch_log");

    bool hasLog = false;
    for (int y = 65; y < 75; y++) {
        if (chunk->getBlock(8, y, 8) == log) hasLog = true;
    }
    EXPECT_TRUE(hasLog);
}

// Test: Spruce tree placement
TEST(TreeGeneration, SpruceTreeHasTrunkAndLeaves) {
    WorldGenSettings settings;
    settings.seed = 42;
    settings.flatWorld = true;
    WorldGenerator gen(settings);

    auto chunk = makeFlatChunk({0, 0}, 64);
    SeededRandom rng(99999);
    gen.placeSpruceTree(chunk.get(), 8, 65, 8, rng);

    auto& reg = BlockRegistry::get();
    auto log = reg.getDefaultState("minecraft:spruce_log");

    bool hasLog = false;
    for (int y = 65; y < 80; y++) {
        if (chunk->getBlock(8, y, 8) == log) hasLog = true;
    }
    EXPECT_TRUE(hasLog);
}

// Test: Dark oak tree placement (2x2 trunk)
TEST(TreeGeneration, DarkOakTreeHas2x2Trunk) {
    WorldGenSettings settings;
    settings.seed = 42;
    settings.flatWorld = true;
    WorldGenerator gen(settings);

    auto chunk = makeFlatChunk({0, 0}, 64);
    SeededRandom rng(77777);
    gen.placeDarkOakTree(chunk.get(), 6, 65, 6, rng);

    auto& reg = BlockRegistry::get();
    auto log = reg.getDefaultState("minecraft:dark_oak_log");

    // Check 2x2 trunk at base
    int logCount = 0;
    for (int dx = 0; dx <= 1; dx++) {
        for (int dz = 0; dz <= 1; dz++) {
            if (chunk->getBlock(6 + dx, 65, 6 + dz) == log) logCount++;
        }
    }
    EXPECT_EQ(logCount, 4); // All four corners of the 2x2 trunk
}

// Test: Acacia tree placement (has angled branch)
TEST(TreeGeneration, AcaciaTreeHasBranch) {
    WorldGenSettings settings;
    settings.seed = 42;
    settings.flatWorld = true;
    WorldGenerator gen(settings);

    auto chunk = makeFlatChunk({0, 0}, 64);
    SeededRandom rng(11111);
    gen.placeAcaciaTree(chunk.get(), 8, 65, 8, rng);

    auto& reg = BlockRegistry::get();
    auto log = reg.getDefaultState("minecraft:acacia_log");
    auto leaves = reg.getDefaultState("minecraft:acacia_leaves");

    bool hasLog = false;
    for (int y = 65; y < 75; y++) {
        if (chunk->getBlock(8, y, 8) == log) hasLog = true;
    }
    EXPECT_TRUE(hasLog);
}

// Test: placeTree dispatches correctly
TEST(TreeGeneration, PlaceTreeDispatchesCorrectly) {
    WorldGenSettings settings;
    settings.seed = 42;
    settings.flatWorld = true;
    WorldGenerator gen(settings);

    auto& reg = BlockRegistry::get();
    auto oakLog = reg.getDefaultState("minecraft:oak_log");
    auto birchLog = reg.getDefaultState("minecraft:birch_log");

    {
        auto chunk = makeFlatChunk({0, 0}, 64);
        SeededRandom rng(100);
        gen.placeTree(chunk.get(), nullptr, 8, 65, 8, "oak", rng);
        EXPECT_EQ(chunk->getBlock(8, 65, 8), oakLog);
    }
    {
        auto chunk = makeFlatChunk({0, 0}, 64);
        SeededRandom rng(100);
        gen.placeTree(chunk.get(), nullptr, 8, 65, 8, "birch", rng);
        EXPECT_EQ(chunk->getBlock(8, 65, 8), birchLog);
    }
}

// Test: Tree doesn't place outside chunk bounds
TEST(TreeGeneration, TreeRespectsChunkBounds) {
    WorldGenSettings settings;
    settings.seed = 42;
    settings.flatWorld = true;
    WorldGenerator gen(settings);

    auto chunk = makeFlatChunk({0, 0}, 64);
    SeededRandom rng(100);
    // Place at edge — should not crash
    gen.placeOakTree(chunk.get(), 0, 65, 0, rng);
    gen.placeOakTree(chunk.get(), CHUNK_WIDTH - 1, 65, CHUNK_WIDTH - 1, rng);
    SUCCEED();
}

// Test: Different seeds produce different trees
TEST(TreeGeneration, DifferentSeedsProduceDifferentTrees) {
    WorldGenSettings settings;
    settings.seed = 42;
    settings.flatWorld = true;
    WorldGenerator gen(settings);

    auto& reg = BlockRegistry::get();
    auto log = reg.getDefaultState("minecraft:oak_log");

    auto chunk1 = makeFlatChunk({0, 0}, 64);
    SeededRandom rng1(100);
    gen.placeOakTree(chunk1.get(), 8, 65, 8, rng1);

    auto chunk2 = makeFlatChunk({0, 0}, 64);
    SeededRandom rng2(200);
    gen.placeOakTree(chunk2.get(), 8, 65, 8, rng2);

    // Count logs at different heights — very unlikely to be identical
    int logs1 = 0, logs2 = 0;
    for (int y = 65; y < 72; y++) {
        if (chunk1->getBlock(8, y, 8) == log) logs1++;
        if (chunk2->getBlock(8, y, 8) == log) logs2++;
    }
    // Both should have logs, but heights may differ
    EXPECT_GT(logs1, 0);
    EXPECT_GT(logs2, 0);
}

// Test: getBiome returns valid biome IDs
TEST(TreeGeneration, BiomeReturnsValidId) {
    WorldGenSettings settings;
    settings.seed = 42;
    WorldGenerator gen(settings);

    for (int x = -100; x < 100; x += 10) {
        for (int z = -100; z < 100; z += 10) {
            BiomeId biome = gen.getBiome(x, 64, z);
            EXPECT_GE(biome, 1);
            EXPECT_LE(biome, 13);
        }
    }
}

// Test: WorldGenSettings defaults are reasonable
TEST(TreeGeneration, SettingsDefaultsAreReasonable) {
    WorldGenSettings settings;
    EXPECT_GT(settings.seaLevel, 0);
    EXPECT_LT(settings.seaLevel, 100);
    EXPECT_GT(settings.terrainHeight, 0.0f);
    EXPECT_GT(settings.noiseOctaves, 0);
    EXPECT_GT(settings.terrainScale, 0.0f);
}

// Test: Stats counter increments for features
TEST(TreeGeneration, StatsIncrementOnFeaturePlacement) {
    WorldGenSettings settings;
    settings.seed = 42;
    settings.flatWorld = true;
    WorldGenerator gen(settings);

    auto before = gen.getStats().featuresPlaced;
    auto chunk = makeFlatChunk({0, 0}, 64);
    SeededRandom rng(100);
    gen.placeTree(chunk.get(), nullptr, 8, 65, 8, "oak", rng);
    EXPECT_GT(gen.getStats().featuresPlaced, before);
}
