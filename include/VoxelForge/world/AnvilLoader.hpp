/**
 * @file AnvilLoader.hpp
 * @brief Minecraft Anvil world format loader
 */

#pragma once

#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/world/ChunkPos.hpp>
#include <string>
#include <memory>
#include <vector>
#include <cstdint>

namespace VoxelForge {

/**
 * @brief NBT compound tag (simplified)
 */
class NBTCompound {
public:
    NBTCompound() = default;
    static NBTCompound parse(const uint8_t* data, size_t size);

    bool has(const std::string& name) const;
    const NBTCompound& getCompound(const std::string& name) const;
    std::vector<NBTCompound> getList(const std::string& name) const;
    int8_t getByte(const std::string& name) const;
    int32_t getInt(const std::string& name) const;
    int64_t getLong(const std::string& name) const;
    const std::vector<int8_t>& getByteArray(const std::string& name) const;
    const std::vector<int64_t>& getLongArray(const std::string& name) const;
    const std::string& getString(const std::string& name) const;

private:
    // Simplified NBT storage
    struct Tag {
        enum class Type { End, Byte, Short, Int, Long, Float, Double,
                         ByteArray, String, List, Compound, LongArray };
        Type type;
        std::string name;
        std::vector<uint8_t> data;
    };
    std::vector<Tag> tags;
};

/**
 * @brief Compression utilities
 */
class Compression {
public:
    static std::vector<uint8_t> decompressGzip(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> decompressZlib(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> compressGzip(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> compressZlib(const std::vector<uint8_t>& data);
};

/**
 * @brief Loader for Minecraft Anvil world format
 *
 * Supports loading chunks from .mca region files with
 * GZIP/Zlib compression. Handles NBT parsing for block
 * data, biomes, and block entities.
 */
class AnvilLoader {
public:
    explicit AnvilLoader(const std::string& worldPath);
    ~AnvilLoader() = default;

    // Load chunk from disk
    std::unique_ptr<Chunk> loadChunk(const ChunkPos& pos);

    // Save chunk to disk
    bool saveChunk(const Chunk* chunk);

    // Check if chunk exists
    bool chunkExists(const ChunkPos& pos) const;

private:
    std::string worldPath_;

    // Parse NBT into chunk data
    std::unique_ptr<Chunk> parseChunkNBT(const NBTCompound& nbt, const ChunkPos& pos);

    // Parse section NBT
    void parseSectionNBT(Chunk* chunk, const NBTCompound& sectionNbt);
};

} // namespace VoxelForge
