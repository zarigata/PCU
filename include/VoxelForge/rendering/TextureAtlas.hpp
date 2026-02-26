/**
 * @file TextureAtlas.hpp
 * @brief Texture atlas for block textures
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <VoxelForge/rendering/VulkanImage.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace VoxelForge {

class VulkanDevice;

// Texture coordinates in atlas
struct AtlasCoords {
    glm::vec2 uvMin;
    glm::vec2 uvMax;
    uint32_t layerIndex;  // For texture arrays
    uint32_t texIndex;    // For bindless textures
};

// Texture info for loading
struct TextureInfo {
    std::string name;
    std::string path;
    bool transparent = false;
    bool animated = false;
    int animationFrames = 1;
    int animationSpeed = 10;  // Ticks per frame
};

// Texture atlas settings
struct TextureAtlasSettings {
    uint32_t textureSize = 16;        // Size of each texture
    uint32_t atlasWidth = 256;        // Width of atlas in pixels
    uint32_t atlasHeight = 256;       // Height of atlas in pixels
    uint32_t maxTextures = 1024;      // Maximum number of textures
    bool useArray = true;             // Use texture array instead of atlas
    bool enableMipmaps = true;        // Generate mipmaps
    vk::Format format = vk::Format::eR8G8B8A8Srgb;
};

// Animated texture data
struct AnimatedTexture {
    uint32_t baseIndex;
    std::vector<uint32_t> frameIndices;
    int currentFrame = 0;
    int frameTimer = 0;
    int frameSpeed;
};

class TextureAtlas {
public:
    TextureAtlas();
    ~TextureAtlas();
    
    // No copy
    TextureAtlas(const TextureAtlas&) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;
    
    void init(VulkanDevice* device, const TextureAtlasSettings& settings = {});
    void cleanup();
    
    // Texture management
    uint32_t addTexture(const std::string& name, const uint8_t* data, 
                        uint32_t width, uint32_t height, uint32_t channels = 4);
    uint32_t addTexture(const std::string& name, const std::string& path);
    uint32_t addTexture(const TextureInfo& info);
    
    // Get texture coordinates
    AtlasCoords getTextureCoords(const std::string& name) const;
    AtlasCoords getTextureCoords(uint32_t index) const;
    
    // Get texture index
    uint32_t getTextureIndex(const std::string& name) const;
    bool hasTexture(const std::string& name) const;
    
    // Build atlas (call after adding all textures)
    void build(vk::CommandBuffer cmd);
    void rebuild(vk::CommandBuffer cmd);
    
    // Update animated textures
    void updateAnimations(float deltaTime);
    
    // Getters
    vk::ImageView getAtlasView() const;
    vk::Sampler getSampler() const;
    vk::DescriptorImageInfo getDescriptorInfo() const;
    
    uint32_t getTextureCount() const { return static_cast<uint32_t>(textures.size()); }
    uint32_t getTextureSize() const { return settings.textureSize; }
    bool isArray() const { return settings.useArray; }
    
    // Get all texture names
    std::vector<std::string> getTextureNames() const;
    
    // Default textures
    void createDefaultTextures();
    uint32_t getMissingTextureIndex() const { return missingTextureIndex; }
    uint32_t getWhiteTextureIndex() const { return whiteTextureIndex; }
    uint32_t getNormalTextureIndex() const { return normalTextureIndex; }
    
private:
    void createAtlasImage();
    void createSampler();
    void copyTextureToArray(vk::CommandBuffer cmd, const uint8_t* data, 
                           uint32_t width, uint32_t height, uint32_t layer);
    void generateMipmaps(vk::CommandBuffer cmd);
    
    VulkanDevice* device = nullptr;
    TextureAtlasSettings settings;
    
    // Atlas image
    vk::Image atlasImage;
    vk::DeviceMemory atlasMemory;
    vk::ImageView atlasView;
    vk::Sampler sampler;
    
    // Texture data
    struct TextureData {
        std::string name;
        std::vector<uint8_t> pixels;
        uint32_t width;
        uint32_t height;
        uint32_t channels;
        uint32_t index;
        bool transparent;
        bool animated;
        int animationFrames;
        int animationSpeed;
    };
    std::vector<TextureData> textures;
    std::unordered_map<std::string, uint32_t> textureMap;
    
    // Animated textures
    std::vector<AnimatedTexture> animatedTextures;
    
    // State
    bool built = false;
    bool dirty = false;
    
    // Default textures
    uint32_t missingTextureIndex = 0;
    uint32_t whiteTextureIndex = 0;
    uint32_t normalTextureIndex = 0;
};

// Texture atlas builder for easier creation
class TextureAtlasBuilder {
public:
    TextureAtlasBuilder();
    
    TextureAtlasBuilder& setTextureSize(uint32_t size);
    TextureAtlasBuilder& setAtlasSize(uint32_t width, uint32_t height);
    TextureAtlasBuilder& setMaxTextures(uint32_t max);
    TextureAtlasBuilder& useArray(bool use = true);
    TextureAtlasBuilder& enableMipmaps(bool enable = true);
    TextureAtlasBuilder& setFormat(vk::Format format);
    
    std::unique_ptr<TextureAtlas> build(VulkanDevice* device);
    
private:
    TextureAtlasSettings settings;
};

} // namespace VoxelForge
