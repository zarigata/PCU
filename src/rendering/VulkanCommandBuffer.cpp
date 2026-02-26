/**
 * @file VulkanCommandBuffer.cpp
 * @brief Vulkan command buffer and pool management implementation
 */

#include "VulkanCommandBuffer.hpp"
#include <VoxelForge/rendering/VulkanDevice.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

// ============== VulkanCommandPool ==============

VulkanCommandPool::VulkanCommandPool() = default;

VulkanCommandPool::~VulkanCommandPool() {
    cleanup();
}

void VulkanCommandPool::init(VulkanDevice* device, Type type, bool transient, bool resetable) {
    this->device = device;
    this->type = type;
    
    auto& indices = device->getQueueFamilies();
    
    switch (type) {
        case Type::Graphics:
            queueFamilyIndex = indices.graphicsFamily.value();
            break;
        case Type::Compute:
            queueFamilyIndex = indices.computeFamily.value_or(indices.graphicsFamily.value());
            break;
        case Type::Transfer:
            queueFamilyIndex = indices.transferFamily.value_or(indices.graphicsFamily.value());
            break;
        case Type::Present:
            queueFamilyIndex = indices.presentFamily.value();
            break;
    }
    
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    
    if (transient) {
        poolInfo.flags |= vk::CommandPoolCreateFlagBits::eTransient;
    }
    if (resetable) {
        poolInfo.flags |= vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    }
    
    try {
        pool = device->getDevice().createCommandPool(poolInfo);
    } catch (const vk::SystemError& e) {
        throw std::runtime_error("Failed to create command pool: " + std::string(e.what()));
    }
    
    Logger::debug("Command pool created for queue family {}", queueFamilyIndex);
}

void VulkanCommandPool::cleanup() {
    if (device && pool) {
        device->getDevice().destroyCommandPool(pool);
        pool = nullptr;
    }
}

vk::CommandBuffer VulkanCommandPool::allocate(bool primary) {
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = pool;
    allocInfo.level = primary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary;
    allocInfo.commandBufferCount = 1;
    
    try {
        return device->getDevice().allocateCommandBuffers(allocInfo)[0];
    } catch (const vk::SystemError& e) {
        throw std::runtime_error("Failed to allocate command buffer: " + std::string(e.what()));
    }
}

std::vector<vk::CommandBuffer> VulkanCommandPool::allocate(uint32_t count, bool primary) {
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = pool;
    allocInfo.level = primary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary;
    allocInfo.commandBufferCount = count;
    
    try {
        return device->getDevice().allocateCommandBuffers(allocInfo);
    } catch (const vk::SystemError& e) {
        throw std::runtime_error("Failed to allocate command buffers: " + std::string(e.what()));
    }
}

void VulkanCommandPool::free(vk::CommandBuffer cmd) {
    if (device && pool && cmd) {
        device->getDevice().freeCommandBuffers(pool, cmd);
    }
}

void VulkanCommandPool::free(const std::vector<vk::CommandBuffer>& cmds) {
    if (device && pool && !cmds.empty()) {
        device->getDevice().freeCommandBuffers(pool, cmds);
    }
}

void VulkanCommandPool::reset(bool releaseResources) {
    if (device && pool) {
        auto flags = releaseResources ? 
            vk::CommandPoolResetFlagBits::eReleaseResources : 
            vk::CommandPoolResetFlags{};
        device->getDevice().resetCommandPool(pool, flags);
    }
}

vk::CommandBuffer VulkanCommandPool::beginSingleTimeCommand() {
    auto cmd = allocate(true);
    
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmd.begin(beginInfo);
    
    return cmd;
}

void VulkanCommandPool::endSingleTimeCommand(vk::CommandBuffer cmd) {
    cmd.end();
    
    vk::Queue queue;
    switch (type) {
        case Type::Graphics: queue = device->getGraphicsQueue(); break;
        case Type::Compute: queue = device->getComputeQueue(); break;
        case Type::Transfer: queue = device->getTransferQueue(); break;
        case Type::Present: queue = device->getPresentQueue(); break;
    }
    
    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    
    queue.submit(submitInfo, nullptr);
    queue.waitIdle();
    
    free(cmd);
}

// ============== VulkanCommandBuffer ==============

VulkanCommandBuffer::VulkanCommandBuffer() = default;

VulkanCommandBuffer::~VulkanCommandBuffer() {
    cleanup();
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer&& other) noexcept
    : device(other.device), pool(other.pool), commandBuffer(other.commandBuffer), state(other.state) {
    other.device = VK_NULL_HANDLE;
    other.pool = VK_NULL_HANDLE;
    other.commandBuffer = VK_NULL_HANDLE;
    other.state = State::Idle;
}

VulkanCommandBuffer& VulkanCommandBuffer::operator=(VulkanCommandBuffer&& other) noexcept {
    if (this != &other) {
        cleanup();
        device = other.device;
        pool = other.pool;
        commandBuffer = other.commandBuffer;
        state = other.state;
        
        other.device = VK_NULL_HANDLE;
        other.pool = VK_NULL_HANDLE;
        other.commandBuffer = VK_NULL_HANDLE;
        other.state = State::Idle;
    }
    return *this;
}

void VulkanCommandBuffer::init(vk::Device device, vk::CommandPool pool, bool primary) {
    this->device = device;
    this->pool = pool;
    
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = pool;
    allocInfo.level = primary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary;
    allocInfo.commandBufferCount = 1;
    
    try {
        commandBuffer = device.allocateCommandBuffers(allocInfo)[0];
    } catch (const vk::SystemError& e) {
        throw std::runtime_error("Failed to allocate command buffer: " + std::string(e.what()));
    }
    
    state = State::Idle;
}

void VulkanCommandBuffer::cleanup() {
    if (device && pool && commandBuffer) {
        device.freeCommandBuffers(pool, commandBuffer);
        commandBuffer = nullptr;
        state = State::Idle;
    }
}

void VulkanCommandBuffer::begin(vk::CommandBufferUsageFlags flags) {
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = flags;
    commandBuffer.begin(beginInfo);
    state = State::Recording;
}

void VulkanCommandBuffer::end() {
    commandBuffer.end();
    state = State::Executable;
}

void VulkanCommandBuffer::reset(bool releaseResources) {
    auto flags = releaseResources ? 
        vk::CommandBufferResetFlagBits::eReleaseResources : 
        vk::CommandBufferResetFlags{};
    commandBuffer.reset(flags);
    state = State::Idle;
}

void VulkanCommandBuffer::beginRenderPass(
    vk::RenderPass renderPass,
    vk::Framebuffer framebuffer,
    vk::Rect2D renderArea,
    const std::vector<vk::ClearValue>& clearValues,
    vk::SubpassContents contents
) {
    vk::RenderPassBeginInfo beginInfo{};
    beginInfo.renderPass = renderPass;
    beginInfo.framebuffer = framebuffer;
    beginInfo.renderArea = renderArea;
    beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    beginInfo.pClearValues = clearValues.data();
    
    commandBuffer.beginRenderPass(beginInfo, contents);
}

void VulkanCommandBuffer::endRenderPass() {
    commandBuffer.endRenderPass();
}

void VulkanCommandBuffer::bindPipeline(vk::PipelineBindPoint bindPoint, vk::Pipeline pipeline) {
    commandBuffer.bindPipeline(bindPoint, pipeline);
}

void VulkanCommandBuffer::bindDescriptorSets(
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout layout,
    uint32_t firstSet,
    const std::vector<vk::DescriptorSet>& descriptorSets,
    const std::vector<uint32_t>& dynamicOffsets
) {
    commandBuffer.bindDescriptorSets(
        bindPoint, layout, firstSet,
        static_cast<uint32_t>(descriptorSets.size()),
        descriptorSets.data(),
        static_cast<uint32_t>(dynamicOffsets.size()),
        dynamicOffsets.empty() ? nullptr : dynamicOffsets.data()
    );
}

void VulkanCommandBuffer::bindVertexBuffer(uint32_t binding, vk::Buffer buffer, vk::DeviceSize offset) {
    commandBuffer.bindVertexBuffers(binding, 1, &buffer, &offset);
}

void VulkanCommandBuffer::bindVertexBuffers(uint32_t firstBinding, 
                                            const std::vector<vk::Buffer>& buffers,
                                            const std::vector<vk::DeviceSize>& offsets) {
    auto offsetData = offsets.empty() ? 
        std::vector<vk::DeviceSize>(buffers.size(), 0) : offsets;
    commandBuffer.bindVertexBuffers(firstBinding, 
        static_cast<uint32_t>(buffers.size()), buffers.data(), offsetData.data());
}

void VulkanCommandBuffer::bindIndexBuffer(vk::Buffer buffer, vk::DeviceSize offset, vk::IndexType indexType) {
    commandBuffer.bindIndexBuffer(buffer, offset, indexType);
}

void VulkanCommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount,
                               uint32_t firstVertex, uint32_t firstInstance) {
    commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                      uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandBuffer::drawIndirect(vk::Buffer buffer, vk::DeviceSize offset, 
                                       uint32_t drawCount, uint32_t stride) {
    commandBuffer.drawIndirect(buffer, offset, drawCount, stride);
}

void VulkanCommandBuffer::drawIndexedIndirect(vk::Buffer buffer, vk::DeviceSize offset, 
                                              uint32_t drawCount, uint32_t stride) {
    commandBuffer.drawIndexedIndirect(buffer, offset, drawCount, stride);
}

void VulkanCommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    commandBuffer.dispatch(groupCountX, groupCountY, groupCountZ);
}

void VulkanCommandBuffer::setViewport(uint32_t firstViewport, const std::vector<vk::Viewport>& viewports) {
    commandBuffer.setViewport(firstViewport, static_cast<uint32_t>(viewports.size()), viewports.data());
}

void VulkanCommandBuffer::setScissor(uint32_t firstScissor, const std::vector<vk::Rect2D>& scissors) {
    commandBuffer.setScissor(firstScissor, static_cast<uint32_t>(scissors.size()), scissors.data());
}

void VulkanCommandBuffer::setLineWidth(float lineWidth) {
    commandBuffer.setLineWidth(lineWidth);
}

void VulkanCommandBuffer::setDepthBias(float depthBiasConstantFactor, float depthBiasClamp, 
                                       float depthBiasSlopeFactor) {
    commandBuffer.setDepthBias(depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

void VulkanCommandBuffer::pushConstants(vk::PipelineLayout layout, vk::ShaderStageFlags stageFlags,
                                        uint32_t offset, uint32_t size, const void* values) {
    commandBuffer.pushConstants(layout, stageFlags, offset, size, values);
}

void VulkanCommandBuffer::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer,
                                     const std::vector<vk::BufferCopy>& regions) {
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 
        static_cast<uint32_t>(regions.size()), regions.data());
}

void VulkanCommandBuffer::copyBufferToImage(vk::Buffer srcBuffer, vk::Image dstImage,
                                            vk::ImageLayout dstImageLayout,
                                            const std::vector<vk::BufferImageCopy>& regions) {
    commandBuffer.copyBufferToImage(srcBuffer, dstImage, dstImageLayout,
        static_cast<uint32_t>(regions.size()), regions.data());
}

void VulkanCommandBuffer::copyImage(vk::Image srcImage, vk::ImageLayout srcImageLayout,
                                    vk::Image dstImage, vk::ImageLayout dstImageLayout,
                                    const std::vector<vk::ImageCopy>& regions) {
    commandBuffer.copyImage(srcImage, srcImageLayout, dstImage, dstImageLayout,
        static_cast<uint32_t>(regions.size()), regions.data());
}

void VulkanCommandBuffer::copyImageToBuffer(vk::Image srcImage, vk::ImageLayout srcImageLayout,
                                            vk::Buffer dstBuffer,
                                            const std::vector<vk::BufferImageCopy>& regions) {
    commandBuffer.copyImageToBuffer(srcImage, srcImageLayout, dstBuffer,
        static_cast<uint32_t>(regions.size()), regions.data());
}

void VulkanCommandBuffer::pipelineBarrier(
    vk::PipelineStageFlags srcStageMask,
    vk::PipelineStageFlags dstStageMask,
    vk::DependencyFlags dependencyFlags,
    const std::vector<vk::MemoryBarrier>& memoryBarriers,
    const std::vector<vk::BufferMemoryBarrier>& bufferBarriers,
    const std::vector<vk::ImageMemoryBarrier>& imageBarriers
) {
    commandBuffer.pipelineBarrier(
        srcStageMask, dstStageMask, dependencyFlags,
        static_cast<uint32_t>(memoryBarriers.size()), memoryBarriers.data(),
        static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
        static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data()
    );
}

void VulkanCommandBuffer::imageMemoryBarrier(
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags srcAccessMask,
    vk::AccessFlags dstAccessMask,
    vk::PipelineStageFlags srcStageMask,
    vk::PipelineStageFlags dstStageMask,
    vk::ImageSubresourceRange subresourceRange
) {
    vk::ImageMemoryBarrier barrier{};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = subresourceRange;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    
    pipelineBarrier(srcStageMask, dstStageMask, vk::DependencyFlagBits::eByRegion,
                   {}, {}, {barrier});
}

// ============== CommandBufferManager ==============

CommandBufferManager::CommandBufferManager() = default;

CommandBufferManager::~CommandBufferManager() {
    cleanup();
}

void CommandBufferManager::init(VulkanDevice* device, uint32_t frameCount) {
    this->device = device;
    this->frameCount = frameCount;
    
    pool = std::make_unique<VulkanCommandPool>();
    pool->init(device, VulkanCommandPool::Type::Graphics, true, true);
    
    buffers.resize(frameCount);
    for (auto& buffer : buffers) {
        buffer.init(device->getDevice(), pool->getPool(), true);
    }
    
    Logger::debug("CommandBufferManager initialized with {} buffers", frameCount);
}

void CommandBufferManager::cleanup() {
    for (auto& buffer : buffers) {
        buffer.cleanup();
    }
    buffers.clear();
    pool.reset();
}

VulkanCommandBuffer& CommandBufferManager::getCurrentBuffer() {
    return buffers[currentFrame];
}

vk::CommandBuffer CommandBufferManager::getCurrentVkBuffer() {
    return buffers[currentFrame].get();
}

void CommandBufferManager::beginFrame() {
    buffers[currentFrame].reset();
    buffers[currentFrame].begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
}

void CommandBufferManager::endFrame() {
    buffers[currentFrame].end();
}

void CommandBufferManager::advanceFrame() {
    currentFrame = (currentFrame + 1) % frameCount;
}

} // namespace VoxelForge
