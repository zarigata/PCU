/**
 * @file VulkanContext.cpp
 * @brief Vulkan instance and device management implementation
 */

#include <VoxelForge/rendering/VulkanContext.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <GLFW/glfw3.h>
#include <set>
#include <cstring>
#include <fstream>

namespace VoxelForge {

// ============================================================================
// VulkanContext Implementation
// ============================================================================

VulkanContext::VulkanContext() {
    LOG_INFO("VulkanContext created");
}

VulkanContext::~VulkanContext() {
    cleanup();
    LOG_INFO("VulkanContext destroyed");
}

void VulkanContext::init(GLFWwindow* window) {
    LOG_INFO("Initializing Vulkan context...");
    
    createInstance();
    setupDebugMessenger();
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();
    
    LOG_INFO("Vulkan context initialized successfully");
    LOG_INFO("  Device: {}", deviceProperties.deviceName);
    LOG_INFO("  API Version: {}.{}.{}", 
             VK_API_VERSION_MAJOR(deviceProperties.apiVersion),
             VK_API_VERSION_MINOR(deviceProperties.apiVersion),
             VK_API_VERSION_PATCH(deviceProperties.apiVersion));
}

void VulkanContext::cleanup() {
    if (device) {
        device.waitIdle();
    }
    
    if (enableValidationLayers && debugMessenger) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)instance.getProcAddr(
            "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(static_cast<VkInstance>(instance), 
                 static_cast<VkDebugUtilsMessengerEXT>(debugMessenger), nullptr);
        }
    }
    
    if (surface) {
        instance.destroySurfaceKHR(surface);
    }
    
    if (device) {
        device.destroy();
    }
    
    if (instance) {
        instance.destroy();
    }
    
    LOG_INFO("Vulkan context cleaned up");
}

void VulkanContext::createInstance() {
    // Check validation layer support
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested but not available!");
    }
    
    // Application info
    vk::ApplicationInfo appInfo{};
    appInfo.sType = vk::StructureType::eApplicationInfo;
    appInfo.pApplicationName = "VoxelForge";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "VoxelForge Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;
    
    // Instance create info
    vk::InstanceCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eInstanceCreateInfo;
    createInfo.pApplicationInfo = &appInfo;
    
    // Get required extensions
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    
    // Debug messenger for instance creation
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }
    
    // Create instance
    try {
        instance = vk::createInstance(createInfo);
        LOG_INFO("Vulkan instance created");
    } catch (const vk::SystemError& e) {
        throw std::runtime_error(std::string("Failed to create Vulkan instance: ") + e.what());
    }
}

void VulkanContext::setupDebugMessenger() {
    if (!enableValidationLayers) return;
    
    vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);
    
    // Load the debug messenger function
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)instance.getProcAddr(
        "vkCreateDebugUtilsMessengerEXT");
    
    if (func != nullptr) {
        VkDebugUtilsMessengerEXT messenger;
        VkResult result = func(static_cast<VkInstance>(instance),
                               reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo),
                               nullptr, &messenger);
        if (result == VK_SUCCESS) {
            debugMessenger = vk::DebugUtilsMessengerEXT(messenger);
            LOG_INFO("Vulkan debug messenger set up");
        } else {
            LOG_ERROR("Failed to set up debug messenger");
        }
    } else {
        LOG_ERROR("Failed to load vkCreateDebugUtilsMessengerEXT");
    }
}

void VulkanContext::populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo.sType = vk::StructureType::eDebugUtilsMessengerCreateInfoEXT;
    createInfo.messageSeverity = 
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    createInfo.messageType = 
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    createInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                     VkDebugUtilsMessageTypeFlagsEXT messageType,
                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                     void* pUserData) -> VKAPI_ATTR VkBool32 {
        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                LOG_TRACE("Vulkan Validation: {}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                LOG_INFO("Vulkan Validation: {}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                LOG_WARN("Vulkan Validation: {}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                LOG_ERROR("Vulkan Validation: {}", pCallbackData->pMessage);
                break;
            default:
                LOG_DEBUG("Vulkan Validation: {}", pCallbackData->pMessage);
        }
        return VK_FALSE;
    };
    createInfo.pUserData = nullptr;
}

void VulkanContext::createSurface(GLFWwindow* window) {
    VkSurfaceKHR rawSurface;
    VkResult result = glfwCreateWindowSurface(
        static_cast<VkInstance>(instance), window, nullptr, &rawSurface);
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
    
    surface = vk::SurfaceKHR(rawSurface);
    LOG_INFO("Vulkan surface created");
}

void VulkanContext::pickPhysicalDevice() {
    auto devices = instance.enumeratePhysicalDevices();
    
    if (devices.empty()) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
    
    LOG_INFO("Found {} physical device(s)", devices.size());
    
    // Rate each device and select the best one
    int bestScore = 0;
    vk::PhysicalDevice bestDevice;
    
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            int score = 0;
            
            auto properties = device.getProperties();
            auto features = device.getFeatures();
            
            // Prefer discrete GPUs
            if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
                score += 1000;
            }
            
            // Maximum texture size affects quality
            score += properties.limits.maxImageDimension2D;
            
            // Prefer devices with geometry and tessellation shaders
            if (features.geometryShader) score += 100;
            if (features.tessellationShader) score += 100;
            
            LOG_DEBUG("  Device '{}' scored {}", properties.deviceName, score);
            
            if (score > bestScore) {
                bestScore = score;
                bestDevice = device;
            }
        }
    }
    
    if (!bestDevice) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
    
    physicalDevice = bestDevice;
    deviceProperties = physicalDevice.getProperties();
    memoryProperties = physicalDevice.getMemoryProperties();
    
    LOG_INFO("Selected physical device: {}", deviceProperties.deviceName);
}

bool VulkanContext::isDeviceSuitable(vk::PhysicalDevice device) {
    auto indices = findQueueFamilies(device);
    
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    
    if (extensionsSupported) {
        auto swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && 
                           !swapChainSupport.presentModes.empty();
    }
    
    auto features = device.getFeatures();
    
    return indices.isComplete() && 
           extensionsSupported && 
           swapChainAdequate && 
           features.samplerAnisotropy &&
           features.fillModeNonSolid; // For wireframe rendering
}

bool VulkanContext::checkDeviceExtensionSupport(vk::PhysicalDevice device) {
    auto availableExtensions = device.enumerateDeviceExtensionProperties();
    
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    
    return requiredExtensions.empty();
}

QueueFamilyIndices VulkanContext::findQueueFamilies(vk::PhysicalDevice device) {
    QueueFamilyIndices indices;
    
    auto queueFamilies = device.getQueueFamilyProperties();
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        // Graphics queue
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = i;
        }
        
        // Compute queue (prefer dedicated)
        if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) {
            if (!indices.computeFamily.has_value() || 
                !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
                indices.computeFamily = i;
            }
        }
        
        // Transfer queue (prefer dedicated)
        if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) {
            if (!indices.transferFamily.has_value() ||
                (!(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) &&
                 !(queueFamily.queueFlags & vk::QueueFlagBits::eCompute))) {
                indices.transferFamily = i;
            }
        }
        
        // Present queue
        if (device.getSurfaceSupportKHR(i, surface)) {
            indices.presentFamily = i;
        }
        
        i++;
    }
    
    // Fallback: use graphics queue for compute and transfer if not found
    if (!indices.computeFamily.has_value() && indices.graphicsFamily.has_value()) {
        indices.computeFamily = indices.graphicsFamily;
    }
    if (!indices.transferFamily.has_value() && indices.graphicsFamily.has_value()) {
        indices.transferFamily = indices.graphicsFamily;
    }
    
    return indices;
}

SwapChainSupportDetails VulkanContext::querySwapChainSupport(vk::PhysicalDevice device) const {
    SwapChainSupportDetails details;
    
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);
    
    return details;
}

SwapChainSupportDetails VulkanContext::querySwapChainSupport() const {
    return querySwapChainSupport(physicalDevice);
}

void VulkanContext::createLogicalDevice() {
    queueIndices = findQueueFamilies(physicalDevice);
    
    // Create unique queue families
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        queueIndices.graphicsFamily.value(),
        queueIndices.presentFamily.value()
    };
    
    if (queueIndices.computeFamily.has_value()) {
        uniqueQueueFamilies.insert(queueIndices.computeFamily.value());
    }
    if (queueIndices.transferFamily.has_value()) {
        uniqueQueueFamilies.insert(queueIndices.transferFamily.value());
    }
    
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = vk::StructureType::eDeviceQueueCreateInfo;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    // Enabled features
    vk::PhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;
    deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
    deviceFeatures.vertexPipelineStoresAndAtomics = VK_TRUE;
    deviceFeatures.shaderClipDistance = VK_TRUE;
    deviceFeatures.shaderCullDistance = VK_TRUE;
    deviceFeatures.geometryShader = VK_TRUE;
    deviceFeatures.multiDrawIndirect = VK_TRUE;
    deviceFeatures.drawIndirectFirstInstance = VK_TRUE;
    
    // Vulkan 1.2+ features
    vk::PhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.sType = vk::StructureType::ePhysicalDeviceVulkan12Features;
    vulkan12Features.descriptorIndexing = VK_TRUE;
    vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    vulkan12Features.runtimeDescriptorArray = VK_TRUE;
    vulkan12Features.scalarBlockLayout = VK_TRUE;
    vulkan12Features.bufferDeviceAddress = VK_TRUE;
    
    // Vulkan 1.3+ features
    vk::PhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.sType = vk::StructureType::ePhysicalDeviceVulkan13Features;
    vulkan13Features.dynamicRendering = VK_TRUE;
    vulkan13Features.synchronization2 = VK_TRUE;
    vulkan13Features.maintenance4 = VK_TRUE;
    
    vulkan12Features.pNext = &vulkan13Features;
    
    // Device create info
    vk::DeviceCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eDeviceCreateInfo;
    createInfo.pNext = &vulkan12Features;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    
    try {
        device = physicalDevice.createDevice(createInfo);
        LOG_INFO("Logical device created");
    } catch (const vk::SystemError& e) {
        throw std::runtime_error(std::string("Failed to create logical device: ") + e.what());
    }
    
    // Get queues
    graphicsQueue = device.getQueue(queueIndices.graphicsFamily.value(), 0);
    presentQueue = device.getQueue(queueIndices.presentFamily.value(), 0);
    
    if (queueIndices.computeFamily.has_value()) {
        computeQueue = device.getQueue(queueIndices.computeFamily.value(), 0);
    } else {
        computeQueue = graphicsQueue;
    }
    
    if (queueIndices.transferFamily.has_value()) {
        transferQueue = device.getQueue(queueIndices.transferFamily.value(), 0);
    } else {
        transferQueue = graphicsQueue;
    }
    
    enabledFeatures = deviceFeatures;
    LOG_INFO("Queues retrieved");
}

std::vector<const char*> VulkanContext::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    
    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    // Additional useful extensions
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    
    return extensions;
}

bool VulkanContext::checkValidationLayerSupport() {
    auto availableLayers = vk::enumerateInstanceLayerProperties();
    
    for (const char* layerName : validationLayers) {
        bool layerFound = false;
        
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        
        if (!layerFound) {
            return false;
        }
    }
    
    return true;
}

uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const {
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}

vk::Format VulkanContext::findSupportedFormat(
    const std::vector<vk::Format>& candidates,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features) const {
    
    for (vk::Format format : candidates) {
        vk::FormatProperties props = physicalDevice.getFormatProperties(format);
        
        if (tiling == vk::ImageTiling::eLinear && 
            (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && 
                   (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    
    throw std::runtime_error("Failed to find supported format!");
}

// ============================================================================
// VulkanUtils Implementation
// ============================================================================

namespace VulkanUtils {

vk::ShaderModule createShaderModule(vk::Device device, const std::vector<uint8_t>& code) {
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    try {
        return device.createShaderModule(createInfo);
    } catch (const vk::SystemError& e) {
        LOG_ERROR("Failed to create shader module: {}", e.what());
        throw;
    }
}

vk::ShaderModule createShaderModuleFromFile(vk::Device device, const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filename);
    }
    
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<uint8_t> buffer(fileSize);
    
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();
    
    return createShaderModule(device, buffer);
}

uint32_t findMemoryType(
    vk::PhysicalDevice physicalDevice,
    uint32_t typeFilter,
    vk::MemoryPropertyFlags properties) {
    
    auto memProperties = physicalDevice.getMemoryProperties();
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}

vk::Format findSupportedFormat(
    vk::PhysicalDevice physicalDevice,
    const std::vector<vk::Format>& candidates,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features) {
    
    for (vk::Format format : candidates) {
        vk::FormatProperties props = physicalDevice.getFormatProperties(format);
        
        if (tiling == vk::ImageTiling::eLinear && 
            (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && 
                   (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    
    throw std::runtime_error("Failed to find supported format!");
}

bool hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint || 
           format == vk::Format::eD24UnormS8Uint;
}

void transitionImageLayout(
    vk::CommandBuffer cmd,
    vk::Image image,
    vk::Format format,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::PipelineStageFlags srcStage,
    vk::PipelineStageFlags dstStage) {
    
    vk::ImageMemoryBarrier barrier{};
    barrier.sType = vk::StructureType::eImageMemoryBarrier;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    // Handle depth formats
    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    }
    
    // Source access masks
    vk::AccessFlags srcAccessMask;
    if (oldLayout == vk::ImageLayout::ePreinitialized) {
        srcAccessMask = vk::AccessFlagBits::eHostWrite;
    } else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    } else if (oldLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    } else if (oldLayout == vk::ImageLayout::eTransferSrcOptimal) {
        srcAccessMask = vk::AccessFlagBits::eTransferRead;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal) {
        srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    } else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        srcAccessMask = vk::AccessFlagBits::eShaderRead;
    }
    
    // Destination access masks
    vk::AccessFlags dstAccessMask;
    if (newLayout == vk::ImageLayout::eTransferDstOptimal) {
        dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    } else if (newLayout == vk::ImageLayout::eTransferSrcOptimal) {
        dstAccessMask = vk::AccessFlagBits::eTransferRead;
    } else if (newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    } else if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    } else if (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        dstAccessMask = vk::AccessFlagBits::eShaderRead;
        if (oldLayout == vk::ImageLayout::eTransferDstOptimal) {
            srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        }
    }
    
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    
    cmd.pipelineBarrier(
        srcStage, dstStage,
        vk::DependencyFlags{},
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

vk::CommandBuffer beginSingleTimeCommands(vk::Device device, vk::CommandPool pool) {
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = pool;
    allocInfo.commandBufferCount = 1;
    
    vk::CommandBuffer commandBuffer = device.allocateCommandBuffers(allocInfo)[0];
    
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    
    commandBuffer.begin(beginInfo);
    return commandBuffer;
}

void endSingleTimeCommands(
    vk::Device device,
    vk::Queue queue,
    vk::CommandPool pool,
    vk::CommandBuffer cmd) {
    
    cmd.end();
    
    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    
    queue.submit(1, &submitInfo, vk::Fence{});
    queue.waitIdle();
    
    device.freeCommandBuffers(pool, 1, &cmd);
}

} // namespace VulkanUtils

} // namespace VoxelForge
