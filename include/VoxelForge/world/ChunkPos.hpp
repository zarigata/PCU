/**
 * @file ChunkPos.hpp
 * @brief Chunk position type
 */

#pragma once

#include <cstdint>
#include <functional>

namespace VoxelForge {

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
}