/**
 * @file VulkanDevice.hpp
 * @brief Vulkan physical and logical device management
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <VoxelForge/rendering/VulkanContext.hpp>
#include <vector>
#include <optional>
#include <set>
#include <string>

namespace VoxelForge {

class VulkanDevice {
public:
    VulkanDevice();
    ~VulkanDevice();
    
    // No copy
    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;
    
    // Initialize with instance and surface
    void init(vk::Instance instance, vk::SurfaceKHR surface);
    void cleanup();
    
    // Wait for device idle
    void waitIdle() const;
    
    // Getters
    vk::PhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    vk::Device getDevice() const { return device; }
    vk::Queue getGraphicsQueue() const { return graphicsQueue; }
    vk::Queue getPresentQueue() const { return presentQueue; }
    vk::Queue getComputeQueue() const { return computeQueue; }
    vk::Queue getTransferQueue() const { return transferQueue; }
    const QueueFamilyIndices& getQueueFamilies() const { return queueIndices; }
    const vk::PhysicalDeviceFeatures& getEnabledFeatures() const { return enabledFeatures; }
    const vk::PhysicalDeviceProperties& getProperties() const { return properties; }
    const vk::PhysicalDeviceMemoryProperties& getMemoryProperties() const { return memoryProperties; }
    
    // Swapchain support
    SwapChainSupportDetails querySwapChainSupport() const;
    
    // Memory allocation helpers
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
    vk::Format findSupportedFormat(
        const std::vector<vk::Format>& candidates,
        vk::ImageTiling tiling,
        vk::FormatFeatureFlags features
    ) const;
    vk::Format findDepthFormat() const;
    
    // Buffer/Image memory requirements
    vk::DeviceSize getMinUniformBufferOffsetAlignment() const {
        return properties.limits.minUniformBufferOffsetAlignment;
    }
    
    vk::DeviceSize getMinStorageBufferOffsetAlignment() const {
        return properties.limits.minStorageBufferOffsetAlignment;
    }
    
    // Device features
    bool supportsAnisotropy() const {
        return enabledFeatures.samplerAnisotropy;
    }
    
    float getMaxSamplerAnisotropy() const {
        return properties.limits.maxSamplerAnisotropy;
    }
    
    // Device info
    std::string getDeviceName() const {
        return properties.deviceName;
    }
    
    // Extensions
    static const std::vector<const char*>& getDeviceExtensions() {
        static const std::vector<const char*> extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
        };
        return extensions;
    }
    
private:
    void pickPhysicalDevice();
    void createLogicalDevice();
    
    bool isDeviceSuitable(vk::PhysicalDevice device);
    bool checkDeviceExtensionSupport(vk::PhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device) const;
    
    vk::Instance instance;
    vk::SurfaceKHR surface;
    
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;
    vk::Queue computeQueue;
    vk::Queue transferQueue;
    
    QueueFamilyIndices queueIndices;
    vk::PhysicalDeviceFeatures enabledFeatures;
    vk::PhysicalDeviceProperties properties;
    vk::PhysicalDeviceMemoryProperties memoryProperties;
    
    // Extensions to enable
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
    };
};

} // namespace VoxelForge
