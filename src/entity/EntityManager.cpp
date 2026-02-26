/**
 * @file EntityManager.cpp
 * @brief Entity manager implementation
 */

#include <VoxelForge/entity/EntityManager.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>

namespace VoxelForge {

EntityManager::EntityManager() 
    : nextEntityId_(1) {
    LOG_INFO("EntityManager created");
}

EntityManager::~EntityManager() {
    entities_.clear();
    LOG_INFO("EntityManager destroyed");
}

Entity EntityManager::createEntity() {
    Entity entity = nextEntityId_++;
    entities_.insert(entity);
    return entity;
}

void EntityManager::destroyEntity(Entity entity) {
    entities_.erase(entity);
    
    // Remove from all spatial partitions
    for (auto& [chunk, entitySet] : spatialIndex_) {
        entitySet.erase(entity);
    }
    
    LOG_DEBUG("Destroyed entity {}", entity);
}

bool EntityManager::exists(Entity entity) const {
    return entities_.find(entity) != entities_.end();
}

void EntityManager::updateEntityPosition(Entity entity, const ChunkPos& oldChunk, const ChunkPos& newChunk) {
    if (oldChunk == newChunk) return;
    
    // Remove from old chunk
    auto oldIt = spatialIndex_.find(oldChunk);
    if (oldIt != spatialIndex_.end()) {
        oldIt->second.erase(entity);
        if (oldIt->second.empty()) {
            spatialIndex_.erase(oldIt);
        }
    }
    
    // Add to new chunk
    spatialIndex_[newChunk].insert(entity);
}

std::vector<Entity> EntityManager::getEntitiesInChunk(const ChunkPos& chunk) const {
    std::vector<Entity> result;
    
    auto it = spatialIndex_.find(chunk);
    if (it != spatialIndex_.end()) {
        result.reserve(it->second.size());
        for (Entity entity : it->second) {
            result.push_back(entity);
        }
    }
    
    return result;
}

std::vector<Entity> EntityManager::getEntitiesInRange(const Vec3& center, float radius) const {
    std::vector<Entity> result;
    float radiusSq = radius * radius;
    
    for (Entity entity : entities_) {
        // Would need position component to check distance
        // For now, return all entities
        result.push_back(entity);
    }
    
    return result;
}

size_t EntityManager::getEntityCount() const {
    return entities_.size();
}

void EntityManager::clear() {
    entities_.clear();
    spatialIndex_.clear();
    nextEntityId_ = 1;
    LOG_INFO("All entities cleared");
}

UUID EntityManager::generateUUID() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    UUID uuid;
    uuid.high = dis(gen);
    uuid.low = dis(gen);
    
    // Set version 4 (random)
    uuid.high = (uuid.high & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    // Set variant
    uuid.low = (uuid.low & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
    
    return uuid;
}

} // namespace VoxelForge
