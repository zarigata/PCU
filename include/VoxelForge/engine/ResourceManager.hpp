/**
 * @file ResourceManager.hpp
 * @brief Resource management for textures, shaders, and models
 */

#pragma once

#include <string>
#include <unordered_map>
#include <memory>

namespace VoxelForge {

/**
 * @class ResourceManager
 * @brief Singleton resource manager for loading and caching assets
 */
class ResourceManager {
public:
    static ResourceManager& get();
    
    void setAssetPath(const std::string& path);
    
    void loadTexture(const std::string& id, const std::string& path);
    void loadShader(const std::string& id, const std::string& vertexPath, const std::string& fragmentPath);
    void loadModel(const std::string& id, const std::string& path);
    
    void unload(const std::string& id);
    void unloadAll();
    
    template<typename T>
    T* get(const std::string& id);
    
private:
    ResourceManager() = default;
    ~ResourceManager() = default;
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    
    std::string assetPath_;
    std::unordered_map<std::string, std::shared_ptr<void>> resources_;
};

} // namespace VoxelForge