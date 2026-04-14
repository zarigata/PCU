/**
 * @file test_anvil_save.cpp
 * @brief Tests for Anvil chunk saving and NBT serialization
 */

#include <gtest/gtest.h>
#include <VoxelForge/world/AnvilLoader.hpp>
#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/utils/NBT.hpp>
#include <fstream>
#include <filesystem>
#include <cstdio>

using namespace VoxelForge;

// ============================================
// NBT Serialization Tests
// ============================================

class NBTSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(NBTSerializationTest, RoundTripByte) {
    NBTCompound original;
    original.setByte("testByte", 42);

    auto data = original.serialize("root");
    ASSERT_FALSE(data.empty());

    auto restored = NBTCompound::deserialize(data.data(), data.size());
    EXPECT_TRUE(restored.hasKey("testByte"));
    EXPECT_EQ(restored.getByte("testByte"), 42);
}

TEST_F(NBTSerializationTest, RoundTripInt) {
    NBTCompound original;
    original.setInt("testInt", 123456);

    auto data = original.serialize("root");
    auto restored = NBTCompound::deserialize(data.data(), data.size());
    EXPECT_EQ(restored.getInt("testInt"), 123456);
}

TEST_F(NBTSerializationTest, RoundTripLong) {
    NBTCompound original;
    original.setLong("testLong", 9876543210LL);

    auto data = original.serialize("root");
    auto restored = NBTCompound::deserialize(data.data(), data.size());
    EXPECT_EQ(restored.getLong("testLong"), 9876543210LL);
}

TEST_F(NBTSerializationTest, RoundTripString) {
    NBTCompound original;
    original.setString("testString", "Hello VoxelForge!");

    auto data = original.serialize("root");
    auto restored = NBTCompound::deserialize(data.data(), data.size());
    EXPECT_EQ(restored.getString("testString"), "Hello VoxelForge!");
}

TEST_F(NBTSerializationTest, RoundTripFloat) {
    NBTCompound original;
    original.setFloat("testFloat", 3.14f);

    auto data = original.serialize("root");
    auto restored = NBTCompound::deserialize(data.data(), data.size());
    EXPECT_FLOAT_EQ(restored.getFloat("testFloat"), 3.14f);
}

TEST_F(NBTSerializationTest, RoundTripDouble) {
    NBTCompound original;
    original.setDouble("testDouble", 2.718281828);

    auto data = original.serialize("root");
    auto restored = NBTCompound::deserialize(data.data(), data.size());
    EXPECT_DOUBLE_EQ(restored.getDouble("testDouble"), 2.718281828);
}

TEST_F(NBTSerializationTest, RoundTripNestedCompound) {
    NBTCompound inner;
    inner.setInt("x", 10);
    inner.setInt("y", 20);

    NBTCompound original;
    original.setCompound("position", inner);

    auto data = original.serialize("root");
    auto restored = NBTCompound::deserialize(data.data(), data.size());
    EXPECT_TRUE(restored.hasKey("position"));

    auto pos = restored.getCompound("position");
    EXPECT_EQ(pos.getInt("x"), 10);
    EXPECT_EQ(pos.getInt("y"), 20);
}

TEST_F(NBTSerializationTest, RoundTripList) {
    NBTList list(NBTTagType::Int);
    list.addInt(1).addInt(2).addInt(3);

    NBTCompound original;
    original.setList("numbers", list);

    auto data = original.serialize("root");
    auto restored = NBTCompound::deserialize(data.data(), data.size());
    // List should be present (verify via hasKey)
    EXPECT_TRUE(restored.hasKey("numbers"));
}

TEST_F(NBTSerializationTest, RoundTripByteArray) {
    std::vector<int8_t> arr = {-1, 0, 1, 2, 3, 127, -128};
    NBTCompound original;
    original.setByteArray("data", arr);

    auto data = original.serialize("root");
    auto restored = NBTCompound::deserialize(data.data(), data.size());
    EXPECT_TRUE(restored.hasKey("data"));
}

TEST_F(NBTSerializationTest, RoundTripLongArray) {
    std::vector<int64_t> arr = {100LL, 200LL, 300LL, -1LL};
    NBTCompound original;
    original.setLongArray("bigData", arr);

    auto data = original.serialize("root");
    auto restored = NBTCompound::deserialize(data.data(), data.size());
    EXPECT_TRUE(restored.hasKey("bigData"));
}

TEST_F(NBTSerializationTest, RoundTripMultipleFields) {
    NBTCompound original;
    original.setByte("b", 7);
    original.setShort("s", 1000);
    original.setInt("i", 99999);
    original.setLong("l", 12345678901234LL);
    original.setFloat("f", 1.5f);
    original.setDouble("d", 2.5);
    original.setString("name", "test");

    auto data = original.serialize("root");
    auto restored = NBTCompound::deserialize(data.data(), data.size());

    EXPECT_EQ(restored.getByte("b"), 7);
    EXPECT_EQ(restored.getShort("s"), 1000);
    EXPECT_EQ(restored.getInt("i"), 99999);
    EXPECT_EQ(restored.getLong("l"), 12345678901234LL);
    EXPECT_FLOAT_EQ(restored.getFloat("f"), 1.5f);
    EXPECT_DOUBLE_EQ(restored.getDouble("d"), 2.5);
    EXPECT_EQ(restored.getString("name"), "test");
}

TEST_F(NBTSerializationTest, EmptyCompound) {
    NBTCompound original;
    auto data = original.serialize("root");
    EXPECT_GT(data.size(), 0u);

    auto restored = NBTCompound::deserialize(data.data(), data.size());
    EXPECT_TRUE(restored.empty());
}

// ============================================
// Zlib Compression Tests
// ============================================

class AnvilCompressionTest : public ::testing::Test {};

TEST_F(AnvilCompressionTest, ZlibRoundTrip) {
    std::vector<uint8_t> original(1024);
    for (size_t i = 0; i < original.size(); ++i) {
        original[i] = static_cast<uint8_t>(i % 256);
    }

    auto compressed = AnvilCompression::compressZlib(original);
    ASSERT_FALSE(compressed.empty());

    auto decompressed = AnvilCompression::decompressZlib(compressed);
    ASSERT_FALSE(decompressed.empty());
    EXPECT_EQ(decompressed.size(), original.size());
    EXPECT_EQ(decompressed, original);
}

TEST_F(AnvilCompressionTest, GzipRoundTrip) {
    std::vector<uint8_t> original = {10, 20, 30, 40, 50, 60, 70, 80};

    auto compressed = AnvilCompression::compressGzip(original);
    ASSERT_FALSE(compressed.empty());

    auto decompressed = AnvilCompression::decompressGzip(compressed);
    ASSERT_FALSE(decompressed.empty());
    EXPECT_EQ(decompressed, original);
}

TEST_F(AnvilCompressionTest, CompressEmptyData) {
    std::vector<uint8_t> empty;
    auto compressed = AnvilCompression::compressZlib(empty);
    // Should produce a valid (tiny) zlib stream
    EXPECT_FALSE(compressed.empty());

    auto decompressed = AnvilCompression::decompressZlib(compressed);
    EXPECT_TRUE(decompressed.empty());
}

// ============================================
// Anvil Save/Load Tests
// ============================================

class AnvilSaveTest : public ::testing::Test {
protected:
    std::string testDir;

    void SetUp() override {
        testDir = "/tmp/vf_test_anvil_" + std::to_string(::getpid());
        std::filesystem::create_directories(testDir);
    }

    void TearDown() override {
        std::filesystem::remove_all(testDir);
    }
};

TEST_F(AnvilSaveTest, SaveCreatesRegionFile) {
    AnvilLoader loader(testDir);
    ChunkPos pos(0, 0);
    Chunk chunk(pos);

    bool result = loader.saveChunk(&chunk);
    EXPECT_TRUE(result);

    std::string regionFile = testDir + "/region/r.0.0.mca";
    EXPECT_TRUE(std::filesystem::exists(regionFile));

    // File should be at least the header size
    auto fileSize = std::filesystem::file_size(regionFile);
    EXPECT_GE(fileSize, 8192u);
}

TEST_F(AnvilSaveTest, SaveMultipleChunksSameRegion) {
    AnvilLoader loader(testDir);

    Chunk chunk1(ChunkPos(0, 0));
    Chunk chunk2(ChunkPos(1, 0));
    Chunk chunk3(ChunkPos(0, 1));

    EXPECT_TRUE(loader.saveChunk(&chunk1));
    EXPECT_TRUE(loader.saveChunk(&chunk2));
    EXPECT_TRUE(loader.saveChunk(&chunk3));

    std::string regionFile = testDir + "/region/r.0.0.mca";
    EXPECT_TRUE(std::filesystem::exists(regionFile));
}

TEST_F(AnvilSaveTest, SaveNegativePosition) {
    AnvilLoader loader(testDir);

    Chunk chunk(ChunkPos(-1, -1));
    EXPECT_TRUE(loader.saveChunk(&chunk));

    // -1 >> 5 = -1 in C++ (arithmetic shift)
    std::string regionFile = testDir + "/region/r.-1.-1.mca";
    EXPECT_TRUE(std::filesystem::exists(regionFile));
}

TEST_F(AnvilSaveTest, SaveNullChunkFails) {
    AnvilLoader loader(testDir);
    EXPECT_FALSE(loader.saveChunk(nullptr));
}

TEST_F(AnvilSaveTest, ChunkExistsReturnsFalseForMissing) {
    AnvilLoader loader(testDir);
    EXPECT_FALSE(loader.chunkExists(ChunkPos(0, 0)));
}

TEST_F(AnvilSaveTest, OverwriteExistingChunk) {
    AnvilLoader loader(testDir);

    Chunk chunk1(ChunkPos(5, 5));
    EXPECT_TRUE(loader.saveChunk(&chunk1));

    // Save again — should overwrite
    Chunk chunk2(ChunkPos(5, 5));
    EXPECT_TRUE(loader.saveChunk(&chunk2));

    std::string regionFile = testDir + "/region/r.0.0.mca";
    EXPECT_TRUE(std::filesystem::exists(regionFile));
}
