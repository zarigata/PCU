/**
 * @file Renderer.cpp
 * @brief Main renderer class implementation
 */

#include <VoxelForge/rendering/Renderer.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <VoxelForge/utils/Profiler.hpp>
#include <algorithm>
#include <array>
#include <fstream>

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
        LOG_WARN("Renderer already initialized");
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
    LOG_INFO("Renderer created");
}

Renderer::~Renderer() {
    shutdown();
    LOG_INFO("Renderer destroyed");
}

void Renderer::init(GLFWwindow* window) {
    PROFILE_FUNCTION();
    
    LOG_INFO("Initializing renderer...");
    
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
    
    LOG_INFO("Renderer initialized successfully");
    LOG_INFO("  Resolution: {}x{}", width, height);
    LOG_INFO("  Swapchain format: {}", static_cast<int>(swapchainFormat));
}

void Renderer::shutdown() {
    if (!context) return;
    
    LOG_INFO("Shutting down renderer...");
    
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
    
    LOG_INFO("Renderer shutdown complete");
}

void Renderer::createSwapChain() {
    PROFILE_FUNCTION();
    
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
    
    LOG_INFO("Swapchain created: {} images, {}x{}", 
             swapchainImages.size(), extent.width, extent.height);
}

void Renderer::createRenderPass() {
    PROFILE_FUNCTION();
    
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
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | 
                              vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcAccessMask = vk::AccessFlags{};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | 
                              vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | 
                               vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    
    // Create render pass
    std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    
    vk::RenderPassCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eRenderPassCreateInfo;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;
    
    try {
        renderPass = context->getDevice().createRenderPass(createInfo);
    } catch (const vk::SystemError& e) {
        throw std::runtime_error(std::string("Failed to create render pass: ") + e.what());
    }
    
    LOG_INFO("Render pass created");
}

void Renderer::createFramebuffers() {
    PROFILE_FUNCTION();
    
    framebuffers.resize(swapchainImageViews.size());
    
    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        std::array<vk::ImageView, 2> attachments = {
            swapchainImageViews[i],
            depthImageView // Will need to create depth image
        };
        
        vk::FramebufferCreateInfo createInfo{};
        createInfo.sType = vk::StructureType::eFramebufferCreateInfo;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        createInfo.pAttachments = attachments.data();
        createInfo.width = swapchainExtent.width;
        createInfo.height = swapchainExtent.height;
        createInfo.layers = 1;
        
        try {
            framebuffers[i] = context->getDevice().createFramebuffer(createInfo);
        } catch (const vk::SystemError& e) {
            throw std::runtime_error(std::string("Failed to create framebuffer: ") + e.what());
        }
    }
    
    LOG_INFO("Framebuffers created: {}", framebuffers.size());
}

void Renderer::createCommandBuffers() {
    PROFILE_FUNCTION();
    
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
    
    LOG_INFO("Command buffers created: {}", commandBuffers.size());
}

void Renderer::createSyncObjects() {
    PROFILE_FUNCTION();
    
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
    
    LOG_INFO("Synchronization objects created");
}

void Renderer::beginFrame() {
    PROFILE_FUNCTION();
    
    // Wait for previous frame
    auto result = context->getDevice().waitForFences(
        1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    
    if (result != vk::Result::eSuccess) {
        LOG_ERROR("Failed to wait for fence");
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
    clearValues[0].color = vk::ClearColorValue{0.0f, 0.0f, 0.2f, 1.0f}; // Sky blue-ish
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
    
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    
    commandBuffers[currentImageIndex].beginRenderPass(
        &renderPassInfo, vk::SubpassContents::eInline);
    
    // Reset stats
    stats = RenderStats{};
}

void Renderer::endFrame() {
    PROFILE_FUNCTION();
    
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
        result = context->getPresentQueue().presentKHR(&presentInfo);
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
    PROFILE_FUNCTION();
    
    if (!world || !camera) {
        LOG_WARN("Render called with null world or camera");
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
    PROFILE_FUNCTION();
    
    // TODO: Implement chunk rendering
    // For now, just update stats
    stats.chunksRendered = 0;
    stats.chunkDrawCalls = 0;
}

void Renderer::renderEntities(World* world, Camera* camera) {
    PROFILE_FUNCTION();
    
    // TODO: Implement entity rendering
    stats.entityDrawCalls = 0;
}

void Renderer::renderParticles(World* world, Camera* camera) {
    PROFILE_FUNCTION();
    
    // TODO: Implement particle rendering
    stats.particlesDrawCalls = 0;
}

void Renderer::renderUI() {
    PROFILE_FUNCTION();
    
    // TODO: Implement UI rendering
}

void Renderer::renderPostProcess() {
    PROFILE_FUNCTION();
    
    // TODO: Implement post-processing (bloom, TAA, FXAA)
}

void Renderer::onResize(int newWidth, int newHeight) {
    if (newWidth == width && newHeight == height) return;
    
    width = newWidth;
    height = newHeight;
    framebufferResized = true;
    
    context->getDevice().waitIdle();
    recreateSwapChain();
    
    LOG_INFO("Window resized: {}x{}", width, height);
}

void Renderer::recreateSwapChain() {
    PROFILE_FUNCTION();
    
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
    PROFILE_FUNCTION();
    
    LOG_INFO("Taking screenshot: {}", path);
    
    // TODO: Implement screenshot functionality
    // 1. Create a buffer image
    // 2. Copy swapchain image to buffer image
    // 3. Map and read the image data
    // 4. Save to file (PNG or BMP)
    
    LOG_WARN("Screenshot not yet implemented");
}

void Renderer::reloadShaders() {
    PROFILE_FUNCTION();
    
    LOG_INFO("Reloading shaders...");
    
    // TODO: Implement shader reloading
    // 1. Destroy old pipelines
    // 2. Recreate shader modules
    // 3. Recreate pipelines
    
    LOG_WARN("Shader reloading not yet implemented");
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

} // namespace VoxelForge
