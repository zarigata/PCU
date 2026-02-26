/**
 * @file VulkanSync.cpp
 * @brief Vulkan synchronization primitives implementation
 */

#include "VulkanSync.hpp"
#include <VoxelForge/rendering/VulkanDevice.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

// ============== VulkanSemaphore ==============

VulkanSemaphore::VulkanSemaphore() = default;

VulkanSemaphore::~VulkanSemaphore() {
    cleanup();
}

VulkanSemaphore::VulkanSemaphore(VulkanSemaphore&& other) noexcept
    : device(other.device), semaphore(other.semaphore) {
    other.device = VK_NULL_HANDLE;
    other.semaphore = VK_NULL_HANDLE;
}

VulkanSemaphore& VulkanSemaphore::operator=(VulkanSemaphore&& other) noexcept {
    if (this != &other) {
        cleanup();
        device = other.device;
        semaphore = other.semaphore;
        other.device = VK_NULL_HANDLE;
        other.semaphore = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanSemaphore::init(vk::Device device) {
    this->device = device;
    
    vk::SemaphoreCreateInfo createInfo{};
    semaphore = device.createSemaphore(createInfo);
}

void VulkanSemaphore::cleanup() {
    if (device && semaphore) {
        device.destroySemaphore(semaphore);
        semaphore = VK_NULL_HANDLE;
    }
}

// ============== VulkanFence ==============

VulkanFence::VulkanFence() = default;

VulkanFence::~VulkanFence() {
    cleanup();
}

VulkanFence::VulkanFence(VulkanFence&& other) noexcept
    : device(other.device), fence(other.fence) {
    other.device = VK_NULL_HANDLE;
    other.fence = VK_NULL_HANDLE;
}

VulkanFence& VulkanFence::operator=(VulkanFence&& other) noexcept {
    if (this != &other) {
        cleanup();
        device = other.device;
        fence = other.fence;
        other.device = VK_NULL_HANDLE;
        other.fence = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanFence::init(vk::Device device, State initialState) {
    this->device = device;
    
    vk::FenceCreateInfo createInfo{};
    if (initialState == State::Signaled) {
        createInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    }
    
    fence = device.createFence(createInfo);
}

void VulkanFence::cleanup() {
    if (device && fence) {
        device.destroyFence(fence);
        fence = VK_NULL_HANDLE;
    }
}

bool VulkanFence::wait(uint64_t timeout) const {
    auto result = device.waitForFences(1, &fence, VK_TRUE, timeout);
    return result == vk::Result::eSuccess;
}

void VulkanFence::reset() {
    device.resetFences(1, &fence);
}

bool VulkanFence::isSignaled() const {
    auto result = device.getFenceStatus(fence);
    return result == vk::Result::eSuccess;
}

bool VulkanFence::waitAndReset(uint64_t timeout) {
    if (wait(timeout)) {
        reset();
        return true;
    }
    return false;
}

// ============== VulkanTimelineSemaphore ==============

VulkanTimelineSemaphore::VulkanTimelineSemaphore() = default;

VulkanTimelineSemaphore::~VulkanTimelineSemaphore() {
    cleanup();
}

VulkanTimelineSemaphore::VulkanTimelineSemaphore(VulkanTimelineSemaphore&& other) noexcept
    : device(other.device), semaphore(other.semaphore) {
    other.device = VK_NULL_HANDLE;
    other.semaphore = VK_NULL_HANDLE;
}

VulkanTimelineSemaphore& VulkanTimelineSemaphore::operator=(VulkanTimelineSemaphore&& other) noexcept {
    if (this != &other) {
        cleanup();
        device = other.device;
        semaphore = other.semaphore;
        other.device = VK_NULL_HANDLE;
        other.semaphore = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanTimelineSemaphore::init(vk::Device device, uint64_t initialValue) {
    this->device = device;
    
    vk::SemaphoreTypeCreateInfo typeInfo{};
    typeInfo.semaphoreType = vk::SemaphoreType::eTimeline;
    typeInfo.initialValue = initialValue;
    
    vk::SemaphoreCreateInfo createInfo{};
    createInfo.pNext = &typeInfo;
    
    semaphore = device.createSemaphore(createInfo);
}

void VulkanTimelineSemaphore::cleanup() {
    if (device && semaphore) {
        device.destroySemaphore(semaphore);
        semaphore = VK_NULL_HANDLE;
    }
}

uint64_t VulkanTimelineSemaphore::getValue() const {
    return device.getSemaphoreCounterValue(semaphore);
}

bool VulkanTimelineSemaphore::wait(uint64_t value, uint64_t timeout) const {
    vk::SemaphoreWaitInfo waitInfo{};
    waitInfo.semaphoreCount = 1;
    waitInfo.pSemaphores = &semaphore;
    waitInfo.pValues = &value;
    
    auto result = device.waitSemaphores(waitInfo, timeout);
    return result == vk::Result::eSuccess;
}

vk::SemaphoreSignalInfo VulkanTimelineSemaphore::getSignalInfo(uint64_t value) const {
    vk::SemaphoreSignalInfo info{};
    info.semaphore = semaphore;
    info.value = value;
    return info;
}

vk::SemaphoreSubmitInfo VulkanTimelineSemaphore::getSubmitInfo(uint64_t value, vk::PipelineStageFlags2 stageMask) const {
    vk::SemaphoreSubmitInfo info{};
    info.semaphore = semaphore;
    info.value = value;
    info.stageMask = stageMask;
    return info;
}

// ============== FrameSync ==============

void FrameSync::init(vk::Device device) {
    imageAvailable.init(device);
    renderFinished.init(device);
    inFlight.init(device, VulkanFence::State::Signaled);
}

void FrameSync::cleanup() {
    imageAvailable.cleanup();
    renderFinished.cleanup();
    inFlight.cleanup();
}

void FrameSync::waitForFence(uint64_t timeout) {
    inFlight.wait(timeout);
}

void FrameSync::resetFence() {
    inFlight.reset();
}

// ============== SyncManager ==============

SyncManager::SyncManager() = default;

SyncManager::~SyncManager() {
    cleanup();
}

void SyncManager::init(vk::Device device, uint32_t frameCount) {
    this->device = device;
    this->frameCount = frameCount;
    
    frameSyncs.resize(frameCount);
    for (auto& sync : frameSyncs) {
        sync.init(device);
    }
    
    Logger::debug("SyncManager initialized with {} frames", frameCount);
}

void SyncManager::cleanup() {
    for (auto& sync : frameSyncs) {
        sync.cleanup();
    }
    frameSyncs.clear();
    device = VK_NULL_HANDLE;
}

FrameSync& SyncManager::getCurrentSync() {
    return frameSyncs[currentFrame];
}

const FrameSync& SyncManager::getCurrentSync() const {
    return frameSyncs[currentFrame];
}

FrameSync& SyncManager::getSync(uint32_t frameIndex) {
    return frameSyncs[frameIndex];
}

const FrameSync& SyncManager::getSync(uint32_t frameIndex) const {
    return frameSyncs[frameIndex];
}

void SyncManager::advanceFrame() {
    currentFrame = (currentFrame + 1) % frameCount;
}

void SyncManager::waitForFrame(uint32_t frameIndex, uint64_t timeout) {
    frameSyncs[frameIndex].waitForFence(timeout);
}

vk::SubmitInfo SyncManager::getSubmitInfo(
    vk::CommandBuffer cmd,
    vk::PipelineStageFlags waitStage) {
    
    auto& sync = getCurrentSync();
    
    vk::SubmitInfo submitInfo{};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &sync.imageAvailable.get();
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &sync.renderFinished.get();
    
    return submitInfo;
}

vk::PresentInfoKHR SyncManager::getPresentInfo(vk::SwapchainKHR swapchain, uint32_t imageIndex) {
    auto& sync = getCurrentSync();
    
    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &sync.renderFinished.get();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    
    return presentInfo;
}

// ============== BarrierHelpers ==============

namespace BarrierHelpers {

vk::ImageMemoryBarrier imageBarrier(
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags srcAccessMask,
    vk::AccessFlags dstAccessMask,
    vk::ImageSubresourceRange subresourceRange) {
    
    vk::ImageMemoryBarrier barrier{};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = subresourceRange;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    
    return barrier;
}

vk::ImageMemoryBarrier transitionColorAttachmentToShaderRead(vk::Image image) {
    return imageBarrier(
        image,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::AccessFlagBits::eShaderRead,
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    );
}

vk::ImageMemoryBarrier transitionShaderReadToColorAttachment(vk::Image image) {
    return imageBarrier(
        image,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::AccessFlagBits::eShaderRead,
        vk::AccessFlagBits::eColorAttachmentWrite,
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    );
}

vk::ImageMemoryBarrier transitionUndefinedToColorAttachment(vk::Image image) {
    return imageBarrier(
        image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::AccessFlagBits::eNoneKHR,
        vk::AccessFlagBits::eColorAttachmentWrite,
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    );
}

vk::ImageMemoryBarrier transitionColorAttachmentToPresent(vk::Image image) {
    return imageBarrier(
        image,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::AccessFlagBits::eMemoryRead,
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    );
}

vk::ImageMemoryBarrier transitionPresentToColorAttachment(vk::Image image) {
    return imageBarrier(
        image,
        vk::ImageLayout::ePresentSrcKHR,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::AccessFlagBits::eMemoryRead,
        vk::AccessFlagBits::eColorAttachmentWrite,
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    );
}

vk::ImageMemoryBarrier transitionUndefinedToDepthAttachment(vk::Image image, bool stencil) {
    auto aspect = stencil ? 
        (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil) :
        vk::ImageAspectFlagBits::eDepth;
    
    return imageBarrier(
        image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
        vk::AccessFlagBits::eNoneKHR,
        vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        {aspect, 0, 1, 0, 1}
    );
}

vk::ImageMemoryBarrier transitionDepthAttachmentToShaderRead(vk::Image image, bool stencil) {
    auto aspect = stencil ? 
        (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil) :
        vk::ImageAspectFlagBits::eDepth;
    
    return imageBarrier(
        image,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits::eShaderRead,
        {aspect, 0, 1, 0, 1}
    );
}

vk::ImageMemoryBarrier transitionShaderReadToDepthAttachment(vk::Image image, bool stencil) {
    auto aspect = stencil ? 
        (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil) :
        vk::ImageAspectFlagBits::eDepth;
    
    return imageBarrier(
        image,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
        vk::AccessFlagBits::eShaderRead,
        vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        {aspect, 0, 1, 0, 1}
    );
}

vk::ImageMemoryBarrier transitionUndefinedToTransferDst(vk::Image image) {
    return imageBarrier(
        image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        vk::AccessFlagBits::eNoneKHR,
        vk::AccessFlagBits::eTransferWrite,
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    );
}

vk::ImageMemoryBarrier transitionTransferDstToShaderRead(vk::Image image) {
    return imageBarrier(
        image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits::eTransferWrite,
        vk::AccessFlagBits::eShaderRead,
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    );
}

vk::ImageMemoryBarrier transitionUndefinedToTransferSrc(vk::Image image) {
    return imageBarrier(
        image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferSrcOptimal,
        vk::AccessFlagBits::eNoneKHR,
        vk::AccessFlagBits::eTransferRead,
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    );
}

vk::ImageMemoryBarrier transferDstToColorAttachment(vk::Image image) {
    return imageBarrier(
        image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::AccessFlagBits::eTransferWrite,
        vk::AccessFlagBits::eColorAttachmentWrite,
        {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    );
}

vk::BufferMemoryBarrier bufferBarrier(
    vk::Buffer buffer,
    vk::AccessFlags srcAccessMask,
    vk::AccessFlags dstAccessMask,
    vk::DeviceSize offset,
    vk::DeviceSize size) {
    
    vk::BufferMemoryBarrier barrier{};
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = buffer;
    barrier.offset = offset;
    barrier.size = size;
    
    return barrier;
}

vk::MemoryBarrier memoryBarrier(
    vk::AccessFlags srcAccessMask,
    vk::AccessFlags dstAccessMask) {
    
    vk::MemoryBarrier barrier{};
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    
    return barrier;
}

void pipelineBarrier(
    vk::CommandBuffer cmd,
    vk::PipelineStageFlags srcStage,
    vk::PipelineStageFlags dstStage,
    const std::vector<vk::ImageMemoryBarrier>& imageBarriers,
    const std::vector<vk::BufferMemoryBarrier>& bufferBarriers,
    const std::vector<vk::MemoryBarrier>& memoryBarriers) {
    
    cmd.pipelineBarrier(
        srcStage, dstStage, vk::DependencyFlagBits::eByRegion,
        static_cast<uint32_t>(memoryBarriers.size()), memoryBarriers.data(),
        static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
        static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data()
    );
}

void transitionImage(
    vk::CommandBuffer cmd,
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::PipelineStageFlags srcStage,
    vk::PipelineStageFlags dstStage,
    vk::AccessFlags srcAccess,
    vk::AccessFlags dstAccess,
    vk::ImageSubresourceRange subresourceRange) {
    
    auto barrier = imageBarrier(image, oldLayout, newLayout, srcAccess, dstAccess, subresourceRange);
    pipelineBarrier(cmd, srcStage, dstStage, {barrier});
}

void transitionColorImage(
    vk::CommandBuffer cmd,
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout) {
    
    vk::PipelineStageFlags srcStage, dstStage;
    vk::AccessFlags srcAccess, dstAccess;
    
    // Determine stages and access based on layouts
    if (oldLayout == vk::ImageLayout::eUndefined) {
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        srcAccess = vk::AccessFlagBits::eNoneKHR;
    } else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        srcStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        srcAccess = vk::AccessFlagBits::eColorAttachmentWrite;
    } else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        srcStage = vk::PipelineStageFlagBits::eFragmentShader;
        srcAccess = vk::AccessFlagBits::eShaderRead;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal) {
        srcStage = vk::PipelineStageFlagBits::eTransfer;
        srcAccess = vk::AccessFlagBits::eTransferWrite;
    } else if (oldLayout == vk::ImageLayout::ePresentSrcKHR) {
        srcStage = vk::PipelineStageFlagBits::eBottomOfPipe;
        srcAccess = vk::AccessFlagBits::eMemoryRead;
    } else {
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        srcAccess = vk::AccessFlagBits::eNoneKHR;
    }
    
    if (newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dstAccess = vk::AccessFlagBits::eColorAttachmentWrite;
    } else if (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;
        dstAccess = vk::AccessFlagBits::eShaderRead;
    } else if (newLayout == vk::ImageLayout::eTransferDstOptimal) {
        dstStage = vk::PipelineStageFlagBits::eTransfer;
        dstAccess = vk::AccessFlagBits::eTransferWrite;
    } else if (newLayout == vk::ImageLayout::ePresentSrcKHR) {
        dstStage = vk::PipelineStageFlagBits::eBottomOfPipe;
        dstAccess = vk::AccessFlagBits::eMemoryRead;
    } else {
        dstStage = vk::PipelineStageFlagBits::eBottomOfPipe;
        dstAccess = vk::AccessFlagBits::eNoneKHR;
    }
    
    transitionImage(cmd, image, oldLayout, newLayout, srcStage, dstStage, 
                   srcAccess, dstAccess, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
}

void transitionDepthImage(
    vk::CommandBuffer cmd,
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    bool hasStencil) {
    
    auto aspect = hasStencil ? 
        (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil) :
        vk::ImageAspectFlagBits::eDepth;
    
    vk::PipelineStageFlags srcStage, dstStage;
    vk::AccessFlags srcAccess, dstAccess;
    
    if (oldLayout == vk::ImageLayout::eUndefined) {
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        srcAccess = vk::AccessFlagBits::eNoneKHR;
    } else if (oldLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        srcStage = vk::PipelineStageFlagBits::eEarlyFragmentTests | 
                  vk::PipelineStageFlagBits::eLateFragmentTests;
        srcAccess = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    } else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        srcStage = vk::PipelineStageFlagBits::eFragmentShader;
        srcAccess = vk::AccessFlagBits::eShaderRead;
    } else {
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        srcAccess = vk::AccessFlagBits::eNoneKHR;
    }
    
    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        dstStage = vk::PipelineStageFlagBits::eEarlyFragmentTests | 
                  vk::PipelineStageFlagBits::eLateFragmentTests;
        dstAccess = vk::AccessFlagBits::eDepthStencilAttachmentRead |
                   vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    } else if (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;
        dstAccess = vk::AccessFlagBits::eShaderRead;
    } else if (newLayout == vk::ImageLayout::eDepthStencilReadOnlyOptimal) {
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;
        dstAccess = vk::AccessFlagBits::eShaderRead;
    } else {
        dstStage = vk::PipelineStageFlagBits::eBottomOfPipe;
        dstAccess = vk::AccessFlagBits::eNoneKHR;
    }
    
    transitionImage(cmd, image, oldLayout, newLayout, srcStage, dstStage,
                   srcAccess, dstAccess, {aspect, 0, 1, 0, 1});
}

} // namespace BarrierHelpers

} // namespace VoxelForge
