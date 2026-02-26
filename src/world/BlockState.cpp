/**
 * @file BlockState.cpp
 * @brief Block state implementation
 */

#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

uint64_t BlockState::encode() const {
    // Encode block ID and property hash into 64-bit value
    return (static_cast<uint64_t>(blockId) << 32) | (propertyHash & 0xFFFFFFFF);
}

BlockState BlockState::decode(uint64_t encoded) {
    BlockState state;
    state.blockId = static_cast<BlockID>(encoded >> 32);
    state.propertyHash = encoded & 0xFFFFFFFF;
    return state;
}

bool BlockState::isAir() const {
    return blockId == 0; // Air is always ID 0
}

bool BlockState::isSolid() const {
    auto& registry = BlockRegistry::get();
    const auto& def = registry.getDefinition(blockId);
    return def.solid;
}

bool BlockState::isOpaque() const {
    auto& registry = BlockRegistry::get();
    const auto& def = registry.getDefinition(blockId);
    return def.opaque;
}

bool BlockState::isTransparent() const {
    return !isOpaque();
}

bool BlockState::operator==(const BlockState& other) const {
    return blockId == other.blockId && propertyHash == other.propertyHash;
}

bool BlockState::operator!=(const BlockState& other) const {
    return !(*this == other);
}

} // namespace VoxelForge
