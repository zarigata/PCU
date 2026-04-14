/**
 * @file test_world_gen_integration.cpp
 * @brief Tests for WorldGenerator integration into World
 */

#include <gtest/gtest.h>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/world/WorldGenerator.hpp>
#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/world/Block.hpp>

class WorldGenIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure BlockRegistry is populated for tests
        auto& reg = VoxelForge::BlockRegistry::get();
        reg.registerBlock("poorcraftultra:air");
        reg.registerBlock("poorcraftultra:stone");
        reg.registerBlock("poorcraftultra:dirt");
        reg.registerBlock("poorcraftultra:grass_block");
        reg.registerBlock("poorcraftultra:bedrock");
        reg.registerBlock("poorcraftultra:water");
        reg.registerBlock("poorcraftultra:sand");
        reg.registerBlock("poorcraftultra:deepslate");
        reg.registerBlock("poorcraftultra:cave_air");
        reg.registerBlock("poorcraftultra:gravel");
        reg.registerBlock("poorcraftultra:snow_block");
        reg.registerBlock("poorcraftultra:red_sand");
        reg.registerBlock("poorcraftultra:sandstone");
        reg.registerBlock("poorcraftultra:red_sandstone");
        reg.registerBlock("poorcraftultra:clay");
        reg.registerBlock("poorcraftultra:oak_log");
        reg.registerBlock("poorcraftultra:oak_leaves");
        reg.registerBlock("poorcraftultra:birch_log");
        reg.registerBlock("poorcraftultra:birch_leaves");
        reg.registerBlock("poorcraftultra:spruce_log");
        reg.registerBlock("poorcraftultra:spruce_leaves");
        reg.registerBlock("poorcraftultra:dark_oak_log");
        reg.registerBlock("poorcraftultra:dark_oak_leaves");
        reg.registerBlock("poorcraftultra:acacia_log");
        reg.registerBlock("poorcraftultra:acacia_leaves");
        reg.registerBlock("poorcraftultra:coal_ore");
        reg.registerBlock("poorcraftultra:iron_ore");
        reg.registerBlock("poorcraftultra:gold_ore");
        reg.registerBlock("poorcraftultra:diamond_ore");
        reg.registerBlock("poorcraftultra:redstone_ore");
        reg.registerBlock("poorcraftultra:lapis_ore");
        reg.registerBlock("poorcraftultra:copper_ore");
    }
};

TEST_F(WorldGenIntegrationTest, WorldHasWorldGenerator) {
    VoxelForge::World world(12345);
    EXPECT_TRUE(world.hasWorldGenerator());
}

TEST_F(WorldGenIntegrationTest, WorldGeneratorSeedMatches) {
    VoxelForge::World world(42);
    ASSERT_TRUE(world.hasWorldGenerator());
    EXPECT_EQ(world.getWorldGenerator().getSeed(), 42u);
}

TEST_F(WorldGenIntegrationTest, ChunkGenerationUsesWorldGenerator) {
    VoxelForge::World world(9999);
    
    // Load a chunk - should use WorldGenerator
    VoxelForge::ChunkPos pos(0, 0);
    VoxelForge::Chunk* chunk = world.getOrCreateChunk(pos);
    
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->getStatus(), VoxelForge::Chunk::Status::Full);
    
    // Check that terrain was actually generated (not empty)
    // At bedrock level there should be blocks
    bool hasBlocks = false;
    for (int x = 0; x < 16 && !hasBlocks; x++) {
        for (int z = 0; z < 16 && !hasBlocks; z++) {
            if (!chunk->getBlock(x, -64, z).isAir()) {
                hasBlocks = true;
            }
        }
    }
    EXPECT_TRUE(hasBlocks) << "Chunk should have blocks at bedrock level";
}

TEST_F(WorldGenIntegrationTest, MultipleChunksConsistent) {
    VoxelForge::World world(7777);
    
    // Generate adjacent chunks
    VoxelForge::Chunk* c00 = world.getOrCreateChunk({0, 0});
    VoxelForge::Chunk* c10 = world.getOrCreateChunk({1, 0});
    VoxelForge::Chunk* c01 = world.getOrCreateChunk({0, 1});
    
    ASSERT_NE(c00, nullptr);
    ASSERT_NE(c10, nullptr);
    ASSERT_NE(c01, nullptr);
    
    // All should be fully generated
    EXPECT_EQ(c00->getStatus(), VoxelForge::Chunk::Status::Full);
    EXPECT_EQ(c10->getStatus(), VoxelForge::Chunk::Status::Full);
    EXPECT_EQ(c01->getStatus(), VoxelForge::Chunk::Status::Full);
}

TEST_F(WorldGenIntegrationTest, BiomeQueryDelegatesToGenerator) {
    VoxelForge::World world(5555);
    
    // Querying biome should not crash
    int biome = world.getBiome(VoxelForge::BlockPos(0, 64, 0));
    EXPECT_GE(biome, 0);
    
    // Different positions should potentially have different biomes
    int biome2 = world.getBiome(VoxelForge::BlockPos(1000, 64, 1000));
    // Just check it's valid, not necessarily different
    EXPECT_GE(biome2, 0);
}

TEST_F(WorldGenIntegrationTest, HeightQueryDelegatesToGenerator) {
    VoxelForge::World world(3333);
    
    // Height at any position should be within world bounds
    int h = world.getHeight(0, 0);
    EXPECT_GT(h, -64);
    EXPECT_LT(h, 320);
}

TEST_F(WorldGenIntegrationTest, SetCustomWorldGenerator) {
    VoxelForge::World world(1111);
    
    VoxelForge::WorldGenSettings customSettings;
    customSettings.seed = 2222;
    customSettings.flatWorld = true;
    auto customGen = std::make_unique<VoxelForge::WorldGenerator>(customSettings);
    
    uint64_t genSeed = customGen->getSeed();
    world.setWorldGenerator(std::move(customGen));
    
    ASSERT_TRUE(world.hasWorldGenerator());
    EXPECT_EQ(world.getWorldGenerator().getSeed(), genSeed);
}

TEST_F(WorldGenIntegrationTest, FlatWorldGeneration) {
    VoxelForge::World world(1111);
    
    VoxelForge::WorldGenSettings flatSettings;
    flatSettings.seed = 1111;
    flatSettings.flatWorld = true;
    flatSettings.seaLevel = 62;
    flatSettings.minHeight = -64;
    flatSettings.maxHeight = 320;
    flatSettings.generateCaves = false;
    flatSettings.generateOres = false;
    flatSettings.generateStructures = false;
    world.setWorldGenerator(std::make_unique<VoxelForge::WorldGenerator>(flatSettings));
    
    // Generate chunk
    VoxelForge::Chunk* chunk = world.getOrCreateChunk({0, 0});
    ASSERT_NE(chunk, nullptr);
    
    // In flat world, all columns should have same height
    int firstHeight = -999;
    bool allSame = true;
    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            int h = chunk->getHeight(VoxelForge::HeightMap::Type::WorldSurface, x, z);
            if (firstHeight == -999) {
                firstHeight = h;
            } else if (h != firstHeight) {
                allSame = false;
            }
        }
    }
    EXPECT_TRUE(allSame) << "Flat world should have uniform height";
}
