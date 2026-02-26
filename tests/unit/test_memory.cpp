/**
 * @file test_memory.cpp
 * @brief Memory system unit tests
 */

#include <gtest/gtest.h>
#include <VoxelForge/core/Memory.hpp>

using namespace VoxelForge;

class MemoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        Memory::init();
    }
    
    void TearDown() override {
        Memory::shutdown();
    }
};

TEST_F(MemoryTest, LinearAllocatorBasicAllocation) {
    LinearAllocator allocator(1024);
    
    void* ptr1 = allocator.allocate(100);
    EXPECT_NE(ptr1, nullptr);
    EXPECT_EQ(allocator.getUsed(), 100);
    
    void* ptr2 = allocator.allocate(200);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_GE(allocator.getUsed(), 300);
}

TEST_F(MemoryTest, LinearAllocatorAlignment) {
    LinearAllocator allocator(1024);
    
    void* ptr1 = allocator.allocate(1, 16);
    EXPECT_NE(ptr1, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr1) % 16, 0);
    
    void* ptr2 = allocator.allocate(1, 32);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % 32, 0);
}

TEST_F(MemoryTest, LinearAllocatorOverflow) {
    LinearAllocator allocator(100);
    
    void* ptr1 = allocator.allocate(50);
    EXPECT_NE(ptr1, nullptr);
    
    void* ptr2 = allocator.allocate(60);
    EXPECT_EQ(ptr2, nullptr); // Should fail - not enough space
}

TEST_F(MemoryTest, LinearAllocatorReset) {
    LinearAllocator allocator(1024);
    
    allocator.allocate(500);
    EXPECT_GE(allocator.getUsed(), 500);
    
    allocator.reset();
    EXPECT_EQ(allocator.getUsed(), 0);
    
    void* ptr = allocator.allocate(500);
    EXPECT_NE(ptr, nullptr); // Should work after reset
}

TEST_F(MemoryTest, PoolAllocatorBasicAllocation) {
    PoolAllocator pool(sizeof(int), 10);
    
    int* ptr1 = static_cast<int*>(pool.allocate());
    EXPECT_NE(ptr1, nullptr);
    *ptr1 = 42;
    
    int* ptr2 = static_cast<int*>(pool.allocate());
    EXPECT_NE(ptr2, nullptr);
    *ptr2 = 100;
    
    EXPECT_EQ(pool.getUsed(), 2);
}

TEST_F(MemoryTest, PoolAllocatorDeallocation) {
    PoolAllocator pool(sizeof(int), 10);
    
    int* ptr = static_cast<int*>(pool.allocate());
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(pool.getUsed(), 1);
    
    pool.deallocate(ptr);
    EXPECT_EQ(pool.getUsed(), 0);
    
    // Should be able to allocate again
    int* ptr2 = static_cast<int*>(pool.allocate());
    EXPECT_NE(ptr2, nullptr);
}

TEST_F(MemoryTest, PoolAllocatorOverflow) {
    PoolAllocator pool(sizeof(int), 2);
    
    void* ptr1 = pool.allocate();
    void* ptr2 = pool.allocate();
    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
    
    void* ptr3 = pool.allocate();
    EXPECT_EQ(ptr3, nullptr); // Pool exhausted
}

TEST_F(MemoryTest, GlobalArenasExist) {
    EXPECT_NO_THROW(Memory::getWorldArena());
    EXPECT_NO_THROW(Memory::getRenderArena());
    EXPECT_NO_THROW(Memory::getTempArena());
}

TEST_F(MemoryTest, GlobalArenaSizes) {
    EXPECT_GE(Memory::getWorldArena().getCapacity(), 512 * 1024 * 1024);
    EXPECT_GE(Memory::getRenderArena().getCapacity(), 256 * 1024 * 1024);
    EXPECT_GE(Memory::getTempArena().getCapacity(), 32 * 1024 * 1024);
}

TEST_F(MemoryTest, TempArenaReset) {
    auto& tempArena = Memory::getTempArena();
    
    // Allocate something
    tempArena.create<int>(42);
    size_t usedBefore = tempArena.getUsed();
    EXPECT_GT(usedBefore, 0);
    
    // Reset
    Memory::resetTempArena();
    EXPECT_EQ(tempArena.getUsed(), 0);
}

TEST_F(MemoryTest, MemoryStatsExists) {
    auto& stats = Memory::getStats();
    EXPECT_EQ(stats.getCurrentUsage(), 0);
}
