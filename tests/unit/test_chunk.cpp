/**
 * @file test_chunk.cpp
 * @brief Chunk system tests
 */

#include <gtest/gtest.h>
#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/world/Block.hpp>

using namespace VoxelForge;

class ChunkTest : public ::testing::Test {
protected:
    void SetUp() override {
        BlockRegistry::get().registerVanillaBlocks();
    }
};

TEST_F(ChunkTest, ChunkCreation) {
    ChunkPos pos(10, 20);
    Chunk chunk(pos);
    
    EXPECT_EQ(chunk.getPosition().x, 10);
    EXPECT_EQ(chunk.getPosition().z, 20);
    EXPECT_EQ(chunk.getStatus(), Chunk::Status::Empty);
}

TEST_F(ChunkTest, BlockSetAndGet) {
    Chunk chunk(ChunkPos(0, 0));
    
    auto stone = BlockRegistry::get().getDefaultState("minecraft:stone");
    chunk.setBlock(5, 70, 7, stone);
    
    BlockState retrieved = chunk.getBlock(5, 70, 7);
    EXPECT_TRUE(retrieved.is(stone.getBlockId()));
}

TEST_F(ChunkTest, BlockOutOfRange) {
    Chunk chunk(ChunkPos(0, 0));
    
    // Out of Y range
    BlockState block = chunk.getBlock(0, 500, 0);
    EXPECT_TRUE(block.isAir());
    
    block = chunk.getBlock(0, -100, 0);
    EXPECT_TRUE(block.isAir());
}

TEST_F(ChunkTest, SectionAccess) {
    Chunk chunk(ChunkPos(0, 0));
    
    // Get section at Y=0 (which is section index 4 for Y range starting at -64)
    const ChunkSection* section = chunk.getSection(4);
    EXPECT_NE(section, nullptr);
}

TEST_F(ChunkTest, ChunkPositionFromBlock) {
    BlockPos blockPos(100, 64, 200);
    ChunkPos chunkPos = ChunkPos::fromBlockPos(blockPos);
    
    EXPECT_EQ(chunkPos.x, 6);  // 100 / 16 = 6
    EXPECT_EQ(chunkPos.z, 12); // 200 / 16 = 12
}

TEST_F(ChunkTest, ChunkPositionNegative) {
    BlockPos blockPos(-100, 64, -200);
    ChunkPos chunkPos = ChunkPos::fromBlockPos(blockPos);
    
    EXPECT_EQ(chunkPos.x, -7);
    EXPECT_EQ(chunkPos.z, -13);
}

TEST_F(ChunkTest, LocalCoordinates) {
    ChunkPos chunkPos(5, 10);
    
    EXPECT_EQ(chunkPos.getLocalX(80), 0);   // 80 - 5*16 = 0
    EXPECT_EQ(chunkPos.getLocalZ(160), 0);  // 160 - 10*16 = 0
    EXPECT_EQ(chunkPos.getLocalX(85), 5);
    EXPECT_EQ(chunkPos.getLocalZ(165), 5);
}

TEST_F(ChunkTest, ChunkDirty) {
    Chunk chunk(ChunkPos(0, 0));
    
    EXPECT_FALSE(chunk.isDirty());
    
    auto stone = BlockRegistry::get().getDefaultState("minecraft:stone");
    chunk.setBlock(0, 0, 0, stone);
    
    EXPECT_TRUE(chunk.isDirty());
}

TEST_F(ChunkTest, ChunkMarkClean) {
    Chunk chunk(ChunkPos(0, 0));
    
    auto stone = BlockRegistry::get().getDefaultState("minecraft:stone");
    chunk.setBlock(0, 0, 0, stone);
    
    EXPECT_TRUE(chunk.isDirty());
    
    chunk.markClean();
    
    EXPECT_FALSE(chunk.isDirty());
}

TEST_F(ChunkTest, HeightMapDefault) {
    Chunk chunk(ChunkPos(0, 0));
    
    // Before generating, height should be min
    int height = chunk.getHeight(HeightMap::Type::WorldSurface, 0, 0);
    EXPECT_EQ(height, CHUNK_MIN_Y);
}

TEST_F(ChunkTest, BiomeSetAndGet) {
    Chunk chunk(ChunkPos(0, 0));
    
    chunk.setBiome(5, 64, 7, 2);
    
    auto biome = chunk.getBiome(5, 64, 7);
    EXPECT_EQ(biome, 2);
}

TEST_F(ChunkTest, LightSetAndGet) {
    Chunk chunk(ChunkPos(0, 0));
    
    chunk.setSkyLight(5, 64, 7, 10);
    chunk.setBlockLight(5, 64, 7, 5);
    
    EXPECT_EQ(chunk.getSkyLight(5, 64, 7), 10);
    EXPECT_EQ(chunk.getBlockLight(5, 64, 7), 5);
}

TEST_F(ChunkTest, ChunkPosHash) {
    ChunkPos pos1(10, 20);
    ChunkPos pos2(10, 20);
    ChunkPos pos3(10, 21);
    
    EXPECT_EQ(pos1.toHash(), pos2.toHash());
    EXPECT_NE(pos1.toHash(), pos3.toHash());
}

TEST_F(ChunkTest, MultipleBlocks) {
    Chunk chunk(ChunkPos(0, 0));
    
    auto stone = BlockRegistry::get().getDefaultState("minecraft:stone");
    auto dirt = BlockRegistry::get().getDefaultState("minecraft:dirt");
    
    for (int y = 60; y < 70; y++) {
        for (int x = 0; x < 16; x++) {
            for (int z = 0; z < 16; z++) {
                if (y < 65) {
                    chunk.setBlock(x, y, z, stone);
                } else {
                    chunk.setBlock(x, y, z, dirt);
                }
            }
        }
    }
    
    // Verify some blocks
    EXPECT_TRUE(chunk.getBlock(5, 62, 5).is(stone.getBlockId()));
    EXPECT_TRUE(chunk.getBlock(5, 67, 5).is(dirt.getBlockId()));
}

TEST_F(ChunkTest, SectionNonAirCount) {
    Chunk chunk(ChunkPos(0, 0));
    
    // Section at Y=64 (index 8)
    ChunkSection* section = chunk.getSection(8);
    ASSERT_NE(section, nullptr);
    
    EXPECT_EQ(section->getNonAirBlockCount(), 0);
    
    auto stone = BlockRegistry::get().getDefaultState("minecraft:stone");
    chunk.setBlock(0, 64, 0, stone);
    
    // Refresh section pointer
    section = chunk.getSection(8);
    EXPECT_GT(section->getNonAirBlockCount(), 0);
}
