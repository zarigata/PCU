#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace VoxelForge {

struct Image {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;
};

class Texture {
public:
    Texture(const std::string& path);
    ~Texture();

    const Image& getImage() const { return m_image; }
    const std::string& getPath() const { return m_path; }

private:
    std::string m_path;
    Image m_image;
};

class VulkanImage {
public:
    static Image createImage(VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageTiling tiling, VkMemoryPropertyFlags properties);
    static VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageViewType viewType, VkImageUsageFlags usage);
    static void transitionImageLayout(VkDevice device, VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
    static void copyBufferToImage(VkDevice device, VkCommandBuffer commandBuffer, VkBuffer buffer, Image& image, uint32_t width, uint32_t height);
    static bool hasStencilComponent(VkFormat format);
};

} // namespace VoxelForge
