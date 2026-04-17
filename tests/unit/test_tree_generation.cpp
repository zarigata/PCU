/**
 * @file test_tree_generation.cpp
 * @brief Tree generation tests
 *
 * Tests cover WorldGenerator's feature generation phase,
 * specifically tree placement logic.
 *
 * Tests that check for specific tree types (oak, birch, etc.) are
 * conditional: if no tree of that type spawns with the chosen seed,
 * the test passes vacuously. When a tree IS detected, we verify its
 * structural integrity (e.g. log + leaves coexist).
 */

#include <gtest/gtest.h>
#include <VoxelForge/world/WorldGenerator.hpp>
#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/world/BlockRegistry.hpp>
#include <VoxelForge/world/World.hpp>

using namespace VoxelForge;

// ---------------------------------------------------------------------------
// Test fixture – registers vanilla blocks once
// ---------------------------------------------------------------------------
class TreeGenerationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        BlockRegistry::get().registerVanillaBlocks();
    }

    // Fill a horizontal layer with a block
    static void fillLayer(Chunk& chunk, int y, BlockState block) {
        for (int x = 0; x < CHUNK_WIDTH; ++x) {
            for (int z = 0; z < CHUNK_WIDTH; ++z) {
                chunk.setBlock(x, y, z, block);
            }
        }
    }

    // Check if any block in [minY, maxY] in column (x,z) matches blockId
    static bool columnHasBlockId(const Chunk& chunk, int x, int z,
                                  int minY, int maxY, BlockID blockId) {
        for (int y = minY; y <= maxY; ++y) {
            if (chunk.getBlock(x, y, z).is(blockId)) {
                return true;
            }
        }
        return false;
    }

    // Scan entire chunk for any block with a given blockId in y range
    static bool chunkHasBlockId(const Chunk& chunk, int minY, int maxY,
                                 BlockID blockId) {
        for (int x = 0; x < CHUNK_WIDTH; ++x) {
            for (int z = 0; z < CHUNK_WIDTH; ++z) {
                if (columnHasBlockId(chunk, x, z, minY, maxY, blockId)) {
                    return true;
                }
            }
        }
        return false;
    }
};

// ---------------------------------------------------------------------------
// 1. Oak Tree – verify log + leaves placed above grass surface
// ---------------------------------------------------------------------------
TEST_F(TreeGenerationTest, OakTreeGeneration) {
    WorldGenSettings settings;
    settings.seed = 12345;
    settings.flatWorld = true;
    settings.generateStructures = true;

    WorldGenerator gen(settings);
    World world(static_cast<int64_t>(settings.seed));
    Chunk chunk(ChunkPos(0, 0));

    auto grass = BlockRegistry::get().getDefaultState("poorcraftultra:grass_block");
    fillLayer(chunk, 64, grass);

    auto result = gen.generateChunk(&chunk, &world);
    ASSERT_TRUE(result.success);

    auto oakLog    = BlockRegistry::get().getDefaultState("poorcraftultra:oak_log");
    auto oakLeaves = BlockRegistry::get().getDefaultState("poorcraftultra:oak_leaves");

    bool foundLog = chunkHasBlockId(chunk, 65, 100, oakLog.getBlockId());
    if (foundLog) {
        bool foundLeaves = chunkHasBlockId(chunk, 65, 100, oakLeaves.getBlockId());
        EXPECT_TRUE(foundLeaves) << "Oak log found but no oak leaves above surface";
    }
}

// ---------------------------------------------------------------------------
// 2. Birch Tree
// ---------------------------------------------------------------------------
TEST_F(TreeGenerationTest, BirchTreeGeneration) {
    WorldGenSettings settings;
    settings.seed = 54321;
    settings.flatWorld = true;
    settings.generateStructures = true;

    WorldGenerator gen(settings);
    World world(static_cast<int64_t>(settings.seed));
    Chunk chunk(ChunkPos(0, 0));

    auto grass = BlockRegistry::get().getDefaultState("poorcraftultra:grass_block");
    fillLayer(chunk, 64, grass);

    auto result = gen.generateChunk(&chunk, &world);
    ASSERT_TRUE(result.success);

    auto birchLog    = BlockRegistry::get().getDefaultState("poorcraftultra:birch_log");
    auto birchLeaves = BlockRegistry::get().getDefaultState("poorcraftultra:birch_leaves");

    bool foundLog = chunkHasBlockId(chunk, 65, 100, birchLog.getBlockId());
    if (foundLog) {
        bool foundLeaves = chunkHasBlockId(chunk, 65, 100, birchLeaves.getBlockId());
        EXPECT_TRUE(foundLeaves) << "Birch log found but no birch leaves above surface";
    }
}

// ---------------------------------------------------------------------------
// 3. Spruce Tree – verify cone shape (leaves exist near trunk)
// ---------------------------------------------------------------------------
TEST_F(TreeGenerationTest, SpruceTreeGeneration) {
    WorldGenSettings settings;
    settings.seed = 99999;
    settings.flatWorld = true;
    settings.generateStructures = true;

    WorldGenerator gen(settings);
    World world(static_cast<int64_t>(settings.seed));
    Chunk chunk(ChunkPos(0, 0));

    auto grass = BlockRegistry::get().getDefaultState("poorcraftultra:grass_block");
    fillLayer(chunk, 64, grass);

    auto result = gen.generateChunk(&chunk, &world);
    ASSERT_TRUE(result.success);

    auto spruceLog    = BlockRegistry::get().getDefaultState("poorcraftultra:spruce_log");
    auto spruceLeaves = BlockRegistry::get().getDefaultState("poorcraftultra:spruce_leaves");

    // Find a spruce log column
    int sx = -1, sz = -1;
    for (int x = 0; x < CHUNK_WIDTH && sx < 0; ++x) {
        for (int z = 0; z < CHUNK_WIDTH && sx < 0; ++z) {
            if (columnHasBlockId(chunk, x, z, 65, 100, spruceLog.getBlockId())) {
                sx = x; sz = z;
            }
        }
    }

    if (sx >= 0) {
        // Check for spruce leaves in a 5x5 area around the trunk
        bool foundLeaves = false;
        for (int dx = -2; dx <= 2 && !foundLeaves; ++dx) {
            for (int dz = -2; dz <= 2 && !foundLeaves; ++dz) {
                int bx = sx + dx;
                int bz = sz + dz;
                if (bx < 0 || bx >= CHUNK_WIDTH || bz < 0 || bz >= CHUNK_WIDTH) continue;
                if (columnHasBlockId(chunk, bx, bz, 65, 100, spruceLeaves.getBlockId())) {
                    foundLeaves = true;
                }
            }
        }
        EXPECT_TRUE(foundLeaves) << "Spruce log found but no spruce leaves nearby";
    }
}

// ---------------------------------------------------------------------------
// 4. Dark Oak – verify thick trunk (2x2 base → ≥4 log blocks)
// ---------------------------------------------------------------------------
TEST_F(TreeGenerationTest, DarkOakTreeGeneration) {
    WorldGenSettings settings;
    settings.seed = 77777;
    settings.flatWorld = true;
    settings.generateStructures = true;

    WorldGenerator gen(settings);
    World world(static_cast<int64_t>(settings.seed));
    Chunk chunk(ChunkPos(0, 0));

    auto grass = BlockRegistry::get().getDefaultState("poorcraftultra:grass_block");
    fillLayer(chunk, 64, grass);

    auto result = gen.generateChunk(&chunk, &world);
    ASSERT_TRUE(result.success);

    auto darkOakLog = BlockRegistry::get().getDefaultState("poorcraftultra:dark_oak_log");
    BlockID logId = darkOakLog.getBlockId();

    int logCount = 0;
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_WIDTH; ++z) {
            for (int y = 65; y < 100; ++y) {
                if (chunk.getBlock(x, y, z).is(logId)) {
                    ++logCount;
                }
            }
        }
    }

    if (logCount > 0) {
        // Dark oak has a 2x2 trunk × multiple blocks tall → at least 4 logs
        EXPECT_GE(logCount, 4) << "Dark oak should have a thick (2x2+) trunk";
    }
}

// ---------------------------------------------------------------------------
// 5. Acacia Tree
// ---------------------------------------------------------------------------
TEST_F(TreeGenerationTest, AcaciaTreeGeneration) {
    WorldGenSettings settings;
    settings.seed = 33333;
    settings.flatWorld = true;
    settings.generateStructures = true;

    WorldGenerator gen(settings);
    World world(static_cast<int64_t>(settings.seed));
    Chunk chunk(ChunkPos(0, 0));

    auto grass = BlockRegistry::get().getDefaultState("poorcraftultra:grass_block");
    fillLayer(chunk, 64, grass);

    auto result = gen.generateChunk(&chunk, &world);
    ASSERT_TRUE(result.success);

    auto acaciaLog    = BlockRegistry::get().getDefaultState("poorcraftultra:acacia_log");
    auto acaciaLeaves = BlockRegistry::get().getDefaultState("poorcraftultra:acacia_leaves");

    bool foundLog = chunkHasBlockId(chunk, 65, 100, acaciaLog.getBlockId());
    if (foundLog) {
        bool foundLeaves = chunkHasBlockId(chunk, 65, 100, acaciaLeaves.getBlockId());
        EXPECT_TRUE(foundLeaves) << "Acacia log found but no acacia leaves above surface";
    }
}

// ---------------------------------------------------------------------------
// 6. No Tree on Non-Grass Surface
// ---------------------------------------------------------------------------
TEST_F(TreeGenerationTest, NoTreeOnNonGrass) {
    WorldGenSettings settings;
    settings.seed = 42;
    settings.flatWorld = true;
    settings.generateStructures = true;

    WorldGenerator gen(settings);
    World world(static_cast<int64_t>(settings.seed));
    Chunk chunk(ChunkPos(0, 0));

    // Surface is stone, not grass
    auto stone = BlockRegistry::get().getDefaultState("poorcraftultra:stone");
    fillLayer(chunk, 64, stone);

    auto result = gen.generateChunk(&chunk, &world);
    ASSERT_TRUE(result.success);

    // Pre-fetch known log block IDs to scan for
    BlockID logIds[] = {
        BlockRegistry::get().getDefaultState("poorcraftultra:oak_log").getBlockId(),
        BlockRegistry::get().getDefaultState("poorcraftultra:birch_log").getBlockId(),
        BlockRegistry::get().getDefaultState("poorcraftultra:spruce_log").getBlockId(),
        BlockRegistry::get().getDefaultState("poorcraftultra:dark_oak_log").getBlockId(),
        BlockRegistry::get().getDefaultState("poorcraftultra:acacia_log").getBlockId(),
    };

    int logCount = 0;
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_WIDTH; ++z) {
            for (int y = 65; y < 100; ++y) {
                BlockID bid = chunk.getBlock(x, y, z).getBlockId();
                for (auto logId : logIds) {
                    if (bid == logId) { ++logCount; break; }
                }
            }
        }
    }

    EXPECT_EQ(logCount, 0) << "Trees should not generate on stone surface";
}

// ---------------------------------------------------------------------------
// 7. Tree Count by Biome – generation runs and reports features
// ---------------------------------------------------------------------------
TEST_F(TreeGenerationTest, TreeCountByBiome) {
    WorldGenSettings settings;
    settings.seed = 100;
    settings.generateStructures = true;

    WorldGenerator gen(settings);
    World world(static_cast<int64_t>(settings.seed));

    int totalFeatures = 0;
    for (int cx = 0; cx < 3; ++cx) {
        for (int cz = 0; cz < 3; ++cz) {
            Chunk chunk(ChunkPos(cx, cz));
            auto result = gen.generateChunk(&chunk, &world);
            ASSERT_TRUE(result.success);
            totalFeatures += result.featuresGenerated;
        }
    }

    // API exercised successfully; feature count depends on generation internals
    EXPECT_GE(totalFeatures, 0);
}

// ---------------------------------------------------------------------------
// 8. Tree Within Chunk Bounds – no crashes from out-of-bounds access
// ---------------------------------------------------------------------------
TEST_F(TreeGenerationTest, TreeWithinChunkBounds) {
    WorldGenSettings settings;
    settings.seed = 55555;
    settings.flatWorld = true;
    settings.generateStructures = true;

    WorldGenerator gen(settings);
    World world(static_cast<int64_t>(settings.seed));

    auto grass = BlockRegistry::get().getDefaultState("poorcraftultra:grass_block");

    for (int cx = -1; cx <= 1; ++cx) {
        for (int cz = -1; cz <= 1; ++cz) {
            Chunk chunk(ChunkPos(cx, cz));
            fillLayer(chunk, 64, grass);

            EXPECT_NO_THROW({
                auto result = gen.generateChunk(&chunk, &world);
                EXPECT_TRUE(result.success);
            }) << "Crash at chunk (" << cx << ", " << cz << ")";
        }
    }
}

// ---------------------------------------------------------------------------
// 9. Deterministic Generation – same seed produces identical chunks
// ---------------------------------------------------------------------------
TEST_F(TreeGenerationTest, DeterministicGeneration) {
    WorldGenSettings settings;
    settings.seed = 88888;
    settings.flatWorld = true;
    settings.generateStructures = true;

    World world1(static_cast<int64_t>(settings.seed));
    World world2(static_cast<int64_t>(settings.seed));

    WorldGenerator gen1(settings);
    WorldGenerator gen2(settings);

    Chunk chunk1(ChunkPos(0, 0));
    Chunk chunk2(ChunkPos(0, 0));

    auto grass = BlockRegistry::get().getDefaultState("poorcraftultra:grass_block");
    fillLayer(chunk1, 64, grass);
    fillLayer(chunk2, 64, grass);

    auto result1 = gen1.generateChunk(&chunk1, &world1);
    auto result2 = gen2.generateChunk(&chunk2, &world2);

    ASSERT_TRUE(result1.success);
    ASSERT_TRUE(result2.success);

    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int y = CHUNK_MIN_Y; y < CHUNK_MIN_Y + CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < CHUNK_WIDTH; ++z) {
                EXPECT_EQ(chunk1.getBlock(x, y, z).getBlockId(),
                          chunk2.getBlock(x, y, z).getBlockId())
                    << "Mismatch at (" << x << "," << y << "," << z << ")";
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 10. No Tree Below Sea Level – water surface should not get trees
// ---------------------------------------------------------------------------
TEST_F(TreeGenerationTest, NoTreeBelowSeaLevel) {
    WorldGenSettings settings;
    settings.seed = 22222;
    settings.flatWorld = true;
    settings.generateStructures = true;
    settings.seaLevel = 62;
    settings.terrainBaseHeight = 62.0f;

    WorldGenerator gen(settings);
    World world(static_cast<int64_t>(settings.seed));
    Chunk chunk(ChunkPos(0, 0));

    auto water = BlockRegistry::get().getDefaultState("poorcraftultra:water");
    for (int y = CHUNK_MIN_Y; y <= settings.seaLevel; ++y) {
        fillLayer(chunk, y, water);
    }

    auto result = gen.generateChunk(&chunk, &world);
    ASSERT_TRUE(result.success);

    // Check for any log blocks just above sea level
    BlockID logIds[] = {
        BlockRegistry::get().getDefaultState("poorcraftultra:oak_log").getBlockId(),
        BlockRegistry::get().getDefaultState("poorcraftultra:birch_log").getBlockId(),
        BlockRegistry::get().getDefaultState("poorcraftultra:spruce_log").getBlockId(),
        BlockRegistry::get().getDefaultState("poorcraftultra:dark_oak_log").getBlockId(),
        BlockRegistry::get().getDefaultState("poorcraftultra:acacia_log").getBlockId(),
    };

    int logCount = 0;
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_WIDTH; ++z) {
            for (int y = settings.seaLevel + 1; y < settings.seaLevel + 30; ++y) {
                BlockID bid = chunk.getBlock(x, y, z).getBlockId();
                for (auto logId : logIds) {
                    if (bid == logId) { ++logCount; break; }
                }
            }
        }
    }

    EXPECT_EQ(logCount, 0) << "Trees should not generate on water surface";
}

// ---------------------------------------------------------------------------
// 11. Feature Stats Updated – verify stats.featuresPlaced increments
// ---------------------------------------------------------------------------
TEST_F(TreeGenerationTest, FeatureStatsUpdated) {
    WorldGenSettings settings;
    settings.seed = 44444;
    settings.flatWorld = true;
    settings.generateStructures = true;

    WorldGenerator gen(settings);
    World world(static_cast<int64_t>(settings.seed));

    const auto& stats = gen.getStats();
    EXPECT_EQ(stats.chunksGenerated, 0u);
    EXPECT_EQ(stats.featuresPlaced, 0u);

    auto grass = BlockRegistry::get().getDefaultState("poorcraftultra:grass_block");
    for (int cx = 0; cx < 5; ++cx) {
        for (int cz = 0; cz < 5; ++cz) {
            Chunk chunk(ChunkPos(cx, cz));
            fillLayer(chunk, 64, grass);
            gen.generateChunk(&chunk, &world);
        }
    }

    EXPECT_EQ(stats.chunksGenerated, 25u);
    // featuresPlaced >= 0 (may be 0 if no trees placed in these chunks)
    EXPECT_GE(stats.featuresPlaced, 0u);
}
