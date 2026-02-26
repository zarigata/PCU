/**
 * @file ChunkRenderer.hpp
 * @brief Chunk rendering system for voxel world
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <VoxelForge/rendering/VulkanBuffer.hpp>
#include <VoxelForge/rendering/VulkanPipeline.hpp>
#include <VoxelForge/rendering/VulkanDescriptor.hpp>
#include <VoxelForge/rendering/Camera.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <unordered_map>

namespace VoxelForge {

class VulkanDevice;
class VulkanCommandBuffer;
class World;
class Chunk;
class ChunkMesh;

// Vertex format for chunk meshes
struct ChunkVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    uint32_t texIndex;  // Texture array index
    uint32_t color;     // Packed RGBA
    uint32_t ao;        // Ambient occlusion (4 values packed)
    
    static vk::VertexInputBindingDescription getBindingDescription() {
        return {0, sizeof(ChunkVertex), vk::VertexInputRate::eVertex};
    }
    
    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() {
        return {
            {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(ChunkVertex, position)},
            {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(ChunkVertex, normal)},
            {2, 0, vk::Format::eR32G32Sfloat, offsetof(ChunkVertex, texCoord)},
            {3, 0, vk::Format::eR32Uint, offsetof(ChunkVertex, texIndex)},
            {4, 0, vk::Format::eR32Uint, offsetof(ChunkVertex, color)},
            {5, 0, vk::Format::eR32Uint, offsetof(ChunkVertex, ao)}
        };
    }
};

// GPU data for a chunk mesh
struct ChunkMeshGPU {
    Buffer vertexBuffer;
    Buffer indexBuffer;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
    glm::ivec3 chunkPos;
    bool valid = false;
    
    // Bounding sphere for frustum culling
    glm::vec3 center;
    float radius;
};

// Uniform buffer for chunk rendering
struct ChunkUniformData {
    alignas(16) glm::mat4 viewProj;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
    alignas(16) glm::vec3 cameraPos;
    alignas(4) float time;
    alignas(4) float fogStart;
    alignas(4) float fogEnd;
    alignas(16) glm::vec4 fogColor;
    alignas(4) int renderDistance;
};

// Chunk renderer settings
struct ChunkRenderSettings {
    bool enableFrustumCulling = true;
    bool enableOcclusionCulling = true;
    bool enableAO = true;
    int maxChunksPerFrame = 4;  // Max chunk mesh uploads per frame
    int renderDistance = 8;
};

// Statistics for chunk rendering
struct ChunkRenderStats {
    uint32_t chunksRendered = 0;
    uint32_t chunksCulled = 0;
    uint32_t chunksUploaded = 0;
    uint32_t drawCalls = 0;
    uint32_t verticesRendered = 0;
    uint32_t trianglesRendered = 0;
};

class ChunkRenderer {
public:
    ChunkRenderer();
    ~ChunkRenderer();
    
    // No copy
    ChunkRenderer(const ChunkRenderer&) = delete;
    ChunkRenderer& operator=(const ChunkRenderer&) = delete;
    
    void init(VulkanDevice* device);
    void cleanup();
    
    // Frame management
    void beginFrame(vk::CommandBuffer cmd, Camera* camera);
    void renderChunks(vk::CommandBuffer cmd, World* world);
    void endFrame();
    
    // Mesh management
    void uploadChunkMesh(ChunkMesh* mesh, const glm::ivec3& chunkPos);
    void removeChunkMesh(const glm::ivec3& chunkPos);
    void updateChunkMesh(ChunkMesh* mesh, const glm::ivec3& chunkPos);
    
    // Settings
    ChunkRenderSettings& getSettings() { return settings; }
    const ChunkRenderSettings& getSettings() const { return settings; }
    
    // Stats
    const ChunkRenderStats& getStats() const { return stats; }
    void resetStats();
    
    // Resize handler
    void onResize(uint32_t width, uint32_t height);
    
private:
    void createPipeline();
    void createDescriptorSets();
    void createUniformBuffers();
    void updateUniformBuffer(Camera* camera);
    
    bool isChunkVisible(const ChunkMeshGPU& mesh, const Camera* camera) const;
    void uploadPendingMeshes();
    
    VulkanDevice* device = nullptr;
    
    // Pipeline
    vk::Pipeline pipeline;
    vk::PipelineLayout pipelineLayout;
    std::unique_ptr<VulkanDescriptorSetLayout> descriptorLayout;
    std::vector<vk::DescriptorSet> descriptorSets;
    
    // Uniform buffers
    std::vector<Buffer> uniformBuffers;
    void* uniformBufferMapped = nullptr;
    
    // Chunk meshes
    std::unordered_map<glm::ivec3, ChunkMeshGPU, glm::ivec3Hash> chunkMeshes;
    std::vector<std::pair<ChunkMesh*, glm::ivec3>> pendingUploads;
    
    // Settings and stats
    ChunkRenderSettings settings;
    ChunkRenderStats stats;
    
    // Frame data
    uint32_t currentFrame = 0;
    uint32_t frameCount = 2;
    vk::Extent2D extent;
    
    // Staging buffer for uploads
    std::unique_ptr<VulkanRingBuffer> stagingBuffer;
};

// Hash function for glm::ivec3
namespace glm {
    struct ivec3Hash {
        size_t operator()(const glm::ivec3& v) const {
            return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1) ^ (std::hash<int>()(v.z) << 2);
        }
    };
}

} // namespace VoxelForge
