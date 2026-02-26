/**
 * @file VulkanImage.cpp
 * @brief Vulkan image management
 */

#include <VoxelForge/rendering/VulkanImage.hpp>
#include <VoxelForge/rendering/VulkanContext.hpp>
#include <VoxelForge/rendering/VulkanBuffer.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

vk::UniqueImage VulkanImage::createImage(
    vk::Device device,
    uint32_t width,
    uint32_t height,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    vk::DeviceMemory& outMemory,
    uint32_t mipLevels,
    vk::SampleCountFlagBits numSamples) {
    
    vk::ImageCreateInfo imageInfo{};
    imageInfo.sType = vk::StructureType::eImageCreateInfo;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    
    vk::UniqueImage image;
    try {
        image = device.createImageUnique(imageInfo);
    } catch (const vk::SystemError& e) {
        LOG_ERROR("Failed to create image: {}", e.what());
        return vk::UniqueImage{};
    }
    
    // Allocate memory
    vk::MemoryRequirements memRequirements = device.getImageMemoryRequirements(image.get());
    
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    allocInfo.allocationSize = memRequirements.size;
    // Find memory type - would need physical device
    // allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    
    try {
        outMemory = device.allocateMemory(allocInfo);
    } catch (const vk::SystemError& e) {
        LOG_ERROR("Failed to allocate image memory: {}", e.what());
        return vk::UniqueImage{};
    }
    
    device.bindImageMemory(image.get(), outMemory, 0);
    
    return image;
}

vk::UniqueImageView VulkanImage::createImageView(
    vk::Device device,
    vk::Image image,
    vk::Format format,
    vk::ImageAspectFlags aspectFlags,
    uint32_t mipLevels) {
    
    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
    viewInfo.image = image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    try {
        return device.createImageViewUnique(viewInfo);
    } catch (const vk::SystemError& e) {
        LOG_ERROR("Failed to create image view: {}", e.what());
        return vk::UniqueImageView{};
    }
}

void VulkanImage::transitionImageLayout(
    vk::Device device,
    vk::CommandBuffer cmd,
    vk::Image image,
    vk::Format format,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    uint32_t mipLevels) {
    
    vk::ImageMemoryBarrier barrier{};
    barrier.sType = vk::StructureType::eImageMemoryBarrier;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    vk::PipelineStageFlags srcStage;
    vk::PipelineStageFlags dstStage;
    
    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    } else {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    }
    
    // Determine access masks and pipeline stages
    if (oldLayout == vk::ImageLayout::eUndefined && 
        newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = vk::AccessFlags{};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && 
               newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        srcStage = vk::PipelineStageFlagBits::eTransfer;
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (oldLayout == vk::ImageLayout::eUndefined && 
               newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.srcAccessMask = vk::AccessFlags{};
        barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | 
                                vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else {
        LOG_WARN("Unsupported layout transition");
        return;
    }
    
    cmd.pipelineBarrier(
        srcStage, dstStage,
        vk::DependencyFlags{},
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

void VulkanImage::copyBufferToImage(
    vk::Device device,
    vk::CommandBuffer cmd,
    vk::Buffer buffer,
    vk::Image image,
    uint32_t width,
    uint32_t height) {
    
    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D{0, 0, 0};
    region.imageExtent = vk::Extent3D{width, height, 1};
    
    cmd.copyBufferToImage(
        buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);
}

bool VulkanImage::hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint || 
           format == vk::Format::eD24UnormS8Uint;
}

// ============================================================================
// Texture Implementation
// ============================================================================

Texture::Texture(vk::Device device, vk::PhysicalDevice physicalDevice,
                 vk::CommandPool commandPool, vk::Queue queue)
    : device_(device)
    , physicalDevice_(physicalDevice)
    , commandPool_(commandPool)
    , queue_(queue) {
}

Texture::~Texture() {
    cleanup();
}

void Texture::createTextureImage(const void* data, uint32_t width, uint32_t height,
                                  vk::Format format, uint32_t mipLevels) {
    width_ = width;
    height_ = height;
    mipLevels_ = mipLevels;
    
    // Create staging buffer
    vk::DeviceSize imageSize = width * height * 4; // Assuming RGBA
    
    auto stagingBuffer = VulkanBuffer::createBuffer(
        device_, physicalDevice_, imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | 
        vk::MemoryPropertyFlagBits::eHostCoherent
    );
    
    // Copy data to staging buffer
    memcpy(stagingBuffer.mapped, data, imageSize);
    
    // Create image
    auto image = VulkanImage::createImage(
        device_, width, height, format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | 
        vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        imageMemory_, mipLevels
    );
    
    image_ = image.release();
    
    // Transition and copy
    // Would need command buffer allocation and submission
    
    // Cleanup staging buffer
    VulkanBuffer::destroyBuffer(device_, stagingBuffer);
    
    LOG_DEBUG("Created texture image: {}x{}, {} mip levels", width, height, mipLevels);
}

void Texture::createTextureImageView(vk::Format format) {
    imageView_ = device_.createImageViewUnique(
        vk::ImageViewCreateInfo{
            vk::StructureType::eImageViewCreateInfo,
            nullptr,
            image_,
            vk::ImageViewType::e2D,
            format,
            vk::ComponentMapping{},
            vk::ImageSubresourceRange{
                vk::ImageAspectFlagBits::eColor,
                0, mipLevels_, 0, 1
            }
        }
    );
}

void Texture::createTextureSampler(vk::Filter magFilter, vk::Filter minFilter,
                                    vk::SamplerAddressMode addressMode) {
    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.sType = vk::StructureType::eSamplerCreateInfo;
    samplerInfo.magFilter = magFilter;
    samplerInfo.minFilter = minFilter;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels_);
    
    sampler_ = device_.createSamplerUnique(samplerInfo);
}

void Texture::cleanup() {
    sampler_.reset();
    imageView_.reset();
    
    if (image_) {
        device_.destroyImage(image_);
        image_ = vk::Image{};
    }
    
    if (imageMemory_) {
        device_.freeMemory(imageMemory_);
        imageMemory_ = vk::DeviceMemory{};
    }
}

} // namespace VoxelForge
