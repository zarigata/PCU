/**
 * @file BiomeBlender.hpp
 * @brief Biome blending for smooth transitions
 *
 * Interpolates between neighboring biomes to create
 * smooth transitions at biome boundaries.
 */

#pragma once

#include <VoxelForge/world/BiomeRegistry.hpp>
#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/utils/Noise.hpp>
#include <VoxelForge/world/ChunkPos.hpp>
#include <glm/glm.hpp>
#include <array>
#include <memory>

namespace VoxelForge {

/**
 * @brief Sample point for biome blending
 */
struct BiomeSample {
    BiomeID biome;
    float weight;
};

/**
 * @brief Biome blend result
 */
struct BiomeBlend {
    std::array<BiomeSample, 4> samples; // Up to 4 biomes to blend
    int sampleCount = 0;
};

/**
 * @brief Handles biome blending and smooth transitions
 *
 * Uses multi-octave noise to create smooth gradients between
 * biomes instead of hard edges. Supports block-level and
 * chunk-level blending.
 */
class BiomeBlender {
public:
    explicit BiomeBlender(uint64_t seed);
    ~BiomeBlender() = default;

    /**
     * @brief Get biome at exact position (no blending)
     */
    BiomeID getBiome(int x, int y, int z) const;

    /**
     * @brief Get blended biome at position
     *
     * Returns multiple biomes with weights for interpolation.
     */
    BiomeBlend getBlendedBiome(int x, int y, int z) const;

    /**
     * @brief Get blended biome at world position
     */
    BiomeBlend getBlendedBiome(const BlockPos& pos) const;

    /**
     * @brief Sample biomes in a chunk
     *
     * Returns a grid of biome samples for the entire chunk.
     * Used for efficient chunk generation.
     */
    void sampleChunk(ChunkPos chunkPos, BiomeID* biomes, int width, int depth);

    /**
     * @brief Blend height values between biomes
     */
    float blendHeight(const BiomeBlend& blend, float x, float z) const;

    /**
     * @brief Blend block selection based on biome weights
     */
    BlockState blendBlock(const BiomeBlend& blend, const BlockPos& pos, BlockState defaultBlock) const;

    /**
     * @brief Interpolate between two values based on biome weights
     */
    template<typename T>
    static T interpolate(const BiomeBlend& blend, const T* values) {
        T result = T();
        float totalWeight = 0.0f;

        for (int i = 0; i < blend.sampleCount; ++i) {
            const auto& sample = blend.samples[i];
            if (sample.weight > 0) {
                result += values[sample.biome] * sample.weight;
                totalWeight += sample.weight;
            }
        }

        if (totalWeight > 0) {
            result /= totalWeight;
        }

        return result;
    }

    /**
     * @brief Get blend weight for a biome in the blend
     */
    float getBiomeWeight(const BiomeBlend& blend, BiomeID biome) const;

private:
    // Noise for smooth biome transitions
    std::unique_ptr<PerlinNoise> biomeBlendNoise;

    // Noise for biome sampling
    std::unique_ptr<PerlinNoise> biomeNoise;

    // Settings
    float blendScale = 0.01f;
    float blendStrength = 0.5f;
    int sampleRadius = 4; // How many blocks to sample around each position

    // Sample biomes in a radius
    void sampleAroundPosition(int x, int y, int z, BiomeBlend& blend) const;

    // Calculate weight for a sample
    float calculateWeight(int dx, int dy, int dz) const;

    // Get biome with noise interpolation
    BiomeID getNoisyBiome(int x, int y, int z) const;
};

} // namespace VoxelForge
