/**
 * @file LightEngine.cpp
 * @brief Lighting calculation engine
 */

#include <VoxelForge/world/LightEngine.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

LightEngine::LightEngine(World* world) : world_(world) {
    LOG_INFO("LightEngine created");
}

void LightEngine::updateChunk(Chunk* chunk) {
    if (!chunk) return;
    
    // Calculate sky light
    calculateSkyLight(chunk);
    
    // Calculate block light
    calculateBlockLight(chunk);
}

void LightEngine::calculateSkyLight(Chunk* chunk) {
    // Set sky light based on height map
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_WIDTH; z++) {
            int height = chunk->getHeight(HeightMap::Type::WorldSurface, x, z);
            
            // Fill with full sky light above surface
            for (int y = CHUNK_MIN_Y + CHUNK_HEIGHT - 1; y > height; y--) {
                chunk->setSkyLight(x, y, z, 15);
            }
            
            // Decrease sky light below surface
            int light = 15;
            for (int y = height; y >= CHUNK_MIN_Y; y--) {
                BlockState state = chunk->getBlock(x, y, z);
                if (!state.isAir() && state.isOpaque()) {
                    light = std::max(0, light - 1);
                }
                chunk->setSkyLight(x, y, z, light);
            }
        }
    }
}

void LightEngine::calculateBlockLight(Chunk* chunk) {
    // Find all light-emitting blocks
    for (int y = CHUNK_MIN_Y; y < CHUNK_MIN_Y + CHUNK_HEIGHT; y++) {
        int sectionY = (y - CHUNK_MIN_Y) / SECTION_HEIGHT;
        ChunkSection* section = chunk->getSection(sectionY);
        if (!section) continue;
        
        int localY = y - CHUNK_MIN_Y - sectionY * SECTION_HEIGHT;
        
        for (int x = 0; x < CHUNK_WIDTH; x++) {
            for (int z = 0; z < CHUNK_WIDTH; z++) {
                BlockState state = section->getBlock(x, localY, z);
                int emission = getLightEmission(state);
                
                if (emission > 0) {
                    propagateBlockLight(chunk, x, y, z, emission);
                }
            }
        }
    }
}

void LightEngine::propagateBlockLight(Chunk* chunk, int x, int y, int z, int level) {
    if (level <= 0) return;
    
    chunk->setBlockLight(x, y, z, level);
    
    // Propagate to neighbors
    static const int dx[] = {1, -1, 0, 0, 0, 0};
    static const int dy[] = {0, 0, 1, -1, 0, 0};
    static const int dz[] = {0, 0, 0, 0, 1, -1};
    
    for (int i = 0; i < 6; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        int nz = z + dz[i];
        
        if (ny >= CHUNK_MIN_Y && ny < CHUNK_MIN_Y + CHUNK_HEIGHT) {
            int currentLight = chunk->getBlockLight(nx, ny, nz);
            if (currentLight < level - 1) {
                propagateBlockLight(chunk, nx, ny, nz, level - 1);
            }
        }
    }
}

int LightEngine::getLightEmission(BlockState state) const {
    auto& registry = BlockRegistry::get();
    const auto& def = registry.getDefinition(state.getBlockId());
    return def.lightEmission;
}

void LightEngine::updateBlockLight(const BlockPos& pos, int newLevel) {
    // TODO: Implement incremental light updates
}

void LightEngine::removeBlockLight(const BlockPos& pos) {
    // TODO: Implement light removal
}

} // namespace VoxelForge
