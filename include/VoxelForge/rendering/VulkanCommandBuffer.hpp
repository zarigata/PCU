/**
 * @file VulkanCommandBuffer.hpp
 * @brief Vulkan command buffer and pool management
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <memory>

namespace VoxelForge {

class VulkanDevice;

// Command pool for a specific queue family
class VulkanCommandPool {
public:
    enum class Type {
        Graphics,
        Compute,
        Transfer,
        Present
    };
    
    VulkanCommandPool();
    ~VulkanCommandPool();
    
    // No copy
    VulkanCommandPool(const VulkanCommandPool&) = delete;
    VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;
    
    void init(VulkanDevice* device, Type type, bool transient = false, bool resetable = true);
    void cleanup();
    
    // Allocate command buffers
    vk::CommandBuffer allocate(bool primary = true);
    std::vector<vk::CommandBuffer> allocate(uint32_t count, bool primary = true);
    
    // Free command buffers
    void free(vk::CommandBuffer cmd);
    void free(const std::vector<vk::CommandBuffer>& cmds);
    
    // Reset pool
    void reset(bool releaseResources = false);
    
    // Getters
    vk::CommandPool getPool() const { return pool; }
    
    // One-time command helpers
    vk::CommandBuffer beginSingleTimeCommand();
    void endSingleTimeCommand(vk::CommandBuffer cmd);
    
private:
    VulkanDevice* device = nullptr;
    vk::CommandPool pool;
    Type type;
    uint32_t queueFamilyIndex = 0;
};

// Command buffer wrapper with state tracking
class VulkanCommandBuffer {
public:
    enum class State {
        Idle,
        Recording,
        Executable,
        Pending
    };
    
    VulkanCommandBuffer();
    ~VulkanCommandBuffer();
    
    // No copy
    VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
    VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;
    
    // Move
    VulkanCommandBuffer(VulkanCommandBuffer&& other) noexcept;
    VulkanCommandBuffer& operator=(VulkanCommandBuffer&& other) noexcept;
    
    void init(vk::Device device, vk::CommandPool pool, bool primary = true);
    void cleanup();
    
    // Recording
    void begin(vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    void end();
    
    // Submission state
    void reset(bool releaseResources = false);
    
    // Getters
    vk::CommandBuffer get() const { return commandBuffer; }
    State getState() const { return state; }
    bool isRecording() const { return state == State::Recording; }
    bool isExecutable() const { return state == State::Executable; }
    
    // Convenience operators
    operator vk::CommandBuffer() const { return commandBuffer; }
    vk::CommandBuffer* operator&() { return &commandBuffer; }
    
    // Command buffer operations
    void beginRenderPass(
        vk::RenderPass renderPass,
        vk::Framebuffer framebuffer,
        vk::Rect2D renderArea,
        const std::vector<vk::ClearValue>& clearValues,
        vk::SubpassContents contents = vk::SubpassContents::eInline
    );
    
    void endRenderPass();
    
    void bindPipeline(vk::PipelineBindPoint bindPoint, vk::Pipeline pipeline);
    void bindDescriptorSets(
        vk::PipelineBindPoint bindPoint,
        vk::PipelineLayout layout,
        uint32_t firstSet,
        const std::vector<vk::DescriptorSet>& descriptorSets,
        const std::vector<uint32_t>& dynamicOffsets = {}
    );
    
    void bindVertexBuffer(uint32_t binding, vk::Buffer buffer, vk::DeviceSize offset = 0);
    void bindVertexBuffers(uint32_t firstBinding, const std::vector<vk::Buffer>& buffers, 
                          const std::vector<vk::DeviceSize>& offsets = {});
    void bindIndexBuffer(vk::Buffer buffer, vk::DeviceSize offset = 0, 
                        vk::IndexType indexType = vk::IndexType::eUint32);
    
    void draw(uint32_t vertexCount, uint32_t instanceCount = 1,
              uint32_t firstVertex = 0, uint32_t firstInstance = 0);
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
                     uint32_t firstIndex = 0, int32_t vertexOffset = 0, 
                     uint32_t firstInstance = 0);
    void drawIndirect(vk::Buffer buffer, vk::DeviceSize offset, uint32_t drawCount, 
                     uint32_t stride = sizeof(vk::DrawIndirectCommand));
    void drawIndexedIndirect(vk::Buffer buffer, vk::DeviceSize offset, uint32_t drawCount,
                            uint32_t stride = sizeof(vk::DrawIndexedIndirectCommand));
    
    void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    
    void setViewport(uint32_t firstViewport, const std::vector<vk::Viewport>& viewports);
    void setScissor(uint32_t firstScissor, const std::vector<vk::Rect2D>& scissors);
    void setLineWidth(float lineWidth);
    void setDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
    
    void pushConstants(vk::PipelineLayout layout, vk::ShaderStageFlags stageFlags,
                      uint32_t offset, uint32_t size, const void* values);
    
    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer,
                   const std::vector<vk::BufferCopy>& regions);
    void copyBufferToImage(vk::Buffer srcBuffer, vk::Image dstImage,
                          vk::ImageLayout dstImageLayout,
                          const std::vector<vk::BufferImageCopy>& regions);
    void copyImage(vk::Image srcImage, vk::ImageLayout srcImageLayout,
                  vk::Image dstImage, vk::ImageLayout dstImageLayout,
                  const std::vector<vk::ImageCopy>& regions);
    void copyImageToBuffer(vk::Image srcImage, vk::ImageLayout srcImageLayout,
                          vk::Buffer dstBuffer,
                          const std::vector<vk::BufferImageCopy>& regions);
    
    void pipelineBarrier(
        vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask,
        vk::DependencyFlags dependencyFlags,
        const std::vector<vk::MemoryBarrier>& memoryBarriers = {},
        const std::vector<vk::BufferMemoryBarrier>& bufferBarriers = {},
        const std::vector<vk::ImageMemoryBarrier>& imageBarriers = {}
    );
    
    void imageMemoryBarrier(
        vk::Image image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags srcAccessMask,
        vk::AccessFlags dstAccessMask,
        vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask,
        vk::ImageSubresourceRange subresourceRange
    );
    
private:
    vk::Device device = VK_NULL_HANDLE;
    vk::CommandPool pool = VK_NULL_HANDLE;
    vk::CommandBuffer commandBuffer = VK_NULL_HANDLE;
    State state = State::Idle;
};

// Command buffer manager for frame-based allocation
class CommandBufferManager {
public:
    CommandBufferManager();
    ~CommandBufferManager();
    
    void init(VulkanDevice* device, uint32_t frameCount);
    void cleanup();
    
    // Get command buffer for current frame
    VulkanCommandBuffer& getCurrentBuffer();
    vk::CommandBuffer getCurrentVkBuffer();
    
    // Frame management
    void beginFrame();
    void endFrame();
    void advanceFrame();
    
    // Getters
    uint32_t getCurrentFrame() const { return currentFrame; }
    uint32_t getFrameCount() const { return frameCount; }
    
private:
    VulkanDevice* device = nullptr;
    std::unique_ptr<VulkanCommandPool> pool;
    std::vector<VulkanCommandBuffer> buffers;
    uint32_t frameCount = 2;
    uint32_t currentFrame = 0;
};

} // namespace VoxelForge
