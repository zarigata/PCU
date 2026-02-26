/**
 * @file Memory.cpp
 * @brief Memory management implementation
 */

#include <VoxelForge/core/Memory.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <cstdlib>
#include <cstring>
#include <new>

namespace VoxelForge {

// ============================================
// Global Memory State
// ============================================

static MemoryStats globalStats;
static MemoryArena* worldArena = nullptr;
static MemoryArena* renderArena = nullptr;
static MemoryArena* tempArena = nullptr;

// ============================================
// Linear Allocator
// ============================================

LinearAllocator::LinearAllocator(size_t size) 
    : capacity(size), offset(0) {
    memory = std::malloc(size);
    if (!memory) {
        throw std::bad_alloc();
    }
    std::memset(memory, 0, size);
}

LinearAllocator::~LinearAllocator() {
    std::free(memory);
}

void* LinearAllocator::allocate(size_t size, size_t alignment) {
    // Calculate aligned offset
    uintptr_t current = reinterpret_cast<uintptr_t>(memory) + offset;
    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    
    size_t padding = aligned - current;
    size_t totalSize = size + padding;
    
    if (offset + totalSize > capacity) {
        return nullptr; // Out of memory
    }
    
    offset += totalSize;
    return reinterpret_cast<void*>(aligned);
}

void LinearAllocator::reset() {
    offset = 0;
}

// ============================================
// Pool Allocator
// ============================================

PoolAllocator::PoolAllocator(size_t objSize, size_t cap)
    : objectSize(objSize), capacity(cap), used(0) {
    // Align object size to pointer size
    objectSize = (objSize + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
    
    memory = std::malloc(objectSize * capacity);
    if (!memory) {
        throw std::bad_alloc();
    }
    
    // Build free list
    freeList = memory;
    char* current = static_cast<char*>(memory);
    for (size_t i = 0; i < capacity - 1; i++) {
        *reinterpret_cast<void**>(current) = current + objectSize;
        current += objectSize;
    }
    *reinterpret_cast<void**>(current) = nullptr;
}

PoolAllocator::~PoolAllocator() {
    std::free(memory);
}

void* PoolAllocator::allocate() {
    if (!freeList) {
        return nullptr; // Pool exhausted
    }
    
    void* ptr = freeList;
    freeList = *reinterpret_cast<void**>(freeList);
    used++;
    return ptr;
}

void PoolAllocator::deallocate(void* ptr) {
    if (!ptr) return;
    
    *reinterpret_cast<void**>(ptr) = freeList;
    freeList = ptr;
    used--;
}

// ============================================
// Memory Arena
// ============================================

MemoryArena::MemoryArena(size_t size, const std::string& name)
    : allocator(size), name(name) {
    VF_CORE_INFO("Created memory arena '{}' ({} MB)", name, size / (1024 * 1024));
}

MemoryArena::~MemoryArena() {
    VF_CORE_INFO("Destroyed memory arena '{}'", name);
}

// ============================================
// Global Memory Functions
// ============================================

namespace Memory {

void init() {
    VF_CORE_INFO("Initializing memory system...");
    
    // Create global arenas
    worldArena = new MemoryArena(512 * 1024 * 1024, "World");   // 512 MB
    renderArena = new MemoryArena(256 * 1024 * 1024, "Render"); // 256 MB
    tempArena = new MemoryArena(32 * 1024 * 1024, "Temp");      // 32 MB
    
    VF_CORE_INFO("Memory system initialized");
    VF_CORE_INFO("  World Arena: {} MB", worldArena->getCapacity() / (1024 * 1024));
    VF_CORE_INFO("  Render Arena: {} MB", renderArena->getCapacity() / (1024 * 1024));
    VF_CORE_INFO("  Temp Arena: {} MB", tempArena->getCapacity() / (1024 * 1024));
}

void shutdown() {
    VF_CORE_INFO("Shutting down memory system...");
    
    delete worldArena;
    delete renderArena;
    delete tempArena;
    
    worldArena = nullptr;
    renderArena = nullptr;
    tempArena = nullptr;
    
    VF_CORE_INFO("Memory system shut down");
    VF_CORE_INFO("  Total allocated: {} bytes", globalStats.getTotalAllocated());
    VF_CORE_INFO("  Total freed: {} bytes", globalStats.getTotalFreed());
}

MemoryStats& getStats() {
    return globalStats;
}

MemoryArena& getWorldArena() {
    return *worldArena;
}

MemoryArena& getRenderArena() {
    return *renderArena;
}

MemoryArena& getTempArena() {
    return *tempArena;
}

void resetTempArena() {
    tempArena->reset();
}

} // namespace Memory

} // namespace VoxelForge
