/**
 * @file VulkanSwapchain.cpp
 * @brief Vulkan swapchain management implementation
 */

#include "VulkanSwapchain.hpp"
#include <VoxelForge/rendering/VulkanDevice.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>

namespace VoxelForge {

VulkanSwapchain::VulkanSwapchain() = default;

VulkanSwapchain::~VulkanSwapchain() {
    cleanup();
}

void VulkanSwapchain::init(VulkanDevice* device, const SwapchainConfig& config) {
    this->device = device;
    this->config = config;
    this->vsync = config.vsync;
    
    createSwapchain();
    createImageViews();
    
    Logger::info("VulkanSwapchain initialized: {}x{}, {} images, vsync={}",
                 extent.width, extent.height, images.size(), vsync);
}

void VulkanSwapchain::cleanup() {
    cleanupSwapchain();
}

void VulkanSwapchain::cleanupSwapchain() {
    if (!device) return;
    
    auto vkDevice = device->getDevice();
    
    for (auto& imageView : imageViews) {
        if (imageView) {
            vkDevice.destroyImageView(imageView);
            imageView = nullptr;
        }
    }
    imageViews.clear();
    
    if (swapchain) {
        vkDevice.destroySwapchainKHR(swapchain);
        swapchain = nullptr;
    }
    
    images.clear();
}

void VulkanSwapchain::recreate(uint32_t width, uint32_t height) {
    config.width = width;
    config.height = height;
    
    device->waitIdle();
    
    oldSwapchain = swapchain;
    cleanupSwapchain();
    
    createSwapchain();
    createImageViews();
    
    oldSwapchain = VK_NULL_HANDLE;
    needsRecreate = false;
    
    Logger::debug("Swapchain recreated: {}x{}", extent.width, extent.height);
}

void VulkanSwapchain::createSwapchain() {
    auto support = device->querySwapChainSupport();
    
    auto surfaceFormat = support.chooseSwapSurfaceFormat();
    auto presentMode = support.chooseSwapPresentMode(vsync);
    extent = support.chooseSwapExtent(config.width, config.height);
    
    // Image count
    uint32_t imageCount = std::max(config.imageCount, support.capabilities.minImageCount);
    if (support.capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, support.capabilities.maxImageCount);
    }
    
    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface = device->getDevice() ? 
        device->getPhysicalDevice().getSurfaceSupportKHR(0, device->getPhysicalDevice().createDevice({}).destroy()) : 
        static_cast<vk::SurfaceKHR>(nullptr);
    
    // Get surface from device
    // This is a workaround - in a real implementation, we'd pass the surface
    createInfo.surface = VK_NULL_HANDLE; // TODO: Get surface properly
    
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment |
                           vk::ImageUsageFlagBits::eTransferDst;  // For screenshots
    
    auto indices = device->getQueueFamilies();
    uint32_t queueFamilyIndices[] = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };
    
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    
    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapchain;
    
    try {
        swapchain = device->getDevice().createSwapchainKHR(createInfo);
    } catch (const vk::SystemError& e) {
        throw std::runtime_error("Failed to create swapchain: " + std::string(e.what()));
    }
    
    images = device->getDevice().getSwapchainImagesKHR(swapchain);
    imageFormat = surfaceFormat.format;
    colorSpace = surfaceFormat.colorSpace;
}

void VulkanSwapchain::createImageViews() {
    imageViews.resize(images.size());
    
    for (size_t i = 0; i < images.size(); i++) {
        vk::ImageViewCreateInfo createInfo{};
        createInfo.image = images[i];
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = imageFormat;
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;
        createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        
        try {
            imageViews[i] = device->getDevice().createImageView(createInfo);
        } catch (const vk::SystemError& e) {
            throw std::runtime_error("Failed to create image view: " + std::string(e.what()));
        }
    }
}

vk::Result VulkanSwapchain::acquireNextImage(vk::Semaphore semaphore, vk::Fence fence) {
    auto result = device->getDevice().acquireNextImageKHR(
        swapchain,
        UINT64_MAX,
        semaphore,
        fence,
        &currentImageIndex
    );
    
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        needsRecreate = true;
    }
    
    return result;
}

vk::Result VulkanSwapchain::present(vk::Semaphore waitSemaphore) {
    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImageIndex;
    
    try {
        auto result = device->getPresentQueue().presentKHR(presentInfo);
        if (result == vk::Result::eSuboptimalKHR) {
            needsRecreate = true;
        }
        return result;
    } catch (const vk::OutOfDateKHRError&) {
        needsRecreate = true;
        return vk::Result::eErrorOutOfDateKHR;
    }
}

} // namespace VoxelForge
