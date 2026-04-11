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
    const OreVein COAL_ORE = {"poorcraftultra:coal_ore", 0, 192, 17, 20, 0.0f};
    const OreVein IRON_ORE = {"poorcraftultra:iron_ore", -64, 320, 9, 10, 0.0f};
    const OreVein IRON_ORE_TUFF = {"poorcraftultra:iron_ore", -64, 72, 9, 10, 0.0f};
    const OreVein GOLD_ORE = {"poorcraftultra:gold_ore", -64, 32, 9, 2, 0.0f};
    const OreVein GOLD_ORE_BADLANDS = {"poorcraftultra:gold_ore", 32, 256, 9, 5, 0.0f};
    const OreVein DIAMOND_ORE = {"poorcraftultra:diamond_ore", -64, 16, 8, 1, 0.5f};
    const OreVein REDSTONE_ORE = {"poorcraftultra:redstone_ore", -64, 16, 8, 4, 0.0f};
    const OreVein LAPIS_ORE = {"poorcraftultra:lapis_ore", -64, 64, 7, 2, 0.0f};
    const OreVein COPPER_ORE = {"poorcraftultra:copper_ore", -16, 112, 10, 6, 0.0f};
    const OreVein EMERALD_ORE = {"poorcraftultra:emerald_ore", -16, 256, 1, 2, 0.0f};
    const OreVein ANCIENT_DEBRIS = {"poorcraftultra:ancient_debris", 8, 119, 2, 1, 0.7f};
}

// ============================================================================
// Biome Implementation
// ============================================================================

Biome::Biome(BiomeId id, const std::string& name)
    : id(id), name(name) {
    // Set default blocks
    surfaceBlock = BlockRegistry::get().getDefaultState("poorcraftultra:grass_block");
    subsurfaceBlock = BlockRegistry::get().getDefaultState("poorcraftultra:dirt");
    underwaterBlock = BlockRegistry::get().getDefaultState("poorcraftultra:gravel");
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
    featureNoise = std::make_unique<PerlinNoise>(seed + 8);
    
    VF_INFO("WorldGenerator created with seed {}", seed);
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
    featureNoise = std::make_unique<PerlinNoise>(settings.seed + 8);
    
    VF_INFO("WorldGenerator created with seed {} and custom settings", settings.seed);
}

WorldGenerator::~WorldGenerator() {
    VF_INFO("WorldGenerator destroyed");
}

void WorldGenerator::resetStats() {
    stats = Stats{};
}

ChunkGenResult WorldGenerator::generateChunk(Chunk* chunk, World* world) {
    VF_PROFILE_FUNCTION();
    
    ChunkGenResult result;
    result.position = chunk->getPosition();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (!chunk) {
        VF_ERROR("Null chunk passed to generateChunk");
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
                    BlockRegistry::get().getDefaultState("poorcraftultra:bedrock"));
                
                // Dirt layers
                for (int y = settings.bedrockFloor + 1; y < waterLevel - 4; y++) {
                    chunk->setBlock(x, y, z,
                        BlockRegistry::get().getDefaultState("poorcraftultra:dirt"));
                }
                
                // Grass on top
                chunk->setBlock(x, waterLevel - 4, z,
                    BlockRegistry::get().getDefaultState("poorcraftultra:grass_block"));
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
    float temp = biomeNoise->noise(x * 0.005f, z * 0.005f);
    float humidity = biomeNoise->noise(x * 0.005f + 1000, z * 0.005f + 1000);
    
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
    VF_PROFILE_FUNCTION();
    
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
                        block = blockRegistry.getDefaultState("poorcraftultra:stone");
                    }
                    
                    chunk->setBlock(x, y, z, block);
                    blocksPlaced++;
                }
            }
            
            // Fill water in low areas
            for (int y = terrainHeight + 1; y <= settings.seaLevel; y++) {
                if (!chunk->getBlock(x, y, z).isSolid()) {
                    chunk->setBlock(x, y, z, 
                        blockRegistry.getDefaultState("poorcraftultra:water"));
                }
            }
        }
    }
    
    stats.blocksPlaced += blocksPlaced;
}

void WorldGenerator::generateBedrock(Chunk* chunk) {
    auto& blockRegistry = BlockRegistry::get();
    auto bedrock = blockRegistry.getDefaultState("poorcraftultra:bedrock");
    
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
    VF_PROFILE_FUNCTION();
    
    ChunkPos chunkPos = chunk->getPosition();
    int worldX = chunkPos.x * CHUNK_WIDTH;
    int worldZ = chunkPos.z * CHUNK_WIDTH;
    
    auto& blockRegistry = BlockRegistry::get();
    auto air = blockRegistry.getDefaultState("poorcraftultra:air");
    auto caveAir = blockRegistry.getDefaultState("poorcraftultra:cave_air");
    
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_WIDTH; z++) {
            int wx = worldX + x;
            int wz = worldZ + z;
            
            for (int y = settings.minHeight + 5; y < settings.maxHeight - 10; y++) {
                // 3D noise for caves
                float noise3D = caveNoise->noise(
                    wx * settings.caveFrequency,
                    y * settings.caveFrequency * 2,
                    wz * settings.caveFrequency
                );
                
                // Carve cave if above threshold
                if (noise3D > settings.caveThreshold) {
                    BlockState current = chunk->getBlock(x, y, z);
                    
                    // Don't carve through bedrock or water
                    if (!current.isAir() && 
                        current.getBlockId() != blockRegistry.getBlockId("poorcraftultra:bedrock") &&
                        current.getBlockId() != blockRegistry.getBlockId("poorcraftultra:water")) {
                        chunk->setBlock(x, y, z, air);
                    }
                }
            }
        }
    }
}

void WorldGenerator::generateOres(Chunk* chunk, SeededRandom& random) {
    VF_PROFILE_FUNCTION();
    
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
    BlockState stoneBlock = blockRegistry.getDefaultState("poorcraftultra:stone");
    BlockState deepslateBlock = blockRegistry.getDefaultState("poorcraftultra:deepslate");
    
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
    BlockState stone = blockRegistry.getDefaultState("poorcraftultra:stone");
    BlockState deepslate = blockRegistry.getDefaultState("poorcraftultra:deepslate");
    
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
    VF_PROFILE_FUNCTION();
    
    SeededRandom chunkRandom(settings.seed ^ 
        (static_cast<uint64_t>(chunk->getPosition().x) << 32) ^
        static_cast<uint64_t>(chunk->getPosition().z) ^ 0xABCD1234ULL);
    
    generateTrees(chunk, world, chunkRandom);
}

void WorldGenerator::generateTrees(Chunk* chunk, World* world, SeededRandom& chunkRandom) {
    auto& blockRegistry = BlockRegistry::get();
    ChunkPos chunkPos = chunk->getPosition();
    int worldX = chunkPos.x * CHUNK_WIDTH;
    int worldZ = chunkPos.z * CHUNK_WIDTH;
    
    auto grassBlock = blockRegistry.getDefaultState("poorcraftultra:grass_block");
    auto dirtBlock = blockRegistry.getDefaultState("poorcraftultra:dirt");
    auto airBlock = blockRegistry.getDefaultState("poorcraftultra:air");
    
    for (int x = 2; x < CHUNK_WIDTH - 2; x++) {
        for (int z = 2; z < CHUNK_WIDTH - 2; z++) {
            int wx = worldX + x;
            int wz = worldZ + z;
            
            // Get biome at this column
            BiomeId biome = getBiome(wx, 64, wz);
            
            // Determine tree type and density based on biome
            std::string treeType;
            int treeCount; // max trees per column attempt
            float chance;  // probability of placing at this column
            
            switch (biome) {
                case 1: // Plains
                    treeType = "oak";
                    treeCount = 1;
                    chance = 0.008f; // sparse: ~1-3 per chunk
                    break;
                case 4: // Forest
                    treeType = chunkRandom.chance(0.5f) ? "oak" : "birch";
                    treeCount = 1;
                    chance = 0.03f; // common: ~4-8 per chunk
                    break;
                case 13: // Snowy taiga -> treat as taiga
                case 3: // Taiga (mapped from getBiome but note getBiome returns 13 for cold/humid)
                    // Check: getBiome can return 3, but looking at the biome code,
                    // it doesn't return 3 directly. Let's use temperature-based check.
                    treeType = "spruce";
                    treeCount = 1;
                    chance = 0.025f; // common: ~4-7 per chunk
                    break;
                case 5: // Dark Oak Forest (Savanna in current getBiome)
                    // getBiome returns: warm/humid -> 6 (Jungle), warm/dry -> 5 (Savanna)
                    // We'll treat biome 5 as savanna with acacia
                    treeType = "acacia";
                    treeCount = 1;
                    chance = 0.008f; // sparse: ~1-3 per chunk
                    break;
                case 9: // Swamp
                    treeType = "oak";
                    treeCount = 1;
                    chance = 0.005f;
                    break;
                default:
                    continue;
            }
            
            if (!chunkRandom.chance(chance)) {
                continue;
            }
            
            // Find surface height
            int surfaceY = chunk->getHeight(HeightMap::Type::WorldSurface, x, z);
            if (surfaceY < settings.minHeight + 1 || surfaceY > settings.maxHeight - 12) {
                continue;
            }
            
            // Check surface block is grass or dirt
            BlockState surface = chunk->getBlock(x, surfaceY, z);
            if (surface != grassBlock && surface != dirtBlock) {
                continue;
            }
            
            // Check block above surface is air
            if (!chunk->getBlock(x, surfaceY + 1, z).isAir()) {
                continue;
            }
            
            placeTree(chunk, world, x, surfaceY + 1, z, treeType, chunkRandom);
        }
    }
}

void WorldGenerator::placeTree(Chunk* chunk, World* world, int x, int y, int z,
                                const std::string& type, SeededRandom& random) {
    if (type == "oak") {
        placeOakTree(chunk, x, y, z, random);
    } else if (type == "birch") {
        placeBirchTree(chunk, x, y, z, random);
    } else if (type == "spruce") {
        placeSpruceTree(chunk, x, y, z, random);
    } else if (type == "dark_oak") {
        placeDarkOakTree(chunk, x, y, z, random);
    } else if (type == "acacia") {
        placeAcaciaTree(chunk, x, y, z, random);
    }
    stats.featuresPlaced++;
}

// Helper lambda to safely set a block within chunk bounds
static void setBlockClamped(Chunk* chunk, int x, int y, int z, BlockState state, int minY, int maxY) {
    if (x >= 0 && x < CHUNK_WIDTH && z >= 0 && z < CHUNK_WIDTH && y >= minY && y <= maxY) {
        if (chunk->getBlock(x, y, z).isAir()) {
            chunk->setBlock(x, y, z, state);
        }
    }
}

void WorldGenerator::placeOakTree(Chunk* chunk, int x, int y, int z, SeededRandom& random) {
    auto& blockRegistry = BlockRegistry::get();
    auto log = blockRegistry.getDefaultState("poorcraftultra:oak_log");
    auto leaves = blockRegistry.getDefaultState("poorcraftultra:oak_leaves");
    
    int trunkHeight = random.nextInt(4, 6);
    
    // Place trunk
    for (int dy = 0; dy < trunkHeight; dy++) {
        setBlockClamped(chunk, x, y + dy, z, log, settings.minHeight, settings.maxHeight);
    }
    
    // Leaf crown - 3x3x3 centered on top of trunk, with corners randomly missing
    int leafBase = y + trunkHeight - 2;
    for (int dy = 0; dy < 3; dy++) {
        int radius = (dy == 2) ? 1 : 2; // top layer is smaller
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dz = -radius; dz <= radius; dz++) {
                // Skip corners randomly
                if (abs(dx) == radius && abs(dz) == radius && random.chance(0.4f)) continue;
                // Don't overwrite trunk
                if (dx == 0 && dz == 0 && dy < 2) continue;
                setBlockClamped(chunk, x + dx, leafBase + dy, z + dz, leaves, settings.minHeight, settings.maxHeight);
            }
        }
    }
    // Top leaf
    setBlockClamped(chunk, x, leafBase + 3, z, leaves, settings.minHeight, settings.maxHeight);
}

void WorldGenerator::placeBirchTree(Chunk* chunk, int x, int y, int z, SeededRandom& random) {
    auto& blockRegistry = BlockRegistry::get();
    auto log = blockRegistry.getDefaultState("poorcraftultra:birch_log");
    auto leaves = blockRegistry.getDefaultState("poorcraftultra:birch_leaves");
    
    int trunkHeight = random.nextInt(5, 7);
    
    // Place trunk
    for (int dy = 0; dy < trunkHeight; dy++) {
        setBlockClamped(chunk, x, y + dy, z, log, settings.minHeight, settings.maxHeight);
    }
    
    // Tall narrow leaf crown - 2 layers
    int leafBase = y + trunkHeight - 3;
    for (int dy = 0; dy < 3; dy++) {
        int radius = (dy == 2) ? 0 : 1;
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dz = -radius; dz <= radius; dz++) {
                if (dx == 0 && dz == 0 && dy < 2) continue; // trunk space
                setBlockClamped(chunk, x + dx, leafBase + dy, z + dz, leaves, settings.minHeight, settings.maxHeight);
            }
        }
    }
    // Extra top leaves for height
    setBlockClamped(chunk, x, leafBase + 3, z, leaves, settings.minHeight, settings.maxHeight);
    setBlockClamped(chunk, x + 1, leafBase + 3, z, leaves, settings.minHeight, settings.maxHeight);
    setBlockClamped(chunk, x, leafBase + 3, z + 1, leaves, settings.minHeight, settings.maxHeight);
}

void WorldGenerator::placeSpruceTree(Chunk* chunk, int x, int y, int z, SeededRandom& random) {
    auto& blockRegistry = BlockRegistry::get();
    auto log = blockRegistry.getDefaultState("poorcraftultra:spruce_log");
    auto leaves = blockRegistry.getDefaultState("poorcraftultra:spruce_leaves");
    
    int trunkHeight = random.nextInt(6, 10);
    
    // Place trunk
    for (int dy = 0; dy < trunkHeight; dy++) {
        setBlockClamped(chunk, x, y + dy, z, log, settings.minHeight, settings.maxHeight);
    }
    
    // Cone-shaped leaves: wider at bottom, narrow at top
    int leafBase = y + trunkHeight - 2;
    int leafLayers = trunkHeight - 3;
    if (leafLayers < 3) leafLayers = 3;
    
    for (int dy = 0; dy < leafLayers; dy++) {
        // Radius decreases from bottom to top
        int radius = (leafLayers - dy + 1) / 2;
        if (radius < 1) radius = 1;
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dz = -radius; dz <= radius; dz++) {
                // Skip corners for rounder shape
                if (abs(dx) == radius && abs(dz) == radius) continue;
                if (dx == 0 && dz == 0) continue; // trunk
                setBlockClamped(chunk, x + dx, leafBase - leafLayers + 1 + dy, z + dz, leaves, settings.minHeight, settings.maxHeight);
            }
        }
    }
    // Top
    setBlockClamped(chunk, x, leafBase + 1, z, leaves, settings.minHeight, settings.maxHeight);
}

void WorldGenerator::placeDarkOakTree(Chunk* chunk, int x, int y, int z, SeededRandom& random) {
    auto& blockRegistry = BlockRegistry::get();
    auto log = blockRegistry.getDefaultState("poorcraftultra:dark_oak_log");
    auto leaves = blockRegistry.getDefaultState("poorcraftultra:dark_oak_leaves");
    
    int trunkHeight = random.nextInt(6, 9);
    
    // 2x2 trunk
    for (int dy = 0; dy < trunkHeight; dy++) {
        setBlockClamped(chunk, x, y + dy, z, log, settings.minHeight, settings.maxHeight);
        setBlockClamped(chunk, x + 1, y + dy, z, log, settings.minHeight, settings.maxHeight);
        setBlockClamped(chunk, x, y + dy, z + 1, log, settings.minHeight, settings.maxHeight);
        setBlockClamped(chunk, x + 1, y + dy, z + 1, log, settings.minHeight, settings.maxHeight);
    }
    
    // Large irregular canopy
    int leafBase = y + trunkHeight - 3;
    for (int dy = 0; dy < 4; dy++) {
        int radius = (dy < 2) ? 3 : 2;
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dz = -radius; dz <= radius; dz++) {
                // Skip some for irregularity
                if (abs(dx) == radius && abs(dz) == radius && random.chance(0.5f)) continue;
                // Don't overwrite trunk area (0,0 to 1,1)
                if (dx >= 0 && dx <= 1 && dz >= 0 && dz <= 1 && dy < 2) continue;
                setBlockClamped(chunk, x + dx, leafBase + dy, z + dz, leaves, settings.minHeight, settings.maxHeight);
            }
        }
    }
}

void WorldGenerator::placeAcaciaTree(Chunk* chunk, int x, int y, int z, SeededRandom& random) {
    auto& blockRegistry = BlockRegistry::get();
    auto log = blockRegistry.getDefaultState("poorcraftultra:acacia_log");
    auto leaves = blockRegistry.getDefaultState("poorcraftultra:acacia_leaves");
    
    // Short trunk (2-3 blocks) then angled branch
    int trunkHeight = random.nextInt(2, 3);
    
    // Place trunk
    for (int dy = 0; dy < trunkHeight; dy++) {
        setBlockClamped(chunk, x, y + dy, z, log, settings.minHeight, settings.maxHeight);
    }
    
    // Angled branch - goes diagonally up 2-3 blocks
    int branchLen = random.nextInt(2, 3);
    int bx = (random.chance(0.5f)) ? 1 : -1;
    int bz = (random.chance(0.5f)) ? 1 : -1;
    int branchX = x;
    int branchY = y + trunkHeight;
    int branchZ = z;
    
    for (int i = 0; i < branchLen; i++) {
        branchX += bx;
        branchY += 1;
        branchZ += bz;
        setBlockClamped(chunk, branchX, branchY, branchZ, log, settings.minHeight, settings.maxHeight);
    }
    
    // Flat canopy at the end of branch
    int canopyY = branchY;
    int canopyX = branchX;
    int canopyZ = branchZ;
    for (int dx = -2; dx <= 2; dx++) {
        for (int dz = -2; dz <= 2; dz++) {
            // Roughly flat, occasional y+1
            int dy = (abs(dx) <= 1 && abs(dz) <= 1 && random.chance(0.3f)) ? 1 : 0;
            // Skip some edges for natural look
            if (abs(dx) == 2 && abs(dz) == 2 && random.chance(0.6f)) continue;
            setBlockClamped(chunk, canopyX + dx, canopyY + dy, canopyZ + dz, leaves, settings.minHeight, settings.maxHeight);
        }
    }
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
        noiseValue += heightNoise->noise(x * frequency, z * frequency) * amplitude;
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
    float density = densityNoise->noise(
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
    return (erosionNoise->noise(x * 0.003f, z * 0.003f) + 1.0f) * 0.5f;
}

float WorldGenerator::calculateContinentalness(int x, int z) const {
    return (continentalnessNoise->noise(x * 0.002f, z * 0.002f) + 1.0f) * 0.5f;
}

float WorldGenerator::calculatePeaksValleys(int x, int z) const {
    return peaksValleysNoise->noise(x * 0.01f, z * 0.01f);
}

BlockState WorldGenerator::getSurfaceBlock(int x, int y, int z, BiomeId biome) const {
    auto& blockRegistry = BlockRegistry::get();
    
    // Biome-specific surface blocks
    switch (biome) {
        case 2: // Desert
            return blockRegistry.getDefaultState("poorcraftultra:sand");
        case 12: // Snowy plains
        case 13: // Snowy taiga
            return blockRegistry.getDefaultState("poorcraftultra:snow_block");
        case 9: // Swamp
            return blockRegistry.getDefaultState("poorcraftultra:grass_block");
        case 7: // Badlands
            return blockRegistry.getDefaultState("poorcraftultra:red_sand");
        default:
            return blockRegistry.getDefaultState("poorcraftultra:grass_block");
    }
}

BlockState WorldGenerator::getSubsurfaceBlock(int y, BiomeId biome) const {
    auto& blockRegistry = BlockRegistry::get();
    
    // Use deepslate below Y=0
    if (y < 0) {
        return blockRegistry.getDefaultState("poorcraftultra:deepslate");
    }
    
    switch (biome) {
        case 2: // Desert
            return blockRegistry.getDefaultState("poorcraftultra:sandstone");
        case 7: // Badlands
            return blockRegistry.getDefaultState("poorcraftultra:red_sandstone");
        default:
            return blockRegistry.getDefaultState("poorcraftultra:dirt");
    }
}

BlockState WorldGenerator::getUnderwaterBlock(int y, BiomeId biome) const {
    auto& blockRegistry = BlockRegistry::get();
    
    switch (biome) {
        case 9: // Swamp
            return blockRegistry.getDefaultState("poorcraftultra:clay");
        default:
            return blockRegistry.getDefaultState("poorcraftultra:gravel");
    }
}

} // namespace VoxelForge
