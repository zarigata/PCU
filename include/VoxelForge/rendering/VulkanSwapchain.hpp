/**
 * @file VulkanSwapchain.hpp
 * @brief Vulkan swapchain management
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <memory>

namespace VoxelForge {

class VulkanDevice;

struct SwapchainConfig {
    uint32_t width = 1280;
    uint32_t height = 720;
    bool vsync = false;
    uint32_t imageCount = 3;  // Triple buffering
    vk::Format preferredFormat = vk::Format::eB8G8R8A8Srgb;
    vk::ColorSpaceKHR preferredColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
};

class VulkanSwapchain {
public:
    VulkanSwapchain();
    ~VulkanSwapchain();
    
    // No copy
    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;
    
    void init(VulkanDevice* device, const SwapchainConfig& config);
    void cleanup();
    
    // Recreate swapchain (on resize)
    void recreate(uint32_t width, uint32_t height);
    
    // Acquire next image
    vk::Result acquireNextImage(vk::Semaphore semaphore, vk::Fence fence = VK_NULL_HANDLE);
    uint32_t getCurrentImageIndex() const { return currentImageIndex; }
    
    // Present
    vk::Result present(vk::Semaphore waitSemaphore);
    
    // Getters
    vk::SwapchainKHR getSwapchain() const { return swapchain; }
    vk::Format getImageFormat() const { return imageFormat; }
    vk::ColorSpaceKHR getColorSpace() const { return colorSpace; }
    vk::Extent2D getExtent() const { return extent; }
    const std::vector<vk::Image>& getImages() const { return images; }
    const std::vector<vk::ImageView>& getImageViews() const { return imageViews; }
    uint32_t getWidth() const { return extent.width; }
    uint32_t getHeight() const { return extent.height; }
    float getAspectRatio() const { 
        return static_cast<float>(extent.width) / static_cast<float>(extent.height); 
    }
    uint32_t getImageCount() const { return static_cast<uint32_t>(images.size()); }
    
    // Check if needs recreation
    bool needsRecreation() const { return needsRecreate; }
    void markForRecreation() { needsRecreate = true; }
    
private:
    void createSwapchain();
    void createImageViews();
    void cleanupSwapchain();
    
    VulkanDevice* device = nullptr;
    vk::SwapchainKHR swapchain;
    
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> imageViews;
    
    vk::Format imageFormat;
    vk::ColorSpaceKHR colorSpace;
    vk::Extent2D extent;
    
    uint32_t currentImageIndex = 0;
    bool needsRecreate = false;
    bool vsync = false;
    
    // For recreation
    SwapchainConfig config;
    vk::SwapchainKHR oldSwapchain = VK_NULL_HANDLE;
};

} // namespace VoxelForge
