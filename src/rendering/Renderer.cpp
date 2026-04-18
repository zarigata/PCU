/**
 * @file Renderer.cpp
 * @brief Main renderer class implementation
 */

#include <VoxelForge/rendering/Renderer.hpp>
#include <VoxelForge/rendering/VulkanPipeline.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <VoxelForge/utils/Profiler.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <fstream>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

namespace VoxelForge {

// Global renderer instance
static Renderer* g_Renderer = nullptr;

Renderer& GetRenderer() {
    if (!g_Renderer) {
        throw std::runtime_error("Renderer not initialized!");
    }
    return *g_Renderer;
}

void InitRenderer(GLFWwindow* window) {
    if (g_Renderer) {
        VF_WARN("Renderer already initialized");
        return;
    }
    g_Renderer = new Renderer();
    g_Renderer->init(window);
}

void ShutdownRenderer() {
    if (g_Renderer) {
        g_Renderer->shutdown();
        delete g_Renderer;
        g_Renderer = nullptr;
    }
}

// ============================================================================
// Renderer Implementation
// ============================================================================

Renderer::Renderer() {
    VF_INFO("Renderer created");
}

Renderer::~Renderer() {
    shutdown();
    VF_INFO("Renderer destroyed");
}

void Renderer::init(GLFWwindow* window) {
    VF_PROFILE_FUNCTION();
    
    VF_INFO("Initializing renderer...");
    
    // Get window size
    glfwGetFramebufferSize(window, &width, &height);
    
    // Create Vulkan context
    context = std::make_unique<VulkanContext>();
    context->init(window);
    
    // Create swapchain
    createSwapChain();
    
    // Create render pass
    createRenderPass();
    
    // Create framebuffers
    createFramebuffers();
    
    // Create command pool and buffers
    createCommandBuffers();
    
    // Create synchronization objects
    createSyncObjects();
    
    VF_INFO("Renderer initialized successfully");
    VF_INFO("  Resolution: {}x{}", width, height);
    VF_INFO("  Swapchain format: {}", static_cast<int>(swapchainFormat));
}

void Renderer::shutdown() {
    if (!context) return;
    
    VF_INFO("Shutting down renderer...");
    
    // Wait for device to be idle
    context->getDevice().waitIdle();
    
    // Cleanup sync objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (renderFinishedSemaphores[i]) {
            context->getDevice().destroySemaphore(renderFinishedSemaphores[i]);
        }
        if (imageAvailableSemaphores[i]) {
            context->getDevice().destroySemaphore(imageAvailableSemaphores[i]);
        }
        if (inFlightFences[i]) {
            context->getDevice().destroyFence(inFlightFences[i]);
        }
    }
    
    // Cleanup command buffers
    if (commandPool) {
        context->getDevice().freeCommandBuffers(commandPool, commandBuffers);
        context->getDevice().destroyCommandPool(commandPool);
    }
    
    // Cleanup framebuffers
    for (auto framebuffer : framebuffers) {
        context->getDevice().destroyFramebuffer(framebuffer);
    }
    
    // Cleanup render pass
    if (renderPass) {
        context->getDevice().destroyRenderPass(renderPass);
    }
    
    // Cleanup swapchain
    for (auto imageView : swapchainImageViews) {
        context->getDevice().destroyImageView(imageView);
    }
    
    if (swapchain) {
        context->getDevice().destroySwapchainKHR(swapchain);
    }
    
    // Context cleanup is handled by VulkanContext destructor
    context.reset();
    
    VF_INFO("Renderer shutdown complete");
}

void Renderer::createSwapChain() {
    VF_PROFILE_FUNCTION();
    
    auto support = context->querySwapChainSupport();
    
    // Choose surface format
    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(support.formats);
    
    // Choose present mode
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(support.presentModes);
    
    // Choose extent
    vk::Extent2D extent = chooseSwapExtent(support.capabilities);
    
    // Image count
    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
        imageCount = support.capabilities.maxImageCount;
    }
    
    // Create swapchain
    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.sType = vk::StructureType::eSwapchainCreateInfoKHR;
    createInfo.surface = context->getSurface();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | 
                            vk::ImageUsageFlagBits::eTransferSrc; // For screenshots
    
    auto indices = context->getQueueFamilies();
    uint32_t queueFamilyIndices[] = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };
    
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    
    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = vk::SwapchainKHR{};
    
    try {
        swapchain = context->getDevice().createSwapchainKHR(createInfo);
    } catch (const vk::SystemError& e) {
        throw std::runtime_error(std::string("Failed to create swapchain: ") + e.what());
    }
    
    // Get swapchain images
    swapchainImages = context->getDevice().getSwapchainImagesKHR(swapchain);
    swapchainFormat = surfaceFormat.format;
    swapchainExtent = extent;
    
    // Create image views
    swapchainImageViews.resize(swapchainImages.size());
    
    for (size_t i = 0; i < swapchainImages.size(); i++) {
        vk::ImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.sType = vk::StructureType::eImageViewCreateInfo;
        viewCreateInfo.image = swapchainImages[i];
        viewCreateInfo.viewType = vk::ImageViewType::e2D;
        viewCreateInfo.format = swapchainFormat;
        viewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
        viewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
        viewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
        viewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
        viewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;
        
        try {
            swapchainImageViews[i] = context->getDevice().createImageView(viewCreateInfo);
        } catch (const vk::SystemError& e) {
            throw std::runtime_error(std::string("Failed to create image view: ") + e.what());
        }
    }
    
    VF_INFO("Swapchain created: {} images, {}x{}", 
             swapchainImages.size(), extent.width, extent.height);
}

void Renderer::createRenderPass() {
    VF_PROFILE_FUNCTION();
    
    // Color attachment
    vk::AttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
    
    // Depth attachment
    vk::AttachmentDescription depthAttachment{};
    depthAttachment.format = context->findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    
    // Color attachment reference
    vk::AttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
    
    // Subpass
    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    
    // Subpass dependency
    vk::SubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = vk::AccessFlags{};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    
    // Create render pass
    vk::AttachmentDescription attachments[] = {colorAttachment};
    
    vk::RenderPassCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eRenderPassCreateInfo;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;
    
    try {
        renderPass = context->getDevice().createRenderPass(createInfo);
    } catch (const vk::SystemError& e) {
        throw std::runtime_error(std::string("Failed to create render pass: ") + e.what());
    }
    
    VF_INFO("Render pass created");
}

void Renderer::createFramebuffers() {
    VF_PROFILE_FUNCTION();
    
    framebuffers.resize(swapchainImageViews.size());
    
    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        vk::ImageView attachments[] = {swapchainImageViews[i]};
        
        vk::FramebufferCreateInfo createInfo{};
        createInfo.sType = vk::StructureType::eFramebufferCreateInfo;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = attachments;
        createInfo.width = swapchainExtent.width;
        createInfo.height = swapchainExtent.height;
        createInfo.layers = 1;
        
        try {
            framebuffers[i] = context->getDevice().createFramebuffer(createInfo);
        } catch (const vk::SystemError& e) {
            throw std::runtime_error(std::string("Failed to create framebuffer: ") + e.what());
        }
    }
    
    VF_INFO("Framebuffers created: {}", framebuffers.size());
}

void Renderer::createCommandBuffers() {
    VF_PROFILE_FUNCTION();
    
    // Create command pool
    auto indices = context->getQueueFamilies();
    
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    
    try {
        commandPool = context->getDevice().createCommandPool(poolInfo);
    } catch (const vk::SystemError& e) {
        throw std::runtime_error(std::string("Failed to create command pool: ") + e.what());
    }
    
    // Allocate command buffers
    commandBuffers.resize(framebuffers.size());
    
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    allocInfo.commandPool = commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    
    try {
        commandBuffers = context->getDevice().allocateCommandBuffers(allocInfo);
    } catch (const vk::SystemError& e) {
        throw std::runtime_error(std::string("Failed to allocate command buffers: ") + e.what());
    }
    
    VF_INFO("Command buffers created: {}", commandBuffers.size());
}

void Renderer::createSyncObjects() {
    VF_PROFILE_FUNCTION();
    
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    
    vk::SemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;
    
    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        try {
            imageAvailableSemaphores[i] = context->getDevice().createSemaphore(semaphoreInfo);
            renderFinishedSemaphores[i] = context->getDevice().createSemaphore(semaphoreInfo);
            inFlightFences[i] = context->getDevice().createFence(fenceInfo);
        } catch (const vk::SystemError& e) {
            throw std::runtime_error(std::string("Failed to create synchronization objects: ") + e.what());
        }
    }
    
    VF_INFO("Synchronization objects created");
}

void Renderer::setClearColor(float r, float g, float b, float a) {
    clearColorValue = {r, g, b, a};
}

void Renderer::beginFrame() {
    VF_PROFILE_FUNCTION();
    
    // Wait for previous frame
    auto result = context->getDevice().waitForFences(
        1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    
    if (result != vk::Result::eSuccess) {
        VF_ERROR("Failed to wait for fence");
    }
    
    // Acquire next image
    try {
        result = context->getDevice().acquireNextImageKHR(
            swapchain, UINT64_MAX, 
            imageAvailableSemaphores[currentFrame], 
            vk::Fence{}, &currentImageIndex);
    } catch (const vk::OutOfDateKHRError&) {
        // Need to recreate swapchain
        recreateSwapChain();
        return;
    } catch (const vk::SystemError& e) {
        throw std::runtime_error(std::string("Failed to acquire swapchain image: ") + e.what());
    }
    
    // Reset fence
    context->getDevice().resetFences(1, &inFlightFences[currentFrame]);
    
    // Reset command buffer
    commandBuffers[currentImageIndex].reset();
    
    // Begin command buffer
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    
    commandBuffers[currentImageIndex].begin(beginInfo);
    
    // Begin render pass
    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = vk::StructureType::eRenderPassBeginInfo;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[currentImageIndex];
    renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent;
    
    // Clear values
    vk::ClearValue clearValue{};
    clearValue.color = vk::ClearColorValue{clearColorValue[0], clearColorValue[1], clearColorValue[2], clearColorValue[3]};
    
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;
    
    commandBuffers[currentImageIndex].beginRenderPass(
        &renderPassInfo, vk::SubpassContents::eInline);
    
    vk::Viewport vp{0.0f, 0.0f, (float)swapchainExtent.width, (float)swapchainExtent.height, 0.0f, 1.0f};
    commandBuffers[currentImageIndex].setViewport(0, 1, &vp);
    commandBuffers[currentImageIndex].setScissor(0, 1, &renderPassInfo.renderArea);
    
    // Reset stats
    stats = RenderStats{};
}

void Renderer::endFrame() {
    VF_PROFILE_FUNCTION();
    
    // End render pass
    commandBuffers[currentImageIndex].endRenderPass();
    
    // End command buffer
    commandBuffers[currentImageIndex].end();
    
    // Submit command buffer
    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;
    
    vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    vk::PipelineStageFlags waitStages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentImageIndex];
    
    vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    try {
        context->getGraphicsQueue().submit(1, &submitInfo, inFlightFences[currentFrame]);
    } catch (const vk::SystemError& e) {
        throw std::runtime_error(std::string("Failed to submit draw command buffer: ") + e.what());
    }
    
    // Present
    vk::PresentInfoKHR presentInfo{};
    presentInfo.sType = vk::StructureType::ePresentInfoKHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImageIndex;
    
    try {
        context->getPresentQueue().presentKHR(&presentInfo);
    } catch (const vk::OutOfDateKHRError&) {
        framebufferResized = false;
        recreateSwapChain();
    } catch (const vk::SystemError& e) {
        throw std::runtime_error(std::string("Failed to present swapchain image: ") + e.what());
    }
    
    // Advance to next frame
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::render(World* world, Camera* camera) {
    VF_PROFILE_FUNCTION();
    
    if (!world || !camera) {
        VF_WARN("Render called with null world or camera");
        return;
    }
    
    // Render chunks
    renderChunks(world, camera);
    
    // Render entities
    renderEntities(world, camera);
    
    // Render particles
    renderParticles(world, camera);
    
    // Render UI
    renderUI();
    
    // Post-processing
    renderPostProcess();
}

void Renderer::renderChunks(World* world, Camera* camera) {
    VF_PROFILE_FUNCTION();
    
    // TODO: Implement chunk rendering
    // For now, just update stats
    stats.chunksRendered = 0;
    stats.chunkDrawCalls = 0;
}

void Renderer::renderEntities(World* world, Camera* camera) {
    VF_PROFILE_FUNCTION();
    
    // TODO: Implement entity rendering
    stats.entityDrawCalls = 0;
}

void Renderer::renderParticles(World* world, Camera* camera) {
    VF_PROFILE_FUNCTION();
    
    // TODO: Implement particle rendering
    stats.particlesDrawCalls = 0;
}

void Renderer::renderUI() {
    VF_PROFILE_FUNCTION();
    
    // TODO: Implement UI rendering
}

void Renderer::renderPostProcess() {
    VF_PROFILE_FUNCTION();
    
    // TODO: Implement post-processing (bloom, TAA, FXAA)
}

void Renderer::onResize(int newWidth, int newHeight) {
    if (newWidth == width && newHeight == height) return;
    
    width = newWidth;
    height = newHeight;
    framebufferResized = true;
    
    context->getDevice().waitIdle();
    recreateSwapChain();
    
    VF_INFO("Window resized: {}x{}", width, height);
}

void Renderer::recreateSwapChain() {
    VF_PROFILE_FUNCTION();
    
    // Wait if minimized
    while (width == 0 || height == 0) {
        glfwWaitEvents();
    }
    
    context->getDevice().waitIdle();
    
    // Cleanup old swapchain
    for (auto framebuffer : framebuffers) {
        context->getDevice().destroyFramebuffer(framebuffer);
    }
    for (auto imageView : swapchainImageViews) {
        context->getDevice().destroyImageView(imageView);
    }
    if (swapchain) {
        context->getDevice().destroySwapchainKHR(swapchain);
    }
    
    // Create new swapchain
    createSwapChain();
    createRenderPass();
    createFramebuffers();
    createCommandBuffers();
}

void Renderer::takeScreenshot(const std::string& path) {
    VF_PROFILE_FUNCTION();
    
    VF_INFO("Taking screenshot: {}", path);
    
    // TODO: Implement screenshot functionality
    // 1. Create a buffer image
    // 2. Copy swapchain image to buffer image
    // 3. Map and read the image data
    // 4. Save to file (PNG or BMP)
    
    VF_WARN("Screenshot not yet implemented");
}

void Renderer::reloadShaders() {
    VF_PROFILE_FUNCTION();
    
    VF_INFO("Reloading shaders...");
    
    // TODO: Implement shader reloading
    // 1. Destroy old pipelines
    // 2. Recreate shader modules
    // 3. Recreate pipelines
    
    VF_WARN("Shader reloading not yet implemented");
}

// ============================================================================
// Helper functions
// ============================================================================

vk::SurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }
    
    return availableFormats[0];
}

vk::PresentModeKHR Renderer::chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    
    // Prefer mailbox (triple buffering)
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }
    
    // Fallback to FIFO (vsync)
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Renderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    
    vk::Extent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };
    
    actualExtent.width = std::clamp(
        actualExtent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );
    
    actualExtent.height = std::clamp(
        actualExtent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );
    
    return actualExtent;
}

vk::ShaderModule Renderer::compileShader(const std::string& source, int stageInt) {
    static bool glslangInitialized = false;
    if (!glslangInitialized) {
        glslang::InitializeProcess();
        glslangInitialized = true;
    }
    
    EShLanguage stage = static_cast<EShLanguage>(stageInt);
    glslang::TShader shader(stage);
    const char* src = source.c_str();
    shader.setStrings(&src, 1);
    
    EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(GetDefaultResources(), 450, false, messages)) {
        VF_ERROR("Shader parse error: {}", shader.getInfoLog());
        return VK_NULL_HANDLE;
    }
    
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        VF_ERROR("Shader link error: {}", program.getInfoLog());
        return VK_NULL_HANDLE;
    }
    
    std::vector<uint32_t> spirv;
    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);
    
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();
    
    auto module = context->getDevice().createShaderModule(createInfo);
    VF_INFO("Compiled shader ({} bytes SPIRV)", spirv.size() * 4);
    return module;
}

void Renderer::createChunkBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                                  vk::MemoryPropertyFlags props,
                                  vk::Buffer& buf, vk::DeviceMemory& mem) {
    vk::BufferCreateInfo bufInfo{};
    bufInfo.size = size;
    bufInfo.usage = usage;
    bufInfo.sharingMode = vk::SharingMode::eExclusive;
    buf = context->getDevice().createBuffer(bufInfo);
    
    auto memReqs = context->getDevice().getBufferMemoryRequirements(buf);
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = context->findMemoryType(memReqs.memoryTypeBits, props);
    mem = context->getDevice().allocateMemory(allocInfo);
    context->getDevice().bindBufferMemory(buf, mem, 0);
}

void Renderer::initChunkRendering() {
    const char* vertSrc = R"(
        #version 450
        layout(location=0) in vec3 inPos;
        layout(location=1) in uint inColor;
        layout(push_constant) uniform PC { mat4 mvp; };
        layout(location=0) out vec4 vColor;
        void main() {
            gl_Position = mvp * vec4(inPos, 1.0);
            float r = float(inColor & 0xFFu) / 255.0;
            float g = float((inColor >> 8u) & 0xFFu) / 255.0;
            float b = float((inColor >> 16u) & 0xFFu) / 255.0;
            vColor = vec4(r, g, b, 1.0);
        }
    )";
    
    const char* fragSrc = R"(
        #version 450
        layout(location=0) in vec4 vColor;
        layout(location=0) out vec4 outColor;
        void main() { outColor = vColor; }
    )";
    
    chunkVertShader = compileShader(vertSrc, EShLangVertex);
    chunkFragShader = compileShader(fragSrc, EShLangFragment);
    
    if (!chunkVertShader || !chunkFragShader) {
        VF_ERROR("Failed to compile chunk shaders");
        return;
    }
    
    VulkanPipelineBuilder builder(context->getDevice());
    builder.setShaders(chunkVertShader, chunkFragShader);
    
    vk::VertexInputBindingDescription binding{0, 16, vk::VertexInputRate::eVertex};
    std::vector<vk::VertexInputAttributeDescription> attrs = {
        {0, 0, vk::Format::eR32G32B32Sfloat, 0},
        {1, 0, vk::Format::eR32Uint, 12}
    };
    builder.setVertexInput(binding, attrs);
    builder.setInputTopology(vk::PrimitiveTopology::eTriangleList);
    builder.setViewport(0, 0, (float)swapchainExtent.width, (float)swapchainExtent.height);
    builder.setScissor(0, 0, swapchainExtent.width, swapchainExtent.height);
    builder.setRasterizer(vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise);
    builder.setMultisampling(vk::SampleCountFlagBits::e1);
    builder.setDepthStencil(false, false, vk::CompareOp::eLess);
    builder.setColorBlendAttachment(
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    builder.addPushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4));
    builder.setRenderPass(renderPass, 0);
    
    auto pipeline = builder.build();
    if (pipeline) {
        chunkPipeline = pipeline.release();  // Transfer ownership — we manage cleanup ourselves
        chunkPipelineLayout = builder.getPipelineLayout();
        chunkPipelineReady = true;
        VF_INFO("Chunk rendering pipeline created");
    } else {
        VF_ERROR("Failed to create chunk rendering pipeline");
    }

    // Initialize UI rendering resources once chunk rendering is ready
    initUIRendering();
}

void Renderer::cleanupChunkRendering() {
    // Cleanup UI resources first to ensure no GPU usage during chunk teardown
    cleanupUIRendering();
    if (!context) return;
    auto dev = context->getDevice();
    
    for (auto& [key, mesh] : chunkMeshes) {
        if (mesh.vertexBuffer) dev.destroyBuffer(mesh.vertexBuffer);
        if (mesh.vertexMemory) dev.freeMemory(mesh.vertexMemory);
        if (mesh.indexBuffer) dev.destroyBuffer(mesh.indexBuffer);
        if (mesh.indexMemory) dev.freeMemory(mesh.indexMemory);
    }
    chunkMeshes.clear();
    
    if (chunkPipeline) { dev.destroyPipeline(chunkPipeline); chunkPipeline = VK_NULL_HANDLE; }
    if (chunkPipelineLayout) { dev.destroyPipelineLayout(chunkPipelineLayout); chunkPipelineLayout = VK_NULL_HANDLE; }
    if (chunkVertShader) { dev.destroyShaderModule(chunkVertShader); chunkVertShader = VK_NULL_HANDLE; }
    if (chunkFragShader) { dev.destroyShaderModule(chunkFragShader); chunkFragShader = VK_NULL_HANDLE; }
    chunkPipelineReady = false;
}

uint64_t Renderer::posKey(int x, int y, int z) {
    return (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 20) | ((uint64_t)(uint32_t)z << 40);
}

void Renderer::uploadChunkMesh(const glm::ivec3& pos, const std::vector<float>& verts, const std::vector<uint32_t>& indices) {
    auto dev = context->getDevice();
    uint64_t key = posKey(pos.x, pos.y, pos.z);
    
    auto it = chunkMeshes.find(key);
    if (it != chunkMeshes.end()) {
        if (it->second.vertexBuffer) dev.destroyBuffer(it->second.vertexBuffer);
        if (it->second.vertexMemory) dev.freeMemory(it->second.vertexMemory);
        if (it->second.indexBuffer) dev.destroyBuffer(it->second.indexBuffer);
        if (it->second.indexMemory) dev.freeMemory(it->second.indexMemory);
    }
    
    ChunkGPUMesh mesh{};
    mesh.chunkPos = pos;
    mesh.indexCount = (uint32_t)indices.size();
    mesh.valid = true;
    
    vk::DeviceSize vsize = verts.size() * sizeof(float);
    vk::DeviceSize isize = indices.size() * sizeof(uint32_t);
    
    vk::Buffer stagingBuf;
    vk::DeviceMemory stagingMem;
    createChunkBuffer(vsize + isize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuf, stagingMem);
    
    void* data = context->getDevice().mapMemory(stagingMem, 0, vsize + isize);
    memcpy(data, verts.data(), vsize);
    memcpy((char*)data + vsize, indices.data(), isize);
    context->getDevice().unmapMemory(stagingMem);
    
    createChunkBuffer(vsize,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        mesh.vertexBuffer, mesh.vertexMemory);
    
    createChunkBuffer(isize,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        mesh.indexBuffer, mesh.indexMemory);
    
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = 1;
    auto cmdBuf = dev.allocateCommandBuffers(allocInfo)[0];
    
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmdBuf.begin(beginInfo);
    
    vk::BufferCopy vertCopy{0, 0, vsize};
    cmdBuf.copyBuffer(stagingBuf, mesh.vertexBuffer, 1, &vertCopy);
    vk::BufferCopy idxCopy{vsize, 0, isize};
    cmdBuf.copyBuffer(stagingBuf, mesh.indexBuffer, 1, &idxCopy);
    
    cmdBuf.end();
    
    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;
    context->getGraphicsQueue().submit(1, &submitInfo, VK_NULL_HANDLE);
    context->getGraphicsQueue().waitIdle();
    
    dev.freeCommandBuffers(commandPool, 1, &cmdBuf);
    dev.destroyBuffer(stagingBuf);
    dev.freeMemory(stagingMem);
    
    chunkMeshes[key] = std::move(mesh);
}

static uint32_t packColor(uint8_t r, uint8_t g, uint8_t b) {
    return r | (g << 8) | (b << 16) | (0xFF << 24);
}

static uint32_t applyFaceLight(uint32_t color, int face) {
    float brightness;
    switch (face) {
        case 4: brightness = 1.0f; break;
        case 5: brightness = 0.5f; break;
        case 2: case 3: brightness = 0.8f; break;
        case 0: case 1: brightness = 0.65f; break;
        default: brightness = 1.0f; break;
    }
    uint8_t r = (uint8_t)(color & 0xFF);
    uint8_t g = (uint8_t)((color >> 8) & 0xFF);
    uint8_t b = (uint8_t)((color >> 16) & 0xFF);
    return (uint8_t)(r * brightness) | ((uint8_t)(g * brightness) << 8) |
           ((uint8_t)(b * brightness) << 16) | (0xFF << 24);
}

static uint32_t blockColor(uint32_t blockId, int face) {
    uint32_t base;
    switch (blockId) {
        case 2: base = face == 4 ? packColor(76,175,80) : packColor(139,119,42); break;
        case 3: base = packColor(139,90,43); break;
        case 1: base = packColor(128,128,128); break;
        case 4: base = packColor(110,110,110); break;
        case 13: base = packColor(244,225,156); break;
        case 14: base = packColor(10,10,10); break;
        case 5: base = packColor(180,140,80); break;
        case 6: case 7: case 8: case 9: case 10: base = packColor(100,70,40); break;
        case 11: base = packColor(52,152,219); break;
        case 12: base = packColor(220,80,20); break;
        case 15: base = packColor(160,143,129); break;
        case 16: base = packColor(80,80,80); break;
        case 17: base = packColor(255,215,0); break;
        case 18: base = packColor(80,220,230); break;
        case 64: case 65: case 66: case 67: case 68: case 69: case 70: case 71:
            base = packColor(50,140,50); break;
        default: base = packColor(150,150,150); break;
    }
    return applyFaceLight(base, face);
}

void Renderer::generateAndUploadChunks(World* world, const glm::vec3& cameraPos) {
    if (!world || !chunkPipelineReady) return;
    
    int rd = settings.renderDistance;
    int cx = (int)floor(cameraPos.x / 16.0f);
    int cz = (int)floor(cameraPos.z / 16.0f);
    
    static bool firstLog = true;
    if (firstLog) {
        VF_INFO("Chunk mesh gen: cameraPos=({:.1f},{:.1f},{:.1f}) cx={} cz={} rd={}",
                cameraPos.x, cameraPos.y, cameraPos.z, cx, cz, rd);
        firstLog = false;
    }
    
    int uploaded = 0;
    int skipped = 0;
    int emptyMesh = 0;
    
    for (int dz = -rd; dz <= rd && uploaded < settings.maxChunksPerFrame; dz++) {
        for (int dx = -rd; dx <= rd && uploaded < settings.maxChunksPerFrame; dx++) {
            ChunkPos cpos{cx + dx, cz + dz};
            uint64_t key = posKey(cpos.x, 0, cpos.z);
            if (chunkMeshes.count(key)) continue;
            
            auto* chunk = world->getChunk(cpos);
            if (!chunk) { skipped++; continue; }
        
        std::vector<float> vertices;
        std::vector<uint32_t> indices;
        
        for (int y = CHUNK_MIN_Y; y < CHUNK_MIN_Y + CHUNK_HEIGHT; y++) {
            for (int z = 0; z < 16; z++) {
                for (int x = 0; x < 16; x++) {
                    auto block = chunk->getBlock(x, y, z);
                    if (block.isAir()) continue;
                    
                    uint32_t bid = block.getBlockId();
                    float fx = (float)x, fy = (float)y, fz = (float)z;
                    
                    auto checkNeighbor = [&](int nx, int ny, int nz) -> bool {
                        if (nx < 0 || nx >= 16 || nz < 0 || nz >= 16) return true;
                        if (ny < CHUNK_MIN_Y || ny >= CHUNK_MIN_Y + CHUNK_HEIGHT) return true;
                        auto nb = chunk->getBlock(nx, ny, nz);
                        return nb.isAir();
                    };
                    
                    auto addFace = [&](float x0,float y0,float z0, float x1,float y1,float z1,
                                       float x2,float y2,float z2, float x3,float y3,float z3, int face) {
                        uint32_t col = blockColor(bid, face);
                        uint32_t base = (uint32_t)(vertices.size() / 4);
                        vertices.insert(vertices.end(), {x0,y0,z0, *(float*)&col, x1,y1,z1, *(float*)&col, x2,y2,z2, *(float*)&col, x3,y3,z3, *(float*)&col});
                        indices.insert(indices.end(), {base, base+1, base+2, base, base+2, base+3});
                    };
                    
                    if (checkNeighbor(x, y+1, z))
                        addFace(fx,fy+1,fz, fx+1,fy+1,fz, fx+1,fy+1,fz+1, fx,fy+1,fz+1, 4);
                    if (checkNeighbor(x, y-1, z))
                        addFace(fx,fy,fz+1, fx+1,fy,fz+1, fx+1,fy,fz, fx,fy,fz, 5);
                    if (checkNeighbor(x, y, z-1))
                        addFace(fx,fy,fz, fx+1,fy,fz, fx+1,fy+1,fz, fx,fy+1,fz, 2);
                    if (checkNeighbor(x, y, z+1))
                        addFace(fx+1,fy,fz+1, fx,fy,fz+1, fx,fy+1,fz+1, fx+1,fy+1,fz+1, 3);
                    if (checkNeighbor(x-1, y, z))
                        addFace(fx,fy,fz+1, fx,fy,fz, fx,fy+1,fz, fx,fy+1,fz+1, 0);
                    if (checkNeighbor(x+1, y, z))
                        addFace(fx+1,fy,fz, fx+1,fy,fz+1, fx+1,fy+1,fz+1, fx+1,fy+1,fz, 1);
                }
            }
        }
        
        if (!vertices.empty()) {
            uploadChunkMesh(glm::ivec3(cpos.x, 0, cpos.z), vertices, indices);
            uploaded++;
            if (uploaded <= 4) {
                VF_INFO("Uploaded chunk ({},{}) : {} verts, {} indices",
                        cpos.x, cpos.z, vertices.size()/4, indices.size());
            }
        } else {
            emptyMesh++;
        }
        }
    }
    
    if (uploaded > 0 || (skipped > 0 && skipped <= 5)) {
        VF_INFO("Chunk mesh pass: uploaded={} skipped={} empty={} total={}",
                uploaded, skipped, emptyMesh, (int)chunkMeshes.size());
    }
}

void Renderer::renderWorldChunks(World* world, Camera* camera) {
    if (!chunkPipelineReady || chunkMeshes.empty()) return;
    
    static bool firstRenderLog = true;
    
    auto& cmd = commandBuffers[currentImageIndex];
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, chunkPipeline);
    
    glm::mat4 vp = camera->getViewProjectionMatrix();
    
    if (firstRenderLog) {
        auto pos = camera->getPosition();
        VF_INFO("First chunk render: cam=({:.1f},{:.1f},{:.1f}) meshes={}", pos.x, pos.y, pos.z, chunkMeshes.size());
        firstRenderLog = false;
    }
    
    for (auto& [key, mesh] : chunkMeshes) {
        if (!mesh.valid || mesh.indexCount == 0) continue;
        
        glm::vec3 offset(mesh.chunkPos.x * 16.0f, 0.0f, mesh.chunkPos.z * 16.0f);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), offset);
        glm::mat4 mvp = vp * model;
        
        cmd.pushConstants(chunkPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &mvp);
        
        vk::DeviceSize offset_v = 0;
        cmd.bindVertexBuffers(0, 1, &mesh.vertexBuffer, &offset_v);
        cmd.bindIndexBuffer(mesh.indexBuffer, 0, vk::IndexType::eUint32);
        cmd.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
        
        stats.chunksRendered++;
        stats.drawCalls++;
    }
}

void Renderer::invalidateChunkMesh(int chunkX, int chunkZ) {
    uint64_t key = posKey(chunkX, 0, chunkZ);
    auto it = chunkMeshes.find(key);
    if (it == chunkMeshes.end()) return;
    
    auto dev = context->getDevice();
    if (it->second.vertexBuffer) dev.destroyBuffer(it->second.vertexBuffer);
    if (it->second.vertexMemory) dev.freeMemory(it->second.vertexMemory);
    if (it->second.indexBuffer) dev.destroyBuffer(it->second.indexBuffer);
    if (it->second.indexMemory) dev.freeMemory(it->second.indexMemory);
    chunkMeshes.erase(it);
}

void Renderer::initUIRendering() {
    if (!context || !chunkPipelineReady) return;
    
    auto dev = context->getDevice();
    
    const char* uiVertSrc = R"(
        #version 450
        layout(location=0) in vec3 inPos;
        layout(location=1) in uint inColor;
        layout(push_constant) uniform PC { mat4 mvp; };
        layout(location=0) out vec4 vColor;
        void main() {
            gl_Position = mvp * vec4(inPos, 1.0);
            float r = float(inColor & 0xFFu) / 255.0;
            float g = float((inColor >> 8u) & 0xFFu) / 255.0;
            float b = float((inColor >> 16u) & 0xFFu) / 255.0;
            float a = float((inColor >> 24u) & 0xFFu) / 255.0;
            vColor = vec4(r, g, b, a);
        }
    )";
    
    const char* uiFragSrc = R"(
        #version 450
        layout(location=0) in vec4 vColor;
        layout(location=0) out vec4 outColor;
        void main() { outColor = vColor; }
    )";
    
    uiVertShader = compileShader(uiVertSrc, EShLangVertex);
    uiFragShader = compileShader(uiFragSrc, EShLangFragment);
    
    if (!uiVertShader || !uiFragShader) {
        VF_ERROR("Failed to compile UI shaders");
        return;
    }
    
    VulkanPipelineBuilder builder(dev);
    builder.setShaders(uiVertShader, uiFragShader);
    
    vk::VertexInputBindingDescription binding{0, 16, vk::VertexInputRate::eVertex};
    std::vector<vk::VertexInputAttributeDescription> attrs = {
        {0, 0, vk::Format::eR32G32B32Sfloat, 0},
        {1, 0, vk::Format::eR32Uint, 12}
    };
    builder.setVertexInput(binding, attrs);
    builder.setInputTopology(vk::PrimitiveTopology::eTriangleList);
    builder.setViewport(0, 0, (float)swapchainExtent.width, (float)swapchainExtent.height);
    builder.setScissor(0, 0, swapchainExtent.width, swapchainExtent.height);
    builder.setRasterizer(vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise);
    builder.setMultisampling(vk::SampleCountFlagBits::e1);
    builder.setDepthStencil(false, false, vk::CompareOp::eLess);
    // Alpha blending: src * srcAlpha + dst * (1 - srcAlpha)
    builder.setColorBlendAttachment(
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        true,
        vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
        vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd);
    builder.addPushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4));
    builder.setRenderPass(renderPass, 0);
    
    auto pipeline = builder.build();
    if (pipeline) {
        uiPipeline = pipeline.release();
        uiPipelineLayout = builder.getPipelineLayout();
        uiPipelineReady = true;
        VF_INFO("UI rendering pipeline created (alpha blending enabled)");
    } else {
        VF_ERROR("Failed to create UI rendering pipeline");
        return;
    }
    
    vk::BufferCreateInfo vbInfo{};
    vbInfo.size = uiVertexBufferSize;
    vbInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    vbInfo.sharingMode = vk::SharingMode::eExclusive;
    uiVertexBuffer = dev.createBuffer(vbInfo);
    
    auto vbMemReqs = dev.getBufferMemoryRequirements(uiVertexBuffer);
    vk::MemoryAllocateInfo vbAlloc{};
    vbAlloc.allocationSize = vbMemReqs.size;
    vbAlloc.memoryTypeIndex = context->findMemoryType(
        vbMemReqs.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    uiVertexMemory = dev.allocateMemory(vbAlloc);
    dev.bindBufferMemory(uiVertexBuffer, uiVertexMemory, 0);
    uiVertexMapped = dev.mapMemory(uiVertexMemory, 0, uiVertexBufferSize);
    
    vk::BufferCreateInfo ibInfo{};
    ibInfo.size = uiIndexBufferSize;
    ibInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
    ibInfo.sharingMode = vk::SharingMode::eExclusive;
    uiIndexBuffer = dev.createBuffer(ibInfo);
    
    auto ibMemReqs = dev.getBufferMemoryRequirements(uiIndexBuffer);
    vk::MemoryAllocateInfo ibAlloc{};
    ibAlloc.allocationSize = ibMemReqs.size;
    ibAlloc.memoryTypeIndex = context->findMemoryType(
        ibMemReqs.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    uiIndexMemory = dev.allocateMemory(ibAlloc);
    dev.bindBufferMemory(uiIndexBuffer, uiIndexMemory, 0);
    uiIndexMapped = dev.mapMemory(uiIndexMemory, 0, uiIndexBufferSize);
    
    VF_INFO("UI host-visible buffers allocated and mapped (64KB each)");
}

void Renderer::cleanupUIRendering() {
    if (!context) return;
    auto dev = context->getDevice();
    
    if (uiVertexMapped) { dev.unmapMemory(uiVertexMemory); uiVertexMapped = nullptr; }
    if (uiIndexMapped)  { dev.unmapMemory(uiIndexMemory); uiIndexMapped = nullptr; }
    
    if (uiVertexBuffer) { dev.destroyBuffer(uiVertexBuffer); uiVertexBuffer = VK_NULL_HANDLE; }
    if (uiVertexMemory) { dev.freeMemory(uiVertexMemory); uiVertexMemory = VK_NULL_HANDLE; }
    if (uiIndexBuffer)  { dev.destroyBuffer(uiIndexBuffer); uiIndexBuffer = VK_NULL_HANDLE; }
    if (uiIndexMemory)  { dev.freeMemory(uiIndexMemory); uiIndexMemory = VK_NULL_HANDLE; }
    
    if (uiPipeline)      { dev.destroyPipeline(uiPipeline); uiPipeline = VK_NULL_HANDLE; }
    if (uiPipelineLayout){ dev.destroyPipelineLayout(uiPipelineLayout); uiPipelineLayout = VK_NULL_HANDLE; }
    if (uiVertShader)    { dev.destroyShaderModule(uiVertShader); uiVertShader = VK_NULL_HANDLE; }
    if (uiFragShader)    { dev.destroyShaderModule(uiFragShader); uiFragShader = VK_NULL_HANDLE; }
    
    uiPipelineReady = false;
    VF_INFO("UI rendering resources cleaned up");
}

void Renderer::drawCrosshair() {
    if (!uiPipelineReady || !uiVertexMapped || !uiIndexMapped) return;
    auto& cmd = commandBuffers[currentImageIndex];
    
    float size = 0.015f;
    float thickness = 0.003f;
    
    struct UIVert { float x, y, z; uint32_t color; };
    uint32_t white = 0xFFFFFFFF;
    
    UIVert verts[12];
    uint32_t indices[18];
    uint32_t v = 0, idx = 0;
    
    auto addRect = [&](float x0, float y0, float x1, float y1, uint32_t col) {
        uint32_t base = v;
        verts[v++] = {x0, y0, 0.0f, col};
        verts[v++] = {x1, y0, 0.0f, col};
        verts[v++] = {x1, y1, 0.0f, col};
        verts[v++] = {x0, y1, 0.0f, col};
        indices[idx++] = base; indices[idx++] = base+1; indices[idx++] = base+2;
        indices[idx++] = base; indices[idx++] = base+2; indices[idx++] = base+3;
    };
    
    addRect(-size, -thickness, size, thickness, white);
    addRect(-thickness, -size, thickness, -thickness, white);
    addRect(-thickness, thickness, thickness, size, white);
    
    memcpy(uiVertexMapped, verts, v * sizeof(UIVert));
    memcpy(uiIndexMapped, indices, idx * sizeof(uint32_t));
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, uiPipeline);
    glm::mat4 identity(1.0f);
    cmd.pushConstants(uiPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &identity);
    vk::DeviceSize off = 0;
    cmd.bindVertexBuffers(0, 1, &uiVertexBuffer, &off);
    cmd.bindIndexBuffer(uiIndexBuffer, 0, vk::IndexType::eUint32);
    cmd.drawIndexed(idx, 1, 0, 0, 0);
}

void Renderer::drawHotbar(int selectedSlot, const std::array<uint32_t, 9>& hotbarBlocks) {
    if (!uiPipelineReady || !uiVertexMapped || !uiIndexMapped) return;
    auto& cmd = commandBuffers[currentImageIndex];
    
    float slotSize = 0.04f;
    float padding = 0.004f;
    float totalWidth = 9.0f * slotSize + 8.0f * padding;
    float startX = -totalWidth * 0.5f;
    float startY = -0.95f;
    
    struct UIVert { float x, y, z; uint32_t color; };
    
    UIVert verts[72];
    uint32_t indices[108];
    uint32_t v = 0, idx = 0;
    
    auto addRect = [&](float x0, float y0, float x1, float y1, uint32_t col) {
        uint32_t base = v;
        verts[v++] = {x0, y0, 0.0f, col};
        verts[v++] = {x1, y0, 0.0f, col};
        verts[v++] = {x1, y1, 0.0f, col};
        verts[v++] = {x0, y1, 0.0f, col};
        indices[idx++] = base; indices[idx++] = base+1; indices[idx++] = base+2;
        indices[idx++] = base; indices[idx++] = base+2; indices[idx++] = base+3;
    };
    
    for (int i = 0; i < 9; i++) {
        float x = startX + i * (slotSize + padding);
        uint32_t slotBg = (i == selectedSlot) ? 0xCCFFFFFF : 0x88000000;
        addRect(x, startY, x + slotSize, startY + slotSize, slotBg);
        
        if (hotbarBlocks[i] != 0) {
            uint32_t blockCol = blockColor(hotbarBlocks[i], 4);
            blockCol = (blockCol & 0x00FFFFFF) | 0xDD000000;
            float inset = slotSize * 0.15f;
            addRect(x + inset, startY + inset, x + slotSize - inset, startY + slotSize - inset, blockCol);
        }
    }
    
    memcpy(uiVertexMapped, verts, v * sizeof(UIVert));
    memcpy(uiIndexMapped, indices, idx * sizeof(uint32_t));
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, uiPipeline);
    glm::mat4 identity(1.0f);
    cmd.pushConstants(uiPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &identity);
    vk::DeviceSize off = 0;
    cmd.bindVertexBuffers(0, 1, &uiVertexBuffer, &off);
    cmd.bindIndexBuffer(uiIndexBuffer, 0, vk::IndexType::eUint32);
    cmd.drawIndexed(idx, 1, 0, 0, 0);
}

void Renderer::drawPauseMenu(int selection, float sensitivity, bool invertY, bool invertX, bool flyMode) {
    if (!uiPipelineReady || !uiVertexMapped || !uiIndexMapped) return;
    auto& cmd = commandBuffers[currentImageIndex];
    
    struct UIVert { float x, y, z; uint32_t color; };
    
    const int MAX_MENU_VERTS = 512;
    const int MAX_MENU_INDICES = 768;
    UIVert verts[MAX_MENU_VERTS];
    uint32_t indices[MAX_MENU_INDICES];
    uint32_t v = 0, idx = 0;
    
    auto addRect = [&](float x0, float y0, float x1, float y1, uint32_t col) {
        if (v + 4 > MAX_MENU_VERTS || idx + 6 > MAX_MENU_INDICES) return;
        uint32_t base = v;
        verts[v++] = {x0, y0, 0.0f, col};
        verts[v++] = {x1, y0, 0.0f, col};
        verts[v++] = {x1, y1, 0.0f, col};
        verts[v++] = {x0, y1, 0.0f, col};
        indices[idx++] = base; indices[idx++] = base+1; indices[idx++] = base+2;
        indices[idx++] = base; indices[idx++] = base+2; indices[idx++] = base+3;
    };
    
    uint32_t bgDim = 0xCC000000;
    uint32_t panelBg = 0xDD1a1a2e;
    uint32_t selectedBg = 0xFFe94560;
    uint32_t normalBg = 0xFF16213e;
    uint32_t barBg = 0xFF0f3460;
    uint32_t barFill = 0xFFe94560;
    uint32_t onColor = 0xFF4ecca3;
    uint32_t offColor = 0xFF6c757d;
    
    addRect(-1.0f, -1.0f, 1.0f, 1.0f, bgDim);
    
    float panelW = 0.7f;
    float panelH = 0.75f;
    addRect(-panelW, -panelH, panelW, panelH, panelBg);
    
    float itemY = 0.55f;
    float itemH = 0.09f;
    float itemW = 0.6f;
    float gap = 0.02f;
    
    struct MenuItem {
        const char* label;
        int type; // 0=action, 1=slider, 2=toggle
        float value;
        bool on;
    };
    
    MenuItem items[] = {
        {"RESUME", 0, 0, false},
        {"MOUSE SENSITIVITY", 1, sensitivity, false},
        {"INVERT MOUSE Y", 2, 0, invertY},
        {"INVERT MOUSE X", 2, 0, invertX},
        {"TOGGLE FLY MODE", 0, 0, flyMode},
    };
    int itemCount = 5;
    
    for (int i = 0; i < itemCount; i++) {
        float y = itemY - i * (itemH + gap);
        uint32_t bg = (i == selection) ? selectedBg : normalBg;
        addRect(-itemW, y - itemH, itemW, y, bg);
        
        if (items[i].type == 1) {
            float barMargin = 0.05f;
            float barH = itemH * 0.3f;
            float barY = y - itemH * 0.65f;
            addRect(-itemW + barMargin, barY - barH, itemW - barMargin, barY, barBg);
            float fillW = (itemW - barMargin) * 2.0f * items[i].value;
            addRect(-itemW + barMargin, barY - barH, -itemW + barMargin + fillW, barY, barFill);
        } else if (items[i].type == 2) {
            float indSize = itemH * 0.3f;
            float indX = itemW - 0.08f;
            float indY = y - itemH * 0.5f;
            addRect(indX - indSize, indY - indSize, indX + indSize, indY + indSize,
                    items[i].on ? onColor : offColor);
        }
    }
    
    memcpy(uiVertexMapped, verts, v * sizeof(UIVert));
    memcpy(uiIndexMapped, indices, idx * sizeof(uint32_t));
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, uiPipeline);
    glm::mat4 identity(1.0f);
    cmd.pushConstants(uiPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &identity);
    vk::DeviceSize off = 0;
    cmd.bindVertexBuffers(0, 1, &uiVertexBuffer, &off);
    cmd.bindIndexBuffer(uiIndexBuffer, 0, vk::IndexType::eUint32);
    cmd.drawIndexed(idx, 1, 0, 0, 0);
}

} // namespace VoxelForge
