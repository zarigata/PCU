/**
 * @file Noise.hpp
 * @brief Noise generation utilities for terrain
 */

#pragma once

#include <cstdint>
#include <cmath>
#include <array>

namespace VoxelForge {

// Permutation table for Perlin noise
class PermutationTable {
public:
    PermutationTable(uint32_t seed = 0);
    
    int operator[](int i) const { return perm[i & 255]; }
    
private:
    std::array<int, 512> perm;
};

// Perlin Noise
class PerlinNoise {
public:
    explicit PerlinNoise(uint32_t seed = 0);
    
    // 2D noise
    float noise(float x, float z) const;
    float noise(float x, float y, float z) const;
    
    // Fractal Brownian Motion (multiple octaves)
    float octaveNoise(float x, float z, int octaves, float persistence = 0.5f) const;
    float octaveNoise(float x, float y, float z, int octaves, float persistence = 0.5f) const;
    
    // Normalized to [0, 1]
    float noise01(float x, float z) const;
    float octaveNoise01(float x, float z, int octaves, float persistence = 0.5f) const;
    
private:
    PermutationTable perm;
    
    static float fade(float t);
    static float lerp(float a, float b, float t);
    static float grad(int hash, float x, float y, float z);
    static float grad(int hash, float x, float z);
};

// Simplex Noise (faster than Perlin)
class SimplexNoise {
public:
    explicit SimplexNoise(uint32_t seed = 0);
    
    float noise2D(float x, float y) const;
    float noise3D(float x, float y, float z) const;
    
    float octaveNoise2D(float x, float y, int octaves, float persistence = 0.5f) const;
    float octaveNoise3D(float x, float y, float z, int octaves, float persistence = 0.5f) const;
    
private:
    PermutationTable perm;
    
    static constexpr float F2 = 0.3660254037844386f;  // (sqrt(3) - 1) / 2
    static constexpr float G2 = 0.21132486540518713f; // (3 - sqrt(3)) / 6
    static constexpr float F3 = 0.3333333333333333f;  // 1 / 3
    static constexpr float G3 = 0.16666666666666666f; // 1 / 6
    
    static float grad2D(int hash, float x, float y);
    static float grad3D(int hash, float x, float y, float z);
};

// Voronoi / Cellular Noise
class VoronoiNoise {
public:
    explicit VoronoiNoise(uint32_t seed = 0);
    
    struct Cell {
        float distance;
        float distanceToSecond;
        int cellX, cellY, cellZ;
    };
    
    Cell noise2D(float x, float y) const;
    Cell noise3D(float x, float y, float z) const;
    
    // Returns just the closest distance
    float distance2D(float x, float y) const;
    float distance3D(float x, float y, float z) const;
    
private:
    uint32_t seed;
    
    static uint32_t hash(uint32_t x, uint32_t y, uint32_t seed);
    static uint32_t hash(uint32_t x, uint32_t y, uint32_t z, uint32_t seed);
};

// Utility functions
namespace NoiseUtils {
    // Combine multiple noise sources
    float combine(float a, float b, float blend = 0.5f);
    
    // Remap from [min, max] to [newMin, newMax]
    float remap(float value, float min, float max, float newMin, float newMax);
    
    // Smooth step
    float smoothstep(float edge0, float edge1, float x);
    
    // Clamp
    float clamp(float value, float min, float max);
    
    // Ridged multifractal (good for mountains)
    float ridgedNoise(PerlinNoise& noise, float x, float z, int octaves, float persistence = 0.5f);
}

} // namespace VoxelForge
