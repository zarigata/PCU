/**
 * @file VulkanSync.hpp
 * @brief Vulkan synchronization primitives
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <memory>

namespace VoxelForge {

class VulkanDevice;

// Semaphore wrapper
class VulkanSemaphore {
public:
    VulkanSemaphore();
    ~VulkanSemaphore();
    
    // No copy
    VulkanSemaphore(const VulkanSemaphore&) = delete;
    VulkanSemaphore& operator=(const VulkanSemaphore&) = delete;
    
    // Move
    VulkanSemaphore(VulkanSemaphore&& other) noexcept;
    VulkanSemaphore& operator=(VulkanSemaphore&& other) noexcept;
    
    void init(vk::Device device);
    void cleanup();
    
    vk::Semaphore get() const { return semaphore; }
    operator vk::Semaphore() const { return semaphore; }
    operator bool() const { return semaphore != VK_NULL_HANDLE; }
    
private:
    vk::Device device = VK_NULL_HANDLE;
    vk::Semaphore semaphore = VK_NULL_HANDLE;
};

// Fence wrapper
class VulkanFence {
public:
    enum class State {
        Signaled,
        Unsignaled
    };
    
    VulkanFence();
    ~VulkanFence();
    
    // No copy
    VulkanFence(const VulkanFence&) = delete;
    VulkanFence& operator=(const VulkanFence&) = delete;
    
    // Move
    VulkanFence(VulkanFence&& other) noexcept;
    VulkanFence& operator=(VulkanFence&& other) noexcept;
    
    void init(vk::Device device, State initialState = State::Unsignaled);
    void cleanup();
    
    vk::Fence get() const { return fence; }
    operator vk::Fence() const { return fence; }
    operator bool() const { return fence != VK_NULL_HANDLE; }
    
    // Wait for fence
    bool wait(uint64_t timeout = UINT64_MAX) const;
    
    // Reset fence
    void reset();
    
    // Check if signaled (non-blocking)
    bool isSignaled() const;
    
    // Wait and reset
    bool waitAndReset(uint64_t timeout = UINT64_MAX);
    
private:
    vk::Device device = VK_NULL_HANDLE;
    vk::Fence fence = VK_NULL_HANDLE;
};

// Timeline semaphore (Vulkan 1.2+)
class VulkanTimelineSemaphore {
public:
    VulkanTimelineSemaphore();
    ~VulkanTimelineSemaphore();
    
    // No copy
    VulkanTimelineSemaphore(const VulkanTimelineSemaphore&) = delete;
    VulkanTimelineSemaphore& operator=(const VulkanTimelineSemaphore&) = delete;
    
    // Move
    VulkanTimelineSemaphore(VulkanTimelineSemaphore&& other) noexcept;
    VulkanTimelineSemaphore& operator=(VulkanTimelineSemaphore&& other) noexcept;
    
    void init(vk::Device device, uint64_t initialValue = 0);
    void cleanup();
    
    vk::Semaphore get() const { return semaphore; }
    operator vk::Semaphore() const { return semaphore; }
    
    // Get current value
    uint64_t getValue() const;
    
    // Wait for value
    bool wait(uint64_t value, uint64_t timeout = UINT64_MAX) const;
    
    // Signal (must be done via queue submit or signal operation)
    // Use this to create signal info for submit
    vk::SemaphoreSignalInfo getSignalInfo(uint64_t value) const;
    vk::SemaphoreSubmitInfo getSubmitInfo(uint64_t value, vk::PipelineStageFlags2 stageMask) const;
    
private:
    vk::Device device = VK_NULL_HANDLE;
    vk::Semaphore semaphore = VK_NULL_HANDLE;
};

// Synchronization set for a frame
struct FrameSync {
    VulkanSemaphore imageAvailable;     // Signaled when swapchain image is ready
    VulkanSemaphore renderFinished;     // Signaled when rendering is complete
    VulkanFence inFlight;               // Signaled when frame is finished
    
    void init(vk::Device device);
    void cleanup();
    
    // Wait for previous frame using this sync
    void waitForFence(uint64_t timeout = UINT64_MAX);
    void resetFence();
};

// Sync object manager for multiple frames in flight
class SyncManager {
public:
    SyncManager();
    ~SyncManager();
    
    void init(vk::Device device, uint32_t frameCount);
    void cleanup();
    
    // Get sync for current frame
    FrameSync& getCurrentSync();
    const FrameSync& getCurrentSync() const;
    
    // Get sync for specific frame
    FrameSync& getSync(uint32_t frameIndex);
    const FrameSync& getSync(uint32_t frameIndex) const;
    
    // Frame management
    void advanceFrame();
    uint32_t getCurrentFrame() const { return currentFrame; }
    uint32_t getFrameCount() const { return frameCount; }
    
    // Wait for frame to complete
    void waitForFrame(uint32_t frameIndex, uint64_t timeout = UINT64_MAX);
    
    // Get submit info for current frame
    vk::SubmitInfo getSubmitInfo(
        vk::CommandBuffer cmd,
        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput
    );
    
    // Get present info for current frame
    vk::PresentInfoKHR getPresentInfo(vk::SwapchainKHR swapchain, uint32_t imageIndex);
    
private:
    vk::Device device = VK_NULL_HANDLE;
    std::vector<FrameSync> frameSyncs;
    uint32_t frameCount = 2;
    uint32_t currentFrame = 0;
};

// Barrier helpers
namespace BarrierHelpers {
    
// Image memory barrier
vk::ImageMemoryBarrier imageBarrier(
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags srcAccessMask,
    vk::AccessFlags dstAccessMask,
    vk::ImageSubresourceRange subresourceRange
);

// Image barrier for common transitions
vk::ImageMemoryBarrier transitionColorAttachmentToShaderRead(vk::Image image);
vk::ImageMemoryBarrier transitionShaderReadToColorAttachment(vk::Image image);
vk::ImageMemoryBarrier transitionUndefinedToColorAttachment(vk::Image image);
vk::ImageMemoryBarrier transitionColorAttachmentToPresent(vk::Image image);
vk::ImageMemoryBarrier transitionPresentToColorAttachment(vk::Image image);

vk::ImageMemoryBarrier transitionUndefinedToDepthAttachment(vk::Image image, bool stencil = false);
vk::ImageMemoryBarrier transitionDepthAttachmentToShaderRead(vk::Image image, bool stencil = false);
vk::ImageMemoryBarrier transitionShaderReadToDepthAttachment(vk::Image image, bool stencil = false);

vk::ImageMemoryBarrier transitionUndefinedToTransferDst(vk::Image image);
vk::ImageMemoryBarrier transitionTransferDstToShaderRead(vk::Image image);
vk::ImageMemoryBarrier transitionUndefinedToTransferSrc(vk::Image image);
vk::ImageMemoryBarrier transferDstToColorAttachment(vk::Image image);

// Buffer memory barrier
vk::BufferMemoryBarrier bufferBarrier(
    vk::Buffer buffer,
    vk::AccessFlags srcAccessMask,
    vk::AccessFlags dstAccessMask,
    vk::DeviceSize offset = 0,
    vk::DeviceSize size = VK_WHOLE_SIZE
);

// Memory barrier
vk::MemoryBarrier memoryBarrier(
    vk::AccessFlags srcAccessMask,
    vk::AccessFlags dstAccessMask
);

// Pipeline barrier helper
void pipelineBarrier(
    vk::CommandBuffer cmd,
    vk::PipelineStageFlags srcStage,
    vk::PipelineStageFlags dstStage,
    const std::vector<vk::ImageMemoryBarrier>& imageBarriers,
    const std::vector<vk::BufferMemoryBarrier>& bufferBarriers = {},
    const std::vector<vk::MemoryBarrier>& memoryBarriers = {}
);

// Common pipeline barrier combinations
void transitionImage(
    vk::CommandBuffer cmd,
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::PipelineStageFlags srcStage,
    vk::PipelineStageFlags dstStage,
    vk::AccessFlags srcAccess,
    vk::AccessFlags dstAccess,
    vk::ImageSubresourceRange subresourceRange
);

void transitionColorImage(
    vk::CommandBuffer cmd,
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout
);

void transitionDepthImage(
    vk::CommandBuffer cmd,
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    bool hasStencil = false
);

} // namespace BarrierHelpers

} // namespace VoxelForge
