/**
 * @file ModRegistry.cpp
 * @brief Mod registry implementation
 */

#include <VoxelForge/modding/ModLoader.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

// ============================================================================
// ModRegistry Implementation
// ============================================================================

void ModRegistry::registerBlock(const std::string& modId, const std::string& blockId, 
                                 sol::table properties) {
    std::string fullId = modId + ":" + blockId;
    
    RegisteredBlock block;
    block.modId = modId;
    block.blockId = blockId;
    block.numericId = nextBlockId++;
    block.properties = properties;
    
    blocks[fullId] = block;
    
    LOG_DEBUG("Registered block: {} -> {}", fullId, block.numericId);
}

void ModRegistry::registerItem(const std::string& modId, const std::string& itemId,
                                sol::table properties) {
    std::string fullId = modId + ":" + itemId;
    
    RegisteredItem item;
    item.modId = modId;
    item.itemId = itemId;
    item.numericId = nextItemId++;
    item.properties = properties;
    
    items[fullId] = item;
    
    LOG_DEBUG("Registered item: {} -> {}", fullId, item.numericId);
}

void ModRegistry::registerEntity(const std::string& modId, const std::string& entityId,
                                  sol::table properties) {
    std::string fullId = modId + ":" + entityId;
    
    RegisteredEntity entity;
    entity.modId = modId;
    entity.entityId = entityId;
    entity.properties = properties;
    
    entities[fullId] = entity;
    
    LOG_DEBUG("Registered entity: {}", fullId);
}

void ModRegistry::registerRecipe(const std::string& modId, sol::table recipe) {
    recipes.push_back(recipe);
    LOG_DEBUG("Registered recipe from mod: {}", modId);
}

const ModRegistry::RegisteredBlock* ModRegistry::getBlock(const std::string& fullId) const {
    auto it = blocks.find(fullId);
    return it != blocks.end() ? &it->second : nullptr;
}

const ModRegistry::RegisteredItem* ModRegistry::getItem(const std::string& fullId) const {
    auto it = items.find(fullId);
    return it != items.end() ? &it->second : nullptr;
}

const ModRegistry::RegisteredEntity* ModRegistry::getEntity(const std::string& fullId) const {
    auto it = entities.find(fullId);
    return it != entities.end() ? &it->second : nullptr;
}

std::vector<std::string> ModRegistry::getBlocksByMod(const std::string& modId) const {
    std::vector<std::string> result;
    for (const auto& [id, block] : blocks) {
        if (block.modId == modId) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<std::string> ModRegistry::getItemsByMod(const std::string& modId) const {
    std::vector<std::string> result;
    for (const auto& [id, item] : items) {
        if (item.modId == modId) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<std::string> ModRegistry::getEntitiesByMod(const std::string& modId) const {
    std::vector<std::string> result;
    for (const auto& [id, entity] : entities) {
        if (entity.modId == modId) {
            result.push_back(id);
        }
    }
    return result;
}

void ModRegistry::clearModContent(const std::string& modId) {
    // Remove blocks
    for (auto it = blocks.begin(); it != blocks.end(); ) {
        if (it->second.modId == modId) {
            it = blocks.erase(it);
        } else {
            ++it;
        }
    }
    
    // Remove items
    for (auto it = items.begin(); it != items.end(); ) {
        if (it->second.modId == modId) {
            it = items.erase(it);
        } else {
            ++it;
        }
    }
    
    // Remove entities
    for (auto it = entities.begin(); it != entities.end(); ) {
        if (it->second.modId == modId) {
            it = entities.erase(it);
        } else {
            ++it;
        }
    }
    
    LOG_INFO("Cleared all content for mod: {}", modId);
}

void ModRegistry::clear() {
    blocks.clear();
    items.clear();
    entities.clear();
    recipes.clear();
    
    nextBlockId = 1000;
    nextItemId = 1000;
    
    LOG_INFO("ModRegistry cleared");
}

} // namespace VoxelForge
