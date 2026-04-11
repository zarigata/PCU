/**
 * @file FluidTypes.hpp
 * @brief Fluid type definitions for VoxelForge
 */

#pragma once

#include <cstdint>

namespace VoxelForge {

/**
 * @brief Fluid types supported by the engine
 */
enum class FluidType : uint8_t {
    Empty = 0,
    Water = 1,
    Lava = 2,
    Custom1 = 3,
    Custom2 = 4,
    Custom3 = 5
};

} // namespace VoxelForge
