/**
 * @file VulkanBuffer.cpp
 * @brief Vulkan buffer management implementation
 */

#include <VoxelForge/rendering/VulkanBuffer.hpp>
#include <VoxelForge/rendering/VulkanContext.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <stdexcept>
#include <cstring>

namespace VoxelForge {

// ============================================================================
// Buffer Implementation
// ============================================================================

void Buffer::map(vk::Device device) {
    if (mapped) return;
    
    auto result = device.mapMemory(memory, 0, size, {}, &mapped);
    if (result != vk::Result::eSuccess) {
        LOG_ERROR("Failed to map buffer memory");
        throw std::runtime_error("Failed to map buffer memory");
    }
}

void Buffer::unmap(vk::Device device) {
    if (!mapped) return;
    
    device.unmapMemory(memory);
    mapped = nullptr;
}

void Buffer::writeToBuffer(vk::Device device, const void* data, vk::DeviceSize dataSize, vk::DeviceSize offset) {
    if (!mapped) {
        map(device);
    }
    
    vk::DeviceSize copySize = (dataSize == VK_WHOLE_SIZE) ? size : dataSize;
    memcpy(static_cast<char*>(mapped) + offset, data, static_cast<size_t>(copySize));
}

void Buffer::flush(vk::Device device, vk::DeviceSize flushSize, vk::DeviceSize offset) {
    vk::MappedMemoryRange mappedRange{};
    mappedRange.sType = vk::StructureType::eMappedMemoryRange;
    mappedRange.memory = memory;
    mappedRange.offset = offset;
    mappedRange.size = flushSize;
    
    device.flushMappedMemoryRanges(1, &mappedRange);
}

void Buffer::invalidate(vk::Device device, vk::DeviceSize invalidateSize, vk::DeviceSize offset) {
    vk::MappedMemoryRange mappedRange{};
    mappedRange.sType = vk::StructureType::eMappedMemoryRange;
    mappedRange.memory = memory;
    mappedRange.offset = offset;
    mappedRange.size = invalidateSize;
    
    device.invalidateMappedMemoryRanges(1, &mappedRange);
}

// ============================================================================
// VulkanBuffer Implementation
// ============================================================================

Buffer VulkanBuffer::createBuffer(
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties) {
    
    Buffer buffer;
    buffer.size = size;
    
    // Create buffer
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.sType = vk::StructureType::eBufferCreateInfo;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    
    try {
        buffer.buffer = device.createBuffer(bufferInfo);
    } catch (const vk::SystemError& e) {
        LOG_ERROR("Failed to create buffer: {}", e.what());
        throw;
    }
    
    // Get memory requirements
    auto memRequirements = device.getBufferMemoryRequirements(buffer.buffer);
    buffer.alignment = memRequirements.alignment;
    
    // Find memory type
    auto memProperties = physicalDevice.getMemoryProperties();
    uint32_t memoryTypeIndex = UINT32_MAX;
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            memoryTypeIndex = i;
            break;
        }
    }
    
    if (memoryTypeIndex == UINT32_MAX) {
        device.destroyBuffer(buffer.buffer);
        throw std::runtime_error("Failed to find suitable memory type!");
    }
    
    // Allocate memory
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    
    // Add device address bit for buffer device address feature
    if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
        vk::MemoryAllocateFlagsInfo flagsInfo{};
        flagsInfo.sType = vk::StructureType::eMemoryAllocateFlagsInfo;
        flagsInfo.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;
        allocInfo.pNext = &flagsInfo;
    }
    
    try {
        buffer.memory = device.allocateMemory(allocInfo);
    } catch (const vk::SystemError& e) {
        device.destroyBuffer(buffer.buffer);
        LOG_ERROR("Failed to allocate buffer memory: {}", e.what());
        throw;
    }
    
    // Bind memory
    device.bindBufferMemory(buffer.buffer, buffer.memory, 0);
    
    // Map if host visible
    if (properties & vk::MemoryPropertyFlagBits::eHostVisible) {
        buffer.map(device);
    }
    
    LOG_DEBUG("Created buffer: size={}, usage={}", size, static_cast<uint32_t>(usage));
    return buffer;
}

void VulkanBuffer::copyBuffer(
    vk::Device device,
    vk::Queue queue,
    vk::CommandPool pool,
    vk::Buffer srcBuffer,
    vk::Buffer dstBuffer,
    vk::DeviceSize size) {
    
    // Allocate command buffer
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = pool;
    allocInfo.commandBufferCount = 1;
    
    vk::CommandBuffer commandBuffer;
    try {
        commandBuffer = device.allocateCommandBuffers(allocInfo)[0];
    } catch (const vk::SystemError& e) {
        LOG_ERROR("Failed to allocate command buffer for copy: {}", e.what());
        throw;
    }
    
    // Begin command buffer
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    
    commandBuffer.begin(beginInfo);
    
    // Copy command
    vk::BufferCopy copyRegion{};
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
    
    commandBuffer.end();
    
    // Submit
    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    try {
        queue.submit(1, &submitInfo, vk::Fence{});
        queue.waitIdle();
    } catch (const vk::SystemError& e) {
        LOG_ERROR("Failed to submit copy command: {}", e.what());
        device.freeCommandBuffers(pool, 1, &commandBuffer);
        throw;
    }
    
    device.freeCommandBuffers(pool, 1, &commandBuffer);
}

void VulkanBuffer::copyBufferToImage(
    vk::Device device,
    vk::Queue queue,
    vk::CommandPool pool,
    vk::Buffer buffer,
    vk::Image image,
    uint32_t width,
    uint32_t height,
    uint32_t layerCount) {
    
    // Allocate command buffer
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = pool;
    allocInfo.commandBufferCount = 1;
    
    vk::CommandBuffer commandBuffer = device.allocateCommandBuffers(allocInfo)[0];
    
    // Begin command buffer
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    
    commandBuffer.begin(beginInfo);
    
    // Copy buffer to image
    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layerCount;
    region.imageOffset = vk::Offset3D{0, 0, 0};
    region.imageExtent = vk::Extent3D{width, height, 1};
    
    commandBuffer.copyBufferToImage(
        buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);
    
    commandBuffer.end();
    
    // Submit
    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    queue.submit(1, &submitInfo, vk::Fence{});
    queue.waitIdle();
    
    device.freeCommandBuffers(pool, 1, &commandBuffer);
}

void VulkanBuffer::destroyBuffer(vk::Device device, Buffer& buffer) {
    if (buffer.mapped) {
        device.unmapMemory(buffer.memory);
        buffer.mapped = nullptr;
    }
    
    if (buffer.memory) {
        device.freeMemory(buffer.memory);
        buffer.memory = vk::DeviceMemory{};
    }
    
    if (buffer.buffer) {
        device.destroyBuffer(buffer.buffer);
        buffer.buffer = vk::Buffer{};
    }
    
    buffer.size = 0;
    buffer.alignment = 0;
}

Buffer VulkanBuffer::createStagingBuffer(
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::DeviceSize size) {
    
    return createBuffer(
        device,
        physicalDevice,
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | 
        vk::MemoryPropertyFlagBits::eHostCoherent
    );
}

Buffer VulkanBuffer::createUniformBuffer(
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::DeviceSize size) {
    
    return createBuffer(
        device,
        physicalDevice,
        size,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | 
        vk::MemoryPropertyFlagBits::eHostCoherent
    );
}

Buffer VulkanBuffer::createVertexBuffer(
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::Queue queue,
    vk::CommandPool pool,
    const void* data,
    vk::DeviceSize size) {
    
    // Create staging buffer
    auto stagingBuffer = createStagingBuffer(device, physicalDevice, size);
    
    // Copy data to staging buffer
    memcpy(stagingBuffer.mapped, data, static_cast<size_t>(size));
    
    // Create vertex buffer on device
    auto vertexBuffer = createBuffer(
        device,
        physicalDevice,
        size,
        vk::BufferUsageFlagBits::eTransferDst | 
        vk::BufferUsageFlagBits::eVertexBuffer | 
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    
    // Copy from staging to vertex buffer
    copyBuffer(device, queue, pool, stagingBuffer.buffer, vertexBuffer.buffer, size);
    
    // Clean up staging buffer
    destroyBuffer(device, stagingBuffer);
    
    LOG_DEBUG("Created vertex buffer: size={}", size);
    return vertexBuffer;
}

Buffer VulkanBuffer::createIndexBuffer(
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::Queue queue,
    vk::CommandPool pool,
    const void* data,
    vk::DeviceSize size,
    vk::IndexType indexType) {
    
    // Create staging buffer
    auto stagingBuffer = createStagingBuffer(device, physicalDevice, size);
    
    // Copy data to staging buffer
    memcpy(stagingBuffer.mapped, data, static_cast<size_t>(size));
    
    // Create index buffer on device
    auto indexBuffer = createBuffer(
        device,
        physicalDevice,
        size,
        vk::BufferUsageFlagBits::eTransferDst | 
        vk::BufferUsageFlagBits::eIndexBuffer |
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    
    // Copy from staging to index buffer
    copyBuffer(device, queue, pool, stagingBuffer.buffer, indexBuffer.buffer, size);
    
    // Clean up staging buffer
    destroyBuffer(device, stagingBuffer);
    
    LOG_DEBUG("Created index buffer: size={}, indexType={}", size, static_cast<uint32_t>(indexType));
    return indexBuffer;
}

Buffer VulkanBuffer::createStorageBuffer(
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::DeviceSize size) {
    
    return createBuffer(
        device,
        physicalDevice,
        size,
        vk::BufferUsageFlagBits::eStorageBuffer | 
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
}

// ============================================================================
// VulkanRingBuffer Implementation
// ============================================================================

VulkanRingBuffer::VulkanRingBuffer(
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::DeviceSize size) 
    : capacity(size), offset(0), device(device) {
    
    buffer = VulkanBuffer::createBuffer(
        device,
        physicalDevice,
        size,
        vk::BufferUsageFlagBits::eUniformBuffer | 
        vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | 
        vk::MemoryPropertyFlagBits::eHostCoherent
    );
    
    LOG_DEBUG("Created ring buffer: size={}", size);
}

VulkanRingBuffer::~VulkanRingBuffer() {
    VulkanBuffer::destroyBuffer(device, buffer);
}

vk::DeviceSize VulkanRingBuffer::allocate(vk::DeviceSize size, vk::DeviceSize alignment) {
    // Align offset
    vk::DeviceSize alignedOffset = (offset + alignment - 1) & ~(alignment - 1);
    
    if (alignedOffset + size > capacity) {
        LOG_ERROR("Ring buffer overflow: requested={}, available={}", 
                  size, capacity - alignedOffset);
        return VK_WHOLE_SIZE; // Indicates failure
    }
    
    offset = alignedOffset + size;
    return alignedOffset;
}

void VulkanRingBuffer::reset() {
    offset = 0;
}

// ============================================================================
// VulkanBufferPool Implementation
// ============================================================================

VulkanBufferPool::VulkanBufferPool(
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::DeviceSize chunkSize)
    : device(device)
    , physicalDevice(physicalDevice)
    , chunkSize(chunkSize) {
    
    LOG_DEBUG("Created buffer pool: chunkSize={}", chunkSize);
}

VulkanBufferPool::~VulkanBufferPool() {
    for (auto& chunk : chunks) {
        VulkanBuffer::destroyBuffer(device, chunk.buffer);
    }
}

VulkanBufferPool::Chunk& VulkanBufferPool::getOrCreateChunk() {
    // Check if current chunk has enough space
    if (!chunks.empty()) {
        auto& lastChunk = chunks.back();
        if (lastChunk.used < chunkSize * 0.9) { // Allow 90% usage
            return lastChunk;
        }
    }
    
    // Create new chunk
    Chunk newChunk;
    newChunk.buffer = VulkanBuffer::createBuffer(
        device,
        physicalDevice,
        chunkSize,
        vk::BufferUsageFlagBits::eVertexBuffer | 
        vk::BufferUsageFlagBits::eIndexBuffer |
        vk::BufferUsageFlagBits::eUniformBuffer |
        vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | 
        vk::MemoryPropertyFlagBits::eHostCoherent
    );
    newChunk.used = 0;
    
    chunks.push_back(std::move(newChunk));
    LOG_DEBUG("Created new buffer pool chunk: total chunks={}", chunks.size());
    
    return chunks.back();
}

VulkanBufferPool::Allocation VulkanBufferPool::allocate(vk::DeviceSize size, vk::DeviceSize alignment) {
    if (size > chunkSize) {
        LOG_ERROR("Allocation size {} exceeds chunk size {}", size, chunkSize);
        return {vk::Buffer{}, 0, 0, nullptr};
    }
    
    auto& chunk = getOrCreateChunk();
    
    // Align offset
    vk::DeviceSize alignedOffset = (chunk.used + alignment - 1) & ~(alignment - 1);
    
    if (alignedOffset + size > chunkSize) {
        // Need new chunk
        chunk = getOrCreateChunk();
        alignedOffset = 0;
    }
    
    Allocation alloc;
    alloc.buffer = chunk.buffer.buffer;
    alloc.offset = alignedOffset;
    alloc.size = size;
    alloc.mapped = static_cast<char*>(chunk.buffer.mapped) + alignedOffset;
    
    chunk.used = alignedOffset + size;
    
    return alloc;
}

void VulkanBufferPool::reset() {
    for (auto& chunk : chunks) {
        chunk.used = 0;
    }
    // Optionally trim excess chunks
    while (chunks.size() > 1) {
        VulkanBuffer::destroyBuffer(device, chunks.back().buffer);
        chunks.pop_back();
    }
}

} // namespace VoxelForge
