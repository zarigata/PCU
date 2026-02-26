/**
 * @file VulkanDevice.cpp
 * @brief Vulkan physical and logical device management implementation
 */

#include "VulkanDevice.hpp"
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>

namespace VoxelForge {

VulkanDevice::VulkanDevice() = default;

VulkanDevice::~VulkanDevice() {
    cleanup();
}

void VulkanDevice::init(vk::Instance instance, vk::SurfaceKHR surface) {
    this->instance = instance;
    this->surface = surface;
    
    pickPhysicalDevice();
    createLogicalDevice();
    
    Logger::info("VulkanDevice initialized: {}", properties.deviceName);
}

void VulkanDevice::cleanup() {
    if (device) {
        device.waitIdle();
        device.destroy();
        device = nullptr;
    }
    physicalDevice = nullptr;
}

void VulkanDevice::waitIdle() const {
    if (device) {
        device.waitIdle();
    }
}

void VulkanDevice::pickPhysicalDevice() {
    auto devices = instance.enumeratePhysicalDevices();
    
    if (devices.empty()) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }
    
    // Rate devices and pick the best one
    int maxScore = -1;
    vk::PhysicalDevice bestDevice;
    
    for (const auto& device : devices) {
        if (!isDeviceSuitable(device)) {
            continue;
        }
        
        auto props = device.getProperties();
        int score = 0;
        
        // Prefer discrete GPUs
        if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            score += 1000;
        }
        
        // Maximum texture size affects quality
        score += props.limits.maxImageDimension2D;
        
        // Prefer devices with all queue families
        auto indices = findQueueFamilies(device);
        if (indices.graphicsFamily && indices.presentFamily && 
            indices.computeFamily && indices.transferFamily) {
            score += 500;
        }
        
        if (score > maxScore) {
            maxScore = score;
            bestDevice = device;
        }
    }
    
    if (!bestDevice) {
        throw std::runtime_error("Failed to find a suitable GPU");
    }
    
    physicalDevice = bestDevice;
    properties = physicalDevice.getProperties();
    memoryProperties = physicalDevice.getMemoryProperties();
    
    Logger::info("Selected GPU: {}", properties.deviceName);
}

void VulkanDevice::createLogicalDevice() {
    queueIndices = findQueueFamilies(physicalDevice);
    
    // Create unique queue create infos
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;
    
    for (uint32_t queueFamily : queueIndices.uniqueFamilies()) {
        vk::DeviceQueueCreateInfo queueInfo{};
        queueInfo.queueFamilyIndex = queueFamily;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueInfo);
    }
    
    // Enable features
    vk::PhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;
    features.fillModeNonSolid = VK_TRUE;
    features.wideLines = VK_TRUE;
    features.geometryShader = VK_TRUE;
    features.shaderClipDistance = VK_TRUE;
    features.shaderCullDistance = VK_TRUE;
    features.textureCompressionBC = VK_TRUE;
    
    // Get supported features and only enable what's available
    auto supportedFeatures = physicalDevice.getFeatures();
    if (supportedFeatures.samplerAnisotropy) {
        features.samplerAnisotropy = VK_TRUE;
    }
    if (supportedFeatures.fillModeNonSolid) {
        features.fillModeNonSolid = VK_TRUE;
    }
    if (supportedFeatures.wideLines) {
        features.wideLines = VK_TRUE;
    }
    if (supportedFeatures.geometryShader) {
        features.geometryShader = VK_TRUE;
    }
    if (supportedFeatures.textureCompressionBC) {
        features.textureCompressionBC = VK_TRUE;
    }
    
    // Device create info
    vk::DeviceCreateInfo createInfo{};
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &features;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    // Enable Vulkan 1.3 features if available
    vk::PhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.dynamicRendering = VK_TRUE;
    vulkan13Features.synchronization2 = VK_TRUE;
    vulkan13Features.maintenance4 = VK_TRUE;
    
    if (properties.apiVersion >= VK_API_VERSION_1_3) {
        createInfo.pNext = &vulkan13Features;
    }
    
    // Descriptor indexing features
    vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndexing{};
    descriptorIndexing.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptorIndexing.descriptorBindingPartiallyBound = VK_TRUE;
    descriptorIndexing.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptorIndexing.runtimeDescriptorArray = VK_TRUE;
    
    if (properties.apiVersion >= VK_API_VERSION_1_2) {
        descriptorIndexing.pNext = createInfo.pNext;
        createInfo.pNext = &descriptorIndexing;
    }
    
    try {
        device = physicalDevice.createDevice(createInfo);
    } catch (const vk::SystemError& e) {
        throw std::runtime_error("Failed to create logical device: " + std::string(e.what()));
    }
    
    // Get queues
    graphicsQueue = device.getQueue(queueIndices.graphicsFamily.value(), 0);
    presentQueue = device.getQueue(queueIndices.presentFamily.value(), 0);
    
    if (queueIndices.computeFamily) {
        computeQueue = device.getQueue(queueIndices.computeFamily.value(), 0);
    } else {
        computeQueue = graphicsQueue;
    }
    
    if (queueIndices.transferFamily) {
        transferQueue = device.getQueue(queueIndices.transferFamily.value(), 0);
    } else {
        transferQueue = graphicsQueue;
    }
    
    enabledFeatures = features;
    
    Logger::debug("Logical device created with {} queue families", 
                  queueIndices.uniqueFamilies().size());
}

bool VulkanDevice::isDeviceSuitable(vk::PhysicalDevice device) {
    auto indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    
    if (extensionsSupported) {
        auto swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && 
                           !swapChainSupport.presentModes.empty();
    }
    
    auto supportedFeatures = device.getFeatures();
    
    return indices.isComplete() && extensionsSupported && 
           swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool VulkanDevice::checkDeviceExtensionSupport(vk::PhysicalDevice device) {
    auto availableExtensions = device.enumerateDeviceExtensionProperties();
    
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    
    return requiredExtensions.empty();
}

QueueFamilyIndices VulkanDevice::findQueueFamilies(vk::PhysicalDevice device) {
    QueueFamilyIndices indices;
    auto queueFamilies = device.getQueueFamilyProperties();
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        // Graphics queue
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = i;
        }
        
        // Present queue
        if (device.getSurfaceSupportKHR(i, surface)) {
            indices.presentFamily = i;
        }
        
        // Compute queue (prefer dedicated)
        if ((queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
            !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
            indices.computeFamily = i;
        } else if (!indices.computeFamily && (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)) {
            indices.computeFamily = i;
        }
        
        // Transfer queue (prefer dedicated)
        if ((queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) &&
            !(queueFamily.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute))) {
            indices.transferFamily = i;
        } else if (!indices.transferFamily && (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)) {
            indices.transferFamily = i;
        }
        
        if (indices.isComplete() && indices.computeFamily && indices.transferFamily) {
            break;
        }
        
        i++;
    }
    
    return indices;
}

SwapChainSupportDetails VulkanDevice::querySwapChainSupport() const {
    return querySwapChainSupport(physicalDevice);
}

SwapChainSupportDetails VulkanDevice::querySwapChainSupport(vk::PhysicalDevice device) const {
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);
    return details;
}

uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const {
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type");
}

vk::Format VulkanDevice::findSupportedFormat(
    const std::vector<vk::Format>& candidates,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features
) const {
    for (vk::Format format : candidates) {
        auto props = physicalDevice.getFormatProperties(format);
        
        if (tiling == vk::ImageTiling::eLinear && 
            (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && 
                   (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    
    throw std::runtime_error("Failed to find supported format");
}

vk::Format VulkanDevice::findDepthFormat() const {
    return findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

} // namespace VoxelForge
