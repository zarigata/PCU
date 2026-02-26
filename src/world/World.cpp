/**
 * @file World.cpp
 * @brief World implementation
 */

#include <VoxelForge/world/World.hpp>
#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <cmath>
#include <algorithm>

namespace VoxelForge {

World::World(int64_t seed) {
    settings.seed = seed;
    
    // Initialize noise generators
    terrainNoise = std::make_unique<PerlinNoise>(static_cast<uint32_t>(seed));
    caveNoise = std::make_unique<PerlinNoise>(static_cast<uint32_t>(seed + 1));
    biomeNoise = std::make_unique<SimplexNoise>(static_cast<uint32_t>(seed + 2));
    
    VF_CORE_INFO("World created with seed: {}", seed);
}

World::~World() {
    VF_CORE_INFO("World destroyed with {} chunks", chunks.size());
}

// ============================================
// Block Access
// ============================================

BlockState World::getBlock(const BlockPos& pos) const {
    return getBlock(pos.x, pos.y, pos.z);
}

BlockState World::getBlock(int x, int y, int z) const {
    if (!isInsideWorld(y)) {
        return BlockState();
    }
    
    ChunkPos chunkPos = getChunkPos(BlockPos(x, y, z));
    
    std::shared_lock lock(chunkMutex);
    auto it = chunks.find(chunkPos);
    if (it != chunks.end()) {
        return it->second->getBlock(x & 15, y, z & 15);
    }
    
    return BlockState();
}

void World::setBlock(const BlockPos& pos, BlockState state) {
    setBlock(pos.x, pos.y, pos.z, state);
}

void World::setBlock(int x, int y, int z, BlockState state) {
    if (!isInsideWorld(y)) {
        return;
    }
    
    ChunkPos chunkPos = getChunkPos(BlockPos(x, y, z));
    
    std::unique_lock lock(chunkMutex);
    auto it = chunks.find(chunkPos);
    if (it != chunks.end()) {
        it->second->setBlock(x & 15, y, z & 15, state);
    }
}

// ============================================
// Chunk Access
// ============================================

Chunk* World::getChunk(const ChunkPos& pos) {
    std::shared_lock lock(chunkMutex);
    auto it = chunks.find(pos);
    return it != chunks.end() ? it->second.get() : nullptr;
}

const Chunk* World::getChunk(const ChunkPos& pos) const {
    std::shared_lock lock(chunkMutex);
    auto it = chunks.find(pos);
    return it != chunks.end() ? it->second.get() : nullptr;
}

Chunk* World::getOrCreateChunk(const ChunkPos& pos) {
    {
        std::shared_lock lock(chunkMutex);
        auto it = chunks.find(pos);
        if (it != chunks.end()) {
            return it->second.get();
        }
    }
    
    std::unique_lock lock(chunkMutex);
    auto chunk = std::make_unique<Chunk>(pos);
    chunk->setWorld(this);
    
    // Generate chunk
    generateChunk(*chunk);
    
    Chunk* ptr = chunk.get();
    chunks[pos] = std::move(chunk);
    
    return ptr;
}

bool World::hasChunk(const ChunkPos& pos) const {
    std::shared_lock lock(chunkMutex);
    return chunks.find(pos) != chunks.end();
}

// ============================================
// Chunk Loading
// ============================================

void World::loadChunk(const ChunkPos& pos) {
    if (!hasChunk(pos)) {
        getOrCreateChunk(pos);
    }
}

void World::unloadChunk(const ChunkPos& pos) {
    std::unique_lock lock(chunkMutex);
    auto it = chunks.find(pos);
    if (it != chunks.end()) {
        // Save chunk before unloading
        // TODO: Implement chunk saving
        chunks.erase(it);
        VF_CORE_DEBUG("Unloaded chunk at ({}, {})", pos.x, pos.z);
    }
}

void World::unloadChunks(int maxToUnload) {
    // TODO: Implement chunk unloading based on distance
}

void World::updateChunks(const glm::vec3& playerPos) {
    // Calculate player chunk position
    ChunkPos playerChunk(
        static_cast<int>(std::floor(playerPos.x / CHUNK_WIDTH)),
        static_cast<int>(std::floor(playerPos.z / CHUNK_WIDTH))
    );
    
    // Load chunks in radius
    int radius = settings.chunkLoadRadius;
    
    for (int dx = -radius; dx <= radius; dx++) {
        for (int dz = -radius; dz <= radius; dz++) {
            if (dx * dx + dz * dz <= radius * radius) {
                ChunkPos pos(playerChunk.x + dx, playerChunk.z + dz);
                loadChunk(pos);
            }
        }
    }
    
    // TODO: Unload distant chunks
}

// ============================================
// World Generation
// ============================================

void World::generateChunk(Chunk& chunk) {
    generateTerrain(chunk);
    
    if (settings.generateFeatures) {
        // TODO: Generate features (trees, ores, etc.)
    }
    
    chunk.recalculateHeightMaps();
    chunk.setStatus(Chunk::Status::Full);
}

void World::generateTerrain(Chunk& chunk) {
    const ChunkPos& chunkPos = chunk.getPosition();
    
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_WIDTH; z++) {
            int worldX = chunkPos.x * CHUNK_WIDTH + x;
            int worldZ = chunkPos.z * CHUNK_WIDTH + z;
            
            // Calculate terrain height
            int height = calculateTerrainHeight(worldX, worldZ);
            
            // Get biome
            int biome = getBiomeAt(worldX, worldZ);
            chunk.setBiome(x, 0, z, static_cast<BiomeStorage::BiomeID>(biome));
            
            // Fill terrain
            for (int y = settings.minBuildHeight; y < settings.maxBuildHeight; y++) {
                BlockState block;
                
                if (y > height) {
                    // Above terrain
                    if (y < settings.seaLevel) {
                        block = BlockRegistry::get().getDefaultState("minecraft:water");
                    } else {
                        // Air
                        continue;
                    }
                } else if (y == height) {
                    // Surface
                    if (y < settings.seaLevel) {
                        block = BlockRegistry::get().getDefaultState("minecraft:gravel");
                    } else {
                        block = BlockRegistry::get().getDefaultState("minecraft:grass_block");
                    }
                } else if (y > height - 4) {
                    // Subsurface
                    block = BlockRegistry::get().getDefaultState("minecraft:dirt");
                } else {
                    // Deep underground
                    if (y < settings.minBuildHeight + 5) {
                        block = BlockRegistry::get().getDefaultState("minecraft:bedrock");
                    } else {
                        block = BlockRegistry::get().getDefaultState("minecraft:stone");
                    }
                }
                
                chunk.setBlock(x, y, z, block);
            }
            
            // TODO: Generate caves using 3D noise
        }
    }
}

int World::calculateTerrainHeight(int x, int z) const {
    // Multi-octave noise for varied terrain
    float scale = 0.01f;
    float height = terrainNoise->octaveNoise(x * scale, z * scale, 4, 0.5f);
    
    // Normalize and scale
    height = (height + 1.0f) * 0.5f;  // 0 to 1
    height = height * 30.0f + 60.0f;   // 60 to 90 base height
    
    // Add variation
    float detail = terrainNoise->octaveNoise(x * 0.1f, z * 0.1f, 2, 0.5f);
    height += detail * 10.0f;
    
    return static_cast<int>(std::floor(height));
}

int World::getBiomeAt(int x, int z) const {
    // Simple biome selection based on noise
    float temperature = biomeNoise->noise2D(x * 0.005f, z * 0.005f);
    float humidity = biomeNoise->noise2D(x * 0.005f + 1000, z * 0.005f + 1000);
    
    // 0 = plains, 1 = desert, 2 = forest, 3 = mountains
    if (temperature > 0.3f) {
        return 1; // Desert
    } else if (humidity > 0.3f) {
        return 2; // Forest
    } else if (temperature < -0.3f) {
        return 3; // Mountains (cold)
    }
    
    return 0; // Plains
}

// ============================================
// Height & Biome
// ============================================

int World::getHeight(int x, int z) const {
    ChunkPos chunkPos(
        x < 0 ? (x - CHUNK_WIDTH + 1) / CHUNK_WIDTH : x / CHUNK_WIDTH,
        z < 0 ? (z - CHUNK_WIDTH + 1) / CHUNK_WIDTH : z / CHUNK_WIDTH
    );
    
    const Chunk* chunk = getChunk(chunkPos);
    if (chunk) {
        return chunk->getHeight(HeightMap::Type::WorldSurface, x & 15, z & 15);
    }
    
    return calculateTerrainHeight(x, z);
}

int World::getBiome(const BlockPos& pos) const {
    ChunkPos chunkPos = getChunkPos(pos);
    
    const Chunk* chunk = getChunk(chunkPos);
    if (chunk) {
        return chunk->getBiome(pos.x & 15, pos.y & 15, pos.z & 15);
    }
    
    return getBiomeAt(pos.x, pos.z);
}

// ============================================
// Raycast
// ============================================

World::RaycastResult World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
    RaycastResult result;
    
    glm::vec3 pos = origin;
    glm::vec3 step = glm::normalize(direction) * 0.05f;
    float distance = 0.0f;
    
    BlockPos lastPos(-999999, -999999, -999999);
    
    while (distance < maxDistance) {
        BlockPos blockPos(
            static_cast<int>(std::floor(pos.x)),
            static_cast<int>(std::floor(pos.y)),
            static_cast<int>(std::floor(pos.z))
        );
        
        if (blockPos != lastPos) {
            BlockState block = getBlock(blockPos);
            
            if (!block.isAir() && block.isSolid()) {
                result.hit = true;
                result.position = blockPos;
                result.hitPos = pos;
                result.block = block;
                result.distance = distance;
                
                // Calculate normal based on which face was hit
                glm::vec3 localPos = pos - glm::vec3(blockPos.x, blockPos.y, blockPos.z);
                
                if (localPos.x < 0.1f) result.normal = glm::vec3(-1, 0, 0);
                else if (localPos.x > 0.9f) result.normal = glm::vec3(1, 0, 0);
                else if (localPos.y < 0.1f) result.normal = glm::vec3(0, -1, 0);
                else if (localPos.y > 0.9f) result.normal = glm::vec3(0, 1, 0);
                else if (localPos.z < 0.1f) result.normal = glm::vec3(0, 0, -1);
                else if (localPos.z > 0.9f) result.normal = glm::vec3(0, 0, 1);
                else result.normal = glm::vec3(0, 1, 0);
                
                return result;
            }
            
            lastPos = blockPos;
        }
        
        pos += step;
        distance += 0.05f;
    }
    
    return result;
}

// ============================================
// Helper Methods
// ============================================

ChunkPos World::getChunkPos(const BlockPos& pos) const {
    return ChunkPos::fromBlockPos(pos);
}

BlockPos World::getLocalPos(const BlockPos& pos) const {
    return BlockPos(
        ((pos.x % CHUNK_WIDTH) + CHUNK_WIDTH) % CHUNK_WIDTH,
        pos.y,
        ((pos.z % CHUNK_WIDTH) + CHUNK_WIDTH) % CHUNK_WIDTH
    );
}

bool World::isInsideWorld(int y) const {
    return y >= settings.minBuildHeight && y < settings.maxBuildHeight;
}

} // namespace VoxelForge
