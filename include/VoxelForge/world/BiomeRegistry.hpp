/**
 * @file BiomeRegistry.hpp
 * @brief Biome registry for world generation
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace VoxelForge {

using BiomeID = uint16_t;
constexpr BiomeID INVALID_BIOME = 0;

struct BiomeClimate {
    float temperature = 0.5f;
    float humidity = 0.5f;
    float downfall = 0.0f;
};

struct Biome {
    BiomeID id = INVALID_BIOME;
    std::string name;
    BiomeClimate climate;
    
    // Generation settings
    int seaLevel = 63;
    float baseHeight = 0.1f;
    float heightVariation = 0.3f;
};

class BiomeRegistry {
public:
    static BiomeRegistry& get();
    
    BiomeID registerBiome(const std::string& name, const Biome& biome);
    const Biome* getBiome(BiomeID id) const;
    const Biome* getBiome(const std::string& name) const;
    BiomeID getBiomeId(const std::string& name) const;
    
    void initialize();
    
private:
    BiomeRegistry() = default;
    ~BiomeRegistry() = default;
    
    std::vector<Biome> biomes;
    std::unordered_map<std::string, BiomeID> nameToId;
};

} // namespace VoxelForge