/**
 * @file VulkanFramebuffer.cpp
 * @brief Vulkan framebuffer management implementation
 */

#include "VulkanFramebuffer.hpp"
#include <VoxelForge/rendering/VulkanDevice.hpp>
#include <VoxelForge/rendering/VulkanImage.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <unordered_map>

namespace VoxelForge {

// ============== FramebufferCreateInfo ==============

FramebufferCreateInfo FramebufferCreateInfo::forSwapchain(
    uint32_t width, uint32_t height,
    vk::ImageView colorView,
    vk::RenderPass renderPass) {
    FramebufferCreateInfo info;
    info.width = width;
    info.height = height;
    info.renderPass = renderPass;
    info.attachments = {colorView};
    return info;
}

FramebufferCreateInfo FramebufferCreateInfo::forOffscreen(
    uint32_t width, uint32_t height,
    vk::ImageView colorView,
    vk::ImageView depthView,
    vk::RenderPass renderPass) {
    FramebufferCreateInfo info;
    info.width = width;
    info.height = height;
    info.renderPass = renderPass;
    info.attachments = {colorView, depthView};
    return info;
}

FramebufferCreateInfo FramebufferCreateInfo::forShadowMap(
    uint32_t size,
    vk::ImageView depthView,
    vk::RenderPass renderPass) {
    FramebufferCreateInfo info;
    info.width = size;
    info.height = size;
    info.renderPass = renderPass;
    info.attachments = {depthView};
    return info;
}

// ============== VulkanFramebuffer ==============

VulkanFramebuffer::VulkanFramebuffer() = default;

VulkanFramebuffer::~VulkanFramebuffer() {
    cleanup();
}

VulkanFramebuffer::VulkanFramebuffer(VulkanFramebuffer&& other) noexcept
    : device(other.device), framebuffer(other.framebuffer),
      width(other.width), height(other.height), layers(other.layers),
      attachments(std::move(other.attachments)) {
    other.device = VK_NULL_HANDLE;
    other.framebuffer = VK_NULL_HANDLE;
}

VulkanFramebuffer& VulkanFramebuffer::operator=(VulkanFramebuffer&& other) noexcept {
    if (this != &other) {
        cleanup();
        device = other.device;
        framebuffer = other.framebuffer;
        width = other.width;
        height = other.height;
        layers = other.layers;
        attachments = std::move(other.attachments);
        
        other.device = VK_NULL_HANDLE;
        other.framebuffer = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanFramebuffer::init(vk::Device device, const FramebufferCreateInfo& createInfo) {
    this->device = device;
    this->width = createInfo.width;
    this->height = createInfo.height;
    this->layers = createInfo.layers;
    this->attachments = createInfo.attachments;
    
    vk::FramebufferCreateInfo fbInfo{};
    fbInfo.renderPass = createInfo.renderPass;
    fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    fbInfo.pAttachments = attachments.data();
    fbInfo.width = width;
    fbInfo.height = height;
    fbInfo.layers = layers;
    
    try {
        framebuffer = device.createFramebuffer(fbInfo);
    } catch (const vk::SystemError& e) {
        throw std::runtime_error("Failed to create framebuffer: " + std::string(e.what()));
    }
}

void VulkanFramebuffer::cleanup() {
    if (device && framebuffer) {
        device.destroyFramebuffer(framebuffer);
        framebuffer = VK_NULL_HANDLE;
    }
    attachments.clear();
}

// ============== VulkanFramebufferWithAttachments ==============

VulkanFramebufferWithAttachments::VulkanFramebufferWithAttachments() = default;

VulkanFramebufferWithAttachments::~VulkanFramebufferWithAttachments() {
    cleanup();
}

VulkanFramebufferWithAttachments::VulkanFramebufferWithAttachments(VulkanFramebufferWithAttachments&& other) noexcept
    : device(other.device), framebuffer(std::move(other.framebuffer)),
      colorAttachments(std::move(other.colorAttachments)),
      depthAttachment(std::move(other.depthAttachment)),
      colorFormat(other.colorFormat), depthFormat(other.depthFormat),
      samples(other.samples) {
    other.device = nullptr;
}

VulkanFramebufferWithAttachments& VulkanFramebufferWithAttachments::operator=(VulkanFramebufferWithAttachments&& other) noexcept {
    if (this != &other) {
        cleanup();
        device = other.device;
        framebuffer = std::move(other.framebuffer);
        colorAttachments = std::move(other.colorAttachments);
        depthAttachment = std::move(other.depthAttachment);
        colorFormat = other.colorFormat;
        depthFormat = other.depthFormat;
        samples = other.samples;
        
        other.device = nullptr;
    }
    return *this;
}

void VulkanFramebufferWithAttachments::createAttachment(
    VulkanDevice* device,
    FramebufferAttachment& attachment,
    vk::Format format,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspectMask) {
    
    auto vkDevice = device->getDevice();
    
    // Create image
    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.format = format;
    imageInfo.extent = {framebuffer.getWidth(), framebuffer.getHeight(), 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = samples;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    
    try {
        attachment.image = vkDevice.createImage(imageInfo);
    } catch (const vk::SystemError& e) {
        throw std::runtime_error("Failed to create attachment image: " + std::string(e.what()));
    }
    
    // Allocate memory
    auto memRequirements = vkDevice.getImageMemoryRequirements(attachment.image);
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device->findMemoryType(
        memRequirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    
    try {
        attachment.memory = vkDevice.allocateMemory(allocInfo);
        vkDevice.bindImageMemory(attachment.image, attachment.memory, 0);
    } catch (const vk::SystemError& e) {
        vkDevice.destroyImage(attachment.image);
        throw std::runtime_error("Failed to allocate attachment memory: " + std::string(e.what()));
    }
    
    // Create image view
    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = attachment.image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    try {
        attachment.imageView = vkDevice.createImageView(viewInfo);
    } catch (const vk::SystemError& e) {
        vkDevice.destroyImage(attachment.image);
        vkDevice.freeMemory(attachment.memory);
        throw std::runtime_error("Failed to create attachment image view: " + std::string(e.what()));
    }
    
    attachment.format = format;
    attachment.extent = {framebuffer.getWidth(), framebuffer.getHeight()};
    attachment.samples = samples;
    attachment.usage = usage;
    attachment.aspectMask = aspectMask;
    attachment.owned = true;
}

void VulkanFramebufferWithAttachments::initColorOnly(
    VulkanDevice* device,
    uint32_t width, uint32_t height,
    vk::Format colorFormat,
    vk::RenderPass renderPass,
    vk::SampleCountFlagBits samples) {
    
    this->device = device;
    this->colorFormat = colorFormat;
    this->samples = samples;
    
    // Create framebuffer with placeholder (we'll add attachments later)
    FramebufferCreateInfo fbInfo;
    fbInfo.width = width;
    fbInfo.height = height;
    fbInfo.renderPass = renderPass;
    framebuffer.init(device->getDevice(), fbInfo);
    
    // Create color attachment
    colorAttachments.resize(1);
    createAttachment(device, colorAttachments[0], colorFormat,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor);
    
    // Recreate framebuffer with proper attachment
    framebuffer.cleanup();
    fbInfo.attachments = {colorAttachments[0].imageView};
    framebuffer.init(device->getDevice(), fbInfo);
    
    Logger::debug("Color-only framebuffer created: {}x{}", width, height);
}

void VulkanFramebufferWithAttachments::initColorDepth(
    VulkanDevice* device,
    uint32_t width, uint32_t height,
    vk::Format colorFormat,
    vk::Format depthFormat,
    vk::RenderPass renderPass,
    vk::SampleCountFlagBits samples) {
    
    this->device = device;
    this->colorFormat = colorFormat;
    this->depthFormat = depthFormat;
    this->samples = samples;
    
    // Create color attachment
    colorAttachments.resize(1);
    createAttachment(device, colorAttachments[0], colorFormat,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor);
    
    // Create depth attachment
    bool hasStencil = (depthFormat == vk::Format::eD32SfloatS8Uint || 
                       depthFormat == vk::Format::eD24UnormS8Uint);
    auto depthAspect = hasStencil ? 
        vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil :
        vk::ImageAspectFlagBits::eDepth;
    
    createAttachment(device, depthAttachment, depthFormat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
        depthAspect);
    
    // Create framebuffer
    FramebufferCreateInfo fbInfo;
    fbInfo.width = width;
    fbInfo.height = height;
    fbInfo.renderPass = renderPass;
    fbInfo.attachments = {colorAttachments[0].imageView, depthAttachment.imageView};
    framebuffer.init(device->getDevice(), fbInfo);
    
    Logger::debug("Color+Depth framebuffer created: {}x{}", width, height);
}

void VulkanFramebufferWithAttachments::initGBuffer(
    VulkanDevice* device,
    uint32_t width, uint32_t height,
    vk::Format albedoFormat,
    vk::Format normalFormat,
    vk::Format positionFormat,
    vk::Format depthFormat,
    vk::RenderPass renderPass) {
    
    this->device = device;
    this->samples = vk::SampleCountFlagBits::e1;
    
    // Create G-Buffer attachments
    colorAttachments.resize(3);
    createAttachment(device, colorAttachments[0], albedoFormat,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor);
    createAttachment(device, colorAttachments[1], normalFormat,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor);
    createAttachment(device, colorAttachments[2], positionFormat,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor);
    
    // Create depth attachment
    createAttachment(device, depthAttachment, depthFormat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::ImageAspectFlagBits::eDepth);
    
    // Create framebuffer
    FramebufferCreateInfo fbInfo;
    fbInfo.width = width;
    fbInfo.height = height;
    fbInfo.renderPass = renderPass;
    fbInfo.attachments = {
        colorAttachments[0].imageView,
        colorAttachments[1].imageView,
        colorAttachments[2].imageView,
        depthAttachment.imageView
    };
    framebuffer.init(device->getDevice(), fbInfo);
    
    Logger::debug("G-Buffer framebuffer created: {}x{}", width, height);
}

void VulkanFramebufferWithAttachments::initShadowMap(
    VulkanDevice* device,
    uint32_t size,
    vk::Format depthFormat,
    vk::RenderPass renderPass) {
    
    this->device = device;
    this->depthFormat = depthFormat;
    this->samples = vk::SampleCountFlagBits::e1;
    
    // Create depth-only attachment
    createAttachment(device, depthAttachment, depthFormat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eDepth);
    
    // Create framebuffer
    FramebufferCreateInfo fbInfo;
    fbInfo.width = size;
    fbInfo.height = size;
    fbInfo.renderPass = renderPass;
    fbInfo.attachments = {depthAttachment.imageView};
    framebuffer.init(device->getDevice(), fbInfo);
    
    Logger::debug("Shadow map framebuffer created: {}x{}", size, size);
}

void VulkanFramebufferWithAttachments::initCubeShadowMap(
    VulkanDevice* device,
    uint32_t size,
    vk::Format depthFormat,
    vk::RenderPass renderPass) {
    
    // Similar to initShadowMap but with cube map
    // For now, delegate to regular shadow map
    initShadowMap(device, size, depthFormat, renderPass);
    Logger::debug("Cube shadow map framebuffer created: {}x{}", size, size);
}

void VulkanFramebufferWithAttachments::cleanup() {
    if (!device) return;
    
    auto vkDevice = device->getDevice();
    
    framebuffer.cleanup();
    
    for (auto& attachment : colorAttachments) {
        if (attachment.owned) {
            if (attachment.imageView) vkDevice.destroyImageView(attachment.imageView);
            if (attachment.image) vkDevice.destroyImage(attachment.image);
            if (attachment.memory) vkDevice.freeMemory(attachment.memory);
        }
    }
    colorAttachments.clear();
    
    if (depthAttachment.owned) {
        if (depthAttachment.imageView) vkDevice.destroyImageView(depthAttachment.imageView);
        if (depthAttachment.image) vkDevice.destroyImage(depthAttachment.image);
        if (depthAttachment.memory) vkDevice.freeMemory(depthAttachment.memory);
    }
    
    device = nullptr;
}

void VulkanFramebufferWithAttachments::resize(uint32_t width, uint32_t height) {
    // Store old render pass
    auto renderPass = VK_NULL_HANDLE; // TODO: Store render pass
    
    cleanup();
    
    // Re-create with new dimensions
    // This requires storing creation parameters
}

const FramebufferAttachment& VulkanFramebufferWithAttachments::getColorAttachment(size_t index) const {
    if (index >= colorAttachments.size()) {
        throw std::out_of_range("Color attachment index out of range");
    }
    return colorAttachments[index];
}

const FramebufferAttachment& VulkanFramebufferWithAttachments::getDepthAttachment() const {
    return depthAttachment;
}

vk::ImageView VulkanFramebufferWithAttachments::getColorImageView(size_t index) const {
    return getColorAttachment(index).imageView;
}

vk::ImageView VulkanFramebufferWithAttachments::getDepthImageView() const {
    return depthAttachment.imageView;
}

vk::Image VulkanFramebufferWithAttachments::getColorImage(size_t index) const {
    return getColorAttachment(index).image;
}

vk::Image VulkanFramebufferWithAttachments::getDepthImage() const {
    return depthAttachment.image;
}

void VulkanFramebufferWithAttachments::transitionColorToShaderRead(vk::CommandBuffer cmd, size_t index) {
    auto& att = colorAttachments[index];
    att.transitionLayout(cmd, vk::ImageLayout::eColorAttachmentOptimal, 
                         vk::ImageLayout::eShaderReadOnlyOptimal);
}

void VulkanFramebufferWithAttachments::transitionColorToColorAttachment(vk::CommandBuffer cmd, size_t index) {
    auto& att = colorAttachments[index];
    att.transitionLayout(cmd, vk::ImageLayout::eShaderReadOnlyOptimal, 
                         vk::ImageLayout::eColorAttachmentOptimal);
}

void VulkanFramebufferWithAttachments::transitionDepthToShaderRead(vk::CommandBuffer cmd) {
    depthAttachment.transitionLayout(cmd, vk::ImageLayout::eDepthStencilAttachmentOptimal, 
                                     vk::ImageLayout::eShaderReadOnlyOptimal);
}

void VulkanFramebufferWithAttachments::transitionDepthToDepthAttachment(vk::CommandBuffer cmd) {
    depthAttachment.transitionLayout(cmd, vk::ImageLayout::eShaderReadOnlyOptimal, 
                                     vk::ImageLayout::eDepthStencilAttachmentOptimal);
}

void FramebufferAttachment::transitionLayout(vk::CommandBuffer cmd, 
                                             vk::ImageLayout oldLayout, 
                                             vk::ImageLayout newLayout) {
    vk::ImageMemoryBarrier barrier{};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    // Determine access masks and pipeline stages
    vk::PipelineStageFlags srcStage, dstStage;
    
    if (oldLayout == vk::ImageLayout::eUndefined && 
        newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    } else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && 
               newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        srcStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal && 
               newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        srcStage = vk::PipelineStageFlagBits::eFragmentShader;
        dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    } else if (oldLayout == vk::ImageLayout::eUndefined && 
               newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
        barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else {
        barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
        barrier.dstAccessMask = vk::AccessFlagBits::eNoneKHR;
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eBottomOfPipe;
    }
    
    cmd.pipelineBarrier(srcStage, dstStage, vk::DependencyFlagBits::eByRegion,
                        0, nullptr, 0, nullptr, 1, &barrier);
}

// ============== FramebufferCache ==============

FramebufferCache::FramebufferCache() = default;

FramebufferCache::~FramebufferCache() {
    cleanup();
}

void FramebufferCache::init(VulkanDevice* device) {
    this->device = device;
}

void FramebufferCache::cleanup() {
    if (!device) return;
    
    auto vkDevice = device->getDevice();
    for (auto& [key, fb] : cache) {
        if (fb) {
            vkDevice.destroyFramebuffer(fb);
        }
    }
    cache.clear();
    device = nullptr;
}

vk::Framebuffer FramebufferCache::getOrCreate(
    vk::ImageView imageView,
    vk::RenderPass renderPass,
    uint32_t width,
    uint32_t height) {
    
    CacheKey key{imageView, renderPass, width, height};
    
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }
    
    // Create new framebuffer
    vk::FramebufferCreateInfo fbInfo{};
    fbInfo.renderPass = renderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = &imageView;
    fbInfo.width = width;
    fbInfo.height = height;
    fbInfo.layers = 1;
    
    auto fb = device->getDevice().createFramebuffer(fbInfo);
    cache[key] = fb;
    
    return fb;
}

void FramebufferCache::clear() {
    if (device) {
        auto vkDevice = device->getDevice();
        for (auto& [key, fb] : cache) {
            if (fb) {
                vkDevice.destroyFramebuffer(fb);
            }
        }
    }
    cache.clear();
}

size_t FramebufferCache::CacheKeyHash::operator()(const CacheKey& key) const {
    size_t h1 = std::hash<void*>{}(key.imageView);
    size_t h2 = std::hash<void*>{}(key.renderPass);
    size_t h3 = std::hash<uint32_t>{}(key.width);
    size_t h4 = std::hash<uint32_t>{}(key.height);
    return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
}

} // namespace VoxelForge
