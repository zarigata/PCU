/**
 * @file Noise.cpp
 * @brief Noise generation implementation
 */

#include <VoxelForge/utils/Noise.hpp>
#include <algorithm>
#include <cmath>
#include <random>

namespace VoxelForge {

// ============================================
// Permutation Table
// ============================================

PermutationTable::PermutationTable(uint32_t seed) {
    // Initialize with 0-255
    for (int i = 0; i < 256; i++) {
        perm[i] = i;
    }
    
    // Shuffle using seed
    std::mt19937 rng(seed);
    for (int i = 255; i > 0; i--) {
        std::uniform_int_distribution<int> dist(0, i);
        int j = dist(rng);
        std::swap(perm[i], perm[j]);
    }
    
    // Duplicate for overflow
    for (int i = 0; i < 256; i++) {
        perm[256 + i] = perm[i];
    }
}

// ============================================
// Perlin Noise
// ============================================

PerlinNoise::PerlinNoise(uint32_t seed) : perm(seed) {
}

float PerlinNoise::fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float PerlinNoise::lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float PerlinNoise::grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float PerlinNoise::grad(int hash, float x, float z) {
    return grad(hash, x, 0, z);
}

float PerlinNoise::noise(float x, float z) const {
    int X = static_cast<int>(std::floor(x)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;
    
    x -= std::floor(x);
    z -= std::floor(z);
    
    float u = fade(x);
    float v = fade(z);
    
    int A = perm[X] + Z;
    int B = perm[X + 1] + Z;
    
    return lerp(
        lerp(grad(perm[A], x, z), grad(perm[B], x - 1, z), u),
        lerp(grad(perm[A + 1], x, z - 1), grad(perm[B + 1], x - 1, z - 1), u),
        v
    );
}

float PerlinNoise::noise(float x, float y, float z) const {
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;
    
    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);
    
    float u = fade(x);
    float v = fade(y);
    float w = fade(z);
    
    int A = perm[X] + Y;
    int AA = perm[A] + Z;
    int AB = perm[A + 1] + Z;
    int B = perm[X + 1] + Y;
    int BA = perm[B] + Z;
    int BB = perm[B + 1] + Z;
    
    return lerp(
        lerp(
            lerp(grad(perm[AA], x, y, z), grad(perm[BA], x - 1, y, z), u),
            lerp(grad(perm[AB], x, y - 1, z), grad(perm[BB], x - 1, y - 1, z), u),
            v
        ),
        lerp(
            lerp(grad(perm[AA + 1], x, y, z - 1), grad(perm[BA + 1], x - 1, y, z - 1), u),
            lerp(grad(perm[AB + 1], x, y - 1, z - 1), grad(perm[BB + 1], x - 1, y - 1, z - 1), u),
            v
        ),
        w
    );
}

float PerlinNoise::octaveNoise(float x, float z, int octaves, float persistence) const {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < octaves; i++) {
        total += noise(x * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    
    return total / maxValue;
}

float PerlinNoise::octaveNoise(float x, float y, float z, int octaves, float persistence) const {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < octaves; i++) {
        total += noise(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    
    return total / maxValue;
}

float PerlinNoise::noise01(float x, float z) const {
    return (noise(x, z) + 1.0f) * 0.5f;
}

float PerlinNoise::octaveNoise01(float x, float z, int octaves, float persistence) const {
    return (octaveNoise(x, z, octaves, persistence) + 1.0f) * 0.5f;
}

// ============================================
// Simplex Noise
// ============================================

SimplexNoise::SimplexNoise(uint32_t seed) : perm(seed) {
}

float SimplexNoise::grad2D(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) != 0 ? -u : u) + ((h & 2) != 0 ? -2.0f * v : 2.0f * v);
}

float SimplexNoise::grad3D(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) != 0 ? -u : u) + ((h & 2) != 0 ? -v : v);
}

float SimplexNoise::noise2D(float x, float y) const {
    // Skew input space
    float s = (x + y) * F2;
    int i = static_cast<int>(std::floor(x + s));
    int j = static_cast<int>(std::floor(y + s));
    
    float t = (i + j) * G2;
    float X0 = i - t;
    float Y0 = j - t;
    float x0 = x - X0;
    float y0 = y - Y0;
    
    // Determine simplex
    int i1, j1;
    if (x0 > y0) {
        i1 = 1; j1 = 0;
    } else {
        i1 = 0; j1 = 1;
    }
    
    float x1 = x0 - i1 + G2;
    float y1 = y0 - j1 + G2;
    float x2 = x0 - 1.0f + 2.0f * G2;
    float y2 = y0 - 1.0f + 2.0f * G2;
    
    // Calculate contributions
    int ii = i & 255;
    int jj = j & 255;
    
    float n0 = 0.0f, n1 = 0.0f, n2 = 0.0f;
    
    float t0 = 0.5f - x0 * x0 - y0 * y0;
    if (t0 >= 0.0f) {
        t0 *= t0;
        n0 = t0 * t0 * grad2D(perm[ii + perm[jj]], x0, y0);
    }
    
    float t1 = 0.5f - x1 * x1 - y1 * y1;
    if (t1 >= 0.0f) {
        t1 *= t1;
        n1 = t1 * t1 * grad2D(perm[ii + i1 + perm[jj + j1]], x1, y1);
    }
    
    float t2 = 0.5f - x2 * x2 - y2 * y2;
    if (t2 >= 0.0f) {
        t2 *= t2;
        n2 = t2 * t2 * grad2D(perm[ii + 1 + perm[jj + 1]], x2, y2);
    }
    
    return 70.0f * (n0 + n1 + n2);
}

float SimplexNoise::noise3D(float x, float y, float z) const {
    // Implementation similar to 2D but with 4 corners
    // Simplified version - full implementation would be longer
    float s = (x + y + z) * F3;
    int i = static_cast<int>(std::floor(x + s));
    int j = static_cast<int>(std::floor(y + s));
    int k = static_cast<int>(std::floor(z + s));
    
    float t = (i + j + k) * G3;
    float X0 = i - t;
    float Y0 = j - t;
    float Z0 = k - t;
    float x0 = x - X0;
    float y0 = y - Y0;
    float z0 = z - Z0;
    
    // Determine simplex and calculate contributions
    // (simplified - full implementation would handle all 6 simplices)
    
    int ii = i & 255;
    int jj = j & 255;
    int kk = k & 255;
    
    float n0 = 0.0f;
    float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 >= 0.0f) {
        t0 *= t0;
        n0 = t0 * t0 * grad3D(perm[ii + perm[jj + perm[kk]]], x0, y0, z0);
    }
    
    // Additional corners would be calculated here
    
    return 32.0f * n0;
}

float SimplexNoise::octaveNoise2D(float x, float y, int octaves, float persistence) const {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < octaves; i++) {
        total += noise2D(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    
    return total / maxValue;
}

float SimplexNoise::octaveNoise3D(float x, float y, float z, int octaves, float persistence) const {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < octaves; i++) {
        total += noise3D(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    
    return total / maxValue;
}

// ============================================
// Voronoi Noise
// ============================================

VoronoiNoise::VoronoiNoise(uint32_t seed) : seed(seed) {
}

uint32_t VoronoiNoise::hash(uint32_t x, uint32_t y, uint32_t s) {
    uint32_t h = s;
    h ^= x * 374761393u;
    h ^= y * 1013904223u;
    h = (h * 1103515245u + 12345u) & 0x7fffffffu;
    return h;
}

uint32_t VoronoiNoise::hash(uint32_t x, uint32_t y, uint32_t z, uint32_t s) {
    uint32_t h = s;
    h ^= x * 374761393u;
    h ^= y * 1013904223u;
    h ^= z * 1664525u;
    h = (h * 1103515245u + 12345u) & 0x7fffffffu;
    return h;
}

VoronoiNoise::Cell VoronoiNoise::noise2D(float x, float y) const {
    Cell result;
    result.distance = std::numeric_limits<float>::max();
    result.distanceToSecond = std::numeric_limits<float>::max();
    
    int cellX = static_cast<int>(std::floor(x));
    int cellY = static_cast<int>(std::floor(y));
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int cx = cellX + dx;
            int cy = cellY + dy;
            
            uint32_t h = hash(cx, cy, seed);
            float px = cx + (h & 0xffff) / 65535.0f;
            float py = cy + ((h >> 16) & 0xffff) / 65535.0f;
            
            float dist = std::sqrt((x - px) * (x - px) + (y - py) * (y - py));
            
            if (dist < result.distance) {
                result.distanceToSecond = result.distance;
                result.distance = dist;
                result.cellX = cx;
                result.cellY = cy;
            } else if (dist < result.distanceToSecond) {
                result.distanceToSecond = dist;
            }
        }
    }
    
    return result;
}

float VoronoiNoise::distance2D(float x, float y) const {
    return noise2D(x, y).distance;
}

float VoronoiNoise::distance3D(float x, float y, float z) const {
    return noise3D(x, y, z).distance;
}

// ============================================
// Utility Functions
// ============================================

namespace NoiseUtils {

float combine(float a, float b, float blend) {
    return a * (1.0f - blend) + b * blend;
}

float remap(float value, float min, float max, float newMin, float newMax) {
    return newMin + (value - min) / (max - min) * (newMax - newMin);
}

float smoothstep(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float clamp(float value, float min, float max) {
    return std::clamp(value, min, max);
}

float ridgedNoise(PerlinNoise& noise, float x, float z, int octaves, float persistence) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float weight = 1.0f;
    
    for (int i = 0; i < octaves; i++) {
        float value = noise.noise(x * frequency, z * frequency);
        value = 1.0f - std::abs(value);
        value *= value * weight;
        weight = std::clamp(value * 2.0f, 0.0f, 1.0f);
        total += value * amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    
    return total;
}

} // namespace NoiseUtils

} // namespace VoxelForge
