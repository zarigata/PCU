/**
 * @file ResourceManager.cpp
 * @brief Resource manager implementation
 */

#include <VoxelForge/engine/ResourceManager.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

ResourceManager& ResourceManager::get() {
    static ResourceManager instance;
    return instance;
}

void ResourceManager::setAssetPath(const std::string& path) {
    assetPath_ = path;
    LOG_INFO("Asset path set to: {}", path);
}

void ResourceManager::loadTexture(const std::string& id, const std::string& path) {
    LOG_DEBUG("Loading texture: {} from {}", id, path);
    // TODO: Implement texture loading
}

void ResourceManager::loadShader(const std::string& id, const std::string& vertexPath, const std::string& fragmentPath) {
    LOG_DEBUG("Loading shader: {} from {}, {}", id, vertexPath, fragmentPath);
    // TODO: Implement shader loading
}

void ResourceManager::loadModel(const std::string& id, const std::string& path) {
    LOG_DEBUG("Loading model: {} from {}", id, path);
    // TODO: Implement model loading
}

void ResourceManager::unload(const std::string& id) {
    resources_.erase(id);
}

void ResourceManager::unloadAll() {
    resources_.clear();
    LOG_INFO("All resources unloaded");
}

} // namespace VoxelForge
