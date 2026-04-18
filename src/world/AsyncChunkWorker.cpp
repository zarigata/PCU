#include <VoxelForge/world/AsyncChunkWorker.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/rendering/BlockColor.hpp>
#include <VoxelForge/engine/JobSystem.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace VoxelForge {

AsyncChunkWorker::AsyncChunkWorker() = default;
AsyncChunkWorker::~AsyncChunkWorker() { shutdown(); }

uint64_t AsyncChunkWorker::posKey(int x, int z) {
    return (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)z << 40);
}

void AsyncChunkWorker::update(World* world, const glm::vec3& cameraPos, int renderDistance,
                              const std::unordered_set<uint64_t>& existingMeshKeys) {
    if (shuttingDown_) return;

    int cx = (int)std::floor(cameraPos.x / 16.0f);
    int cz = (int)std::floor(cameraPos.z / 16.0f);

    std::unordered_set<uint64_t> pendingSnapshot;
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        if ((int)pendingChunks_.size() >= MAX_PENDING) return;
        pendingSnapshot = pendingChunks_;
    }

    struct Candidate { int x, z; int distSq; };
    std::vector<Candidate> candidates;
    candidates.reserve(renderDistance * 2);

    for (int dz = -renderDistance; dz <= renderDistance; dz++) {
        for (int dx = -renderDistance; dx <= renderDistance; dx++) {
            int dsq = dx * dx + dz * dz;
            if (dsq > renderDistance * renderDistance) continue;

            int chunkX = cx + dx;
            int chunkZ = cz + dz;
            uint64_t key = posKey(chunkX, chunkZ);

            if (knownMeshKeys_.count(key)) continue;
            if (pendingSnapshot.count(key)) continue;
            if (!world->hasChunk(ChunkPos{chunkX, chunkZ})) continue;

            candidates.push_back({chunkX, chunkZ, dsq});
        }
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) { return a.distSq < b.distSq; });

    int submitted = 0;
    int budget = MAX_PENDING - (int)pendingSnapshot.size();
    for (auto& cand : candidates) {
        if (submitted >= budget) break;

        uint64_t key = posKey(cand.x, cand.z);
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            if (pendingChunks_.count(key)) continue;
            pendingChunks_.insert(key);
        }
        submitted++;

        GetJobSystem().submit([this, world, chunkX = cand.x, chunkZ = cand.z]() {
            if (shuttingDown_) {
                std::lock_guard<std::mutex> lock(pendingMutex_);
                pendingChunks_.erase(posKey(chunkX, chunkZ));
                return;
            }

            AsyncMeshResult result;
            result.chunkX = chunkX;
            result.chunkZ = chunkZ;
            generateMeshForChunk(world, chunkX, chunkZ, result.vertices, result.indices);

            {
                std::lock_guard<std::mutex> lock(completedMutex_);
                completedMeshes_.push_back(std::move(result));
            }
            {
                std::lock_guard<std::mutex> lock(pendingMutex_);
                pendingChunks_.erase(posKey(chunkX, chunkZ));
            }
        }, Priority::Normal);
    }
}

void AsyncChunkWorker::generateMeshForChunk(World* world, int chunkX, int chunkZ,
                                            std::vector<float>& outVerts,
                                            std::vector<uint32_t>& outIndices) {
    ChunkPos cpos{chunkX, chunkZ};
    const Chunk* chunk = world->getChunk(cpos);
    if (!chunk) return;

    using BlockColor::compute;

    outVerts.reserve(65536);
    outIndices.reserve(98304);

    for (int y = 0; y < CHUNK_HEIGHT; y++) {
        int worldY = y + CHUNK_MIN_Y;
        for (int z = 0; z < 16; z++) {
            for (int x = 0; x < 16; x++) {
                auto block = chunk->getBlock(x, worldY, z);
                if (block.isAir()) continue;

                uint32_t bid = block.getBlockId();
                float fx = (float)x, fy = (float)worldY, fz = (float)z;
                int wx = chunkX * 16 + x;
                int wz = chunkZ * 16 + z;

                auto isAirAt = [&](int nx, int ny, int nz) -> bool {
                    if (ny < CHUNK_MIN_Y || ny >= CHUNK_MIN_Y + CHUNK_HEIGHT) return true;
                    if (nx >= 0 && nx < 16 && nz >= 0 && nz < 16) {
                        return chunk->getBlock(nx, ny, nz).isAir();
                    }
                    return world->getBlock(chunkX * 16 + nx, ny, chunkZ * 16 + nz).isAir();
                };

                auto addFace = [&](float x0, float y0, float z0,
                                   float x1, float y1, float z1,
                                   float x2, float y2, float z2,
                                   float x3, float y3, float z3, int face) {
                    uint32_t col = compute(bid, face, wx, worldY, wz);
                    float colAsFloat;
                    std::memcpy(&colAsFloat, &col, sizeof(float));
                    uint32_t base = (uint32_t)(outVerts.size() / 4);
                    outVerts.insert(outVerts.end(), {
                        x0,y0,z0, colAsFloat, x1,y1,z1, colAsFloat,
                        x2,y2,z2, colAsFloat, x3,y3,z3, colAsFloat
                    });
                    outIndices.insert(outIndices.end(), {base, base+1, base+2, base, base+2, base+3});
                };

                if (isAirAt(x, y+1, z))
                    addFace(fx,fy+1,fz, fx+1,fy+1,fz, fx+1,fy+1,fz+1, fx,fy+1,fz+1, 4);
                if (isAirAt(x, y-1, z))
                    addFace(fx,fy,fz+1, fx+1,fy,fz+1, fx+1,fy,fz, fx,fy,fz, 5);
                if (isAirAt(x, y, z-1))
                    addFace(fx,fy,fz, fx+1,fy,fz, fx+1,fy+1,fz, fx,fy+1,fz, 2);
                if (isAirAt(x, y, z+1))
                    addFace(fx+1,fy,fz+1, fx,fy,fz+1, fx,fy+1,fz+1, fx+1,fy+1,fz+1, 3);
                if (isAirAt(x-1, y, z))
                    addFace(fx,fy,fz+1, fx,fy,fz, fx,fy+1,fz, fx,fy+1,fz+1, 0);
                if (isAirAt(x+1, y, z))
                    addFace(fx+1,fy,fz, fx+1,fy,fz+1, fx+1,fy+1,fz+1, fx+1,fy+1,fz, 1);
            }
        }
    }
}

std::vector<AsyncMeshResult> AsyncChunkWorker::pollCompleted(int maxCount) {
    std::lock_guard<std::mutex> lock(completedMutex_);
    std::vector<AsyncMeshResult> result;
    int count = std::min(maxCount, (int)completedMeshes_.size());
    if (count == 0) return result;

    result.assign(completedMeshes_.begin(), completedMeshes_.begin() + count);
    for (int i = 0; i < count; i++) {
        knownMeshKeys_.insert(posKey(result[i].chunkX, result[i].chunkZ));
    }
    completedMeshes_.erase(completedMeshes_.begin(), completedMeshes_.begin() + count);
    return result;
}

bool AsyncChunkWorker::isPending(int chunkX, int chunkZ) const {
    std::lock_guard<std::mutex> lock(pendingMutex_);
    return pendingChunks_.count(posKey(chunkX, chunkZ)) > 0;
}

std::unordered_set<uint64_t> AsyncChunkWorker::getPendingSnapshot() const {
    std::lock_guard<std::mutex> lock(pendingMutex_);
    return pendingChunks_;
}

void AsyncChunkWorker::shutdown() {
    shuttingDown_ = true;
}

void AsyncChunkWorker::forgetMesh(int chunkX, int chunkZ) {
    knownMeshKeys_.erase(posKey(chunkX, chunkZ));
}

} // namespace VoxelForge
