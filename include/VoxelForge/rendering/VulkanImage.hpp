/**
 * @file VulkanImage.hpp
 * @brief Vulkan image management
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>

namespace VoxelForge {

class VulkanContext;

/**
 * VulkanImage utility class for image operations
 */
class VulkanImage {
public:
    /**
     * Create a Vulkan image with memory allocation
     */
    static vk::UniqueImage createImage(
        vk::Device device,
        uint32_t width,
        uint32_t height,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vk::DeviceMemory& outMemory,
        uint32_t mipLevels = 1,
        vk::SampleCountFlagBits numSamples = vk::SampleCountFlagBits::e1
    );

    /**
     * Create an image view
     */
    static vk::UniqueImageView createImageView(
        vk::Device device,
        vk::Image image,
        vk::Format format,
        vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor,
        uint32_t mipLevels = 1
    );

    /**
     * Transition image layout
     */
    static void transitionImageLayout(
        vk::Device device,
        vk::Queue queue,
        vk::CommandPool pool,
        vk::Image image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor,
        uint32_t mipLevels = 1
    );

    /**
     * Copy buffer to image
     */
    static void copyBufferToImage(
        vk::Device device,
        vk::Queue queue,
        vk::CommandPool pool,
        vk::Buffer buffer,
        vk::Image image,
        uint32_t width,
        uint32_t height
    );

    /**
     * Check if format has stencil component
     */
    static bool hasStencilComponent(vk::Format format);
};

} // namespace VoxelForge
