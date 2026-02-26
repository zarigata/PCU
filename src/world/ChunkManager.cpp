/**
 * @file ChunkManager.cpp
 * @brief Chunk loading/unloading management
 */

#include <VoxelForge/world/ChunkManager.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

ChunkManager::ChunkManager(World* world, int viewDistance)
    : world_(world), viewDistance_(viewDistance) {
    LOG_INFO("ChunkManager created with view distance {}", viewDistance);
}

ChunkManager::~ChunkManager() {
    unloadAllChunks();
}

void ChunkManager::update(const glm::vec3& playerPos) {
    ChunkPos playerChunk = ChunkPos::fromBlockPos(BlockPos(
        static_cast<int>(playerPos.x),
        static_cast<int>(playerPos.y),
        static_cast<int>(playerPos.z)
    ));
    
    // Load chunks around player
    for (int dx = -viewDistance_; dx <= viewDistance_; dx++) {
        for (int dz = -viewDistance_; dz <= viewDistance_; dz++) {
            ChunkPos pos(playerChunk.x + dx, playerChunk.z + dz);
            if (!hasChunk(pos)) {
                loadQueue_.push(pos);
            }
        }
    }
    
    // Unload distant chunks
    std::vector<ChunkPos> toUnload;
    for (const auto& [pos, chunk] : loadedChunks_) {
        int dx = pos.x - playerChunk.x;
        int dz = pos.z - playerChunk.z;
        if (std::abs(dx) > viewDistance_ + 2 || std::abs(dz) > viewDistance_ + 2) {
            toUnload.push_back(pos);
        }
    }
    
    for (const auto& pos : toUnload) {
        unloadChunk(pos);
    }
    
    // Process load queue
    int loaded = 0;
    while (!loadQueue_.empty() && loaded < maxLoadsPerFrame_) {
        ChunkPos pos = loadQueue_.front();
        loadQueue_.pop();
        
        if (!hasChunk(pos)) {
            loadChunk(pos);
            loaded++;
        }
    }
}

Chunk* ChunkManager::getChunk(const ChunkPos& pos) {
    auto it = loadedChunks_.find(pos);
    if (it != loadedChunks_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool ChunkManager::hasChunk(const ChunkPos& pos) const {
    return loadedChunks_.find(pos) != loadedChunks_.end();
}

void ChunkManager::loadChunk(const ChunkPos& pos) {
    if (hasChunk(pos)) return;
    
    auto chunk = std::make_unique<Chunk>(pos);
    chunk->setWorld(world_);
    
    // Generate chunk
    // TODO: Call world generator
    
    loadedChunks_[pos] = std::move(chunk);
    LOG_DEBUG("Loaded chunk ({}, {})", pos.x, pos.z);
}

void ChunkManager::unloadChunk(const ChunkPos& pos) {
    auto it = loadedChunks_.find(pos);
    if (it != loadedChunks_.end()) {
        // TODO: Save chunk before unloading
        loadedChunks_.erase(it);
        LOG_DEBUG("Unloaded chunk ({}, {})", pos.x, pos.z);
    }
}

void ChunkManager::unloadAllChunks() {
    loadedChunks_.clear();
    LOG_INFO("Unloaded all chunks");
}

size_t ChunkManager::getLoadedCount() const {
    return loadedChunks_.size();
}

} // namespace VoxelForge
