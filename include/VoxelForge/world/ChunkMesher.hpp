/**
 * @file ChunkMesher.hpp
 * @brief Chunk mesh generation system
 * 
 * Converts chunk block data into renderable meshes with:
 * - Face culling (don't render hidden faces)
 * - Ambient occlusion
 * - Light data packing
 * - Multiple render layers (solid, cutout, translucent)
 */

#pragma once

#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/world/Block.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <functional>

namespace VoxelForge {

// Forward declarations
class World;
class TextureAtlas;

/**
 * @brief GPU-side chunk mesh buffer
 */
struct ChunkMeshBuffers {
    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexMemory;
    vk::Buffer indexBuffer;
    vk::DeviceMemory indexMemory;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
    
    // Separate buffers for different render types
    vk::Buffer solidVertexBuffer;
    vk::DeviceMemory solidVertexMemory;
    vk::Buffer solidIndexBuffer;
    vk::DeviceMemory solidIndexMemory;
    uint32_t solidIndexCount = 0;
    
    vk::Buffer cutoutVertexBuffer;
    vk::DeviceMemory cutoutVertexMemory;
    vk::Buffer cutoutIndexBuffer;
    vk::DeviceMemory cutoutIndexMemory;
    uint32_t cutoutIndexCount = 0;
    
    vk::Buffer translucentVertexBuffer;
    vk::DeviceMemory translucentVertexMemory;
    vk::Buffer translucentIndexBuffer;
    vk::DeviceMemory translucentIndexMemory;
    uint32_t translucentIndexCount = 0;
};

/**
 * @brief Chunk mesh generation result
 */
struct ChunkMeshResult {
    ChunkPos position;
    ChunkMeshData meshData;
    bool success = false;
    float generationTimeMs = 0.0f;
};

/**
 * @brief Chunk mesher configuration
 */
struct ChunkMesherConfig {
    // Ambient occlusion
    bool enableAO = true;
    int aoSampleRadius = 1;
    
    // Light interpolation
    bool enableLightInterpolation = true;
    
    // Face culling
    bool cullHiddenFaces = true;
    
    // Mesh simplification
    bool enableGreedyMeshing = false;  // Experimental
    
    // Performance
    int maxMeshesPerFrame = 4;
    bool useThreadedGeneration = true;
};

/**
 * @brief Generates renderable meshes from chunk data
 */
class ChunkMesher {
public:
    ChunkMesher();
    ~ChunkMesher();
    
    // Non-copyable
    ChunkMesher(const ChunkMesher&) = delete;
    ChunkMesher& operator=(const ChunkMesher&) = delete;
    
    /**
     * @brief Generate mesh for a chunk
     * @param chunk The chunk to generate mesh for
     * @param world World for neighbor access
     * @return Generated mesh data
     */
    ChunkMeshResult generateMesh(const Chunk* chunk, World* world);
    
    /**
     * @brief Generate mesh for a chunk section
     * @param chunk The chunk containing the section
     * @param sectionY Section Y index (0-23)
     * @param world World for neighbor access
     * @return Generated mesh data
     */
    ChunkMeshResult generateSectionMesh(const Chunk* chunk, int sectionY, World* world);
    
    /**
     * @brief Set configuration
     */
    void setConfig(const ChunkMesherConfig& config) { this->config = config; }
    const ChunkMesherConfig& getConfig() const { return config; }
    
    /**
     * @brief Set texture atlas
     */
    void setTextureAtlas(TextureAtlas* atlas) { textureAtlas = atlas; }
    
    /**
     * @brief Get statistics
     */
    struct Stats {
        uint64_t meshesGenerated = 0;
        uint64_t totalVerticesGenerated = 0;
        uint64_t totalIndicesGenerated = 0;
        double totalTimeMs = 0.0;
        double averageTimeMs = 0.0;
    };
    const Stats& getStats() const { return stats; }
    void resetStats();
    
private:
    // Face generation
    void addBlockFaces(
        ChunkMeshData& mesh,
        const Chunk* chunk,
        int localX, int localY, int localZ,
        BlockState blockState,
        World* world
    );
    
    void addFace(
        ChunkMeshData& mesh,
        BlockState blockState,
        const glm::ivec3& position,
        int face,
        const std::array<uint8_t, 4>& vertexAO,
        const std::array<uint32_t, 4>& vertexLight,
        World* world
    );
    
    // Neighbor checks
    bool shouldRenderFace(
        BlockState currentBlock,
        BlockState neighborBlock,
        int face
    ) const;
    
    BlockState getNeighborBlock(
        const Chunk* chunk,
        int localX, int localY, int localZ,
        int dx, int dy, int dz,
        World* world
    ) const;
    
    // Ambient occlusion
    std::array<uint8_t, 4> calculateVertexAO(
        const Chunk* chunk,
        int x, int y, int z,
        int face,
        World* world
    ) const;
    
    uint8_t calculateAOTerm(
        bool side1, bool side2, bool corner
    ) const;
    
    // Light calculation
    std::array<uint32_t, 4> calculateVertexLight(
        const Chunk* chunk,
        int x, int y, int z,
        int face,
        World* world
    ) const;
    
    uint32_t packLight(uint8_t skyLight, uint8_t blockLight) const;
    
    // Texture coordinates
    glm::vec4 getTextureCoords(BlockState block, int face) const;
    
    // Vertex data
    void addQuad(
        ChunkMeshData& mesh,
        const glm::vec3 positions[4],
        const glm::vec3& normal,
        const glm::vec2 uvs[4],
        const std::array<uint8_t, 4>& ao,
        const std::array<uint32_t, 4>& light,
        uint32_t color,
        BlockRenderType renderType
    );
    
    // Face data (positions, normals, UVs)
    static const glm::vec3 FACE_NORMALS[6];
    static const glm::ivec3 FACE_OFFSETS[6];
    static const int FACE_VERTICES[6][4][3];  // 6 faces, 4 vertices, 3 coords
    static const glm::vec2 FACE_UVS[4];
    
    // Configuration
    ChunkMesherConfig config;
    TextureAtlas* textureAtlas = nullptr;
    
    // Statistics
    Stats stats;
};

/**
 * @brief Manages GPU buffers for chunk meshes
 */
class ChunkMeshManager {
public:
    ChunkMeshManager();
    ~ChunkMeshManager();
    
    /**
     * @brief Initialize with Vulkan context
     */
    void init(vk::Device device, vk::PhysicalDevice physicalDevice, 
              vk::Queue queue, vk::CommandPool commandPool);
    
    /**
     * @brief Upload mesh data to GPU
     */
    bool uploadMesh(const ChunkPos& pos, const ChunkMeshData& meshData);
    
    /**
     * @brief Get mesh buffers for a chunk
     */
    const ChunkMeshBuffers* getMesh(const ChunkPos& pos) const;
    
    /**
     * @brief Remove mesh for a chunk
     */
    void removeMesh(const ChunkPos& pos);
    
    /**
     * @brief Clear all meshes
     */
    void clear();
    
    /**
     * @brief Get total memory usage
     */
    size_t getTotalMemoryUsage() const { return totalMemoryUsage; }
    
    /**
     * @brief Get mesh count
     */
    size_t getMeshCount() const { return meshes.size(); }
    
private:
    void destroyBuffers(ChunkMeshBuffers& buffers);
    
    vk::Device device;
    vk::PhysicalDevice physicalDevice;
    vk::Queue queue;
    vk::CommandPool commandPool;
    
    std::unordered_map<ChunkPos, ChunkMeshBuffers, std::hash<ChunkPos>> meshes;
    size_t totalMemoryUsage = 0;
};

} // namespace VoxelForge
