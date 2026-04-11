/**
 * @file BlockState.cpp
 * @brief Block state implementation (non-inline methods only)
 */

#include <VoxelForge/world/Block.hpp>

namespace VoxelForge {

uint64_t BlockState::encode() const {
    return (static_cast<uint64_t>(blockId) << 32) | (propertyHash & 0xFFFFFFFF);
}

BlockState BlockState::decode(uint64_t encoded) {
    BlockState state;
    state.blockId = static_cast<BlockID>(encoded >> 32);
    state.propertyHash = encoded & 0xFFFFFFFF;
    return state;
}

} // namespace VoxelForge
