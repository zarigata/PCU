/**
 * @file VulkanRenderPass.cpp
 * @brief Vulkan render pass management implementation
 */

#include "VulkanRenderPass.hpp"
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

// ============== SubpassInfo ==============

vk::SubpassDescription SubpassInfo::toDescription(
    const std::vector<vk::AttachmentReference>& colorRefs,
    const std::vector<vk::AttachmentReference>& inputRefs,
    const vk::AttachmentReference* depthRef,
    const std::vector<vk::AttachmentReference>& resolveRefs
) const {
    vk::SubpassDescription desc{};
    desc.pipelineBindPoint = bindPoint;
    desc.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
    desc.pColorAttachments = colorRefs.empty() ? nullptr : colorRefs.data();
    desc.inputAttachmentCount = static_cast<uint32_t>(inputRefs.size());
    desc.pInputAttachments = inputRefs.empty() ? nullptr : inputRefs.data();
    desc.pDepthStencilAttachment = depthRef;
    desc.preserveAttachmentCount = static_cast<uint32_t>(preserveAttachments.size());
    desc.pPreserveAttachments = preserveAttachments.empty() ? nullptr : preserveAttachments.data();
    desc.pResolveAttachments = resolveRefs.empty() ? nullptr : resolveRefs.data();
    return desc;
}

// ============== VulkanRenderPassBuilder ==============

VulkanRenderPassBuilder::VulkanRenderPassBuilder() = default;

VulkanRenderPassBuilder& VulkanRenderPassBuilder::addAttachment(const AttachmentInfo& attachment) {
    attachments.push_back(attachment);
    return *this;
}

VulkanRenderPassBuilder& VulkanRenderPassBuilder::addSubpass(const SubpassInfo& subpass) {
    subpasses.push_back(subpass);
    return *this;
}

VulkanRenderPassBuilder& VulkanRenderPassBuilder::addDependency(const SubpassDependencyInfo& dependency) {
    dependencies.push_back(dependency);
    return *this;
}

VulkanRenderPassBuilder& VulkanRenderPassBuilder::forForwardRendering(vk::Format colorFormat, vk::Format depthFormat) {
    attachments.clear();
    subpasses.clear();
    dependencies.clear();
    
    // Color attachment
    addAttachment(AttachmentInfo::colorAttachment(colorFormat));
    
    // Depth attachment
    addAttachment(AttachmentInfo::depthAttachment(depthFormat));
    
    // Subpass
    SubpassInfo subpass;
    subpass.colorAttachments = {0};
    subpass.depthStencilAttachment = 1;
    addSubpass(subpass);
    
    // Dependencies
    addDependency(SubpassDependencyInfo::externalToColor());
    addDependency(SubpassDependencyInfo::colorToPresent());
    
    // Additional depth dependency
    SubpassDependencyInfo depthDep;
    depthDep.srcSubpass = VK_SUBPASS_EXTERNAL;
    depthDep.dstSubpass = 0;
    depthDep.srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | 
                           vk::PipelineStageFlagBits::eLateFragmentTests;
    depthDep.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | 
                           vk::PipelineStageFlagBits::eLateFragmentTests;
    depthDep.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
    depthDep.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
                            vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    addDependency(depthDep);
    
    return *this;
}

VulkanRenderPassBuilder& VulkanRenderPassBuilder::forDeferredRendering(
    vk::Format albedoFormat, vk::Format normalFormat,
    vk::Format positionFormat, vk::Format depthFormat) {
    
    attachments.clear();
    subpasses.clear();
    dependencies.clear();
    
    // G-Buffer attachments
    addAttachment(AttachmentInfo::colorAttachment(albedoFormat));
    addAttachment(AttachmentInfo::colorAttachment(normalFormat));
    addAttachment(AttachmentInfo::colorAttachment(positionFormat));
    addAttachment(AttachmentInfo::depthAttachment(depthFormat));
    
    // Geometry subpass
    SubpassInfo geometryPass;
    geometryPass.colorAttachments = {0, 1, 2};
    geometryPass.depthStencilAttachment = 3;
    addSubpass(geometryPass);
    
    // Lighting subpass (reads G-Buffer as input)
    SubpassInfo lightingPass;
    lightingPass.inputAttachments = {0, 1, 2, 3};  // Read G-Buffer
    lightingPass.colorAttachments = {0};  // Output to final color
    addSubpass(lightingPass);
    
    // Dependencies
    addDependency(SubpassDependencyInfo::externalToColor());
    addDependency(SubpassDependencyInfo::colorToPresent());
    
    return *this;
}

VulkanRenderPassBuilder& VulkanRenderPassBuilder::forShadowPass(vk::Format depthFormat) {
    attachments.clear();
    subpasses.clear();
    dependencies.clear();
    
    // Depth-only attachment
    AttachmentInfo depthAtt = AttachmentInfo::depthAttachment(depthFormat);
    depthAtt.finalLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    addAttachment(depthAtt);
    
    // Subpass
    SubpassInfo subpass;
    subpass.depthStencilAttachment = 0;
    addSubpass(subpass);
    
    // Dependencies
    SubpassDependencyInfo beginDep;
    beginDep.srcSubpass = VK_SUBPASS_EXTERNAL;
    beginDep.dstSubpass = 0;
    beginDep.srcStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    beginDep.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | 
                           vk::PipelineStageFlagBits::eLateFragmentTests;
    beginDep.srcAccessMask = vk::AccessFlagBits::eShaderRead;
    beginDep.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    addDependency(beginDep);
    
    SubpassDependencyInfo endDep;
    endDep.srcSubpass = 0;
    endDep.dstSubpass = VK_SUBPASS_EXTERNAL;
    endDep.srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | 
                         vk::PipelineStageFlagBits::eLateFragmentTests;
    endDep.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    endDep.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    endDep.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    addDependency(endDep);
    
    return *this;
}

VulkanRenderPassBuilder& VulkanRenderPassBuilder::forPostProcess(vk::Format colorFormat) {
    attachments.clear();
    subpasses.clear();
    dependencies.clear();
    
    // Input attachment (from previous pass)
    AttachmentInfo inputAtt;
    inputAtt.format = colorFormat;
    inputAtt.loadOp = vk::AttachmentLoadOp::eLoad;
    inputAtt.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
    inputAtt.finalLayout = vk::ImageLayout::ePresentSrcKHR;
    addAttachment(inputAtt);
    
    // Subpass
    SubpassInfo subpass;
    subpass.colorAttachments = {0};
    addSubpass(subpass);
    
    // Dependencies
    SubpassDependencyInfo dep;
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dep.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    dep.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    dep.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    addDependency(dep);
    
    return *this;
}

VulkanRenderPassBuilder& VulkanRenderPassBuilder::forUI(vk::Format colorFormat) {
    attachments.clear();
    subpasses.clear();
    dependencies.clear();
    
    // Color attachment (load existing)
    AttachmentInfo colorAtt;
    colorAtt.format = colorFormat;
    colorAtt.loadOp = vk::AttachmentLoadOp::eLoad;
    colorAtt.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAtt.finalLayout = vk::ImageLayout::ePresentSrcKHR;
    addAttachment(colorAtt);
    
    // Subpass
    SubpassInfo subpass;
    subpass.colorAttachments = {0};
    addSubpass(subpass);
    
    // Dependencies
    addDependency(SubpassDependencyInfo::externalToColor());
    addDependency(SubpassDependencyInfo::colorToPresent());
    
    return *this;
}

vk::RenderPass VulkanRenderPassBuilder::build(vk::Device device) {
    // Convert attachments
    std::vector<vk::AttachmentDescription> vkAttachments;
    for (const auto& att : attachments) {
        vk::AttachmentDescription desc{};
        desc.format = att.format;
        desc.samples = att.samples;
        desc.loadOp = att.loadOp;
        desc.storeOp = att.storeOp;
        desc.stencilLoadOp = att.stencilLoadOp;
        desc.stencilStoreOp = att.stencilStoreOp;
        desc.initialLayout = att.initialLayout;
        desc.finalLayout = att.finalLayout;
        vkAttachments.push_back(desc);
    }
    
    // Convert subpasses with references
    std::vector<std::vector<vk::AttachmentReference>> allColorRefs;
    std::vector<std::vector<vk::AttachmentReference>> allInputRefs;
    std::vector<std::vector<vk::AttachmentReference>> allResolveRefs;
    std::vector<vk::AttachmentReference> depthRefs;
    std::vector<vk::SubpassDescription> vkSubpasses;
    
    for (size_t i = 0; i < subpasses.size(); i++) {
        const auto& subpass = subpasses[i];
        
        // Color references
        std::vector<vk::AttachmentReference> colorRefs;
        for (uint32_t idx : subpass.colorAttachments) {
            vk::AttachmentReference ref{};
            ref.attachment = idx;
            ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
            colorRefs.push_back(ref);
        }
        allColorRefs.push_back(colorRefs);
        
        // Input references
        std::vector<vk::AttachmentReference> inputRefs;
        for (uint32_t idx : subpass.inputAttachments) {
            vk::AttachmentReference ref{};
            ref.attachment = idx;
            ref.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
            inputRefs.push_back(ref);
        }
        allInputRefs.push_back(inputRefs);
        
        // Resolve references
        std::vector<vk::AttachmentReference> resolveRefs;
        for (uint32_t idx : subpass.resolveAttachments) {
            vk::AttachmentReference ref{};
            ref.attachment = idx;
            ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
            resolveRefs.push_back(ref);
        }
        allResolveRefs.push_back(resolveRefs);
        
        // Depth reference
        vk::AttachmentReference depthRef{};
        const vk::AttachmentReference* depthPtr = nullptr;
        if (subpass.depthStencilAttachment) {
            depthRef.attachment = *subpass.depthStencilAttachment;
            depthRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            depthRefs.push_back(depthRef);
            depthPtr = &depthRefs.back();
        }
        
        vkSubpasses.push_back(subpass.toDescription(
            allColorRefs.back(), allInputRefs.back(), depthPtr, allResolveRefs.back()
        ));
    }
    
    // Convert dependencies
    std::vector<vk::SubpassDependency> vkDependencies;
    for (const auto& dep : dependencies) {
        vk::SubpassDependency vkDep{};
        vkDep.srcSubpass = dep.srcSubpass;
        vkDep.dstSubpass = dep.dstSubpass;
        vkDep.srcStageMask = dep.srcStageMask;
        vkDep.dstStageMask = dep.dstStageMask;
        vkDep.srcAccessMask = dep.srcAccessMask;
        vkDep.dstAccessMask = dep.dstAccessMask;
        vkDep.dependencyFlags = dep.dependencyFlags;
        vkDependencies.push_back(vkDep);
    }
    
    vk::RenderPassCreateInfo createInfo{};
    createInfo.attachmentCount = static_cast<uint32_t>(vkAttachments.size());
    createInfo.pAttachments = vkAttachments.data();
    createInfo.subpassCount = static_cast<uint32_t>(vkSubpasses.size());
    createInfo.pSubpasses = vkSubpasses.data();
    createInfo.dependencyCount = static_cast<uint32_t>(vkDependencies.size());
    createInfo.pDependencies = vkDependencies.data();
    
    try {
        return device.createRenderPass(createInfo);
    } catch (const vk::SystemError& e) {
        throw std::runtime_error("Failed to create render pass: " + std::string(e.what()));
    }
}

// ============== VulkanRenderPass ==============

VulkanRenderPass::VulkanRenderPass() = default;

VulkanRenderPass::~VulkanRenderPass() {
    cleanup();
}

VulkanRenderPass::VulkanRenderPass(VulkanRenderPass&& other) noexcept
    : device(other.device), renderPass(other.renderPass), attachmentCount(other.attachmentCount) {
    other.device = VK_NULL_HANDLE;
    other.renderPass = VK_NULL_HANDLE;
    other.attachmentCount = 0;
}

VulkanRenderPass& VulkanRenderPass::operator=(VulkanRenderPass&& other) noexcept {
    if (this != &other) {
        cleanup();
        device = other.device;
        renderPass = other.renderPass;
        attachmentCount = other.attachmentCount;
        
        other.device = VK_NULL_HANDLE;
        other.renderPass = VK_NULL_HANDLE;
        other.attachmentCount = 0;
    }
    return *this;
}

void VulkanRenderPass::init(vk::Device device, vk::RenderPass renderPass) {
    this->device = device;
    this->renderPass = renderPass;
}

void VulkanRenderPass::init(vk::Device device, const VulkanRenderPassBuilder& builder) {
    this->device = device;
    renderPass = builder.build(device);
    attachmentCount = static_cast<uint32_t>(builder.attachments.size());
}

void VulkanRenderPass::cleanup() {
    if (device && renderPass) {
        device.destroyRenderPass(renderPass);
        renderPass = VK_NULL_HANDLE;
    }
}

void VulkanRenderPass::begin(vk::CommandBuffer cmd, vk::Framebuffer framebuffer,
                             vk::Rect2D renderArea, const std::vector<vk::ClearValue>& clearValues,
                             vk::SubpassContents contents) {
    vk::RenderPassBeginInfo beginInfo{};
    beginInfo.renderPass = renderPass;
    beginInfo.framebuffer = framebuffer;
    beginInfo.renderArea = renderArea;
    beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    beginInfo.pClearValues = clearValues.data();
    
    cmd.beginRenderPass(beginInfo, contents);
}

void VulkanRenderPass::nextSubpass(vk::CommandBuffer cmd, vk::SubpassContents contents) {
    cmd.nextSubpass(contents);
}

void VulkanRenderPass::end(vk::CommandBuffer cmd) {
    cmd.endRenderPass();
}

// ============== VulkanDynamicRendering ==============

VulkanDynamicRendering::VulkanDynamicRendering() {
    renderingInfo.sType = vk::StructureType::eRenderingInfo;
}

VulkanDynamicRendering& VulkanDynamicRendering::addColorAttachment(vk::Format format) {
    vk::RenderingAttachmentInfo attInfo{};
    attInfo.sType = vk::StructureType::eRenderingAttachmentInfo;
    attInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attInfo.loadOp = vk::AttachmentLoadOp::eClear;
    attInfo.storeOp = vk::AttachmentStoreOp::eStore;
    
    colorAttachments.push_back(attInfo);
    return *this;
}

VulkanDynamicRendering& VulkanDynamicRendering::setDepthAttachment(vk::Format format, bool stencil) {
    depthAttachment.sType = vk::StructureType::eRenderingAttachmentInfo;
    depthAttachment.imageLayout = stencil ? 
        vk::ImageLayout::eDepthStencilAttachmentOptimal : 
        vk::ImageLayout::eDepthAttachmentOptimal;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    hasDepth = true;
    return *this;
}

void VulkanDynamicRendering::begin(vk::CommandBuffer cmd, vk::Rect2D renderArea,
                                   const std::vector<vk::ImageView>& colorViews,
                                   vk::ImageView depthView,
                                   const std::vector<vk::ClearValue>& clearValues) {
    // Set up color attachment infos
    for (size_t i = 0; i < colorAttachments.size() && i < colorViews.size(); i++) {
        colorAttachments[i].imageView = colorViews[i];
        if (i < clearValues.size()) {
            colorAttachments[i].clearValue = clearValues[i];
        }
    }
    
    // Set up depth attachment
    if (hasDepth && depthView) {
        depthAttachment.imageView = depthView;
        if (clearValues.size() > colorAttachments.size()) {
            depthAttachment.clearValue = clearValues[colorAttachments.size()];
        }
    }
    
    renderingInfo.renderArea = renderArea;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderingInfo.pColorAttachments = colorAttachments.data();
    renderingInfo.pDepthAttachment = hasDepth ? &depthAttachment : nullptr;
    renderingInfo.pStencilAttachment = nullptr;
    
    cmd.beginRendering(renderingInfo);
}

void VulkanDynamicRendering::end(vk::CommandBuffer cmd) {
    cmd.endRendering();
}

} // namespace VoxelForge
