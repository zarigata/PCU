/**
 * @file BiomeRegistry.cpp
 * @brief Biome registry implementation
 */

#include <VoxelForge/world/BiomeRegistry.hpp>
#include <VoxelForge/world/BlockRegistry.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

BiomeRegistry& BiomeRegistry::get() {
    static BiomeRegistry instance;
    return instance;
}

BiomeRegistry::BiomeRegistry() {
    registerVanillaBiomes();
}

void BiomeRegistry::registerVanillaBiomes() {
    LOG_INFO("Registering vanilla biomes...");
    
    auto& blocks = BlockRegistry::get();
    
    // Plains
    auto plains = std::make_unique<Biome>(1, "minecraft:plains");
    plains->setTemperature(0.8f);
    plains->setDownfall(0.4f);
    plains->setSurfaceBlock(blocks.getDefaultState("minecraft:grass_block"));
    plains->setSubsurfaceBlock(blocks.getDefaultState("minecraft:dirt"));
    plains->setCategory(Biome::Category::Plains);
    registerBiome(std::move(plains));
    
    // Desert
    auto desert = std::make_unique<Biome>(2, "minecraft:desert");
    desert->setTemperature(2.0f);
    desert->setDownfall(0.0f);
    desert->setSurfaceBlock(blocks.getDefaultState("minecraft:sand"));
    desert->setSubsurfaceBlock(blocks.getDefaultState("minecraft:sandstone"));
    desert->setCategory(Biome::Category::Desert);
    registerBiome(std::move(desert));
    
    // Forest
    auto forest = std::make_unique<Biome>(4, "minecraft:forest");
    forest->setTemperature(0.7f);
    forest->setDownfall(0.8f);
    forest->setSurfaceBlock(blocks.getDefaultState("minecraft:grass_block"));
    forest->setSubsurfaceBlock(blocks.getDefaultState("minecraft:dirt"));
    forest->setCategory(Biome::Category::Forest);
    registerBiome(std::move(forest));
    
    // Jungle
    auto jungle = std::make_unique<Biome>(6, "minecraft:jungle");
    jungle->setTemperature(0.95f);
    jungle->setDownfall(0.9f);
    jungle->setSurfaceBlock(blocks.getDefaultState("minecraft:grass_block"));
    jungle->setSubsurfaceBlock(blocks.getDefaultState("minecraft:dirt"));
    jungle->setCategory(Biome::Category::Jungle);
    registerBiome(std::move(jungle));
    
    // Swamp
    auto swamp = std::make_unique<Biome>(9, "minecraft:swamp");
    swamp->setTemperature(0.8f);
    swamp->setDownfall(0.9f);
    swamp->setSurfaceBlock(blocks.getDefaultState("minecraft:grass_block"));
    swamp->setSubsurfaceBlock(blocks.getDefaultState("minecraft:dirt"));
    swamp->setCategory(Biome::Category::Swamp);
    registerBiome(std::move(swamp));
    
    // Savanna
    auto savanna = std::make_unique<Biome>(5, "minecraft:savanna");
    savanna->setTemperature(1.2f);
    savanna->setDownfall(0.0f);
    savanna->setSurfaceBlock(blocks.getDefaultState("minecraft:grass_block"));
    savanna->setSubsurfaceBlock(blocks.getDefaultState("minecraft:dirt"));
    savanna->setCategory(Biome::Category::Savanna);
    registerBiome(std::move(savanna));
    
    // Badlands
    auto badlands = std::make_unique<Biome>(7, "minecraft:badlands");
    badlands->setTemperature(2.0f);
    badlands->setDownfall(0.0f);
    badlands->setSurfaceBlock(blocks.getDefaultState("minecraft:red_sand"));
    badlands->setSubsurfaceBlock(blocks.getDefaultState("minecraft:red_sandstone"));
    badlands->setCategory(Biome::Category::Mesa);
    registerBiome(std::move(badlands));
    
    // Snowy Plains
    auto snowyPlains = std::make_unique<Biome>(12, "minecraft:snowy_plains");
    snowyPlains->setTemperature(0.0f);
    snowyPlains->setDownfall(0.5f);
    snowyPlains->setPrecipitation(Biome::Precipitation::Snow);
    snowyPlains->setSurfaceBlock(blocks.getDefaultState("minecraft:snow_block"));
    snowyPlains->setSubsurfaceBlock(blocks.getDefaultState("minecraft:dirt"));
    snowyPlains->setCategory(Biome::Category::Icy);
    registerBiome(std::move(snowyPlains));
    
    // Ocean
    auto ocean = std::make_unique<Biome>(0, "minecraft:ocean");
    ocean->setTemperature(0.5f);
    ocean->setDownfall(0.5f);
    ocean->setBaseHeight(-1.0f);
    ocean->setSurfaceBlock(blocks.getDefaultState("minecraft:gravel"));
    ocean->setSubsurfaceBlock(blocks.getDefaultState("minecraft:gravel"));
    ocean->setCategory(Biome::Category::Ocean);
    registerBiome(std::move(ocean));
    
    LOG_INFO("Registered {} vanilla biomes", biomes.size());
}

BiomeId BiomeRegistry::registerBiome(std::unique_ptr<Biome> biome) {
    BiomeId id = biome->getId();
    biomes[id] = std::move(biome);
    return id;
}

const Biome* BiomeRegistry::getBiome(BiomeId id) const {
    auto it = biomes.find(id);
    if (it != biomes.end()) {
        return it->second.get();
    }
    return nullptr;
}

} // namespace VoxelForge
