/**
 * @file World.hpp
 * @brief World container and management
 */

#pragma once

#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/utils/Noise.hpp>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <functional>

namespace VoxelForge {

class Player;

struct WorldSettings {
    int64_t seed = 0;
    int chunkLoadRadius = 8;
    int simulationDistance = 6;
    bool generateFeatures = true;
    int seaLevel = 62;
    int minBuildHeight = -64;
    int maxBuildHeight = 320;
};

class World {
public:
    explicit World(int64_t seed = 0);
    ~World();
    
    // Block access
    BlockState getBlock(const BlockPos& pos) const;
    BlockState getBlock(int x, int y, int z) const;
    void setBlock(const BlockPos& pos, BlockState state);
    void setBlock(int x, int y, int z, BlockState state);
    
    // Chunk access
    Chunk* getChunk(const ChunkPos& pos);
    const Chunk* getChunk(const ChunkPos& pos) const;
    Chunk* getOrCreateChunk(const ChunkPos& pos);
    bool hasChunk(const ChunkPos& pos) const;
    
    // Chunk loading
    void loadChunk(const ChunkPos& pos);
    void unloadChunk(const ChunkPos& pos);
    void unloadChunks(int maxToUnload = 1);
    
    // Chunk management
    void updateChunks(const glm::vec3& playerPos);
    size_t getLoadedChunkCount() const { return chunks.size(); }
    
    // World generation
    void generateChunk(Chunk& chunk);
    void generateTerrain(Chunk& chunk);
    void generateFeatures(Chunk& chunk);
    
    // Light
    uint8_t getSkyLight(const BlockPos& pos) const;
    uint8_t getBlockLight(const BlockPos& pos) const;
    void setSkyLight(const BlockPos& pos, uint8_t value);
    void setBlockLight(const BlockPos& pos, uint8_t value);
    
    // Height
    int getHeight(int x, int z) const;
    
    // Biomes
    int getBiome(const BlockPos& pos) const;
    
    // Time
    int64_t getGameTime() const { return gameTime; }
    void setGameTime(int64_t time) { gameTime = time; }
    float getDayTime() const { return dayTime; }
    void setDayTime(float time) { dayTime = time; }
    bool isDay() const { return dayTime >= 0 && dayTime < 13000; }
    
    // World info
    int64_t getSeed() const { return settings.seed; }
    const WorldSettings& getSettings() const { return settings; }
    
    // Tick
    void tick();
    
    // For each chunk
    void forEachChunk(const std::function<void(Chunk&)>& func);
    void forEachChunk(const std::function<void(const Chunk&)>& func) const;
    
    // Raycast
    struct RaycastResult {
        bool hit = false;
        BlockPos position;
        glm::vec3 hitPos;
        glm::vec3 normal;
        float distance = 0;
        BlockState block;
    };
    
    RaycastResult raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance = 100.0f) const;
    
private:
    // Chunk storage
    std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash> chunks;
    mutable std::shared_mutex chunkMutex;
    
    // Settings
    WorldSettings settings;
    
    // Time
    int64_t gameTime = 0;
    float dayTime = 0.0f;
    
    // Noise generators
    std::unique_ptr<PerlinNoise> terrainNoise;
    std::unique_ptr<PerlinNoise> caveNoise;
    std::unique_ptr<SimplexNoise> biomeNoise;
    
    // Chunk loading queue
    std::vector<ChunkPos> loadQueue;
    std::vector<ChunkPos> unloadQueue;
    
    // Helper methods
    ChunkPos getChunkPos(const BlockPos& pos) const;
    BlockPos getLocalPos(const BlockPos& pos) const;
    bool isInsideWorld(int y) const;
    
    // Generation helpers
    int calculateTerrainHeight(int x, int z) const;
    int getBiomeAt(int x, int z) const;
    
    // Chunk hash
    struct ChunkPosHash {
        size_t operator()(const ChunkPos& pos) const {
            return std::hash<int64_t>()(pos.toHash());
        }
    };
};

} // namespace VoxelForge
