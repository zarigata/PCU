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

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <stb_easy_font.h>

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
    
    // Create depth buffer (must be before framebuffers since they reference depthImageView)
    createDepthResources();
    
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
    
    // Cleanup depth buffer
    if (depthImageView) { context->getDevice().destroyImageView(depthImageView); depthImageView = vk::ImageView{}; }
    if (depthImage) { context->getDevice().destroyImage(depthImage); depthImage = vk::Image{}; }
    if (depthImageMemory) { context->getDevice().freeMemory(depthImageMemory); depthImageMemory = vk::DeviceMemory{}; }
    
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
    
    // Depth attachment reference
    vk::AttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    
    // Subpass
    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;
    
    // Subpass dependency
    vk::SubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcAccessMask = vk::AccessFlags{};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    
    // Create render pass
    vk::AttachmentDescription attachments[] = {colorAttachment, depthAttachment};
    
    vk::RenderPassCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eRenderPassCreateInfo;
    createInfo.attachmentCount = 2;
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

void Renderer::createDepthResources() {
    depthFormat = context->findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
    
    vk::ImageCreateInfo imageInfo{};
    imageInfo.sType = vk::StructureType::eImageCreateInfo;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.format = depthFormat;
    imageInfo.extent = vk::Extent3D{swapchainExtent.width, swapchainExtent.height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    
    depthImage = context->getDevice().createImage(imageInfo);
    
    vk::MemoryRequirements memReqs = context->getDevice().getImageMemoryRequirements(depthImage);
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = context->findMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    
    depthImageMemory = context->getDevice().allocateMemory(allocInfo);
    context->getDevice().bindImageMemory(depthImage, depthImageMemory, 0);
    
    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
    viewInfo.image = depthImage;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = (depthFormat == vk::Format::eD32Sfloat)
        ? vk::ImageAspectFlagBits::eDepth
        : vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    depthImageView = context->getDevice().createImageView(viewInfo);
    
    VF_INFO("Depth buffer created: {}x{}, format={}", swapchainExtent.width, swapchainExtent.height, (int)depthFormat);
}

void Renderer::createFramebuffers() {
    VF_PROFILE_FUNCTION();
    
    framebuffers.resize(swapchainImageViews.size());
    
    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        vk::ImageView attachments[] = {swapchainImageViews[i], depthImageView};
        
        vk::FramebufferCreateInfo createInfo{};
        createInfo.sType = vk::StructureType::eFramebufferCreateInfo;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 2;
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
    std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0].color = vk::ClearColorValue{clearColorValue[0], clearColorValue[1], clearColorValue[2], clearColorValue[3]};
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
    
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues.data();
    
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
    if (depthImageView) { context->getDevice().destroyImageView(depthImageView); depthImageView = vk::ImageView{}; }
    if (depthImage) { context->getDevice().destroyImage(depthImage); depthImage = vk::Image{}; }
    if (depthImageMemory) { context->getDevice().freeMemory(depthImageMemory); depthImageMemory = vk::DeviceMemory{}; }
    if (swapchain) {
        context->getDevice().destroySwapchainKHR(swapchain);
    }
    
    // Create new swapchain
    createSwapChain();
    createRenderPass();
    createDepthResources();
    createFramebuffers();
    createCommandBuffers();
}

void Renderer::takeScreenshot(const std::string& path) {
    VF_PROFILE_FUNCTION();
    
    auto dev = context->getDevice();
    
    // Wait for rendering to finish
    dev.waitIdle();
    
    // Get the swapchain image
    vk::Image srcImage = swapchainImages[currentImageIndex];
    
    // Create a host-visible destination image
    vk::ImageCreateInfo dstImageInfo{};
    dstImageInfo.sType = vk::StructureType::eImageCreateInfo;
    dstImageInfo.imageType = vk::ImageType::e2D;
    dstImageInfo.format = vk::Format::eR8G8B8A8Unorm;
    dstImageInfo.extent = vk::Extent3D{swapchainExtent.width, swapchainExtent.height, 1};
    dstImageInfo.mipLevels = 1;
    dstImageInfo.arrayLayers = 1;
    dstImageInfo.samples = vk::SampleCountFlagBits::e1;
    dstImageInfo.tiling = vk::ImageTiling::eLinear;
    dstImageInfo.usage = vk::ImageUsageFlagBits::eTransferDst;
    dstImageInfo.sharingMode = vk::SharingMode::eExclusive;
    dstImageInfo.initialLayout = vk::ImageLayout::eUndefined;
    
    vk::Image dstImage;
    vk::DeviceMemory dstMemory;
    try {
        dstImage = dev.createImage(dstImageInfo);
        auto memReqs = dev.getImageMemoryRequirements(dstImage);
        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = context->findMemoryType(
            memReqs.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        dstMemory = dev.allocateMemory(allocInfo);
        dev.bindImageMemory(dstImage, dstMemory, 0);
    } catch (const vk::SystemError& e) {
        VF_ERROR("Screenshot: failed to create dest image: {}", e.what());
        return;
    }
    
    // Create a one-shot command buffer for the copy
    vk::CommandBufferAllocateInfo cmdAlloc{};
    cmdAlloc.sType = vk::StructureType::eCommandBufferAllocateInfo;
    cmdAlloc.commandPool = commandPool;
    cmdAlloc.level = vk::CommandBufferLevel::ePrimary;
    cmdAlloc.commandBufferCount = 1;
    auto cmdBuf = dev.allocateCommandBuffers(cmdAlloc)[0];
    
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmdBuf.begin(beginInfo);
    
    // Transition source (swapchain) image for transfer
    vk::ImageMemoryBarrier srcBarrier{};
    srcBarrier.sType = vk::StructureType::eImageMemoryBarrier;
    srcBarrier.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    srcBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
    srcBarrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
    srcBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    srcBarrier.image = srcImage;
    srcBarrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlags{}, 0, nullptr, 0, nullptr, 1, &srcBarrier);
    
    // Transition dest image for transfer write
    vk::ImageMemoryBarrier dstBarrier{};
    dstBarrier.sType = vk::StructureType::eImageMemoryBarrier;
    dstBarrier.srcAccessMask = vk::AccessFlags{};
    dstBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    dstBarrier.oldLayout = vk::ImageLayout::eUndefined;
    dstBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
    dstBarrier.image = dstImage;
    dstBarrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlags{}, 0, nullptr, 0, nullptr, 1, &dstBarrier);
    
    // Copy image (may need format conversion — B8G8R8A8 → R8G8B8A8)
    vk::ImageCopy copyRegion{};
    copyRegion.srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
    copyRegion.srcOffset = vk::Offset3D{0, 0, 0};
    copyRegion.dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
    copyRegion.dstOffset = vk::Offset3D{0, 0, 0};
    copyRegion.extent = vk::Extent3D{swapchainExtent.width, swapchainExtent.height, 1};
    cmdBuf.copyImage(srcImage, vk::ImageLayout::eTransferSrcOptimal,
                     dstImage, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);
    
    // Transition dest to general for reading
    vk::ImageMemoryBarrier readBarrier{};
    readBarrier.sType = vk::StructureType::eImageMemoryBarrier;
    readBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    readBarrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
    readBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    readBarrier.newLayout = vk::ImageLayout::eGeneral;
    readBarrier.image = dstImage;
    readBarrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eHost,
        vk::DependencyFlags{}, 0, nullptr, 0, nullptr, 1, &readBarrier);
    
    // Transition source back to present
    srcBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
    srcBarrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
    srcBarrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
    srcBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlags{}, 0, nullptr, 0, nullptr, 1, &srcBarrier);
    
    cmdBuf.end();
    
    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;
    context->getGraphicsQueue().submit(1, &submitInfo, vk::Fence{});
    context->getGraphicsQueue().waitIdle();
    
    // Read back pixel data
    auto imgReqs = dev.getImageMemoryRequirements(dstImage);
    void* mapped = dev.mapMemory(dstMemory, 0, imgReqs.size);
    
    auto subRes = dev.getImageSubresourceLayout(dstImage, {vk::ImageAspectFlagBits::eColor, 0, 0});
    auto* pixels = static_cast<unsigned char*>(mapped) + subRes.offset;
    
    // Convert BGR to RGB and strip alpha, write PNG
    int w = (int)swapchainExtent.width;
    int h = (int)swapchainExtent.height;
    std::vector<unsigned char> rgb(w * h * 3);
    
    for (int row = 0; row < h; row++) {
        auto* srcRow = pixels + row * subRes.rowPitch;
        auto* dstRow = rgb.data() + row * w * 3;
        for (int col = 0; col < w; col++) {
            dstRow[col * 3 + 0] = srcRow[col * 4 + 2]; // R from B
            dstRow[col * 3 + 1] = srcRow[col * 4 + 1]; // G
            dstRow[col * 3 + 2] = srcRow[col * 4 + 0]; // B from R
        }
    }
    
    dev.unmapMemory(dstMemory);
    
    stbi_write_png(path.c_str(), w, h, 3, rgb.data(), w * 3);
    
    // Cleanup
    dev.freeCommandBuffers(commandPool, 1, &cmdBuf);
    dev.destroyImage(dstImage);
    dev.freeMemory(dstMemory);
    
    VF_INFO("Screenshot saved: {} ({}x{})", path, w, h);
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
    
    if (settings.enableVsync) {
        bool hasRelaxed = false;
        for (const auto& mode : availablePresentModes) {
            if (mode == vk::PresentModeKHR::eFifoRelaxed) return mode;
            if (mode == vk::PresentModeKHR::eFifo) hasRelaxed = true;
        }
        if (hasRelaxed) return vk::PresentModeKHR::eFifo;
    }
    
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }
    
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
        layout(push_constant) uniform PC {
            mat4 mvp;
            vec4 fogColor;
            vec4 fogDist;
        };
        layout(location=0) out vec4 vColor;
        layout(location=1) out float vFog;
        void main() {
            gl_Position = mvp * vec4(inPos, 1.0);
            float r = float(inColor & 0xFFu) / 255.0;
            float g = float((inColor >> 8u) & 0xFFu) / 255.0;
            float b = float((inColor >> 16u) & 0xFFu) / 255.0;
            vColor = vec4(r, g, b, 1.0);
            float d = gl_Position.w;
            vFog = clamp((d - fogDist.x) / (fogDist.y - fogDist.x), 0.0, 1.0);
        }
    )";
    
    const char* fragSrc = R"(
        #version 450
        layout(location=0) in vec4 vColor;
        layout(location=1) in float vFog;
        layout(push_constant) uniform PC {
            mat4 mvp;
            vec4 fogColor;
            vec4 fogDist;
        };
        layout(location=0) out vec4 outColor;
        void main() {
            outColor = mix(vColor, fogColor, vFog * vFog);
        }
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
    builder.setRasterizer(vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise);
    builder.setMultisampling(vk::SampleCountFlagBits::e1);
    builder.setDepthStencil(true, true, vk::CompareOp::eLess);
    builder.setColorBlendAttachment(
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    builder.addPushConstantRange(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, 96);
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

static unsigned int posHash(int x, int y, int z) {
    unsigned int h = (unsigned int)(x * 374761393u + y * 668265263u + z * 1274126177u);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static int colorNoise(int x, int y, int z, int range) {
    return (int)(posHash(x, y, z) % (unsigned int)(range * 2 + 1)) - range;
}

static uint32_t blockColor(uint32_t blockId, int face, int wx, int wy, int wz) {
    int n = colorNoise(wx, wy, wz, 10);
    uint32_t base;
    switch (blockId) {
        case 1: base = packColor(128+n, 128+n, 128+n); break;
        case 2: base = face == 4
                        ? packColor(70+n, 168+n/2, 60+n)
                        : packColor(134+n, 116+n/2, 40+n); break;
        case 3: base = packColor(134+n, 86+n/2, 40+n); break;
        case 4: base = packColor(108+n, 108+n, 108+n); break;
        case 5: base = packColor(178+n, 138+n/2, 78+n); break;
        case 6: base = packColor(94+n, 66+n/2, 36+n); break;
        case 7: base = packColor(184+n, 170+n/2, 130+n); break;
        case 8: base = packColor(70+n, 50+n/2, 30+n); break;
        case 9: base = packColor(60+n, 42+n/2, 26+n); break;
        case 10: base = packColor(86+n, 58+n/2, 32+n); break;
        case 11: base = packColor(42+n/3, 130+n/2, 200); break;
        case 12: base = packColor(210+n/2, 70+n/3, 15); break;
        case 13: base = packColor(220+n, 206+n/2, 150+n); break;
        case 14: base = packColor(8, 8, 8); break;
        case 15: base = packColor(156+n, 140+n/2, 126+n); break;
        case 16: base = packColor(70+n, 70+n, 70+n); break;
        case 17: base = packColor(240+n/3, 200+n/3, 20+n/3); break;
        case 18: base = packColor(70+n/3, 210+n/3, 220); break;
        case 19: base = packColor(160+n, 120+n/2, 60+n); break;
        case 20: base = packColor(140+n, 140+n, 140+n); break;
        case 21: base = packColor(140+n, 110+n/2, 60+n); break;
        case 22: base = packColor(220+n, 190+n/2, 50+n); break;
        case 23: base = packColor(200, 200, 210); break;
        case 24: base = packColor(148+n, 130+n/2, 116+n); break;
        case 25: base = packColor(160+n, 142+n/2, 128+n); break;
        case 26: base = packColor(180+n, 178+n/2, 178+n); break;
        case 27: base = packColor(190+n, 186+n/2, 182+n); break;
        case 28: base = packColor(138+n, 128+n/2, 120+n); break;
        case 29: base = packColor(150+n, 142+n/2, 134+n); break;
        case 30: base = packColor(80+n/2, 76+n/2, 76+n/2); break;
        case 31: base = packColor(86+n, 82+n/2, 80+n); break;
        case 32: base = packColor(100+n, 98+n/2, 94+n); break;
        case 33: case 34: case 35: case 36:
            base = packColor(118+n, 114+n/2, 108+n); break;
        case 37: base = packColor(156+n, 80+n/2, 50+n); break;
        case 38: base = packColor(156+n, 120+n/2, 80+n); break;
        case 39: base = packColor(100+n, 80+n/2, 60+n); break;
        case 40: base = packColor(60+n, 140+n/2, 50+n); break;
        case 41: base = packColor(80+n, 60+n/2, 50+n); break;
        case 42: base = packColor(140+n, 30+n/3, 30+n/3); break;
        case 43: base = packColor(110+n, 30+n/3, 30+n/3); break;
        case 44: base = packColor(40+n/3, 50+n/2, 130); break;
        case 45: base = packColor(30+n/3, 40+n/2, 100); break;
        case 46: base = packColor(30+n/2, 30+n/2, 30+n/2); break;
        case 47: base = packColor(210+n/3, 210+n/3, 210+n/3); break;
        case 48: base = packColor(240+n/3, 210+n/3, 40+n/3); break;
        case 49: base = packColor(80+n/3, 220+n/3, 230+n/3); break;
        case 50: base = packColor(40+n/3, 180+n/3, 50+n/3); break;
        case 51: base = packColor(190+n/3, 120+n/3, 60+n/3); break;
        case 52: base = packColor(30+n/3, 50+n/2, 140); break;
        case 53: base = packColor(160+n, 20+n/3, 20+n/3); break;
        case 54: base = packColor(98+n, 70+n/2, 40+n); break;
        case 55: base = packColor(200+n/3, 190+n/3, 150+n/3); break;
        case 56: base = packColor(120+n, 90+n/2, 50+n); break;
        case 57: base = packColor(178+n, 120+n/2, 56+n); break;
        case 58: base = packColor(60+n, 40+n/2, 24+n); break;
        case 59: base = packColor(110+n, 80+n/2, 45+n); break;
        case 60: base = packColor(210+n/3, 170+n/3, 140+n/3); break;
        case 61: base = packColor(190+n/3, 160+n/3, 110+n/3); break;
        case 62: base = packColor(70+n/2, 30+n/2, 70+n/2); break;
        case 63: base = packColor(30+n/2, 120+n/2, 110+n/2); break;
        case 64: case 65: case 66: case 67: case 68: case 69: case 70: case 71: {
            int ln = colorNoise(wx, 0, wz, 15);
            int g = 80 + (blockId - 64) * 8 + ln;
            if (g > 200) g = 200;
            base = packColor(30 + ln/3, g, 30 + ln/3);
            break;
        }
        case 72: base = packColor(130+n, 120+n/2, 110+n); break;
        case 73: base = packColor(120+n, 100+n/2, 70+n); break;
        case 74: base = packColor(100+n, 130+n/2, 60+n); break;
        case 75: base = packColor(110+n, 90+n/2, 60+n); break;
        case 76: base = packColor(80+n, 130+n/2, 70+n); break;
        case 77: base = packColor(90+n, 85+n/2, 80+n); break;
        case 78: base = packColor(200+n, 160+n/2, 100+n); break;
        case 79: base = packColor(210+n, 195+n/2, 150+n); break;
        case 80: base = packColor(180+n, 110+n/2, 70+n); break;
        case 81: base = packColor(110+n, 50+n/2, 50+n); break;
        case 82: base = packColor(110+n, 90+n/2, 70+n); break;
        case 83: base = packColor(100+n, 80+n/2, 60+n); break;
        case 84: base = packColor(80+n, 75+n/2, 80+n); break;
        case 85: base = packColor(50+n, 50+n/2, 50+n); break;
        case 86: base = packColor(190+n, 170+n/2, 50+n); break;
        case 87: base = packColor(220+n/3, 220+n/3, 210+n/3); break;
        case 88: base = packColor(20+n/3, 15+n/3, 25+n/3); break;
        case 89: base = packColor(50+n/3, 10+n/3, 60+n/3); break;
        case 128: base = packColor(230+n/3, 235+n/3, 240+n/3); break;
        case 129: base = packColor(160+n, 200+n/2, 230); break;
        case 130: base = packColor(170+n, 200+n/2, 220); break;
        case 131: base = packColor(130+n, 170+n/2, 210); break;
        case 132: base = packColor(160+n, 155+n/2, 140+n); break;
        case 133: base = packColor(180+n, 180+n/2, 160+n); break;
        default: base = packColor(130+n, 130+n, 130+n); break;
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
                        uint32_t col = blockColor(bid, face, (int)fx, (int)fy, (int)fz);
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

    evictDistantMeshes(cameraPos);
}

void Renderer::renderWorldChunks(World* world, Camera* camera, float fogR, float fogG, float fogB) {
    if (!chunkPipelineReady || chunkMeshes.empty()) return;
    
    auto& cmd = commandBuffers[currentImageIndex];
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, chunkPipeline);
    
    glm::mat4 vp = camera->getViewProjectionMatrix();

    float nearPlane = 0.1f;
    float farPlane = (float)(settings.renderDistance * 16) * 1.414f;
    float fogStart = farPlane * 0.55f;
    float fogEnd = farPlane * 0.95f;

    struct PushData {
        glm::mat4 mvp;
        glm::vec4 fogColor;
        glm::vec4 fogDist;
    };

    for (auto& [key, mesh] : chunkMeshes) {
        if (!mesh.valid || mesh.indexCount == 0) continue;
        
        glm::vec3 offset(mesh.chunkPos.x * 16.0f, 0.0f, mesh.chunkPos.z * 16.0f);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), offset);
        glm::mat4 mvp = vp * model;
        
        PushData pc;
        pc.mvp = mvp;
        pc.fogColor = glm::vec4(fogR, fogG, fogB, 1.0f);
        pc.fogDist = glm::vec4(fogStart, fogEnd, 0.0f, 0.0f);
        cmd.pushConstants(chunkPipelineLayout,
                          vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                          0, sizeof(PushData), &pc);
        
        vk::DeviceSize offset_v = 0;
        cmd.bindVertexBuffers(0, 1, &mesh.vertexBuffer, &offset_v);
        cmd.bindIndexBuffer(mesh.indexBuffer, 0, vk::IndexType::eUint32);
        cmd.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
        
        stats.chunksRendered++;
        stats.drawCalls++;
    }
}

void Renderer::evictDistantMeshes(const glm::vec3& cameraPos) {
    if (!context) return;
    auto dev = context->getDevice();

    int cx = (int)floor(cameraPos.x / 16.0f);
    int cz = (int)floor(cameraPos.z / 16.0f);
    int evictDist = settings.renderDistance + 3;

    std::vector<uint64_t> toEvict;
    for (auto& [key, mesh] : chunkMeshes) {
        int dx = mesh.chunkPos.x - cx;
        int dz = mesh.chunkPos.z - cz;
        if (dx * dx + dz * dz > evictDist * evictDist) {
            toEvict.push_back(key);
        }
    }

    for (auto key : toEvict) {
        auto it = chunkMeshes.find(key);
        if (it == chunkMeshes.end()) continue;
        if (it->second.vertexBuffer) dev.destroyBuffer(it->second.vertexBuffer);
        if (it->second.vertexMemory) dev.freeMemory(it->second.vertexMemory);
        if (it->second.indexBuffer) dev.destroyBuffer(it->second.indexBuffer);
        if (it->second.indexMemory) dev.freeMemory(it->second.indexMemory);
        chunkMeshes.erase(it);
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

void Renderer::resetUIBatch() {
    uiBatchV = 0;
    uiBatchI = 0;
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
    
    auto* dstVerts = reinterpret_cast<UIVert*>((char*)uiVertexMapped + uiBatchV * sizeof(UIVert));
    auto* dstIdx = (uint32_t*)uiIndexMapped + uiBatchI;
    memcpy(dstVerts, verts, v * sizeof(UIVert));
    memcpy(dstIdx, indices, idx * sizeof(uint32_t));
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, uiPipeline);
    glm::mat4 uiMvp(1.0f);
    uiMvp[1][1] = -1.0f;
    cmd.pushConstants(uiPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &uiMvp);
    vk::DeviceSize voff = uiBatchV * sizeof(UIVert);
    cmd.bindVertexBuffers(0, 1, &uiVertexBuffer, &voff);
    cmd.bindIndexBuffer(uiIndexBuffer, 0, vk::IndexType::eUint32);
    cmd.drawIndexed(idx, 1, uiBatchI, 0, 0);
    uiBatchV += v;
    uiBatchI += idx;
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
            uint32_t blockCol = blockColor(hotbarBlocks[i], 4, 0, 0, 0);
            blockCol = (blockCol & 0x00FFFFFF) | 0xDD000000;
            float inset = slotSize * 0.15f;
            addRect(x + inset, startY + inset, x + slotSize - inset, startY + slotSize - inset, blockCol);
        }
    }
    
    auto* dstVerts = reinterpret_cast<UIVert*>((char*)uiVertexMapped + uiBatchV * sizeof(UIVert));
    auto* dstIdx = (uint32_t*)uiIndexMapped + uiBatchI;
    memcpy(dstVerts, verts, v * sizeof(UIVert));
    memcpy(dstIdx, indices, idx * sizeof(uint32_t));
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, uiPipeline);
    glm::mat4 uiMvp(1.0f);
    uiMvp[1][1] = -1.0f;
    cmd.pushConstants(uiPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &uiMvp);
    vk::DeviceSize voff = uiBatchV * sizeof(UIVert);
    cmd.bindVertexBuffers(0, 1, &uiVertexBuffer, &voff);
    cmd.bindIndexBuffer(uiIndexBuffer, 0, vk::IndexType::eUint32);
    cmd.drawIndexed(idx, 1, uiBatchI, 0, 0);
    uiBatchV += v;
    uiBatchI += idx;
}

void Renderer::drawClickToPlay() {
    if (!uiPipelineReady || !uiVertexMapped || !uiIndexMapped) return;
    
    struct UIVert { float x, y, z; uint32_t color; };
    auto& cmd = commandBuffers[currentImageIndex];
    
    UIVert verts[64];
    uint32_t indices[96];
    uint32_t v = 0, idx = 0;
    
    auto addRect = [&](float x0, float y0, float x1, float y1, uint32_t col) {
        if (v + 4 > 64 || idx + 6 > 96) return;
        uint32_t base = v;
        verts[v++] = {x0, y0, 0.0f, col};
        verts[v++] = {x1, y0, 0.0f, col};
        verts[v++] = {x1, y1, 0.0f, col};
        verts[v++] = {x0, y1, 0.0f, col};
        indices[idx++] = base; indices[idx++] = base+1; indices[idx++] = base+2;
        indices[idx++] = base; indices[idx++] = base+2; indices[idx++] = base+3;
    };
    
    addRect(-1.0f, -1.0f, 1.0f, 1.0f, 0x88000000);
    addRect(-0.35f, -0.08f, 0.35f, 0.08f, 0xDD202030);
    
    auto* dstV = reinterpret_cast<UIVert*>((char*)uiVertexMapped + uiBatchV * sizeof(UIVert));
    auto* dstI = (uint32_t*)uiIndexMapped + uiBatchI;
    memcpy(dstV, verts, v * sizeof(UIVert));
    memcpy(dstI, indices, idx * sizeof(uint32_t));
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, uiPipeline);
    glm::mat4 uiMvp(1.0f);
    uiMvp[1][1] = -1.0f;
    cmd.pushConstants(uiPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &uiMvp);
    vk::DeviceSize voff = uiBatchV * sizeof(UIVert);
    cmd.bindVertexBuffers(0, 1, &uiVertexBuffer, &voff);
    cmd.bindIndexBuffer(uiIndexBuffer, 0, vk::IndexType::eUint32);
    cmd.drawIndexed(idx, 1, uiBatchI, 0, 0);
    uiBatchV += v;
    uiBatchI += idx;
    
    float pxW = (float)swapchainExtent.width;
    float pxH = (float)swapchainExtent.height;
    float textX = (-0.28f + 1.0f) * pxW / (2.0f * 2.0f);
    float textY = (1.0f - 0.03f) * pxH / (2.0f * 2.0f);
    drawText("CLICK TO PLAY", textX, textY, 0xFFFFFFFF, 2.0f);
}

void Renderer::drawPauseMenu(const UIMenuState& menu) {
    if (!uiPipelineReady || !uiVertexMapped || !uiIndexMapped) return;
    auto& cmd = commandBuffers[currentImageIndex];
    
    struct UIVert { float x, y, z; uint32_t color; };
    
    constexpr int MAX_VERTS = 1024;
    constexpr int MAX_INDICES = 1536;
    UIVert verts[MAX_VERTS];
    uint32_t indices[MAX_INDICES];
    uint32_t v = 0, idx = 0;
    
    auto addRect = [&](float x0, float y0, float x1, float y1, uint32_t col) {
        if (v + 4 > MAX_VERTS || idx + 6 > MAX_INDICES) return;
        uint32_t base = v;
        verts[v++] = {x0, y0, 0.0f, col};
        verts[v++] = {x1, y0, 0.0f, col};
        verts[v++] = {x1, y1, 0.0f, col};
        verts[v++] = {x0, y1, 0.0f, col};
        indices[idx++] = base; indices[idx++] = base+1; indices[idx++] = base+2;
        indices[idx++] = base; indices[idx++] = base+2; indices[idx++] = base+3;
    };
    
    uint32_t bgDim = 0xCC000000;
    uint32_t panelBg = 0xDD181828;
    uint32_t selBg = 0xFFe94560;
    uint32_t hovBg = 0xFF2a2a4a;
    uint32_t normBg = 0xFF16213e;
    uint32_t barBg = 0xFF0f3460;
    uint32_t barFill = 0xFFe94560;
    uint32_t onCol = 0xFF4ecca3;
    uint32_t offCol = 0xFF6c757d;
    
    float pxW = (float)swapchainExtent.width;
    float pxH = (float)swapchainExtent.height;
    
    addRect(-1.0f, -1.0f, 1.0f, 1.0f, bgDim);
    
    float panelW = 0.65f;
    float panelH = 0.85f;
    addRect(-panelW, -panelH, panelW, panelH, panelBg);
    
    constexpr float itemY = 0.58f;
    constexpr float itemH = 0.058f;
    constexpr float gap = 0.008f;
    constexpr float itemW = 0.55f;
    constexpr int MENU_COUNT = 9;
    
    struct MenuItem {
        const char* label;
        int type;
        float valNorm;
        bool on;
    };
    
    MenuItem items[MENU_COUNT] = {
        {"RESUME",           0, 0,    false},
        {"FOV",              1, (menu.fov - 30.0f) / 90.0f, false},
        {"RENDER DISTANCE",  1, (float)(menu.renderDistance - 2) / 14.0f, false},
        {"MAX FPS",          1, (float)(menu.maxFPS - 30) / 270.0f, false},
        {"SENSITIVITY",      1, (menu.sensitivity - 0.01f) / 0.99f, false},
        {"INVERT Y",         2, 0, menu.invertY},
        {"INVERT X",         2, 0, menu.invertX},
        {"VSYNC",            2, 0, menu.vsync},
        {"FLY MODE",         2, 0, menu.flyMode},
    };
    
    for (int i = 0; i < MENU_COUNT; i++) {
        float y = itemY - i * (itemH + gap);
        uint32_t bg;
        if (i == menu.hovered) bg = hovBg;
        else if (i == menu.selected) bg = selBg;
        else bg = normBg;
        addRect(-itemW, y - itemH, itemW, y, bg);
        
        if (items[i].type == 1) {
            float barM = 0.05f;
            float barH = itemH * 0.3f;
            float barY = y - itemH * 0.65f;
            addRect(-itemW + barM, barY - barH, itemW - barM, barY, barBg);
            float fillW = (itemW - barM) * 2.0f * items[i].valNorm;
            addRect(-itemW + barM, barY - barH, -itemW + barM + fillW, barY, barFill);
        } else if (items[i].type == 2) {
            float indS = itemH * 0.3f;
            float indX = itemW - 0.06f;
            float indY = y - itemH * 0.5f;
            addRect(indX - indS, indY - indS, indX + indS, indY + indS,
                    items[i].on ? onCol : offCol);
        }
    }
    
    auto* dstV = reinterpret_cast<UIVert*>((char*)uiVertexMapped + uiBatchV * sizeof(UIVert));
    auto* dstI = (uint32_t*)uiIndexMapped + uiBatchI;
    memcpy(dstV, verts, v * sizeof(UIVert));
    memcpy(dstI, indices, idx * sizeof(uint32_t));
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, uiPipeline);
    glm::mat4 uiMvp(1.0f);
    uiMvp[1][1] = -1.0f;
    cmd.pushConstants(uiPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &uiMvp);
    vk::DeviceSize voff = uiBatchV * sizeof(UIVert);
    cmd.bindVertexBuffers(0, 1, &uiVertexBuffer, &voff);
    cmd.bindIndexBuffer(uiIndexBuffer, 0, vk::IndexType::eUint32);
    cmd.drawIndexed(idx, 1, uiBatchI, 0, 0);
    uiBatchV += v;
    uiBatchI += idx;
    
    auto ndcPx = [&](float ndcX, float s) -> float {
        return (ndcX + 1.0f) * pxW / (2.0f * s);
    };
    auto ndcPy = [&](float ndcY, float s) -> float {
        return (1.0f - ndcY) * pxH / (2.0f * s);
    };
    
    float titleY = itemY + itemH + gap + 0.03f;
    drawText("SETTINGS", ndcPx(-0.12f, 2.5f), ndcPy(titleY, 2.5f), 0xFFFFFFFF, 2.5f);
    
    for (int i = 0; i < MENU_COUNT; i++) {
        float y = itemY - i * (itemH + gap);
        float tNdcY = y - itemH * 0.38f;
        float tNdcX = -itemW + 0.03f;
        uint32_t col = (i == menu.selected || i == menu.hovered) ? 0xFFFFFFFF : 0xFFCCCCCC;
        drawText(items[i].label, ndcPx(tNdcX, 1.2f), ndcPy(tNdcY, 1.2f), col, 1.2f);
        
        char valBuf[32];
        if (items[i].type == 1) {
            switch (i) {
                case 1: snprintf(valBuf, sizeof(valBuf), "%.0f", menu.fov); break;
                case 2: snprintf(valBuf, sizeof(valBuf), "%d", menu.renderDistance); break;
                case 3: snprintf(valBuf, sizeof(valBuf), "%d", menu.maxFPS); break;
                case 4: snprintf(valBuf, sizeof(valBuf), "%.2f", menu.sensitivity); break;
                default: valBuf[0] = '\0'; break;
            }
            float valNdcX = itemW - 0.2f;
            drawText(valBuf, ndcPx(valNdcX, 1.1f), ndcPy(tNdcY, 1.1f), 0xFFCCCCCC, 1.1f);
        } else if (items[i].type == 2) {
            float indNdcX = itemW - 0.1f;
            const char* toggleStr = items[i].on ? "ON" : "OFF";
            drawText(toggleStr, ndcPx(indNdcX, 1.1f), ndcPy(tNdcY, 1.1f),
                     items[i].on ? onCol : offCol, 1.1f);
        }
    }
}

void Renderer::drawText(const char* text, float x, float y, uint32_t color, float scale) {
    if (!uiPipelineReady || !uiVertexMapped || !uiIndexMapped) return;
    if (!text || !text[0]) return;
    
    char qbuffer[4096];
    int num_quads = stb_easy_font_print(x, y, const_cast<char*>(text), nullptr, qbuffer, sizeof(qbuffer));
    if (num_quads <= 0) return;
    
    struct EasyVert { float x, y, z; uint32_t col; };
    
    auto* src = reinterpret_cast<EasyVert*>(qbuffer);
    auto& cmd = commandBuffers[currentImageIndex];
    
    int totalVerts = num_quads * 4;
    int totalIndices = num_quads * 6;
    
    auto* verts = reinterpret_cast<EasyVert*>((char*)uiVertexMapped + uiBatchV * sizeof(EasyVert));
    auto* indices = (uint32_t*)uiIndexMapped + uiBatchI;
    
    float scaleX = scale * 2.0f / (float)swapchainExtent.width;
    float scaleY = scale * 2.0f / (float)swapchainExtent.height;
    
    uint32_t v = 0, idx = 0;
    for (int q = 0; q < num_quads; q++) {
        uint32_t base = v;
        for (int i = 0; i < 4; i++) {
            auto& sv = src[q * 4 + i];
            verts[v].x = sv.x * scaleX - 1.0f;
            verts[v].y = -(sv.y * scaleY - 1.0f);
            verts[v].z = 0.0f;
            verts[v].col = color;
            v++;
        }
        indices[idx++] = base;
        indices[idx++] = base + 1;
        indices[idx++] = base + 2;
        indices[idx++] = base;
        indices[idx++] = base + 2;
        indices[idx++] = base + 3;
    }
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, uiPipeline);
    glm::mat4 uiMvp(1.0f);
    uiMvp[1][1] = -1.0f;
    cmd.pushConstants(uiPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &uiMvp);
    vk::DeviceSize voff = uiBatchV * sizeof(EasyVert);
    cmd.bindVertexBuffers(0, 1, &uiVertexBuffer, &voff);
    cmd.bindIndexBuffer(uiIndexBuffer, 0, vk::IndexType::eUint32);
    cmd.drawIndexed(idx, 1, uiBatchI, 0, 0);
    uiBatchV += v;
    uiBatchI += idx;
}

void Renderer::drawDebugOverlay(float fps, float frameTime, int chunks, int drawCalls, float pitch, float yaw) {
    if (!uiPipelineReady || !uiVertexMapped || !uiIndexMapped) return;
    
    char buf[256];
    snprintf(buf, sizeof(buf), "FPS: %.0f  FT: %.1fms  Chunks: %d  Draws: %d  Pitch: %.1f  Yaw: %.1f",
             fps, frameTime * 1000.0f, chunks, drawCalls, pitch, yaw);
    drawText(buf, 4.0f, 4.0f, 0xFFFFFFFF, 1.5f);
}

static uint32_t packCloudColor(uint8_t a) {
    return 0xFF | (0xFF << 8) | (0xFF << 16) | ((uint32_t)a << 24);
}

void Renderer::drawClouds(Camera* camera, float gameTime) {
    if (!uiPipelineReady || !uiVertexMapped || !uiIndexMapped) return;
    auto& cmd = commandBuffers[currentImageIndex];

    struct UIVert { float x, y, z; uint32_t color; };

    constexpr int GRID = 24;
    constexpr float CLOUD_Y = 180.0f;
    constexpr float TILE = 12.0f;
    constexpr float DRIFT_SPEED = 1.5f;

    float drift = std::fmod(gameTime * DRIFT_SPEED, TILE);

    glm::vec3 camPos = camera->getPosition();
    float halfGrid = GRID * TILE * 0.5f;
    float originX = camPos.x - halfGrid;
    float originZ = camPos.z - halfGrid;

    int baseCX = (int)floor(originX / TILE);
    int baseCZ = (int)floor(originZ / TILE);
    float offX = originX - baseCX * TILE;
    float offZ = originZ - baseCZ * TILE;

    constexpr int MAX_CLOUD_VERTS = 4096;
    constexpr int MAX_CLOUD_INDICES = 6144;
    UIVert verts[MAX_CLOUD_VERTS];
    uint32_t indices[MAX_CLOUD_INDICES];
    uint32_t v = 0, idx = 0;

    auto addQuad = [&](float x0, float y0, float z0,
                       float x1, float y1, float z1,
                       float x2, float y2, float z2,
                       float x3, float y3, float z3, uint32_t col) {
        if (v + 4 > MAX_CLOUD_VERTS || idx + 6 > MAX_CLOUD_INDICES) return;
        uint32_t base = v;
        verts[v++] = {x0, y0, z0, col};
        verts[v++] = {x1, y1, z1, col};
        verts[v++] = {x2, y2, z2, col};
        verts[v++] = {x3, y3, z3, col};
        indices[idx++] = base; indices[idx++] = base+1; indices[idx++] = base+2;
        indices[idx++] = base; indices[idx++] = base+2; indices[idx++] = base+3;
    };

    for (int gz = 0; gz < GRID; gz++) {
        for (int gx = 0; gx < GRID; gx++) {
            int cx = baseCX + gx;
            int cz = baseCZ + gz;

            unsigned int h = (unsigned int)(cx * 374761393u + cz * 668265263u);
            h = (h ^ (h >> 13)) * 1274126177u;
            h = h ^ (h >> 16);
            if ((h & 7) < 3) continue;

            uint8_t alpha = (uint8_t)(140 + (h & 63));
            uint32_t col = packCloudColor(alpha);

            float x = (gx * TILE) - offX - drift;
            float z = (gz * TILE) - offZ;
            float thickness = 2.0f;

            addQuad(x, CLOUD_Y + thickness, z,
                    x + TILE, CLOUD_Y + thickness, z,
                    x + TILE, CLOUD_Y + thickness, z + TILE,
                    x, CLOUD_Y + thickness, z + TILE, col);
            addQuad(x, CLOUD_Y, z + TILE,
                    x + TILE, CLOUD_Y, z + TILE,
                    x + TILE, CLOUD_Y, z,
                    x, CLOUD_Y, z, packCloudColor((uint8_t)(alpha * 0.7f)));
            addQuad(x, CLOUD_Y, z,
                    x + TILE, CLOUD_Y, z,
                    x + TILE, CLOUD_Y + thickness, z,
                    x, CLOUD_Y + thickness, z, packCloudColor((uint8_t)(alpha * 0.85f)));
            addQuad(x + TILE, CLOUD_Y, z,
                    x + TILE, CLOUD_Y, z + TILE,
                    x + TILE, CLOUD_Y + thickness, z + TILE,
                    x + TILE, CLOUD_Y + thickness, z, packCloudColor((uint8_t)(alpha * 0.85f)));
            addQuad(x, CLOUD_Y, z + TILE,
                    x, CLOUD_Y, z,
                    x, CLOUD_Y + thickness, z,
                    x, CLOUD_Y + thickness, z + TILE, packCloudColor((uint8_t)(alpha * 0.75f)));
            addQuad(x, CLOUD_Y, z,
                    x + TILE, CLOUD_Y, z,
                    x + TILE, CLOUD_Y, z + TILE,
                    x, CLOUD_Y, z + TILE, packCloudColor((uint8_t)(alpha * 0.6f)));
        }
    }

    if (v == 0) return;

    auto* dstV = reinterpret_cast<UIVert*>((char*)uiVertexMapped + uiBatchV * sizeof(UIVert));
    auto* dstI = (uint32_t*)uiIndexMapped + uiBatchI;
    memcpy(dstV, verts, v * sizeof(UIVert));
    memcpy(dstI, indices, idx * sizeof(uint32_t));

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, uiPipeline);
    glm::mat4 vp = camera->getViewProjectionMatrix();
    cmd.pushConstants(uiPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &vp);
    vk::DeviceSize voff = uiBatchV * sizeof(UIVert);
    cmd.bindVertexBuffers(0, 1, &uiVertexBuffer, &voff);
    cmd.bindIndexBuffer(uiIndexBuffer, 0, vk::IndexType::eUint32);
    cmd.drawIndexed(idx, 1, uiBatchI, 0, 0);
    uiBatchV += v;
    uiBatchI += idx;
}

} // namespace VoxelForge
