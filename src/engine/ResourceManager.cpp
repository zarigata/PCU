/**
 * @file ResourceManager.cpp
 * @brief Resource manager implementation
 */

#include <VoxelForge/engine/ResourceManager.hpp>
#include <spdlog/spdlog.h>

namespace VoxelForge {

ResourceManager& ResourceManager::get() {
    static ResourceManager instance;
    return instance;
}

void ResourceManager::setAssetPath(const std::string& path) {
    assetPath_ = path;
    spdlog::info("Asset path set to: {}", path);
}

void ResourceManager::loadTexture(const std::string& id, const std::string& path) {
    spdlog::debug("Loading texture: {} from {}", id, path);
    // TODO: Implement texture loading
}

void ResourceManager::loadShader(const std::string& id, const std::string& vertexPath, const std::string& fragmentPath) {
    spdlog::debug("Loading shader: {} from {}, {}", id, vertexPath, fragmentPath);
    // TODO: Implement shader loading
}

void ResourceManager::loadModel(const std::string& id, const std::string& path) {
    spdlog::debug("Loading model: {} from {}", id, path);
    // TODO: Implement model loading
}

void ResourceManager::unload(const std::string& id) {
    resources_.erase(id);
}

void ResourceManager::unloadAll() {
    resources_.clear();
    spdlog::info("All resources unloaded");
}

} // namespace VoxelForge