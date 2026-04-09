/**
 * @file BlockTypes.hpp
 * @brief Block type definitions
 */

#pragma once

#include "Block.hpp"
#include <cstdint>

namespace VoxelForge {

// Block type IDs
enum class BlockTypeID : uint32_t {
    Air = 0,
    Stone = 1,
    Grass = 2,
    Dirt = 3,
    Cobblestone = 4,
    Wood = 5,
    Leaves = 6,
    Sand = 7,
    Water = 8,
    Lava = 9,
    Glass = 10,
    Bedrock = 11,
    Custom = 1000
};

// Block state flags
enum class BlockFlags : uint32_t {
    None = 0,
    Solid = 1 << 0,
    Transparent = 1 << 1,
    Liquid = 1 << 2,
    Flammable = 1 << 3,
    Breakable = 1 << 4,
    Interactable = 1 << 5
};

inline BlockFlags operator|(BlockFlags a, BlockFlags b) {
    return static_cast<BlockFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline BlockFlags operator&(BlockFlags a, BlockFlags b) {
    return static_cast<BlockFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

} // namespace VoxelForge