/**
 * @file Memory.hpp
 * @brief Memory management utilities for VoxelForge
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <mutex>
#include <atomic>
#include <string>

namespace VoxelForge {

// ============================================
// Memory Statistics
// ============================================

struct MemoryStats {
    std::atomic<size_t> totalAllocated{0};
    std::atomic<size_t> totalFreed{0};
    std::atomic<size_t> currentUsage{0};
    std::atomic<size_t> allocationCount{0};
    
    size_t getCurrentUsage() const { return currentUsage.load(); }
    size_t getTotalAllocated() const { return totalAllocated.load(); }
    size_t getTotalFreed() const { return totalFreed.load(); }
    size_t getAllocationCount() const { return allocationCount.load(); }
};

// ============================================
// Linear Allocator (Arena)
// ============================================

class LinearAllocator {
public:
    explicit LinearAllocator(size_t size);
    ~LinearAllocator();
    
    void* allocate(size_t size, size_t alignment = 8);
    void reset();
    
    size_t getCapacity() const { return capacity; }
    size_t getUsed() const { return offset; }
    size_t getRemaining() const { return capacity - offset; }
    
    // No copy
    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;
    
private:
    void* memory;
    size_t capacity;
    size_t offset;
};

// ============================================
// Pool Allocator
// ============================================

class PoolAllocator {
public:
    PoolAllocator(size_t objectSize, size_t capacity);
    ~PoolAllocator();
    
    void* allocate();
    void deallocate(void* ptr);
    
    size_t getObjectSize() const { return objectSize; }
    size_t getCapacity() const { return capacity; }
    size_t getUsed() const { return used; }
    
private:
    void* memory;
    void* freeList;
    size_t objectSize;
    size_t capacity;
    size_t used;
};

// ============================================
// Memory Arena (RAII)
// ============================================

class MemoryArena {
public:
    explicit MemoryArena(size_t size, const std::string& name = "Arena");
    ~MemoryArena();
    
    template<typename T, typename... Args>
    T* create(Args&&... args) {
        void* ptr = allocator.allocate(sizeof(T), alignof(T));
        return new (ptr) T(std::forward<Args>(args)...);
    }
    
    template<typename T>
    void destroy(T* obj) {
        obj->~T();
        // Don't deallocate - arena is reset as a whole
    }
    
    void reset() { allocator.reset(); }
    
    size_t getUsed() const { return allocator.getUsed(); }
    size_t getCapacity() const { return allocator.getCapacity(); }
    const std::string& getName() const { return name; }
    
private:
    LinearAllocator allocator;
    std::string name;
};

// ============================================
// Global Memory Functions
// ============================================

namespace Memory {
    void init();
    void shutdown();
    
    MemoryStats& getStats();
    
    // Arena access
    MemoryArena& getWorldArena();
    MemoryArena& getRenderArena();
    MemoryArena& getTempArena();
    
    // Reset temp arena each frame
    void resetTempArena();
}

} // namespace VoxelForge
