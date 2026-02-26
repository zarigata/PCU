/**
 * @file VulkanRenderPass.hpp
 * @brief Vulkan render pass management
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <memory>

namespace VoxelForge {

class VulkanDevice;

// Attachment description helper
struct AttachmentInfo {
    vk::Format format = vk::Format::eUndefined;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
    vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
    vk::ImageLayout finalLayout = vk::ImageLayout::ePresentSrcKHR;
    
    // Presets
    static AttachmentInfo colorAttachment(vk::Format format) {
        AttachmentInfo info;
        info.format = format;
        info.initialLayout = vk::ImageLayout::eUndefined;
        info.finalLayout = vk::ImageLayout::ePresentSrcKHR;
        return info;
    }
    
    static AttachmentInfo depthAttachment(vk::Format format) {
        AttachmentInfo info;
        info.format = format;
        info.loadOp = vk::AttachmentLoadOp::eClear;
        info.storeOp = vk::AttachmentStoreOp::eDontCare;
        info.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        info.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        info.initialLayout = vk::ImageLayout::eUndefined;
        info.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        return info;
    }
    
    static AttachmentInfo depthAttachmentReadOnly(vk::Format format) {
        AttachmentInfo info;
        info.format = format;
        info.loadOp = vk::AttachmentLoadOp::eLoad;
        info.storeOp = vk::AttachmentStoreOp::eDontCare;
        info.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        info.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        info.initialLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
        info.finalLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
        return info;
    }
};

// Subpass description helper
struct SubpassInfo {
    std::vector<uint32_t> colorAttachments;
    std::vector<uint32_t> inputAttachments;
    std::vector<uint32_t> preserveAttachments;
    std::optional<uint32_t> depthStencilAttachment;
    std::vector<uint32_t> resolveAttachments;
    vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics;
    
    vk::SubpassDescription toDescription(
        const std::vector<vk::AttachmentReference>& colorRefs,
        const std::vector<vk::AttachmentReference>& inputRefs,
        const vk::AttachmentReference* depthRef,
        const std::vector<vk::AttachmentReference>& resolveRefs
    ) const;
};

// Subpass dependency helper
struct SubpassDependencyInfo {
    uint32_t srcSubpass = VK_SUBPASS_EXTERNAL;
    uint32_t dstSubpass = 0;
    vk::PipelineStageFlags srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::PipelineStageFlags dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::AccessFlags srcAccessMask = vk::AccessFlagBits::eNoneKHR;
    vk::AccessFlags dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    vk::DependencyFlags dependencyFlags = vk::DependencyFlagBits::eByRegion;
    
    // Common presets
    static SubpassDependencyInfo externalToColor() {
        SubpassDependencyInfo dep;
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dep.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dep.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
        dep.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        return dep;
    }
    
    static SubpassDependencyInfo colorToPresent() {
        SubpassDependencyInfo dep;
        dep.srcSubpass = 0;
        dep.dstSubpass = VK_SUBPASS_EXTERNAL;
        dep.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dep.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        dep.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        dep.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
        return dep;
    }
    
    static SubpassDependencyInfo depthPrePass() {
        SubpassDependencyInfo dep;
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | 
                          vk::PipelineStageFlagBits::eLateFragmentTests;
        dep.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | 
                          vk::PipelineStageFlagBits::eLateFragmentTests;
        dep.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
        dep.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        return dep;
    }
};

// Render pass builder for flexible creation
class VulkanRenderPassBuilder {
public:
    VulkanRenderPassBuilder();
    
    VulkanRenderPassBuilder& addAttachment(const AttachmentInfo& attachment);
    VulkanRenderPassBuilder& addSubpass(const SubpassInfo& subpass);
    VulkanRenderPassBuilder& addDependency(const SubpassDependencyInfo& dependency);
    
    // Common presets
    VulkanRenderPassBuilder& forForwardRendering(vk::Format colorFormat, vk::Format depthFormat);
    VulkanRenderPassBuilder& forDeferredRendering(vk::Format albedoFormat, vk::Format normalFormat,
                                                  vk::Format positionFormat, vk::Format depthFormat);
    VulkanRenderPassBuilder& forShadowPass(vk::Format depthFormat);
    VulkanRenderPassBuilder& forPostProcess(vk::Format colorFormat);
    VulkanRenderPassBuilder& forUI(vk::Format colorFormat);
    
    vk::RenderPass build(vk::Device device);
    
private:
    std::vector<AttachmentInfo> attachments;
    std::vector<SubpassInfo> subpasses;
    std::vector<SubpassDependencyInfo> dependencies;
};

// Render pass wrapper
class VulkanRenderPass {
public:
    VulkanRenderPass();
    ~VulkanRenderPass();
    
    // No copy
    VulkanRenderPass(const VulkanRenderPass&) = delete;
    VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;
    
    // Move
    VulkanRenderPass(VulkanRenderPass&& other) noexcept;
    VulkanRenderPass& operator=(VulkanRenderPass&& other) noexcept;
    
    // Create from builder
    void init(vk::Device device, vk::RenderPass renderPass);
    
    // Create from description
    void init(vk::Device device, const VulkanRenderPassBuilder& builder);
    
    void cleanup();
    
    // Getters
    vk::RenderPass get() const { return renderPass; }
    operator vk::RenderPass() const { return renderPass; }
    operator bool() const { return renderPass != VK_NULL_HANDLE; }
    
    // Begin render pass helper
    void begin(vk::CommandBuffer cmd, vk::Framebuffer framebuffer, 
               vk::Rect2D renderArea, const std::vector<vk::ClearValue>& clearValues,
               vk::SubpassContents contents = vk::SubpassContents::eInline);
    
    // Next subpass
    void nextSubpass(vk::CommandBuffer cmd, vk::SubpassContents contents = vk::SubpassContents::eInline);
    
    // End render pass
    void end(vk::CommandBuffer cmd);
    
    // Get attachment count
    uint32_t getAttachmentCount() const { return attachmentCount; }
    
private:
    vk::Device device = VK_NULL_HANDLE;
    vk::RenderPass renderPass = VK_NULL_HANDLE;
    uint32_t attachmentCount = 0;
};

// Dynamic rendering support (Vulkan 1.3+)
class VulkanDynamicRendering {
public:
    struct Attachment {
        vk::Format format;
        vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
        vk::ImageLayout finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
    };
    
    VulkanDynamicRendering();
    
    VulkanDynamicRendering& addColorAttachment(vk::Format format);
    VulkanDynamicRendering& setDepthAttachment(vk::Format format, bool stencil = false);
    void begin(vk::CommandBuffer cmd, vk::Rect2D renderArea, 
               const std::vector<vk::ImageView>& colorViews,
               vk::ImageView depthView = VK_NULL_HANDLE,
               const std::vector<vk::ClearValue>& clearValues = {});
    void end(vk::CommandBuffer cmd);
    
    vk::RenderingInfo getRenderingInfo() const { return renderingInfo; }
    
private:
    std::vector<vk::RenderingAttachmentInfo> colorAttachments;
    vk::RenderingAttachmentInfo depthAttachment;
    bool hasDepth = false;
    vk::RenderingInfo renderingInfo;
};

} // namespace VoxelForge
