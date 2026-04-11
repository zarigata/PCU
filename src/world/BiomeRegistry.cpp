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

    // ============================================
    // FOREST BIOMES
    // ============================================

    // Birch Forest
    Biome birchForest;
    birchForest.id = 14;
    birchForest.name = "minecraft:birch_forest";
    birchForest.climate.temperature = 0.6f;
    birchForest.climate.humidity = 0.6f;
    birchForest.baseHeight = 0.1f;
    birchForest.heightVariation = 0.3f;
    registerBiome("minecraft:birch_forest", birchForest);

    // Dark Forest
    Biome darkForest;
    darkForest.id = 15;
    darkForest.name = "minecraft:dark_forest";
    darkForest.climate.temperature = 0.7f;
    darkForest.climate.humidity = 0.8f;
    darkForest.baseHeight = 0.1f;
    darkForest.heightVariation = 0.2f;
    registerBiome("minecraft:dark_forest", darkForest);

    // Flower Forest
    Biome flowerForest;
    flowerForest.id = 16;
    flowerForest.name = "minecraft:flower_forest";
    flowerForest.climate.temperature = 0.7f;
    flowerForest.climate.humidity = 0.8f;
    flowerForest.baseHeight = 0.1f;
    flowerForest.heightVariation = 0.3f;
    registerBiome("minecraft:flower_forest", flowerForest);

    // ============================================
    // MOUNTAIN BIOMES
    // ============================================

    // Windswept Hills
    Biome windsweptHills;
    windsweptHills.id = 17;
    windsweptHills.name = "minecraft:windswept_hills";
    windsweptHills.climate.temperature = 0.2f;
    windsweptHills.climate.humidity = 0.3f;
    windsweptHills.baseHeight = 0.5f;
    windsweptHills.heightVariation = 0.8f;
    registerBiome("minecraft:windswept_hills", windsweptHills);

    // Windswept Gravelly Hills
    Biome windsweptGravellyHills;
    windsweptGravellyHills.id = 18;
    windsweptGravellyHills.name = "minecraft:windswept_gravelly_hills";
    windsweptGravellyHills.climate.temperature = 0.2f;
    windsweptGravellyHills.climate.humidity = 0.3f;
    windsweptGravellyHills.baseHeight = 0.5f;
    windsweptGravellyHills.heightVariation = 0.9f;
    registerBiome("minecraft:windswept_gravelly_hills", windsweptGravellyHills);

    // Windswept Savanna
    Biome windsweptSavanna;
    windsweptSavanna.id = 19;
    windsweptSavanna.name = "minecraft:windswept_savanna";
    windsweptSavanna.climate.temperature = 1.0f;
    windsweptSavanna.climate.humidity = 0.0f;
    windsweptSavanna.baseHeight = 0.4f;
    windsweptSavanna.heightVariation = 0.7f;
    registerBiome("minecraft:windswept_savanna", windsweptSavanna);

    // Jagged Peaks
    Biome jaggedPeaks;
    jaggedPeaks.id = 20;
    jaggedPeaks.name = "minecraft:jagged_peaks";
    jaggedPeaks.climate.temperature = -0.7f;
    jaggedPeaks.climate.humidity = 0.3f;
    jaggedPeaks.baseHeight = 1.5f;
    jaggedPeaks.heightVariation = 1.2f;
    registerBiome("minecraft:jagged_peaks", jaggedPeaks);

    // Frozen Peaks
    Biome frozenPeaks;
    frozenPeaks.id = 21;
    frozenPeaks.name = "minecraft:frozen_peaks";
    frozenPeaks.climate.temperature = -0.7f;
    frozenPeaks.climate.humidity = 0.4f;
    frozenPeaks.baseHeight = 1.3f;
    frozenPeaks.heightVariation = 0.9f;
    registerBiome("minecraft:frozen_peaks", frozenPeaks);

    // Stony Peaks
    Biome stonyPeaks;
    stonyPeaks.id = 22;
    stonyPeaks.name = "minecraft:stony_peaks";
    stonyPeaks.climate.temperature = 0.2f;
    stonyPeaks.climate.humidity = 0.3f;
    stonyPeaks.baseHeight = 1.4f;
    stonyPeaks.heightVariation = 1.0f;
    registerBiome("minecraft:stony_peaks", stonyPeaks);

    // Snowy Slopes
    Biome snowySlopes;
    snowySlopes.id = 23;
    snowySlopes.name = "minecraft:snowy_slopes";
    snowySlopes.climate.temperature = -0.3f;
    snowySlopes.climate.humidity = 0.4f;
    snowySlopes.baseHeight = 0.8f;
    snowySlopes.heightVariation = 0.6f;
    registerBiome("minecraft:snowy_slopes", snowySlopes);

    // ============================================
    // SNOWY BIOMES
    // ============================================

    // Ice Spikes
    Biome iceSpikes;
    iceSpikes.id = 24;
    iceSpikes.name = "minecraft:ice_spikes";
    iceSpikes.climate.temperature = 0.0f;
    iceSpikes.climate.humidity = 0.5f;
    iceSpikes.baseHeight = 0.1f;
    iceSpikes.heightVariation = 0.3f;
    registerBiome("minecraft:ice_spikes", iceSpikes);

    // Snowy Taiga
    Biome snowyTaiga;
    snowyTaiga.id = 25;
    snowyTaiga.name = "minecraft:snowy_taiga";
    snowyTaiga.climate.temperature = -0.5f;
    snowyTaiga.climate.humidity = 0.6f;
    snowyTaiga.baseHeight = 0.2f;
    snowyTaiga.heightVariation = 0.3f;
    registerBiome("minecraft:snowy_taiga", snowyTaiga);

    // ============================================
    // JUNGLE BIOMES
    // ============================================

    // Bamboo Jungle
    Biome bambooJungle;
    bambooJungle.id = 26;
    bambooJungle.name = "minecraft:bamboo_jungle";
    bambooJungle.climate.temperature = 0.95f;
    bambooJungle.climate.humidity = 0.9f;
    bambooJungle.baseHeight = 0.1f;
    bambooJungle.heightVariation = 0.4f;
    registerBiome("minecraft:bamboo_jungle", bambooJungle);

    // Sparse Jungle
    Biome sparseJungle;
    sparseJungle.id = 27;
    sparseJungle.name = "minecraft:sparse_jungle";
    sparseJungle.climate.temperature = 0.95f;
    sparseJungle.climate.humidity = 0.9f;
    sparseJungle.baseHeight = 0.1f;
    sparseJungle.heightVariation = 0.3f;
    registerBiome("minecraft:sparse_jungle", sparseJungle);

    // ============================================
    // OCEAN BIOMES
    // ============================================

    // Deep Ocean
    Biome deepOcean;
    deepOcean.id = 28;
    deepOcean.name = "minecraft:deep_ocean";
    deepOcean.climate.temperature = 0.5f;
    deepOcean.climate.humidity = 0.5f;
    deepOcean.baseHeight = -1.5f;
    deepOcean.heightVariation = 0.1f;
    deepOcean.seaLevel = 63;
    registerBiome("minecraft:deep_ocean", deepOcean);

    // Frozen Ocean
    Biome frozenOcean;
    frozenOcean.id = 29;
    frozenOcean.name = "minecraft:frozen_ocean";
    frozenOcean.climate.temperature = 0.0f;
    frozenOcean.climate.humidity = 0.5f;
    frozenOcean.baseHeight = -1.0f;
    frozenOcean.heightVariation = 0.1f;
    frozenOcean.seaLevel = 63;
    registerBiome("minecraft:frozen_ocean", frozenOcean);

    // Deep Frozen Ocean
    Biome deepFrozenOcean;
    deepFrozenOcean.id = 30;
    deepFrozenOcean.name = "minecraft:deep_frozen_ocean";
    deepFrozenOcean.climate.temperature = 0.0f;
    deepFrozenOcean.climate.humidity = 0.5f;
    deepFrozenOcean.baseHeight = -1.5f;
    deepFrozenOcean.heightVariation = 0.1f;
    deepFrozenOcean.seaLevel = 63;
    registerBiome("minecraft:deep_frozen_ocean", deepFrozenOcean);

    // Cold Ocean
    Biome coldOcean;
    coldOcean.id = 31;
    coldOcean.name = "minecraft:cold_ocean";
    coldOcean.climate.temperature = 0.0f;
    coldOcean.climate.humidity = 0.5f;
    coldOcean.baseHeight = -1.0f;
    coldOcean.heightVariation = 0.1f;
    coldOcean.seaLevel = 63;
    registerBiome("minecraft:cold_ocean", coldOcean);

    // Deep Cold Ocean
    Biome deepColdOcean;
    deepColdOcean.id = 32;
    deepColdOcean.name = "minecraft:deep_cold_ocean";
    deepColdOcean.climate.temperature = 0.0f;
    deepColdOcean.climate.humidity = 0.5f;
    deepColdOcean.baseHeight = -1.5f;
    deepColdOcean.heightVariation = 0.1f;
    deepColdOcean.seaLevel = 63;
    registerBiome("minecraft:deep_cold_ocean", deepColdOcean);

    // Lukewarm Ocean
    Biome lukewarmOcean;
    lukewarmOcean.id = 33;
    lukewarmOcean.name = "minecraft:lukewarm_ocean";
    lukewarmOcean.climate.temperature = 0.8f;
    lukewarmOcean.climate.humidity = 0.5f;
    lukewarmOcean.baseHeight = -1.0f;
    lukewarmOcean.heightVariation = 0.1f;
    lukewarmOcean.seaLevel = 63;
    registerBiome("minecraft:lukewarm_ocean", lukewarmOcean);

    // Deep Lukewarm Ocean
    Biome deepLukewarmOcean;
    deepLukewarmOcean.id = 34;
    deepLukewarmOcean.name = "minecraft:deep_lukewarm_ocean";
    deepLukewarmOcean.climate.temperature = 0.8f;
    deepLukewarmOcean.climate.humidity = 0.5f;
    deepLukewarmOcean.baseHeight = -1.5f;
    deepLukewarmOcean.heightVariation = 0.1f;
    deepLukewarmOcean.seaLevel = 63;
    registerBiome("minecraft:deep_lukewarm_ocean", deepLukewarmOcean);

    // Warm Ocean
    Biome warmOcean;
    warmOcean.id = 35;
    warmOcean.name = "minecraft:warm_ocean";
    warmOcean.climate.temperature = 1.0f;
    warmOcean.climate.humidity = 0.5f;
    warmOcean.baseHeight = -1.0f;
    warmOcean.heightVariation = 0.1f;
    warmOcean.seaLevel = 63;
    registerBiome("minecraft:warm_ocean", warmOcean);

    // ============================================
    // BADLANDS BIOMES
    // ============================================

    // Eroded Badlands
    Biome erodedBadlands;
    erodedBadlands.id = 36;
    erodedBadlands.name = "minecraft:eroded_badlands";
    erodedBadlands.climate.temperature = 2.0f;
    erodedBadlands.climate.humidity = 0.0f;
    erodedBadlands.baseHeight = 0.2f;
    erodedBadlands.heightVariation = 0.6f;
    registerBiome("minecraft:eroded_badlands", erodedBadlands);

    // Wooded Badlands
    Biome woodedBadlands;
    woodedBadlands.id = 37;
    woodedBadlands.name = "minecraft:wooded_badlands";
    woodedBadlands.climate.temperature = 2.0f;
    woodedBadlands.climate.humidity = 0.0f;
    woodedBadlands.baseHeight = 0.3f;
    woodedBadlands.heightVariation = 0.5f;
    registerBiome("minecraft:wooded_badlands", woodedBadlands);

    // ============================================
    // SWAMP BIOMES
    // ============================================

    // Mangrove Swamp
    Biome mangroveSwamp;
    mangroveSwamp.id = 38;
    mangroveSwamp.name = "minecraft:mangrove_swamp";
    mangroveSwamp.climate.temperature = 0.8f;
    mangroveSwamp.climate.humidity = 0.9f;
    mangroveSwamp.baseHeight = -0.1f;
    mangroveSwamp.heightVariation = 0.2f;
    registerBiome("minecraft:mangrove_swamp", mangroveSwamp);

    // ============================================
    // BEACH BIOMES
    // ============================================

    // Snowy Beach
    Biome snowyBeach;
    snowyBeach.id = 39;
    snowyBeach.name = "minecraft:snowy_beach";
    snowyBeach.climate.temperature = 0.0f;
    snowyBeach.climate.humidity = 0.4f;
    snowyBeach.baseHeight = 0.0f;
    snowyBeach.heightVariation = 0.1f;
    registerBiome("minecraft:snowy_beach", snowyBeach);

    // Stony Shore
    Biome stonyShore;
    stonyShore.id = 40;
    stonyShore.name = "minecraft:stony_shore";
    stonyShore.climate.temperature = 0.2f;
    stonyShore.climate.humidity = 0.3f;
    stonyShore.baseHeight = 0.0f;
    stonyShore.heightVariation = 0.2f;
    registerBiome("minecraft:stony_shore", stonyShore);

    // ============================================
    // DESERT BIOMES
    // ============================================

    // Desert Hills
    Biome desertHills;
    desertHills.id = 41;
    desertHills.name = "minecraft:desert_hills";
    desertHills.climate.temperature = 2.0f;
    desertHills.climate.humidity = 0.0f;
    desertHills.baseHeight = 0.3f;
    desertHills.heightVariation = 0.4f;
    registerBiome("minecraft:desert_hills", desertHills);

    // ============================================
    // TAIGA BIOMES
    // ============================================

    // Old Growth Birch Forest
    Biome oldGrowthBirchForest;
    oldGrowthBirchForest.id = 42;
    oldGrowthBirchForest.name = "minecraft:old_growth_birch_forest";
    oldGrowthBirchForest.climate.temperature = 0.6f;
    oldGrowthBirchForest.climate.humidity = 0.6f;
    oldGrowthBirchForest.baseHeight = 0.1f;
    oldGrowthBirchForest.heightVariation = 0.3f;
    registerBiome("minecraft:old_growth_birch_forest", oldGrowthBirchForest);

    // Old Growth Pine Taiga
    Biome oldGrowthPineTaiga;
    oldGrowthPineTaiga.id = 43;
    oldGrowthPineTaiga.name = "minecraft:old_growth_pine_taiga";
    oldGrowthPineTaiga.climate.temperature = 0.3f;
    oldGrowthPineTaiga.climate.humidity = 0.8f;
    oldGrowthPineTaiga.baseHeight = 0.2f;
    oldGrowthPineTaiga.heightVariation = 0.4f;
    registerBiome("minecraft:old_growth_pine_taiga", oldGrowthPineTaiga);

    // Old Growth Spruce Taiga
    Biome oldGrowthSpruceTaiga;
    oldGrowthSpruceTaiga.id = 44;
    oldGrowthSpruceTaiga.name = "minecraft:old_growth_spruce_taiga";
    oldGrowthSpruceTaiga.climate.temperature = 0.25f;
    oldGrowthSpruceTaiga.climate.humidity = 0.8f;
    oldGrowthSpruceTaiga.baseHeight = 0.2f;
    oldGrowthSpruceTaiga.heightVariation = 0.4f;
    registerBiome("minecraft:old_growth_spruce_taiga", oldGrowthSpruceTaiga);

    // ============================================
    // SAVANNA BIOMES
    // ============================================

    // Savanna Plateau
    Biome savannaPlateau;
    savannaPlateau.id = 45;
    savannaPlateau.name = "minecraft:savanna_plateau";
    savannaPlateau.climate.temperature = 1.2f;
    savannaPlateau.climate.humidity = 0.0f;
    savannaPlateau.baseHeight = 0.8f;
    savannaPlateau.heightVariation = 0.4f;
    registerBiome("minecraft:savanna_plateau", savannaPlateau);

    // Windswept Savanna
    Biome windsweptSavanna2;
    windsweptSavanna2.id = 46;
    windsweptSavanna2.name = "minecraft:windswept_savanna";
    windsweptSavanna2.climate.temperature = 1.0f;
    windsweptSavanna2.climate.humidity = 0.0f;
    windsweptSavanna2.baseHeight = 0.4f;
    windsweptSavanna2.heightVariation = 0.7f;
    registerBiome("minecraft:windswept_savanna_alt", windsweptSavanna2);

    // ============================================
    // MUSHROOM BIOMES
    // ============================================

    // Mushroom Fields
    Biome mushroomFields;
    mushroomFields.id = 47;
    mushroomFields.name = "minecraft:mushroom_fields";
    mushroomFields.climate.temperature = 0.9f;
    mushroomFields.climate.humidity = 1.0f;
    mushroomFields.baseHeight = 0.1f;
    mushroomFields.heightVariation = 0.2f;
    registerBiome("minecraft:mushroom_fields", mushroomFields);

    // ============================================
    // MEADOW BIOMES
    // ============================================

    // Meadow
    Biome meadow;
    meadow.id = 48;
    meadow.name = "minecraft:meadow";
    meadow.climate.temperature = 0.5f;
    meadow.climate.humidity = 0.6f;
    meadow.baseHeight = 0.1f;
    meadow.heightVariation = 0.3f;
    registerBiome("minecraft:meadow", meadow);

    // ============================================
    // GROVE BIOMES
    // ============================================

    // Grove
    Biome grove;
    grove.id = 49;
    grove.name = "minecraft:grove";
    grove.climate.temperature = -0.2f;
    grove.climate.humidity = 0.6f;
    grove.baseHeight = 0.4f;
    grove.heightVariation = 0.5f;
    registerBiome("minecraft:grove", grove);

    // Snowy Grove
    Biome snowyGrove;
    snowyGrove.id = 50;
    snowyGrove.name = "minecraft:snowy_grove";
    snowyGrove.climate.temperature = -0.3f;
    snowyGrove.climate.humidity = 0.6f;
    snowyGrove.baseHeight = 0.4f;
    snowyGrove.heightVariation = 0.5f;
    registerBiome("minecraft:snowy_grove", snowyGrove);

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
