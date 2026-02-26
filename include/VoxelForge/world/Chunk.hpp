/**
 * @file Chunk.hpp
 * @brief Chunk system for VoxelForge
 * 
 * Implements the chunk-based world storage system with:
 * - Paletted block storage for memory efficiency
 * - Light data storage
 * - Biome data
 * - Height maps
 */

#pragma once

#include "Block.hpp"
#include <array>
#include <memory>
#include <atomic>
#include <mutex>

namespace VoxelForge {

// ============================================
// Constants
// ============================================

constexpr int CHUNK_WIDTH = 16;
constexpr int CHUNK_HEIGHT = 384;
constexpr int CHUNK_MIN_Y = -64;
constexpr int SECTION_HEIGHT = 16;
constexpr int SECTIONS_PER_CHUNK = CHUNK_HEIGHT / SECTION_HEIGHT;
constexpr int BLOCKS_PER_SECTION = CHUNK_WIDTH * SECTION_HEIGHT * CHUNK_WIDTH;
constexpr int BLOCKS_PER_CHUNK = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH;

// ============================================
// Chunk Position
// ============================================

struct ChunkPos {
    int x, z;
    
    ChunkPos() : x(0), z(0) {}
    ChunkPos(int x, int z) : x(x), z(z) {}
    
    bool operator==(const ChunkPos& other) const { return x == other.x && z == other.z; }
    bool operator!=(const ChunkPos& other) const { return !(*this == other); }
    
    BlockPos toBlockPos(int localX, int y, int localZ) const {
        return BlockPos(x * CHUNK_WIDTH + localX, y, z * CHUNK_WIDTH + localZ);
    }
    
    static ChunkPos fromBlockPos(const BlockPos& pos) {
        return ChunkPos(
            pos.x < 0 ? (pos.x - CHUNK_WIDTH + 1) / CHUNK_WIDTH : pos.x / CHUNK_WIDTH,
            pos.z < 0 ? (pos.z - CHUNK_WIDTH + 1) / CHUNK_WIDTH : pos.z / CHUNK_WIDTH
        );
    }
    
    i64 toHash() const {
        return (static_cast<i64>(x) & 0xFFFFFFFFLL) | (static_cast<i64>(z) << 32);
    }
    
    static ChunkPos fromHash(i64 hash) {
        return ChunkPos(static_cast<int>(hash & 0xFFFFFFFFLL), 
                        static_cast<int>((hash >> 32) & 0xFFFFFFFFLL));
    }
    
    int getLocalX(int blockX) const { return ((blockX % CHUNK_WIDTH) + CHUNK_WIDTH) % CHUNK_WIDTH; }
    int getLocalZ(int blockZ) const { return ((blockZ % CHUNK_WIDTH) + CHUNK_WIDTH) % CHUNK_WIDTH; }
};

// ============================================
// Section Position
// ============================================

struct SectionPos {
    int x, y, z;
    
    SectionPos() : x(0), y(0), z(0) {}
    SectionPos(int x, int y, int z) : x(x), y(y), z(z) {}
    SectionPos(const BlockPos& pos) 
        : x(pos.x >> 4), y((pos.y - CHUNK_MIN_Y) >> 4), z(pos.z >> 4) {}
    
    bool operator==(const SectionPos& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
    
    BlockPos toBlockPos(int localX, int localY, int localZ) const {
        return BlockPos(x * CHUNK_WIDTH + localX, 
                        y * SECTION_HEIGHT + CHUNK_MIN_Y + localY, 
                        z * CHUNK_WIDTH + localZ);
    }
    
    ChunkPos toChunkPos() const { return ChunkPos(x, z); }
    int getSectionY() const { return y; }
};

// ============================================
// Light Data
// ============================================

struct LightData {
    // 4 bits for sky light, 4 bits for block light per position
    // Total: 2048 bytes per section (BLOCKS_PER_SECTION / 2 * 2 bytes)
    std::array<uint8_t, BLOCKS_PER_SECTION> skyLight{};
    std::array<uint8_t, BLOCKS_PER_SECTION> blockLight{};
    
    uint8_t getSkyLight(int x, int y, int z) const;
    uint8_t getBlockLight(int x, int y, int z) const;
    void setSkyLight(int x, int y, int z, uint8_t value);
    void setBlockLight(int x, int y, int z, uint8_t value);
    
private:
    static int getIndex(int x, int y, int z) {
        return y << 8 | z << 4 | x;
    }
};

// ============================================
// Block Palette
// ============================================

class BlockPalette {
public:
    BlockPalette();
    
    // Get block state at position
    BlockState get(int x, int y, int z) const;
    
    // Set block state at position, returns true if palette changed
    bool set(int x, int y, int z, BlockState state);
    
    // Palette info
    int getBitsPerBlock() const { return bitsPerBlock; }
    size_t getPaletteSize() const { return palette.size(); }
    size_t getDataSize() const { return data.size(); }
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    static BlockPalette deserialize(const uint8_t* data, size_t size);
    
private:
    void resize(int newBitsPerBlock);
    int findInPalette(BlockState state) const;
    int addToPalette(BlockState state);
    
    int bitsPerBlock;
    std::vector<BlockState> palette;  // Maps palette index -> BlockState
    std::vector<uint64_t> data;       // Packed block data
    
    static constexpr int MIN_BITS = 2;
    static constexpr int MAX_BITS = 16;
    static constexpr int SINGLE_VALUE_BITS = 0;
    
    // For single-value optimization
    bool isSingleValue = true;
    BlockState singleValue = BlockState();
};

// ============================================
// Chunk Section (16x16x16)
// ============================================

class ChunkSection {
public:
    ChunkSection();
    ~ChunkSection() = default;
    
    // Block access
    BlockState getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockState state);
    
    // Light access
    uint8_t getSkyLight(int x, int y, int z) const;
    uint8_t getBlockLight(int x, int y, int z) const;
    void setSkyLight(int x, int y, int z, uint8_t value);
    void setBlockLight(int x, int y, int z, uint8_t value);
    
    // Block entity support
    bool hasBlockEntity(int x, int y, int z) const;
    void setBlockEntity(int x, int y, int z, std::unique_ptr<class BlockEntity> entity);
    BlockEntity* getBlockEntity(int x, int y, int z) const;
    void removeBlockEntity(int x, int y, int z);
    
    // Statistics
    int getNonAirBlockCount() const { return nonAirBlocks; }
    bool isEmpty() const { return nonAirBlocks == 0; }
    
    // State tracking
    bool isDirty() const { return dirty; }
    void markDirty() { dirty = true; }
    void markClean() { dirty = false; }
    
    // Mesh state
    bool needsMeshRebuild() const { return meshDirty; }
    void markMeshDirty() { meshDirty = true; }
    void markMeshClean() { meshDirty = false; }
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    static std::unique_ptr<ChunkSection> deserialize(const uint8_t* data, size_t size);
    
private:
    BlockPalette blocks;
    LightData lighting;
    std::unordered_map<uint16_t, std::unique_ptr<BlockEntity>> blockEntities;
    
    std::atomic<int> nonAirBlocks{0};
    std::atomic<bool> dirty{false};
    std::atomic<bool> meshDirty{true};
    
    static uint16_t getBlockEntityIndex(int x, int y, int z) {
        return static_cast<uint16_t>((y << 8) | (z << 4) | x);
    }
};

// ============================================
// Height Map
// ============================================

class HeightMap {
public:
    static constexpr int HEIGHTMAP_SIZE = CHUNK_WIDTH * CHUNK_WIDTH;
    
    enum class Type {
        WorldSurface,       // Highest non-air block
        MotionBlocking,     // Highest solid block
        OceanFloor,         // Highest solid block below water
        MotionBlockingNoLeaves
    };
    
    int getHeight(int x, int z) const;
    void setHeight(int x, int z, int height);
    void update(const ChunkSection* section, int sectionY, Type type);
    
    const std::array<int16_t, HEIGHTMAP_SIZE>& getData() const { return data; }
    
private:
    std::array<int16_t, HEIGHTMAP_SIZE> data{};
    
    static int getIndex(int x, int z) { return z * CHUNK_WIDTH + x; }
};

// ============================================
// Biome Storage
// ============================================

class BiomeStorage {
public:
    using BiomeID = uint8_t;
    
    BiomeStorage();
    
    BiomeID get(int x, int y, int z) const;
    void set(int x, int y, int z, BiomeID biome);
    
    // For 2D biomes (same biome per column)
    BiomeID getColumn(int x, int z) const;
    void setColumn(int x, int z, BiomeID biome);
    
private:
    // 4x4x4 sections, each with a biome ID
    // 16x16x16 blocks / 4 = 4x4x4 = 64 entries
    std::array<BiomeID, 64> data{};
    
    static int getIndex(int x, int y, int z) {
        int sx = x / 4;
        int sy = y / 4;
        int sz = z / 4;
        return sy * 16 + sz * 4 + sx;
    }
};

// ============================================
// Chunk (Column)
// ============================================

class Chunk {
public:
    enum class Status {
        Empty,
        StructureStarts,
        StructureReferences,
        Biomes,
        Noise,
        Surface,
        Carvers,
        Features,
        Light,
        Spawn,
        Full,
        
        // Client-side statuses
        Queued,
        Loading,
        Loaded,
        Unloading
    };
    
    Chunk(const ChunkPos& pos);
    ~Chunk();
    
    // Position
    const ChunkPos& getPosition() const { return position; }
    
    // Block access (absolute Y from -64 to 319)
    BlockState getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockState state);
    
    // Section access
    ChunkSection* getSection(int sectionY);
    const ChunkSection* getSection(int sectionY) const;
    bool hasSection(int sectionY) const;
    
    // Light access
    uint8_t getSkyLight(int x, int y, int z) const;
    uint8_t getBlockLight(int x, int y, int z) const;
    void setSkyLight(int x, int y, int z, uint8_t value);
    void setBlockLight(int x, int y, int z, uint8_t value);
    
    // Block entities
    BlockEntity* getBlockEntity(const BlockPos& pos);
    void setBlockEntity(const BlockPos& pos, std::unique_ptr<BlockEntity> entity);
    void removeBlockEntity(const BlockPos& pos);
    const std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>>& getBlockEntities() const;
    
    // Height maps
    int getHeight(HeightMap::Type type, int x, int z) const;
    void recalculateHeightMaps();
    
    // Biomes
    BiomeStorage::BiomeID getBiome(int x, int y, int z) const;
    void setBiome(int x, int y, int z, BiomeStorage::BiomeID biome);
    
    // Status
    Status getStatus() const { return status; }
    void setStatus(Status s) { status = s; }
    
    // Dirty tracking
    bool isDirty() const;
    void markDirty();
    void markClean();
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    static std::unique_ptr<Chunk> deserialize(const uint8_t* data, size_t size, const ChunkPos& pos);
    
    // World reference (set when chunk is added to world)
    void setWorld(World* world) { worldPtr = world; }
    World* getWorld() const { return worldPtr; }
    
    // Inhabited time (for regional difficulty)
    void setInhabitedTime(i64 time) { inhabitedTime = time; }
    i64 getInhabitedTime() const { return inhabitedTime; }
    
private:
    ChunkPos position;
    std::array<std::unique_ptr<ChunkSection>, SECTIONS_PER_CHUNK> sections;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> blockEntities;
    
    HeightMap worldSurfaceHeightMap;
    HeightMap motionBlockingHeightMap;
    
    BiomeStorage biomes;
    
    Status status = Status::Empty;
    std::atomic<bool> dirty{false};
    
    World* worldPtr = nullptr;
    i64 inhabitedTime = 0;
    
    int getSectionIndex(int y) const {
        return (y - CHUNK_MIN_Y) / SECTION_HEIGHT;
    }
    
    bool isValidY(int y) const {
        return y >= CHUNK_MIN_Y && y < CHUNK_MIN_Y + CHUNK_HEIGHT;
    }
    
    bool isValidXZ(int x, int z) const {
        return x >= 0 && x < CHUNK_WIDTH && z >= 0 && z < CHUNK_WIDTH;
    }
};

// ============================================
// Chunk Mesh
// ============================================

struct ChunkVertex {
    float x, y, z;        // Position
    float nx, ny, nz;     // Normal
    float u, v;           // UV
    float ao;             // Ambient occlusion
    uint32_t light;       // Packed light data
    uint32_t color;       // Vertex color
};

struct ChunkMeshData {
    std::vector<ChunkVertex> vertices;
    std::vector<uint32_t> indices;
    
    // Per-render-type buffers
    std::vector<ChunkVertex> solidVertices;
    std::vector<uint32_t> solidIndices;
    
    std::vector<ChunkVertex> cutoutVertices;
    std::vector<uint32_t> cutoutIndices;
    
    std::vector<ChunkVertex> translucentVertices;
    std::vector<uint32_t> translucentIndices;
    
    size_t getTotalVertexCount() const {
        return solidVertices.size() + cutoutVertices.size() + translucentVertices.size();
    }
    
    size_t getTotalIndexCount() const {
        return solidIndices.size() + cutoutIndices.size() + translucentIndices.size();
    }
    
    bool isEmpty() const {
        return getTotalVertexCount() == 0;
    }
    
    void clear() {
        solidVertices.clear();
        solidIndices.clear();
        cutoutVertices.clear();
        cutoutIndices.clear();
        translucentVertices.clear();
        translucentIndices.clear();
    }
};

} // namespace VoxelForge

// Hash function for ChunkPos
namespace std {
    template<>
    struct hash<VoxelForge::ChunkPos> {
        size_t operator()(const VoxelForge::ChunkPos& pos) const {
            return hash<int64_t>()(pos.toHash());
        }
    };
    
    template<>
    struct hash<VoxelForge::SectionPos> {
        size_t operator()(const VoxelForge::SectionPos& pos) const {
            size_t h1 = hash<int>()(pos.x);
            size_t h2 = hash<int>()(pos.y);
            size_t h3 = hash<int>()(pos.z);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}
