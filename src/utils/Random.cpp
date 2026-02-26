/**
 * @file Random.cpp
 * @brief Random number generation implementation
 */

#include <VoxelForge/utils/Random.hpp>
#include <cmath>

namespace VoxelForge {

uint64_t SeededRandom::positionHash(int x, int z, uint64_t baseSeed) {
    uint64_t h = baseSeed;
    h ^= static_cast<uint64_t>(x) * 374761393u;
    h ^= static_cast<uint64_t>(z) * 1013904223u;
    h = (h ^ (h >> 13)) * 127412617731153u;
    return h;
}

uint64_t SeededRandom::positionHash(int x, int y, int z, uint64_t baseSeed) {
    uint64_t h = baseSeed;
    h ^= static_cast<uint64_t>(x) * 374761393u;
    h ^= static_cast<uint64_t>(y) * 1013904223u;
    h ^= static_cast<uint64_t>(z) * 1664525u;
    h = (h ^ (h >> 13)) * 127412617731153u;
    return h;
}

void SeededRandom::setSeed(int x, int z) {
    // Combine position with some seed constant
    uint64_t combined = positionHash(x, z, 1234567890123456789ULL);
    rng.seedState(combined);
}

void SeededRandom::setSeed(int x, int y, int z) {
    uint64_t combined = positionHash(x, y, z, 1234567890123456789ULL);
    rng.seedState(combined);
}

} // namespace VoxelForge
