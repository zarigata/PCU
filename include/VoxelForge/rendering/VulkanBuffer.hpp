/**
 * @file VulkanBuffer.hpp
 * @brief Vulkan buffer management
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>

namespace VoxelForge {

class VulkanContext;

struct Buffer {
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    void* mapped = nullptr;
    vk::DeviceSize size = 0;
    vk::DeviceSize alignment = 0;
    
    void map(vk::Device device);
    void unmap(vk::Device device);
    void writeToBuffer(vk::Device device, const void* data, vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
    void flush(vk::Device device, vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
    void invalidate(vk::Device device, vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
};

class VulkanBuffer {
public:
    static Buffer createBuffer(
        vk::Device device,
        vk::PhysicalDevice physicalDevice,
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties
    );
    
    static void copyBuffer(
        vk::Device device,
        vk::Queue queue,
        vk::CommandPool pool,
        vk::Buffer srcBuffer,
        vk::Buffer dstBuffer,
        vk::DeviceSize size
    );
    
    static void copyBufferToImage(
        vk::Device device,
        vk::Queue queue,
        vk::CommandPool pool,
        vk::Buffer buffer,
        vk::Image image,
        uint32_t width,
        uint32_t height,
        uint32_t layerCount = 1
    );
    
    static void destroyBuffer(vk::Device device, Buffer& buffer);
    
    // Staging buffer helper
    static Buffer createStagingBuffer(
        vk::Device device,
        vk::PhysicalDevice physicalDevice,
        vk::DeviceSize size
    );
    
    // Uniform buffer helpers
    static Buffer createUniformBuffer(
        vk::Device device,
        vk::PhysicalDevice physicalDevice,
        vk::DeviceSize size
    );
    
    // Vertex buffer helpers
    static Buffer createVertexBuffer(
        vk::Device device,
        vk::PhysicalDevice physicalDevice,
        vk::Queue queue,
        vk::CommandPool pool,
        const void* data,
        vk::DeviceSize size
    );
    
    // Index buffer helpers
    static Buffer createIndexBuffer(
        vk::Device device,
        vk::PhysicalDevice physicalDevice,
        vk::Queue queue,
        vk::CommandPool pool,
        const void* data,
        vk::DeviceSize size,
        vk::IndexType indexType = vk::IndexType::eUint32
    );
    
    // Storage buffer helpers
    static Buffer createStorageBuffer(
        vk::Device device,
        vk::PhysicalDevice physicalDevice,
        vk::DeviceSize size
    );
};

// Ring buffer for dynamic allocations
class VulkanRingBuffer {
public:
    VulkanRingBuffer(
        vk::Device device,
        vk::PhysicalDevice physicalDevice,
        vk::DeviceSize size
    );
    
    ~VulkanRingBuffer();
    
    vk::DeviceSize allocate(vk::DeviceSize size, vk::DeviceSize alignment);
    void reset();
    
    vk::Buffer getBuffer() const { return buffer.buffer; }
    void* getMapped() const { return buffer.mapped; }
    
private:
    Buffer buffer;
    vk::DeviceSize capacity;
    vk::DeviceSize offset;
    vk::Device device;
};

// Buffer pool for reusable allocations
class VulkanBufferPool {
public:
    VulkanBufferPool(
        vk::Device device,
        vk::PhysicalDevice physicalDevice,
        vk::DeviceSize chunkSize = 64 * 1024 * 1024  // 64 MB default
    );
    
    ~VulkanBufferPool();
    
    struct Allocation {
        vk::Buffer buffer;
        vk::DeviceSize offset;
        vk::DeviceSize size;
        void* mapped;
    };
    
    Allocation allocate(vk::DeviceSize size, vk::DeviceSize alignment);
    void reset();  // Reset all allocations
    
private:
    struct Chunk {
        Buffer buffer;
        vk::DeviceSize used;
    };
    
    vk::Device device;
    vk::PhysicalDevice physicalDevice;
    vk::DeviceSize chunkSize;
    std::vector<Chunk> chunks;
    
    Chunk& getOrCreateChunk();
};

} // namespace VoxelForge
