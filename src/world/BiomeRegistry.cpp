/**
 * @file BiomeRegistry.cpp
 * @brief Biome registry implementation
 */

#include <VoxelForge/world/BiomeRegistry.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

BiomeRegistry& BiomeRegistry::get() {
    static BiomeRegistry instance;
    return instance;
}

void BiomeRegistry::initialize() {
    if (!biomes.empty()) return;

    // Plains
    Biome plains;
    plains.id = 1;
    plains.name = "minecraft:plains";
    plains.climate.temperature = 0.8f;
    plains.climate.humidity = 0.4f;
    plains.baseHeight = 0.1f;
    plains.heightVariation = 0.3f;
    registerBiome("minecraft:plains", plains);

    // Desert
    Biome desert;
    desert.id = 2;
    desert.name = "minecraft:desert";
    desert.climate.temperature = 2.0f;
    desert.climate.humidity = 0.0f;
    desert.baseHeight = 0.1f;
    desert.heightVariation = 0.2f;
    registerBiome("minecraft:desert", desert);

    // Forest
    Biome forest;
    forest.id = 3;
    forest.name = "minecraft:forest";
    forest.climate.temperature = 0.7f;
    forest.climate.humidity = 0.8f;
    forest.baseHeight = 0.1f;
    forest.heightVariation = 0.3f;
    registerBiome("minecraft:forest", forest);

    // Taiga
    Biome taiga;
    taiga.id = 4;
    taiga.name = "minecraft:taiga";
    taiga.climate.temperature = 0.25f;
    taiga.climate.humidity = 0.8f;
    taiga.baseHeight = 0.2f;
    taiga.heightVariation = 0.3f;
    registerBiome("minecraft:taiga", taiga);

    // Swamp
    Biome swamp;
    swamp.id = 5;
    swamp.name = "minecraft:swamp";
    swamp.climate.temperature = 0.8f;
    swamp.climate.humidity = 0.9f;
    swamp.baseHeight = -0.1f;
    swamp.heightVariation = 0.2f;
    registerBiome("minecraft:swamp", swamp);

    // Jungle
    Biome jungle;
    jungle.id = 6;
    jungle.name = "minecraft:jungle";
    jungle.climate.temperature = 0.95f;
    jungle.climate.humidity = 0.9f;
    jungle.baseHeight = 0.1f;
    jungle.heightVariation = 0.4f;
    registerBiome("minecraft:jungle", jungle);

    // Savanna
    Biome savanna;
    savanna.id = 7;
    savanna.name = "minecraft:savanna";
    savanna.climate.temperature = 1.2f;
    savanna.climate.humidity = 0.0f;
    savanna.baseHeight = 0.1f;
    savanna.heightVariation = 0.2f;
    registerBiome("minecraft:savanna", savanna);

    // Badlands
    Biome badlands;
    badlands.id = 8;
    badlands.name = "minecraft:badlands";
    badlands.climate.temperature = 2.0f;
    badlands.climate.humidity = 0.0f;
    badlands.baseHeight = 0.3f;
    badlands.heightVariation = 0.5f;
    registerBiome("minecraft:badlands", badlands);

    // Snowy Plains
    Biome snowyPlains;
    snowyPlains.id = 9;
    snowyPlains.name = "minecraft:snowy_plains";
    snowyPlains.climate.temperature = 0.0f;
    snowyPlains.climate.humidity = 0.5f;
    snowyPlains.baseHeight = 0.1f;
    snowyPlains.heightVariation = 0.3f;
    registerBiome("minecraft:snowy_plains", snowyPlains);

    // Ocean
    Biome ocean;
    ocean.id = 10;
    ocean.name = "minecraft:ocean";
    ocean.climate.temperature = 0.5f;
    ocean.climate.humidity = 0.5f;
    ocean.baseHeight = -1.0f;
    ocean.heightVariation = 0.1f;
    ocean.seaLevel = 63;
    registerBiome("minecraft:ocean", ocean);

    // Mountains
    Biome mountains;
    mountains.id = 11;
    mountains.name = "minecraft:windswept_hills";
    mountains.climate.temperature = 0.2f;
    mountains.climate.humidity = 0.3f;
    mountains.baseHeight = 0.5f;
    mountains.heightVariation = 0.8f;
    registerBiome("minecraft:windspept_hills", mountains);

    // Beach
    Biome beach;
    beach.id = 12;
    beach.name = "minecraft:beach";
    beach.climate.temperature = 0.8f;
    beach.climate.humidity = 0.4f;
    beach.baseHeight = 0.0f;
    beach.heightVariation = 0.1f;
    registerBiome("minecraft:beach", beach);

    // River
    Biome river;
    river.id = 13;
    river.name = "minecraft:river";
    river.climate.temperature = 0.5f;
    river.climate.humidity = 0.5f;
    river.baseHeight = -0.5f;
    river.heightVariation = 0.1f;
    registerBiome("minecraft:river", river);

    VF_INFO("Registered {} vanilla biomes", biomes.size());
}

BiomeID BiomeRegistry::registerBiome(const std::string& name, const Biome& biome) {
    BiomeID id = static_cast<BiomeID>(biomes.size());
    Biome& stored = biomes.emplace_back(biome);
    stored.id = id;
    nameToId[name] = id;
    return id;
}

const Biome* BiomeRegistry::getBiome(BiomeID id) const {
    if (id < biomes.size()) {
        return &biomes[id];
    }
    return nullptr;
}

const Biome* BiomeRegistry::getBiome(const std::string& name) const {
    auto it = nameToId.find(name);
    if (it != nameToId.end()) {
        return getBiome(it->second);
    }
    return nullptr;
}

BiomeID BiomeRegistry::getBiomeId(const std::string& name) const {
    auto it = nameToId.find(name);
    if (it != nameToId.end()) {
        return it->second;
    }
    return INVALID_BIOME;
}

} // namespace VoxelForge
