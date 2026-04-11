/**
 * @file ChunkManager.hpp
 * @brief Chunk loading/unloading management
 */

#pragma once

#include <VoxelForge/world/ChunkPos.hpp>
#include <VoxelForge/world/Block.hpp>
#include <glm/glm.hpp>
#include <unordered_map>
#include <memory>
#include <queue>
#include <vector>

namespace VoxelForge {

class Chunk;
class World;

/**
 * @brief Manages chunk loading/unloading around the player
 *
 * Maintains chunks within view distance, loading new chunks and
 * unloading distant ones. Uses a load queue to distribute work
 * across frames.
 */
class ChunkManager {
public:
    explicit ChunkManager(World* world, int viewDistance = 8);
    ~ChunkManager();

    // Update chunk loading/unloading
    void update(const glm::vec3& playerPos);

    // Get chunk at position
    Chunk* getChunk(const ChunkPos& pos);
    bool hasChunk(const ChunkPos& pos) const;

    // Load/unload chunks
    void loadChunk(const ChunkPos& pos);
    void unloadChunk(const ChunkPos& pos);
    void unloadAllChunks();

    // Statistics
    size_t getLoadedCount() const;
    size_t getLoadQueueSize() const { return loadQueue_.size(); }

    // Configuration
    void setViewDistance(int distance) { viewDistance_ = distance; }
    int getViewDistance() const { return viewDistance_; }
    void setMaxLoadsPerFrame(int max) { maxLoadsPerFrame_ = max; }
    int getMaxLoadsPerFrame() const { return maxLoadsPerFrame_; }

private:
    World* world_;
    int viewDistance_;
    int maxLoadsPerFrame_ = 4;

    // Loaded chunks
    std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash> loadedChunks_;

    // Load queue (FIFO)
    std::queue<ChunkPos> loadQueue_;
};

} // namespace VoxelForge
