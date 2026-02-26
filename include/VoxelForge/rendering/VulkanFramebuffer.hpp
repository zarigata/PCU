/**
 * @file VulkanFramebuffer.hpp
 * @brief Vulkan framebuffer management
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <memory>

namespace VoxelForge {

class VulkanDevice;
class VulkanRenderPass;

// Framebuffer attachment
struct FramebufferAttachment {
    vk::Image image = VK_NULL_HANDLE;
    vk::ImageView imageView = VK_NULL_HANDLE;
    vk::DeviceMemory memory = VK_NULL_HANDLE;
    vk::Format format;
    vk::Extent2D extent;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    vk::ImageUsageFlags usage;
    vk::ImageAspectFlags aspectMask;
    bool owned = true;  // Whether we own the image (and should free it)
    
    void transitionLayout(vk::CommandBuffer cmd, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
};

// Framebuffer creation info
struct FramebufferCreateInfo {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layers = 1;
    vk::RenderPass renderPass = VK_NULL_HANDLE;
    std::vector<vk::ImageView> attachments;
    
    // Helpers for common configurations
    static FramebufferCreateInfo forSwapchain(
        uint32_t width, uint32_t height,
        vk::ImageView colorView,
        vk::RenderPass renderPass
    );
    
    static FramebufferCreateInfo forOffscreen(
        uint32_t width, uint32_t height,
        vk::ImageView colorView,
        vk::ImageView depthView,
        vk::RenderPass renderPass
    );
    
    static FramebufferCreateInfo forShadowMap(
        uint32_t size,
        vk::ImageView depthView,
        vk::RenderPass renderPass
    );
}

// Framebuffer wrapper
class VulkanFramebuffer {
public:
    VulkanFramebuffer();
    ~VulkanFramebuffer();
    
    // No copy
    VulkanFramebuffer(const VulkanFramebuffer&) = delete;
    VulkanFramebuffer& operator=(const VulkanFramebuffer&) = delete;
    
    // Move
    VulkanFramebuffer(VulkanFramebuffer&& other) noexcept;
    VulkanFramebuffer& operator=(VulkanFramebuffer&& other) noexcept;
    
    void init(vk::Device device, const FramebufferCreateInfo& createInfo);
    void cleanup();
    
    // Getters
    vk::Framebuffer get() const { return framebuffer; }
    operator vk::Framebuffer() const { return framebuffer; }
    operator bool() const { return framebuffer != VK_NULL_HANDLE; }
    
    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
    uint32_t getLayers() const { return layers; }
    vk::Extent2D getExtent() const { return {width, height}; }
    const std::vector<vk::ImageView>& getAttachments() const { return attachments; }
    
private:
    vk::Device device = VK_NULL_HANDLE;
    vk::Framebuffer framebuffer = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layers = 1;
    std::vector<vk::ImageView> attachments;
};

// Framebuffer with attachments (manages its own images)
class VulkanFramebufferWithAttachments {
public:
    VulkanFramebufferWithAttachments();
    ~VulkanFramebufferWithAttachments();
    
    // No copy
    VulkanFramebufferWithAttachments(const VulkanFramebufferWithAttachments&) = delete;
    VulkanFramebufferWithAttachments& operator=(const VulkanFramebufferWithAttachments&) = delete;
    
    // Move
    VulkanFramebufferWithAttachments(VulkanFramebufferWithAttachments&& other) noexcept;
    VulkanFramebufferWithAttachments& operator=(VulkanFramebufferWithAttachments&& other) noexcept;
    
    // Create with color attachment only
    void initColorOnly(
        VulkanDevice* device,
        uint32_t width, uint32_t height,
        vk::Format colorFormat,
        vk::RenderPass renderPass,
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1
    );
    
    // Create with color and depth
    void initColorDepth(
        VulkanDevice* device,
        uint32_t width, uint32_t height,
        vk::Format colorFormat,
        vk::Format depthFormat,
        vk::RenderPass renderPass,
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1
    );
    
    // Create for deferred rendering (G-buffer)
    void initGBuffer(
        VulkanDevice* device,
        uint32_t width, uint32_t height,
        vk::Format albedoFormat,
        vk::Format normalFormat,
        vk::Format positionFormat,
        vk::Format depthFormat,
        vk::RenderPass renderPass
    );
    
    // Create for shadow mapping
    void initShadowMap(
        VulkanDevice* device,
        uint32_t size,
        vk::Format depthFormat,
        vk::RenderPass renderPass
    );
    
    // Create for cube shadow map
    void initCubeShadowMap(
        VulkanDevice* device,
        uint32_t size,
        vk::Format depthFormat,
        vk::RenderPass renderPass
    );
    
    void cleanup();
    
    // Resize
    void resize(uint32_t width, uint32_t height);
    
    // Getters
    vk::Framebuffer getFramebuffer() const { return framebuffer.get(); }
    const VulkanFramebuffer& getFramebufferObject() const { return framebuffer; }
    
    uint32_t getWidth() const { return framebuffer.getWidth(); }
    uint32_t getHeight() const { return framebuffer.getHeight(); }
    
    const FramebufferAttachment& getColorAttachment(size_t index = 0) const;
    const FramebufferAttachment& getDepthAttachment() const;
    
    vk::ImageView getColorImageView(size_t index = 0) const;
    vk::ImageView getDepthImageView() const;
    
    vk::Image getColorImage(size_t index = 0) const;
    vk::Image getDepthImage() const;
    
    bool hasDepth() const { return depthAttachment.imageView != VK_NULL_HANDLE; }
    size_t getColorAttachmentCount() const { return colorAttachments.size(); }
    
    // Transition layouts
    void transitionColorToShaderRead(vk::CommandBuffer cmd, size_t index = 0);
    void transitionColorToColorAttachment(vk::CommandBuffer cmd, size_t index = 0);
    void transitionDepthToShaderRead(vk::CommandBuffer cmd);
    void transitionDepthToDepthAttachment(vk::CommandBuffer cmd);
    
private:
    void createAttachment(
        VulkanDevice* device,
        FramebufferAttachment& attachment,
        vk::Format format,
        vk::ImageUsageFlags usage,
        vk::ImageAspectFlags aspectMask
    );
    
    VulkanDevice* device = nullptr;
    VulkanFramebuffer framebuffer;
    
    std::vector<FramebufferAttachment> colorAttachments;
    FramebufferAttachment depthAttachment;
    
    vk::Format colorFormat;
    vk::Format depthFormat;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
};

// Framebuffer cache for swapchain framebuffers
class FramebufferCache {
public:
    FramebufferCache();
    ~FramebufferCache();
    
    void init(VulkanDevice* device);
    void cleanup();
    
    // Get or create framebuffer for swapchain image view
    vk::Framebuffer getOrCreate(
        vk::ImageView imageView,
        vk::RenderPass renderPass,
        uint32_t width,
        uint32_t height
    );
    
    // Clear cache (call on swapchain recreation)
    void clear();
    
private:
    struct CacheKey {
        vk::ImageView imageView;
        vk::RenderPass renderPass;
        uint32_t width;
        uint32_t height;
        
        bool operator==(const CacheKey& other) const {
            return imageView == other.imageView && 
                   renderPass == other.renderPass &&
                   width == other.width && 
                   height == other.height;
        }
    };
    
    struct CacheKeyHash {
        size_t operator()(const CacheKey& key) const;
    };
    
    VulkanDevice* device = nullptr;
    std::unordered_map<CacheKey, vk::Framebuffer, CacheKeyHash> cache;
};

} // namespace VoxelForge
