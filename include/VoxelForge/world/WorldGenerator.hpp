/**
 * @file WorldGenerator.hpp
 * @brief World terrain generation system
 * 
 * Implements procedural terrain generation with:
 * - Multi-octave noise for height variation
 * - Biome-based terrain features
 * - Cave generation
 * - Ore distribution
 * - Structure placement
 */

#pragma once

#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/utils/Noise.hpp>
#include <VoxelForge/utils/Random.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <functional>

namespace VoxelForge {

// Forward declarations
class Biome;
class Structure;

/**
 * @brief World generation settings
 */
struct WorldGenSettings {
    // Seed
    uint64_t seed = 0;
    
    // Terrain settings
    int seaLevel = 62;
    int bedrockFloor = -64;
    int minHeight = -64;
    int maxHeight = 320;
    
    // Noise settings
    float terrainScale = 0.01f;
    float terrainHeight = 64.0f;
    float terrainBaseHeight = 64.0f;
    
    int noiseOctaves = 8;
    float noisePersistence = 0.5f;
    float noiseLacunarity = 2.0f;
    
    // Cave settings
    bool generateCaves = true;
    float caveFrequency = 0.02f;
    float caveThreshold = 0.6f;
    
    // Ore settings
    bool generateOres = true;
    
    // Structure settings
    bool generateStructures = true;
    
    // Biome settings
    float biomeScale = 0.005f;
    
    // Debug
    bool flatWorld = false;
    bool debugWorld = false;
};

/**
 * @brief Result of chunk generation
 */
struct ChunkGenResult {
    ChunkPos position;
    bool success = false;
    float generationTimeMs = 0.0f;
    int blocksGenerated = 0;
    int featuresGenerated = 0;
};

/**
 * @brief Feature placement context
 */
struct FeaturePlacementContext {
    World* world;
    Chunk* chunk;
    const Biome* biome;
    BlockPos position;
    SeededRandom random;
};

/**
 * @brief Main world generator class
 */
class WorldGenerator {
public:
    WorldGenerator(uint64_t seed);
    WorldGenerator(const WorldGenSettings& settings);
    ~WorldGenerator();
    
    // Non-copyable
    WorldGenerator(const WorldGenerator&) = delete;
    WorldGenerator& operator=(const WorldGenerator&) = delete;
    
    /**
     * @brief Generate a chunk
     * @param chunk The chunk to populate
     * @param world The world context
     * @return Generation result
     */
    ChunkGenResult generateChunk(Chunk* chunk, World* world);
    
    /**
     * @brief Generate chunk asynchronously
     */
    std::future<ChunkGenResult> generateChunkAsync(Chunk* chunk, World* world);
    
    /**
     * @brief Get settings
     */
    const WorldGenSettings& getSettings() const { return settings; }
    WorldGenSettings& getSettings() { return settings; }
    
    /**
     * @brief Get seed
     */
    uint64_t getSeed() const { return settings.seed; }
    
    /**
     * @brief Get height at position (for pre-generation queries)
     */
    int getHeight(int x, int z) const;
    
    /**
     * @brief Get biome at position
     */
    BiomeId getBiome(int x, int y, int z) const;
    
    /**
     * @brief Register a custom feature
     */
    using FeatureGenerator = std::function<void(FeaturePlacementContext&)>;
    void registerFeature(const std::string& id, FeatureGenerator generator);
    
    /**
     * @brief Statistics
     */
    struct Stats {
        uint64_t chunksGenerated = 0;
        uint64_t blocksPlaced = 0;
        uint64_t featuresPlaced = 0;
        double totalTimeMs = 0.0;
        double averageTimeMs = 0.0;
    };
    const Stats& getStats() const { return stats; }
    void resetStats();
    
private:
    // Generation phases
    void generateTerrain(Chunk* chunk);
    void generateBedrock(Chunk* chunk);
    void generateCaves(Chunk* chunk);
    void generateOres(Chunk* chunk, SeededRandom& random);
    void generateFeatures(Chunk* chunk, World* world);
    void generateBiomes(Chunk* chunk);
    
    // Terrain helpers
    float calculateHeight(int x, int z) const;
    float calculateDensity(int x, int y, int z) const;
    float calculateErosion(int x, int z) const;
    float calculateContinentalness(int x, int z) const;
    float calculatePeaksValleys(int x, int z) const;
    
    // Block selection helpers
    BlockState getSurfaceBlock(int x, int y, int z, BiomeId biome) const;
    BlockState getSubsurfaceBlock(int y, BiomeId biome) const;
    BlockState getUnderwaterBlock(int y, BiomeId biome) const;
    
    // Noise generators
    std::unique_ptr<PerlinNoise> heightNoise;
    std::unique_ptr<PerlinNoise> densityNoise;
    std::unique_ptr<PerlinNoise> caveNoise;
    std::unique_ptr<PerlinNoise> biomeNoise;
    std::unique_ptr<PerlinNoise> erosionNoise;
    std::unique_ptr<PerlinNoise> continentalnessNoise;
    std::unique_ptr<PerlinNoise> peaksValleysNoise;
    std::unique_ptr<SimplexNoise> oreNoise;
    
    // Settings
    WorldGenSettings settings;
    
    // Feature generators
    std::unordered_map<std::string, FeatureGenerator> features;
    
    // Statistics
    Stats stats;
    
    // Utility
    SeededRandom random;
};

/**
 * @brief Biome definition for world generation
 */
class Biome {
public:
    enum class Category {
        None,
        Taiga,
        ExtremeHills,
        Jungle,
        Mesa,
        Plains,
        Savanna,
        Icy,
        TheEnd,
        Beach,
        Forest,
        Ocean,
        Desert,
        River,
        Swamp,
        Mushroom,
        Nether,
        Underground
    };
    
    enum class Precipitation {
        None,
        Rain,
        Snow
    };
    
    Biome(BiomeId id, const std::string& name);
    
    // Properties
    BiomeId getId() const { return id; }
    const std::string& getName() const { return name; }
    
    // Climate
    float getTemperature() const { return temperature; }
    void setTemperature(float temp) { temperature = temp; }
    
    float getDownfall() const { return downfall; }
    void setDownfall(float df) { downfall = df; }
    
    Precipitation getPrecipitation() const { return precipitation; }
    void setPrecipitation(Precipitation p) { precipitation = p; }
    
    // Terrain
    float getBaseHeight() const { return baseHeight; }
    void setBaseHeight(float height) { baseHeight = height; }
    
    float getHeightVariation() const { return heightVariation; }
    void setHeightVariation(float variation) { heightVariation = variation; }
    
    // Blocks
    BlockState getSurfaceBlock() const { return surfaceBlock; }
    void setSurfaceBlock(BlockState block) { surfaceBlock = block; }
    
    BlockState getSubsurfaceBlock() const { return subsurfaceBlock; }
    void setSubsurfaceBlock(BlockState block) { subsurfaceBlock = block; }
    
    BlockState getUnderwaterBlock() const { return underwaterBlock; }
    void setUnderwaterBlock(BlockState block) { underwaterBlock = block; }
    
    // Features
    void addFeature(const std::string& featureId, int weight);
    const std::vector<std::pair<std::string, int>>& getFeatures() const { return features; }
    
    // Category
    Category getCategory() const { return category; }
    void setCategory(Category cat) { category = cat; }
    
private:
    BiomeId id;
    std::string name;
    
    float temperature = 0.5f;
    float downfall = 0.5f;
    Precipitation precipitation = Precipitation::Rain;
    
    float baseHeight = 0.1f;
    float heightVariation = 0.2f;
    
    BlockState surfaceBlock;
    BlockState subsurfaceBlock;
    BlockState underwaterBlock;
    
    std::vector<std::pair<std::string, int>> features;
    
    Category category = Category::None;
};

/**
 * @brief Ore vein definition
 */
struct OreVein {
    std::string oreBlock;
    int minY;
    int maxY;
    int veinSize;
    int veinsPerChunk;
    float discardChance = 0.0f; // Chance to discard based on density
};

/**
 * @brief Standard ore definitions
 */
namespace Ores {
    extern const OreVein COAL_ORE;
    extern const OreVein IRON_ORE;
    extern const OreVein GOLD_ORE;
    extern const OreVein DIAMOND_ORE;
    extern const OreVein REDSTONE_ORE;
    extern const OreVein LAPIS_ORE;
    extern const OreVein COPPER_ORE;
    extern const OreVein EMERALD_ORE;
    extern const OreVein ANCIENT_DEBRIS;
}

} // namespace VoxelForge
