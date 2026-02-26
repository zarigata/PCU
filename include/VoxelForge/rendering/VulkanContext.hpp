/**
 * @file VulkanContext.hpp
 * @brief Vulkan instance and device management
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>
#include <memory>

namespace VoxelForge {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;
    
    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();
    
    // No copy
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    
    void init(GLFWwindow* window);
    void cleanup();
    
    // Getters
    vk::Instance getInstance() const { return instance; }
    vk::PhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    vk::Device getDevice() const { return device; }
    vk::SurfaceKHR getSurface() const { return surface; }
    vk::Queue getGraphicsQueue() const { return graphicsQueue; }
    vk::Queue getPresentQueue() const { return presentQueue; }
    vk::Queue getComputeQueue() const { return computeQueue; }
    vk::Queue getTransferQueue() const { return transferQueue; }
    
    const QueueFamilyIndices& getQueueFamilies() const { return queueIndices; }
    
    // Swapchain support
    SwapChainSupportDetails querySwapChainSupport() const;
    
    // Memory allocation
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
    
    // Format support
    vk::Format findSupportedFormat(
        const std::vector<vk::Format>& candidates,
        vk::ImageTiling tiling,
        vk::FormatFeatureFlags features
    ) const;
    
    // Device features
    const vk::PhysicalDeviceFeatures& getEnabledFeatures() const { return enabledFeatures; }
    
    // Validation layers
    static constexpr bool enableValidationLayers = 
#ifdef NDEBUG
        false;
#else
        true;
#endif
    
private:
    void createInstance();
    void setupDebugMessenger();
    void createSurface(GLFWwindow* window);
    void pickPhysicalDevice();
    void createLogicalDevice();
    
    // Helper functions
    bool isDeviceSuitable(vk::PhysicalDevice device);
    bool checkDeviceExtensionSupport(vk::PhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device) const;
    std::vector<const char*> getRequiredExtensions();
    bool checkValidationLayerSupport();
    void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);
    
    // Vulkan handles
    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::SurfaceKHR surface;
    
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;
    vk::Queue computeQueue;
    vk::Queue transferQueue;
    
    QueueFamilyIndices queueIndices;
    vk::PhysicalDeviceFeatures enabledFeatures;
    vk::PhysicalDeviceProperties deviceProperties;
    vk::PhysicalDeviceMemoryProperties memoryProperties;
    
    // Extensions
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
    };
};

// Vulkan utility functions
namespace VulkanUtils {
    vk::ShaderModule createShaderModule(vk::Device device, const std::vector<uint8_t>& code);
    
    uint32_t findMemoryType(
        vk::PhysicalDevice physicalDevice,
        uint32_t typeFilter,
        vk::MemoryPropertyFlags properties
    );
    
    vk::Format findSupportedFormat(
        vk::PhysicalDevice physicalDevice,
        const std::vector<vk::Format>& candidates,
        vk::ImageTiling tiling,
        vk::FormatFeatureFlags features
    );
    
    bool hasStencilComponent(vk::Format format);
    
    void transitionImageLayout(
        vk::CommandBuffer cmd,
        vk::Image image,
        vk::Format format,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::PipelineStageFlags srcStage,
        vk::PipelineStageFlags dstStage
    );
    
    vk::CommandBuffer beginSingleTimeCommands(vk::Device device, vk::CommandPool pool);
    void endSingleTimeCommands(
        vk::Device device,
        vk::Queue queue,
        vk::CommandPool pool,
        vk::CommandBuffer cmd
    );
}

} // namespace VoxelForge
