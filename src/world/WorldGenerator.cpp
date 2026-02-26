/**
 * @file WorldGenerator.cpp
 * @brief World terrain generation implementation
 */

#include <VoxelForge/world/WorldGenerator.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/world/BlockRegistry.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <VoxelForge/utils/Profiler.hpp>
#include <glm/gtc/noise.hpp>
#include <chrono>
#include <future>
#include <algorithm>

namespace VoxelForge {

// ============================================================================
// Ore Definitions
// ============================================================================

namespace Ores {
    const OreVein COAL_ORE = {"minecraft:coal_ore", 0, 192, 17, 20, 0.0f};
    const OreVein IRON_ORE = {"minecraft:iron_ore", -64, 320, 9, 10, 0.0f};
    const OreVein IRON_ORE_TUFF = {"minecraft:iron_ore", -64, 72, 9, 10, 0.0f};
    const OreVein GOLD_ORE = {"minecraft:gold_ore", -64, 32, 9, 2, 0.0f};
    const OreVein GOLD_ORE_BADLANDS = {"minecraft:gold_ore", 32, 256, 9, 5, 0.0f};
    const OreVein DIAMOND_ORE = {"minecraft:diamond_ore", -64, 16, 8, 1, 0.5f};
    const OreVein REDSTONE_ORE = {"minecraft:redstone_ore", -64, 16, 8, 4, 0.0f};
    const OreVein LAPIS_ORE = {"minecraft:lapis_ore", -64, 64, 7, 2, 0.0f};
    const OreVein COPPER_ORE = {"minecraft:copper_ore", -16, 112, 10, 6, 0.0f};
    const OreVein EMERALD_ORE = {"minecraft:emerald_ore", -16, 256, 1, 2, 0.0f};
    const OreVein ANCIENT_DEBRIS = {"minecraft:ancient_debris", 8, 119, 2, 1, 0.7f};
}

// ============================================================================
// Biome Implementation
// ============================================================================

Biome::Biome(BiomeId id, const std::string& name)
    : id(id), name(name) {
    // Set default blocks
    surfaceBlock = BlockRegistry::get().getDefaultState("minecraft:grass_block");
    subsurfaceBlock = BlockRegistry::get().getDefaultState("minecraft:dirt");
    underwaterBlock = BlockRegistry::get().getDefaultState("minecraft:gravel");
}

void Biome::addFeature(const std::string& featureId, int weight) {
    features.emplace_back(featureId, weight);
}

// ============================================================================
// WorldGenerator Implementation
// ============================================================================

WorldGenerator::WorldGenerator(uint64_t seed) 
    : random(seed) {
    settings.seed = seed;
    
    // Initialize noise generators
    heightNoise = std::make_unique<PerlinNoise>(seed);
    densityNoise = std::make_unique<PerlinNoise>(seed + 1);
    caveNoise = std::make_unique<PerlinNoise>(seed + 2);
    biomeNoise = std::make_unique<PerlinNoise>(seed + 3);
    erosionNoise = std::make_unique<PerlinNoise>(seed + 4);
    continentalnessNoise = std::make_unique<PerlinNoise>(seed + 5);
    peaksValleysNoise = std::make_unique<PerlinNoise>(seed + 6);
    oreNoise = std::make_unique<SimplexNoise>(seed + 7);
    
    LOG_INFO("WorldGenerator created with seed {}", seed);
}

WorldGenerator::WorldGenerator(const WorldGenSettings& settings)
    : settings(settings), random(settings.seed) {
    
    // Initialize noise generators
    heightNoise = std::make_unique<PerlinNoise>(settings.seed);
    densityNoise = std::make_unique<PerlinNoise>(settings.seed + 1);
    caveNoise = std::make_unique<PerlinNoise>(settings.seed + 2);
    biomeNoise = std::make_unique<PerlinNoise>(settings.seed + 3);
    erosionNoise = std::make_unique<PerlinNoise>(settings.seed + 4);
    continentalnessNoise = std::make_unique<PerlinNoise>(settings.seed + 5);
    peaksValleysNoise = std::make_unique<PerlinNoise>(settings.seed + 6);
    oreNoise = std::make_unique<SimplexNoise>(settings.seed + 7);
    
    LOG_INFO("WorldGenerator created with seed {} and custom settings", settings.seed);
}

WorldGenerator::~WorldGenerator() {
    LOG_INFO("WorldGenerator destroyed");
}

void WorldGenerator::resetStats() {
    stats = Stats{};
}

ChunkGenResult WorldGenerator::generateChunk(Chunk* chunk, World* world) {
    PROFILE_FUNCTION();
    
    ChunkGenResult result;
    result.position = chunk->getPosition();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (!chunk) {
        LOG_ERROR("Null chunk passed to generateChunk");
        return result;
    }
    
    // Flat world for testing
    if (settings.flatWorld) {
        // Simple flat terrain
        int waterLevel = settings.seaLevel;
        
        for (int x = 0; x < CHUNK_WIDTH; x++) {
            for (int z = 0; z < CHUNK_WIDTH; z++) {
                // Bedrock
                chunk->setBlock(x, settings.bedrockFloor, z,
                    BlockRegistry::get().getDefaultState("minecraft:bedrock"));
                
                // Dirt layers
                for (int y = settings.bedrockFloor + 1; y < waterLevel - 4; y++) {
                    chunk->setBlock(x, y, z,
                        BlockRegistry::get().getDefaultState("minecraft:dirt"));
                }
                
                // Grass on top
                chunk->setBlock(x, waterLevel - 4, z,
                    BlockRegistry::get().getDefaultState("minecraft:grass_block"));
            }
        }
        
        result.success = true;
        result.blocksGenerated = CHUNK_WIDTH * CHUNK_WIDTH * (waterLevel - settings.bedrockFloor - 3);
    } else {
        // Full terrain generation
        generateBiomes(chunk);
        generateTerrain(chunk);
        generateBedrock(chunk);
        
        if (settings.generateCaves) {
            generateCaves(chunk);
        }
        
        if (settings.generateOres) {
            SeededRandom chunkRandom(settings.seed ^ 
                (static_cast<uint64_t>(chunk->getPosition().x) << 32) ^
                static_cast<uint64_t>(chunk->getPosition().z));
            generateOres(chunk, chunkRandom);
        }
        
        if (settings.generateStructures) {
            generateFeatures(chunk, world);
        }
    }
    
    // Mark chunk as fully generated
    chunk->setStatus(Chunk::Status::Full);
    chunk->recalculateHeightMaps();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.generationTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    result.success = true;
    
    // Update statistics
    stats.chunksGenerated++;
    stats.totalTimeMs += result.generationTimeMs;
    stats.averageTimeMs = stats.totalTimeMs / stats.chunksGenerated;
    
    return result;
}

std::future<ChunkGenResult> WorldGenerator::generateChunkAsync(Chunk* chunk, World* world) {
    return std::async(std::launch::async, [this, chunk, world]() {
        return generateChunk(chunk, world);
    });
}

int WorldGenerator::getHeight(int x, int z) const {
    if (settings.flatWorld) {
        return settings.seaLevel - 4;
    }
    
    float height = calculateHeight(x, z);
    return static_cast<int>(height);
}

BiomeId WorldGenerator::getBiome(int x, int y, int z) const {
    // Simple biome selection based on temperature and humidity
    float temp = biomeNoise->noise2D(x * 0.005f, z * 0.005f);
    float humidity = biomeNoise->noise2D(x * 0.005f + 1000, z * 0.005f + 1000);
    
    // Normalize to 0-1
    temp = (temp + 1.0f) * 0.5f;
    humidity = (humidity + 1.0f) * 0.5f;
    
    // Biome categories (simplified)
    if (temp < 0.25f) {
        // Cold biomes
        if (humidity < 0.5f) return 12; // Snowy plains
        else return 13; // Snowy taiga
    } else if (temp < 0.5f) {
        // Temperate biomes
        if (humidity < 0.3f) return 1; // Plains
        else if (humidity < 0.6f) return 4; // Forest
        else return 9; // Swamp
    } else if (temp < 0.75f) {
        // Warm biomes
        if (humidity < 0.3f) return 2; // Desert
        else if (humidity < 0.6f) return 5; // Savanna
        else return 6; // Jungle
    } else {
        // Hot biomes
        if (humidity < 0.3f) return 2; // Desert
        else return 7; // Badlands
    }
}

void WorldGenerator::registerFeature(const std::string& id, FeatureGenerator generator) {
    features[id] = std::move(generator);
}

void WorldGenerator::generateTerrain(Chunk* chunk) {
    PROFILE_FUNCTION();
    
    auto& blockRegistry = BlockRegistry::get();
    
    ChunkPos chunkPos = chunk->getPosition();
    int worldX = chunkPos.x * CHUNK_WIDTH;
    int worldZ = chunkPos.z * CHUNK_WIDTH;
    
    int blocksPlaced = 0;
    
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_WIDTH; z++) {
            int wx = worldX + x;
            int wz = worldZ + z;
            
            // Calculate terrain height
            float height = calculateHeight(wx, wz);
            int terrainHeight = static_cast<int>(height);
            
            // Get biome
            BiomeId biomeId = getBiome(wx, terrainHeight, wz);
            
            // Fill terrain
            for (int y = settings.minHeight; y <= std::min(terrainHeight, settings.maxHeight); y++) {
                float density = calculateDensity(wx, y, wz);
                
                if (density > 0.0f) {
                    BlockState block;
                    
                    if (y == terrainHeight) {
                        // Surface block
                        block = getSurfaceBlock(wx, y, wz, biomeId);
                    } else if (y > terrainHeight - 4) {
                        // Subsurface
                        block = getSubsurfaceBlock(y, biomeId);
                    } else {
                        // Deep underground
                        block = blockRegistry.getDefaultState("minecraft:stone");
                    }
                    
                    chunk->setBlock(x, y, z, block);
                    blocksPlaced++;
                }
            }
            
            // Fill water in low areas
            for (int y = terrainHeight + 1; y <= settings.seaLevel; y++) {
                if (!chunk->getBlock(x, y, z).isSolid()) {
                    chunk->setBlock(x, y, z, 
                        blockRegistry.getDefaultState("minecraft:water"));
                }
            }
        }
    }
    
    stats.blocksPlaced += blocksPlaced;
}

void WorldGenerator::generateBedrock(Chunk* chunk) {
    auto& blockRegistry = BlockRegistry::get();
    auto bedrock = blockRegistry.getDefaultState("minecraft:bedrock");
    
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_WIDTH; z++) {
            // Bottom bedrock layer
            chunk->setBlock(x, settings.minHeight, z, bedrock);
            
            // Random bedrock above (fades out)
            for (int y = settings.minHeight + 1; y < settings.minHeight + 5; y++) {
                float chance = 1.0f - (y - settings.minHeight) / 5.0f;
                if (random.nextFloat() < chance) {
                    chunk->setBlock(x, y, z, bedrock);
                }
            }
        }
    }
}

void WorldGenerator::generateCaves(Chunk* chunk) {
    PROFILE_FUNCTION();
    
    ChunkPos chunkPos = chunk->getPosition();
    int worldX = chunkPos.x * CHUNK_WIDTH;
    int worldZ = chunkPos.z * CHUNK_WIDTH;
    
    auto& blockRegistry = BlockRegistry::get();
    auto air = blockRegistry.getDefaultState("minecraft:air");
    auto caveAir = blockRegistry.getDefaultState("minecraft:cave_air");
    
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_WIDTH; z++) {
            int wx = worldX + x;
            int wz = worldZ + z;
            
            for (int y = settings.minHeight + 5; y < settings.maxHeight - 10; y++) {
                // 3D noise for caves
                float noise3D = caveNoise->noise3D(
                    wx * settings.caveFrequency,
                    y * settings.caveFrequency * 2,
                    wz * settings.caveFrequency
                );
                
                // Carve cave if above threshold
                if (noise3D > settings.caveThreshold) {
                    BlockState current = chunk->getBlock(x, y, z);
                    
                    // Don't carve through bedrock or water
                    if (!current.isAir() && 
                        current.getBlockId() != blockRegistry.getBlockId("minecraft:bedrock") &&
                        current.getBlockId() != blockRegistry.getBlockId("minecraft:water")) {
                        chunk->setBlock(x, y, z, air);
                    }
                }
            }
        }
    }
}

void WorldGenerator::generateOres(Chunk* chunk, SeededRandom& random) {
    PROFILE_FUNCTION();
    
    std::vector<OreVein> ores = {
        Ores::COAL_ORE,
        Ores::IRON_ORE,
        Ores::GOLD_ORE,
        Ores::DIAMOND_ORE,
        Ores::REDSTONE_ORE,
        Ores::LAPIS_ORE,
        Ores::COPPER_ORE
    };
    
    for (const auto& ore : ores) {
        placeOreVein(chunk, ore, random);
    }
}

void WorldGenerator::placeOreVein(Chunk* chunk, const OreVein& ore, SeededRandom& random) {
    auto& blockRegistry = BlockRegistry::get();
    BlockState oreBlock = blockRegistry.getDefaultState(ore.oreBlock);
    BlockState stoneBlock = blockRegistry.getDefaultState("minecraft:stone");
    BlockState deepslateBlock = blockRegistry.getDefaultState("minecraft:deepslate");
    
    for (int i = 0; i < ore.veinsPerChunk; i++) {
        // Random position
        int x = random.nextInt(CHUNK_WIDTH);
        int z = random.nextInt(CHUNK_WIDTH);
        int y = random.nextInt(ore.maxY - ore.minY) + ore.minY;
        
        // Check discard chance
        if (random.nextFloat() < ore.discardChance) {
            continue;
        }
        
        // Determine ore variant (deepslate vs stone)
        BlockState oreToPlace = oreBlock;
        if (y < 0) {
            // Use deepslate variant if available
            std::string deepslateOre = ore.oreBlock;
            size_t pos = deepslateOre.find("_ore");
            if (pos != std::string::npos) {
                deepslateOre.insert(pos, "_deepslate");
                BlockState deepslateOreBlock = blockRegistry.getDefaultState(deepslateOre);
                if (!deepslateOreBlock.isAir()) {
                    oreToPlace = deepslateOreBlock;
                }
            }
        }
        
        // Place ore blob
        placeOreBlob(chunk, x, y, z, oreToPlace, ore.veinSize, random);
    }
}

void WorldGenerator::placeOreBlob(Chunk* chunk, int startX, int startY, int startZ,
                                   BlockState ore, int size, SeededRandom& random) {
    auto& blockRegistry = BlockRegistry::get();
    BlockState stone = blockRegistry.getDefaultState("minecraft:stone");
    BlockState deepslate = blockRegistry.getDefaultState("minecraft:deepslate");
    
    // Simple flood-fill ore placement
    for (int i = 0; i < size; i++) {
        int dx = random.nextInt(3) - 1;
        int dy = random.nextInt(3) - 1;
        int dz = random.nextInt(3) - 1;
        
        int x = startX + dx;
        int y = startY + dy;
        int z = startZ + dz;
        
        if (x >= 0 && x < CHUNK_WIDTH && z >= 0 && z < CHUNK_WIDTH &&
            y >= settings.minHeight && y <= settings.maxHeight) {
            
            BlockState current = chunk->getBlock(x, y, z);
            if (current == stone || current == deepslate) {
                chunk->setBlock(x, y, z, ore);
            }
        }
        
        // Random walk
        startX += dx;
        startY += dy;
        startZ += dz;
        startX = std::clamp(startX, 0, CHUNK_WIDTH - 1);
        startY = std::clamp(startY, settings.minHeight, settings.maxHeight);
        startZ = std::clamp(startZ, 0, CHUNK_WIDTH - 1);
    }
}

void WorldGenerator::generateFeatures(Chunk* chunk, World* world) {
    PROFILE_FUNCTION();
    
    // TODO: Implement structure and feature generation
    // Trees, flowers, grass, structures, etc.
}

void WorldGenerator::generateBiomes(Chunk* chunk) {
    // Set biome data for the chunk
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_WIDTH; z++) {
            BiomeId biome = getBiome(
                chunk->getPosition().x * CHUNK_WIDTH + x,
                64, // Sample at mid-height
                chunk->getPosition().z * CHUNK_WIDTH + z
            );
            
            // Set biome for entire column
            chunk->setBiome(x, 0, z, biome);
        }
    }
}

float WorldGenerator::calculateHeight(int x, int z) const {
    if (settings.flatWorld) {
        return settings.terrainBaseHeight;
    }
    
    float height = settings.terrainBaseHeight;
    
    // Multi-octave noise for terrain height
    float amplitude = 1.0f;
    float frequency = settings.terrainScale;
    float noiseValue = 0.0f;
    float maxAmplitude = 0.0f;
    
    for (int i = 0; i < settings.noiseOctaves; i++) {
        noiseValue += heightNoise->noise2D(x * frequency, z * frequency) * amplitude;
        maxAmplitude += amplitude;
        amplitude *= settings.noisePersistence;
        frequency *= settings.noiseLacunarity;
    }
    
    // Normalize
    noiseValue /= maxAmplitude;
    
    // Apply height variation
    height += noiseValue * settings.terrainHeight;
    
    // Add continentalness effect (makes oceans deeper, mountains higher)
    float continentalness = calculateContinentalness(x, z);
    height += continentalness * 30.0f;
    
    // Add erosion (smooths terrain)
    float erosion = calculateErosion(x, z);
    height -= erosion * 10.0f;
    
    return std::clamp(height, static_cast<float>(settings.minHeight + 5), 
                       static_cast<float>(settings.maxHeight - 5));
}

float WorldGenerator::calculateDensity(int x, int y, int z) const {
    // 3D noise for terrain density (used for overhangs, caves, etc.)
    float density = densityNoise->noise3D(
        x * 0.01f,
        y * 0.01f,
        z * 0.01f
    );
    
    // Add height falloff
    float heightFactor = 1.0f - (y - settings.minHeight) / 
                          static_cast<float>(settings.maxHeight - settings.minHeight);
    
    density += heightFactor * 0.5f;
    
    return density;
}

float WorldGenerator::calculateErosion(int x, int z) const {
    return (erosionNoise->noise2D(x * 0.003f, z * 0.003f) + 1.0f) * 0.5f;
}

float WorldGenerator::calculateContinentalness(int x, int z) const {
    return (continentalnessNoise->noise2D(x * 0.002f, z * 0.002f) + 1.0f) * 0.5f;
}

float WorldGenerator::calculatePeaksValleys(int x, int z) const {
    return peaksValleysNoise->noise2D(x * 0.01f, z * 0.01f);
}

BlockState WorldGenerator::getSurfaceBlock(int x, int y, int z, BiomeId biome) const {
    auto& blockRegistry = BlockRegistry::get();
    
    // Biome-specific surface blocks
    switch (biome) {
        case 2: // Desert
            return blockRegistry.getDefaultState("minecraft:sand");
        case 12: // Snowy plains
        case 13: // Snowy taiga
            return blockRegistry.getDefaultState("minecraft:snow_block");
        case 9: // Swamp
            return blockRegistry.getDefaultState("minecraft:grass_block");
        case 7: // Badlands
            return blockRegistry.getDefaultState("minecraft:red_sand");
        default:
            return blockRegistry.getDefaultState("minecraft:grass_block");
    }
}

BlockState WorldGenerator::getSubsurfaceBlock(int y, BiomeId biome) const {
    auto& blockRegistry = BlockRegistry::get();
    
    // Use deepslate below Y=0
    if (y < 0) {
        return blockRegistry.getDefaultState("minecraft:deepslate");
    }
    
    switch (biome) {
        case 2: // Desert
            return blockRegistry.getDefaultState("minecraft:sandstone");
        case 7: // Badlands
            return blockRegistry.getDefaultState("minecraft:red_sandstone");
        default:
            return blockRegistry.getDefaultState("minecraft:dirt");
    }
}

BlockState WorldGenerator::getUnderwaterBlock(int y, BiomeId biome) const {
    auto& blockRegistry = BlockRegistry::get();
    
    switch (biome) {
        case 9: // Swamp
            return blockRegistry.getDefaultState("minecraft:clay");
        default:
            return blockRegistry.getDefaultState("minecraft:gravel");
    }
}

} // namespace VoxelForge
