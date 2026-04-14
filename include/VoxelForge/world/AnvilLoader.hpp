/**
 * @file AnvilLoader.hpp
 * @brief Minecraft Anvil world format loader/saver
 */

#pragma once

#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/world/ChunkPos.hpp>
#include <VoxelForge/utils/NBT.hpp>
#include <string>
#include <memory>
#include <vector>
#include <cstdint>

namespace VoxelForge {

class ChunkSection;

/**
 * @brief Compression utilities for Anvil format (GZIP/Zlib)
 */
class AnvilCompression {
public:
    static std::vector<uint8_t> decompressGzip(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> decompressZlib(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> compressGzip(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> compressZlib(const std::vector<uint8_t>& data);
};

/**
 * @brief Simplified NBT compound for legacy Anvil parsing
 */
class AnvilNBTCompound {
public:
    AnvilNBTCompound() = default;
    static AnvilNBTCompound parse(const uint8_t* data, size_t size);

    bool has(const std::string& name) const;
    const AnvilNBTCompound& getCompound(const std::string& name) const;
    std::vector<AnvilNBTCompound> getList(const std::string& name) const;
    int8_t getByte(const std::string& name) const;
    int32_t getInt(const std::string& name) const;
    int64_t getLong(const std::string& name) const;
    const std::vector<int8_t>& getByteArray(const std::string& name) const;
    const std::vector<int64_t>& getLongArray(const std::string& name) const;
    const std::string& getString(const std::string& name) const;

private:
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
 * @brief Loader/saver for Minecraft Anvil world format
 *
 * Supports loading and saving chunks from/to .mca region files.
 * Uses zlib compression (type 2) for saved chunks.
 * Implements the 1.18+ block_states format with paletted storage.
 */
class AnvilLoader {
public:
    explicit AnvilLoader(const std::string& worldPath);
    ~AnvilLoader() = default;

    // Load chunk from disk
    std::unique_ptr<Chunk> loadChunk(const ChunkPos& pos);

    // Save chunk to disk
    bool saveChunk(const Chunk* chunk);

    // Check if chunk exists on disk
    bool chunkExists(const ChunkPos& pos) const;

private:
    std::string worldPath_;

    // Legacy parsing
    std::unique_ptr<Chunk> parseChunkNBT(const AnvilNBTCompound& nbt, const ChunkPos& pos);
    void parseSectionNBT(Chunk* chunk, const AnvilNBTCompound& sectionNbt);

    // New serialization helpers
    void serializeChunkToNBT(const Chunk* chunk, NBTCompound& level);
    void serializeBlockStates(const ChunkSection* section, NBTCompound& sectionNbt);
};

} // namespace VoxelForge
