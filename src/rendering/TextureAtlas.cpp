/**
 * @file TextureAtlas.cpp
 * @brief Texture atlas implementation
 */

#include "TextureAtlas.hpp"
#include <VoxelForge/rendering/VulkanDevice.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace VoxelForge {

TextureAtlas::TextureAtlas() = default;

TextureAtlas::~TextureAtlas() {
    cleanup();
}

void TextureAtlas::init(VulkanDevice* device, const TextureAtlasSettings& settings) {
    this->device = device;
    this->settings = settings;
    
    createAtlasImage();
    createSampler();
    createDefaultTextures();
    
    Logger::info("TextureAtlas initialized ({}x{}, {} textures max)",
                 settings.atlasWidth, settings.atlasHeight, settings.maxTextures);
}

void TextureAtlas::cleanup() {
    if (!device) return;
    
    auto vkDevice = device->getDevice();
    
    if (sampler) {
        vkDevice.destroySampler(sampler);
        sampler = nullptr;
    }
    if (atlasView) {
        vkDevice.destroyImageView(atlasView);
        atlasView = nullptr;
    }
    if (atlasImage) {
        vkDevice.destroyImage(atlasImage);
        atlasImage = nullptr;
    }
    if (atlasMemory) {
        vkDevice.freeMemory(atlasMemory);
        atlasMemory = nullptr;
    }
    
    textures.clear();
    textureMap.clear();
    animatedTextures.clear();
    
    device = nullptr;
}

void TextureAtlas::createAtlasImage() {
    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.format = settings.format;
    
    if (settings.useArray) {
        imageInfo.extent = {settings.textureSize, settings.textureSize, 1};
        imageInfo.arrayLayers = settings.maxTextures;
    } else {
        imageInfo.extent = {settings.atlasWidth, settings.atlasHeight, 1};
        imageInfo.arrayLayers = 1;
    }
    
    imageInfo.mipLevels = settings.enableMipmaps ? 
        static_cast<uint32_t>(std::floor(std::log2(settings.textureSize))) + 1 : 1;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | 
                      vk::ImageUsageFlagBits::eSampled |
                      (settings.enableMipmaps ? vk::ImageUsageFlagBits::eTransferSrc : vk::ImageUsageFlags{});
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    
    atlasImage = device->getDevice().createImage(imageInfo);
    
    // Allocate memory
    auto memRequirements = device->getDevice().getImageMemoryRequirements(atlasImage);
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device->findMemoryType(
        memRequirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    
    atlasMemory = device->getDevice().allocateMemory(allocInfo);
    device->getDevice().bindImageMemory(atlasImage, atlasMemory, 0);
    
    // Create image view
    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = atlasImage;
    viewInfo.viewType = settings.useArray ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D;
    viewInfo.format = settings.format;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = imageInfo.arrayLayers;
    
    atlasView = device->getDevice().createImageView(viewInfo);
}

void TextureAtlas::createSampler() {
    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.magFilter = vk::Filter::eNearest;  // Pixelated look for Minecraft
    samplerInfo.minFilter = vk::Filter::eNearest;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.anisotropyEnable = device->supportsAnisotropy() ? VK_TRUE : VK_FALSE;
    samplerInfo.maxAnisotropy = std::min(4.0f, device->getMaxSamplerAnisotropy());
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = settings.enableMipmaps ? 
        static_cast<float>(std::floor(std::log2(settings.textureSize))) : 0.0f;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    
    sampler = device->getDevice().createSampler(samplerInfo);
}

void TextureAtlas::createDefaultTextures() {
    // Missing texture (magenta/black checkerboard)
    std::vector<uint8_t> missingData(settings.textureSize * settings.textureSize * 4);
    for (uint32_t y = 0; y < settings.textureSize; y++) {
        for (uint32_t x = 0; x < settings.textureSize; x++) {
            bool checker = ((x / 8) + (y / 8)) % 2 == 0;
            size_t idx = (y * settings.textureSize + x) * 4;
            missingData[idx + 0] = checker ? 255 : 0;    // R
            missingData[idx + 1] = 0;                     // G
            missingData[idx + 2] = checker ? 255 : 0;    // B
            missingData[idx + 3] = 255;                   // A
        }
    }
    missingTextureIndex = addTexture("missing", missingData.data(), 
                                     settings.textureSize, settings.textureSize, 4);
    
    // White texture
    std::vector<uint8_t> whiteData(settings.textureSize * settings.textureSize * 4, 255);
    whiteTextureIndex = addTexture("white", whiteData.data(),
                                   settings.textureSize, settings.textureSize, 4);
    
    // Normal texture (flat normal map)
    std::vector<uint8_t> normalData(settings.textureSize * settings.textureSize * 4);
    for (size_t i = 0; i < normalData.size(); i += 4) {
        normalData[i + 0] = 128;  // X
        normalData[i + 1] = 128;  // Y
        normalData[i + 2] = 255;  // Z
        normalData[i + 3] = 255;  // W
    }
    normalTextureIndex = addTexture("flat_normal", normalData.data(),
                                    settings.textureSize, settings.textureSize, 4);
}

uint32_t TextureAtlas::addTexture(const std::string& name, const uint8_t* data,
                                   uint32_t width, uint32_t height, uint32_t channels) {
    if (textureMap.find(name) != textureMap.end()) {
        return textureMap[name];
    }
    
    uint32_t index = static_cast<uint32_t>(textures.size());
    if (index >= settings.maxTextures) {
        Logger::warn("Texture atlas full, cannot add texture: {}", name);
        return missingTextureIndex;
    }
    
    TextureData texData;
    texData.name = name;
    texData.width = width;
    texData.height = height;
    texData.channels = channels;
    texData.index = index;
    texData.transparent = false;
    texData.animated = false;
    
    // Copy pixel data
    texData.pixels.resize(width * height * 4);
    
    if (channels == 4) {
        std::copy(data, data + width * height * 4, texData.pixels.begin());
        // Check for transparency
        for (size_t i = 3; i < texData.pixels.size(); i += 4) {
            if (texData.pixels[i] < 255) {
                texData.transparent = true;
                break;
            }
        }
    } else if (channels == 3) {
        // Convert RGB to RGBA
        for (size_t i = 0; i < width * height; i++) {
            texData.pixels[i * 4 + 0] = data[i * 3 + 0];
            texData.pixels[i * 4 + 1] = data[i * 3 + 1];
            texData.pixels[i * 4 + 2] = data[i * 3 + 2];
            texData.pixels[i * 4 + 3] = 255;
        }
    }
    
    textures.push_back(std::move(texData));
    textureMap[name] = index;
    dirty = true;
    
    return index;
}

uint32_t TextureAtlas::addTexture(const std::string& name, const std::string& path) {
    int width, height, channels;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    
    if (!pixels) {
        Logger::error("Failed to load texture: {} ({})", path, stbi_failure_reason());
        return missingTextureIndex;
    }
    
    uint32_t index = addTexture(name, pixels, width, height, 4);
    stbi_image_free(pixels);
    
    return index;
}

uint32_t TextureAtlas::addTexture(const TextureInfo& info) {
    uint32_t index = addTexture(info.name, info.path);
    
    if (info.animated && index < textures.size()) {
        AnimatedTexture anim{};
        anim.baseIndex = index;
        anim.frameSpeed = info.animationSpeed;
        animatedTextures.push_back(anim);
        textures[index].animated = true;
        textures[index].animationFrames = info.animationFrames;
        textures[index].animationSpeed = info.animationSpeed;
    }
    
    return index;
}

AtlasCoords TextureAtlas::getTextureCoords(const std::string& name) const {
    auto it = textureMap.find(name);
    if (it == textureMap.end()) {
        return getTextureCoords(missingTextureIndex);
    }
    return getTextureCoords(it->second);
}

AtlasCoords TextureAtlas::getTextureCoords(uint32_t index) const {
    AtlasCoords coords{};
    
    if (settings.useArray) {
        // Texture array - each texture is its own layer
        coords.uvMin = glm::vec2(0.0f, 0.0f);
        coords.uvMax = glm::vec2(1.0f, 1.0f);
        coords.layerIndex = index;
        coords.texIndex = index;
    } else {
        // Texture atlas - calculate position in atlas
        uint32_t texturesPerRow = settings.atlasWidth / settings.textureSize;
        uint32_t row = index / texturesPerRow;
        uint32_t col = index % texturesPerRow;
        
        float uSize = static_cast<float>(settings.textureSize) / settings.atlasWidth;
        float vSize = static_cast<float>(settings.textureSize) / settings.atlasHeight;
        
        coords.uvMin = glm::vec2(col * uSize, row * vSize);
        coords.uvMax = glm::vec2((col + 1) * uSize, (row + 1) * vSize);
        coords.layerIndex = 0;
        coords.texIndex = index;
    }
    
    return coords;
}

uint32_t TextureAtlas::getTextureIndex(const std::string& name) const {
    auto it = textureMap.find(name);
    if (it == textureMap.end()) {
        return missingTextureIndex;
    }
    return it->second;
}

bool TextureAtlas::hasTexture(const std::string& name) const {
    return textureMap.find(name) != textureMap.end();
}

void TextureAtlas::build(vk::CommandBuffer cmd) {
    if (!dirty || textures.empty()) return;
    
    // Transition image to transfer destination
    vk::ImageMemoryBarrier barrier{};
    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = atlasImage;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = settings.useArray ? settings.maxTextures : 1;
    barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                        vk::PipelineStageFlagBits::eTransfer,
                        vk::DependencyFlagBits::eByRegion,
                        0, nullptr, 0, nullptr, 1, &barrier);
    
    // Upload each texture
    for (const auto& tex : textures) {
        copyTextureToArray(cmd, tex.pixels.data(), tex.width, tex.height, tex.index);
    }
    
    // Generate mipmaps
    if (settings.enableMipmaps) {
        generateMipmaps(cmd);
    }
    
    // Transition to shader read
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eFragmentShader,
                        vk::DependencyFlagBits::eByRegion,
                        0, nullptr, 0, nullptr, 1, &barrier);
    
    built = true;
    dirty = false;
    
    Logger::debug("TextureAtlas built with {} textures", textures.size());
}

void TextureAtlas::rebuild(vk::CommandBuffer cmd) {
    dirty = true;
    build(cmd);
}

void TextureAtlas::copyTextureToArray(vk::CommandBuffer cmd, const uint8_t* data,
                                       uint32_t width, uint32_t height, uint32_t layer) {
    // Create staging buffer
    vk::DeviceSize imageSize = width * height * 4;
    Buffer stagingBuffer = VulkanBuffer::createStagingBuffer(
        device->getDevice(),
        device->getPhysicalDevice(),
        imageSize
    );
    
    // Copy data to staging buffer
    stagingBuffer.map(device->getDevice());
    memcpy(stagingBuffer.mapped, data, imageSize);
    stagingBuffer.unmap(device->getDevice());
    
    // Copy to image
    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = settings.useArray ? layer : 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    
    cmd.copyBufferToImage(stagingBuffer.buffer, atlasImage,
                          vk::ImageLayout::eTransferDstOptimal, 1, &region);
    
    // Cleanup staging buffer
    VulkanBuffer::destroyBuffer(device->getDevice(), stagingBuffer);
}

void TextureAtlas::generateMipmaps(vk::CommandBuffer cmd) {
    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(settings.textureSize))) + 1;
    
    vk::ImageMemoryBarrier barrier{};
    barrier.image = atlasImage;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = settings.useArray ? settings.maxTextures : 1;
    
    int32_t mipWidth = settings.textureSize;
    int32_t mipHeight = settings.textureSize;
    
    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.subresourceRange.levelCount = 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                            vk::PipelineStageFlagBits::eTransfer,
                            vk::DependencyFlagBits::eByRegion,
                            0, nullptr, 0, nullptr, 1, &barrier);
        
        vk::ImageBlit blit{};
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = settings.useArray ? settings.maxTextures : 1;
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = settings.useArray ? settings.maxTextures : 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1,
                             mipHeight > 1 ? mipHeight / 2 : 1, 1};
        
        cmd.blitImage(atlasImage, vk::ImageLayout::eTransferSrcOptimal,
                     atlasImage, vk::ImageLayout::eTransferDstOptimal,
                     1, &blit, vk::Filter::eNearest);
        
        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                            vk::PipelineStageFlagBits::eFragmentShader,
                            vk::DependencyFlagBits::eByRegion,
                            0, nullptr, 0, nullptr, 1, &barrier);
        
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }
    
    // Transition last mip level
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eFragmentShader,
                        vk::DependencyFlagBits::eByRegion,
                        0, nullptr, 0, nullptr, 1, &barrier);
}

void TextureAtlas::updateAnimations(float deltaTime) {
    for (auto& anim : animatedTextures) {
        anim.frameTimer++;
        if (anim.frameTimer >= anim.frameSpeed) {
            anim.frameTimer = 0;
            anim.currentFrame = (anim.currentFrame + 1) % anim.frameIndices.size();
        }
    }
}

vk::ImageView TextureAtlas::getAtlasView() const {
    return atlasView;
}

vk::Sampler TextureAtlas::getSampler() const {
    return sampler;
}

vk::DescriptorImageInfo TextureAtlas::getDescriptorInfo() const {
    vk::DescriptorImageInfo info{};
    info.imageView = atlasView;
    info.sampler = sampler;
    info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    return info;
}

std::vector<std::string> TextureAtlas::getTextureNames() const {
    std::vector<std::string> names;
    names.reserve(textures.size());
    for (const auto& tex : textures) {
        names.push_back(tex.name);
    }
    return names;
}

// TextureAtlasBuilder

TextureAtlasBuilder::TextureAtlasBuilder() {
    settings = {};
}

TextureAtlasBuilder& TextureAtlasBuilder::setTextureSize(uint32_t size) {
    settings.textureSize = size;
    return *this;
}

TextureAtlasBuilder& TextureAtlasBuilder::setAtlasSize(uint32_t width, uint32_t height) {
    settings.atlasWidth = width;
    settings.atlasHeight = height;
    return *this;
}

TextureAtlasBuilder& TextureAtlasBuilder::setMaxTextures(uint32_t max) {
    settings.maxTextures = max;
    return *this;
}

TextureAtlasBuilder& TextureAtlasBuilder::useArray(bool use) {
    settings.useArray = use;
    return *this;
}

TextureAtlasBuilder& TextureAtlasBuilder::enableMipmaps(bool enable) {
    settings.enableMipmaps = enable;
    return *this;
}

TextureAtlasBuilder& TextureAtlasBuilder::setFormat(vk::Format format) {
    settings.format = format;
    return *this;
}

std::unique_ptr<TextureAtlas> TextureAtlasBuilder::build(VulkanDevice* device) {
    auto atlas = std::make_unique<TextureAtlas>();
    atlas->init(device, settings);
    return atlas;
}

} // namespace VoxelForge
