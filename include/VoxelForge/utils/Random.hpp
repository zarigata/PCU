/**
 * @file Random.hpp
 * @brief Random number generation utilities
 */

#pragma once

#include <cstdint>
#include <random>
#include <algorithm>

namespace VoxelForge {

// XorShift128+ - fast, high-quality PRNG
class XorShift128Plus {
public:
    explicit XorShift128Plus(uint64_t seed = 0) {
        seedState(seed);
    }
    
    void seedState(uint64_t seed) {
        std::mt19937_64 seeder(seed);
        state[0] = seeder();
        state[1] = seeder();
        
        // Ensure non-zero state
        if (state[0] == 0 && state[1] == 0) {
            state[0] = 1;
        }
    }
    
    uint64_t next() {
        uint64_t s1 = state[0];
        uint64_t s0 = state[1];
        uint64_t result = s0 + s1;
        state[0] = s0;
        s1 ^= s1 << 23;
        state[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
        return result;
    }
    
    // Uniform random bits
    uint32_t nextUInt32() {
        return static_cast<uint32_t>(next() >> 32);
    }
    
    // Float in [0, 1)
    float nextFloat() {
        return static_cast<float>(next() >> 11) * (1.0f / 9007199254740992.0f);
    }
    
    // Float in [0, 1]
    float nextFloatClosed() {
        return static_cast<float>(next() >> 11) / static_cast<float>((1ULL << 53) - 1);
    }
    
    // Float in [-1, 1]
    float nextFloatSigned() {
        return nextFloat() * 2.0f - 1.0f;
    }
    
    // Double in [0, 1)
    double nextDouble() {
        return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0);
    }
    
    // Int in [min, max]
    int nextInt(int min, int max) {
        return min + static_cast<int>(nextFloat() * (max - min + 1));
    }
    
    // Int in [0, max)
    int nextInt(int max) {
        return static_cast<int>(nextFloat() * max);
    }
    
private:
    uint64_t state[2];
};

// Thread-local random for convenience
class Random {
public:
    static XorShift128Plus& get() {
        thread_local XorShift128Plus rng(std::random_device{}());
        return rng;
    }
    
    static float nextFloat() { return get().nextFloat(); }
    static double nextDouble() { return get().nextDouble(); }
    static int nextInt(int min, int max) { return get().nextInt(min, max); }
    static int nextInt(int max) { return get().nextInt(max); }
    static uint64_t next() { return get().next(); }
    
    // Random chance (0-1 probability)
    static bool chance(float probability) {
        return nextFloat() < probability;
    }
    
    // Random boolean
    static bool nextBool() {
        return (next() & 1) == 1;
    }
    
    // Random index based on weights
    template<typename Iterator>
    static size_t weightedChoice(Iterator begin, Iterator end) {
        float total = 0.0f;
        for (auto it = begin; it != end; ++it) {
            total += static_cast<float>(*it);
        }
        
        float r = nextFloat() * total;
        float cumulative = 0.0f;
        
        for (auto it = begin; it != end; ++it) {
            cumulative += static_cast<float>(*it);
            if (r < cumulative) {
                return static_cast<size_t>(std::distance(begin, it));
            }
        }
        
        return static_cast<size_t>(std::distance(begin, end) - 1);
    }
    
    // Shuffle a container
    template<typename Iterator>
    static void shuffle(Iterator begin, Iterator end) {
        auto n = std::distance(begin, end);
        for (auto i = n - 1; i > 0; --i) {
            auto j = nextInt(static_cast<int>(i + 1));
            std::swap(*(begin + i), *(begin + j));
        }
    }
    
    // Random element from container
    template<typename Container>
    static auto& choice(Container& c) {
        auto size = std::distance(c.begin(), c.end());
        return *(c.begin() + nextInt(static_cast<int>(size)));
    }
    
    // Random Gaussian (Box-Muller transform)
    static float nextGaussian(float mean = 0.0f, float stddev = 1.0f) {
        float u1 = nextFloat();
        float u2 = nextFloat();
        
        // Avoid log(0)
        while (u1 == 0) u1 = nextFloat();
        
        float z0 = std::sqrt(-2.0f * std::log(u1)) * std::cos(2.0f * 3.14159265358979f * u2);
        return z0 * stddev + mean;
    }
};

// Seeded random for reproducible generation
class SeededRandom {
public:
    explicit SeededRandom(uint64_t seed) : rng(seed) {}
    
    float nextFloat() { return rng.nextFloat(); }
    int nextInt(int min, int max) { return rng.nextInt(min, max); }
    int nextInt(int max) { return rng.nextInt(max); }
    uint64_t next() { return rng.next(); }
    bool chance(float p) { return rng.nextFloat() < p; }
    
    // Set position-based seed (for chunk generation)
    void setSeed(int x, int z);
    void setSeed(int x, int y, int z);
    
private:
    XorShift128Plus rng;
    
    static uint64_t positionHash(int x, int z, uint64_t baseSeed);
    static uint64_t positionHash(int x, int y, int z, uint64_t baseSeed);
};

} // namespace VoxelForge
