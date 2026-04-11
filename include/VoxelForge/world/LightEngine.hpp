/**
 * @file LightEngine.hpp
 * @brief Lighting calculation engine
 */

#pragma once

#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/world/ChunkPos.hpp>
#include <VoxelForge/world/BlockRegistry.hpp>
#include <vector>
#include <memory>

namespace VoxelForge {

class Chunk;
class World;

/**
 * @brief Handles sky and block light calculations
 *
 * Calculates lighting for chunks including sky light
 * (from sunlight) and block light (from emissive blocks).
 * Uses a flood-fill approach for propagation.
 */
class LightEngine {
public:
    explicit LightEngine(World* world);
    ~LightEngine() = default;

    // Update lighting for a chunk
    void updateChunk(Chunk* chunk);

    // Update lighting around a changed block
    void updateBlockLight(const BlockPos& pos, int newLevel);
    void removeBlockLight(const BlockPos& pos);

    // Get light level at position
    uint8_t getSkyLight(const BlockPos& pos) const;
    uint8_t getBlockLight(const BlockPos& pos) const;

private:
    World* world_;

    // Calculate sky light for a chunk
    void calculateSkyLight(Chunk* chunk);

    // Calculate block light for a chunk
    void calculateBlockLight(Chunk* chunk);

    // Propagate block light using flood-fill
    void propagateBlockLight(Chunk* chunk, int x, int y, int z, int level);

    // Get light emission from block
    int getLightEmission(BlockState state) const;
};

} // namespace VoxelForge
