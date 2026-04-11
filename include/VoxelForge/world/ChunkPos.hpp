/**
 * @file ChunkPos.hpp
 * @brief Chunk position type
 */

#pragma once

#include <cstdint>
#include <functional>
#include <glm/glm.hpp>

namespace VoxelForge {

// Chunk constants
constexpr static int CHUNK_WIDTH = 16;
constexpr static int CHUNK_DEPTH = 16;
constexpr static int CHUNK_HEIGHT = 384;
constexpr static int SECTION_HEIGHT = 16;
constexpr static int SECTIONS_PER_CHUNK = CHUNK_HEIGHT / SECTION_HEIGHT;
constexpr static int CHUNK_MIN_Y = -64;
constexpr static int CHUNK_MAX_Y = CHUNK_MIN_Y + CHUNK_HEIGHT - 1;

// Chunk position (2D: x, z)
struct ChunkPos {
    int x, z;
    
    ChunkPos() : x(0), z(0) {}
    ChunkPos(int x, int z) : x(x), z(z) {}
    
    bool operator==(const ChunkPos& other) const { return x == other.x && z == other.z; }
    bool operator!=(const ChunkPos& other) const { return !(*this == other); }
    
    int64_t toHash() const {
        return (static_cast<int64_t>(x) & 0xFFFFFFFFLL) | (static_cast<int64_t>(z) << 32);
    }
    
    static ChunkPos fromHash(int64_t hash) {
        return ChunkPos(static_cast<int>(hash & 0xFFFFFFFFLL), 
                        static_cast<int>((hash >> 32) & 0xFFFFFFFFLL));
    }
    
    // BlockPos is Vec3i (defined in Engine.hpp)
    template<typename Vec3I>
    static ChunkPos fromBlockPos(const Vec3I& pos) {
        return ChunkPos(
            pos.x < 0 ? (pos.x - 15) / 16 : pos.x / 16,
            pos.z < 0 ? (pos.z - 15) / 16 : pos.z / 16
        );
    }
};

// Hash functor for ChunkPos
struct ChunkPosHash {
    size_t operator()(const ChunkPos& pos) const {
        return std::hash<int64_t>()(pos.toHash());
    }
};

} // namespace VoxelForge

// Standard hash specialization
namespace std {
    template<>
    struct hash<VoxelForge::ChunkPos> {
        size_t operator()(const VoxelForge::ChunkPos& pos) const {
            return std::hash<int64_t>()(pos.toHash());
        }
    };
    
    // Hash for glm::ivec3 (BlockPos)
} // namespace VoxelForge