/**
 * @file BiomeBlender.cpp
 * @brief Biome blending for smooth transitions
 */

#include <VoxelForge/world/BiomeBlender.hpp>
#include <VoxelForge/world/BiomeRegistry.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>
#include <cmath>

namespace VoxelForge {

// ============================================
// Constructor
// ============================================

BiomeBlender::BiomeBlender(uint64_t seed) {
    biomeBlendNoise = std::make_unique<PerlinNoise>(seed);
    biomeNoise = std::make_unique<PerlinNoise>(seed + 1);

    VF_INFO("BiomeBlender created with seed {}", seed);
}

// ============================================
// Get biome at exact position (no blending)
// ============================================

BiomeID BiomeBlender::getBiome(int x, int y, int z) const {
    return getNoisyBiome(x, y, z);
}

// ============================================
// Get blended biome at position
// ============================================

BiomeBlend BiomeBlender::getBlendedBiome(int x, int y, int z) const {
    BiomeBlend blend;
    sampleAroundPosition(x, y, z, blend);
    return blend;
}

BiomeBlend BiomeBlender::getBlendedBiome(const BlockPos& pos) const {
    return getBlendedBiome(pos.x, pos.y, pos.z);
}

// ============================================
// Sample biomes in a chunk
// ============================================

void BiomeBlender::sampleChunk(ChunkPos chunkPos, BiomeID* biomes, int width, int depth) {
    int worldX = chunkPos.x * VoxelForge::CHUNK_WIDTH;
    int worldZ = chunkPos.z * VoxelForge::CHUNK_WIDTH;

    for (int x = 0; x < width; ++x) {
        for (int z = 0; z < depth; ++z) {
            int idx = x * depth + z;
            biomes[idx] = getNoisyBiome(worldX + x, 0, worldZ + z);
        }
    }
}

// ============================================
// Blend height values between biomes
// ============================================

float BiomeBlender::blendHeight(const BiomeBlend& blend, float x, float z) const {
    auto& registry = BiomeRegistry::get();
    std::array<float, 4> heights;

    for (int i = 0; i < blend.sampleCount; ++i) {
        const auto& sample = blend.samples[i];
        const Biome* biome = registry.getBiome(sample.biome);
        heights[i] = biome ? biome->baseHeight : 0.0f;
    }

    return interpolate<float>(blend, heights.data());
}

// ============================================
// Blend block selection based on biome weights
// ============================================

BlockState BiomeBlender::blendBlock(const BiomeBlend& blend, const BlockPos& pos,
                                    BlockState defaultBlock) const {
    // For now, return the block from the highest-weighted biome
    // A full implementation would interpolate block properties

    float maxWeight = 0.0f;
    BiomeID dominantBiome = INVALID_BIOME;

    for (int i = 0; i < blend.sampleCount; ++i) {
        const auto& sample = blend.samples[i];
        if (sample.weight > maxWeight) {
            maxWeight = sample.weight;
            dominantBiome = sample.biome;
        }
    }

    // Get block from dominant biome
    auto& registry = BiomeRegistry::get();
    const Biome* biome = registry.getBiome(dominantBiome);

    if (biome) {
        // TODO: Implement proper block selection based on height and biome
        // For now, return the default block
        return defaultBlock;
    }

    return defaultBlock;
}

// ============================================
// Get blend weight for a biome in the blend
// ============================================

float BiomeBlender::getBiomeWeight(const BiomeBlend& blend, BiomeID biome) const {
    for (int i = 0; i < blend.sampleCount; ++i) {
        if (blend.samples[i].biome == biome) {
            return blend.samples[i].weight;
        }
    }
    return 0.0f;
}

// ============================================
// Sample biomes in a radius
// ============================================

void BiomeBlender::sampleAroundPosition(int x, int y, int z, BiomeBlend& blend) const {
    blend.sampleCount = 0;

    // Sample biomes in a radius around the position
    for (int dx = -sampleRadius; dx <= sampleRadius; ++dx) {
        for (int dz = -sampleRadius; dz <= sampleRadius; ++dz) {
            // Sample at different heights for vertical blending
            for (int dy = -1; dy <= 1; ++dy) {
                int sx = x + dx;
                int sy = y + dy;
                int sz = z + dz;

                BiomeID biome = getNoisyBiome(sx, sy, sz);
                float weight = calculateWeight(dx, dy, dz);

                if (weight <= 0.0f) {
                    continue;
                }

                // Check if this biome is already in the blend
                bool found = false;
                for (int i = 0; i < blend.sampleCount; ++i) {
                    if (blend.samples[i].biome == biome) {
                        blend.samples[i].weight += weight;
                        found = true;
                        break;
                    }
                }

                // Add new biome if not already in blend
                if (!found && blend.sampleCount < 4) {
                    blend.samples[blend.sampleCount].biome = biome;
                    blend.samples[blend.sampleCount].weight = weight;
                    blend.sampleCount++;
                }
            }
        }
    }

    // Normalize weights
    float totalWeight = 0.0f;
    for (int i = 0; i < blend.sampleCount; ++i) {
        totalWeight += blend.samples[i].weight;
    }

    if (totalWeight > 0.0f) {
        for (int i = 0; i < blend.sampleCount; ++i) {
            blend.samples[i].weight /= totalWeight;
        }
    }
}

// ============================================
// Calculate weight for a sample
// ============================================

float BiomeBlender::calculateWeight(int dx, int dy, int dz) const {
    // Use distance-based weighting with noise
    float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy + dz * dz));

    if (dist > sampleRadius) {
        return 0.0f;
    }

    // Linear falloff
    float falloff = 1.0f - (dist / sampleRadius);
    falloff = std::max(0.0f, falloff);

    // Apply blend strength
    return falloff * blendStrength;
}

// ============================================
// Get biome with noise interpolation
// ============================================

BiomeID BiomeBlender::getNoisyBiome(int x, int y, int z) const {
    // Get biome ID from registry based on temperature/humidity noise
    // This is a simplified version - a full implementation would use
    // the WorldGenerator's biome system

    float temp = biomeNoise->noise(x * 0.01f, y * 0.01f, z * 0.01f);
    float humid = biomeNoise->noise(x * 0.01f + 1000, y * 0.01f, z * 0.01f + 1000);

    // Normalize to 0-1 range
    temp = (temp + 1.0f) * 0.5f;
    humid = (humid + 1.0f) * 0.5f;

    // Map temperature and humidity to biome
    // This is a simplified mapping - full version would be more complex
    if (temp < 0.3f) {
        if (humid < 0.3f) return 9; // Snowy Plains
        if (humid < 0.7f) return 25; // Snowy Taiga
        return 24; // Ice Spikes
    } else if (temp < 0.7f) {
        if (humid < 0.3f) return 1; // Plains
        if (humid < 0.7f) return 3; // Forest
        return 5; // Swamp
    } else if (temp < 1.2f) {
        if (humid < 0.3f) return 2; // Desert
        if (humid < 0.7f) return 7; // Savanna
        return 6; // Jungle
    } else {
        if (humid < 0.3f) return 8; // Badlands
        if (humid < 0.7f) return 7; // Savanna
        return 6; // Jungle
    }
}

} // namespace VoxelForge
