/**
 * @file AsyncChunkWorker.hpp
 * @brief Async chunk mesh generation using the JobSystem thread pool
 */

#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <cstdint>

namespace VoxelForge {

class World;

struct AsyncMeshResult {
    int chunkX = 0;
    int chunkZ = 0;
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
};

class AsyncChunkWorker {
public:
    AsyncChunkWorker();
    ~AsyncChunkWorker();

    void update(World* world, const glm::vec3& cameraPos, int renderDistance,
                const std::unordered_set<uint64_t>& existingMeshKeys);

    std::vector<AsyncMeshResult> pollCompleted(int maxCount);

    bool isPending(int chunkX, int chunkZ) const;
    std::unordered_set<uint64_t> getPendingSnapshot() const;

    void shutdown();

    void forgetMesh(int chunkX, int chunkZ);

private:
    static void generateMeshForChunk(World* world, int chunkX, int chunkZ,
                                     std::vector<float>& outVerts,
                                     std::vector<uint32_t>& outIndices);
    static uint64_t posKey(int x, int z);

    mutable std::mutex completedMutex_;
    std::vector<AsyncMeshResult> completedMeshes_;

    mutable std::mutex pendingMutex_;
    std::unordered_set<uint64_t> pendingChunks_;
    std::unordered_set<uint64_t> knownMeshKeys_;

    std::atomic<bool> shuttingDown_{false};

    static constexpr int MAX_PENDING = 16;
    static constexpr int MAX_COMPLETED = 32;
};

} // namespace VoxelForge
