/**
 * @file EntityRenderer.hpp
 * @brief Entity rendering system
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <VoxelForge/rendering/VulkanBuffer.hpp>
#include <VoxelForge/rendering/Camera.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace VoxelForge {

class VulkanDevice;
class EntityManager;
class Entity;

// Vertex format for entity meshes
struct EntityVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec4 color;
    uint32_t boneIds[4];  // For skeletal animation
    float boneWeights[4];
    
    static vk::VertexInputBindingDescription getBindingDescription() {
        return {0, sizeof(EntityVertex), vk::VertexInputRate::eVertex};
    }
    
    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() {
        return {
            {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(EntityVertex, position)},
            {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(EntityVertex, normal)},
            {2, 0, vk::Format::eR32G32Sfloat, offsetof(EntityVertex, texCoord)},
            {3, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(EntityVertex, color)},
            {4, 0, vk::Format::eR32G32B32A32Uint, offsetof(EntityVertex, boneIds)},
            {5, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(EntityVertex, boneWeights)}
        };
    }
};

// Instance data for instanced rendering
struct EntityInstanceData {
    glm::mat4 model;
    glm::vec4 color;
    uint32_t texIndex;
    uint32_t entityId;
    glm::vec2 _padding;
};

// Uniform buffer for entity rendering
struct EntityUniformData {
    alignas(16) glm::mat4 viewProj;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
    alignas(16) glm::vec3 cameraPos;
    alignas(4) float time;
    alignas(16) glm::vec3 lightDir;
    alignas(4) float lightIntensity;
    alignas(4) uint32_t enableShadows;
};

// Entity model (shared mesh data)
struct EntityModel {
    Buffer vertexBuffer;
    Buffer indexBuffer;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
    std::string name;
    
    // Animation data
    struct Bone {
        std::string name;
        glm::mat4 offset;
        int parentIndex;
    };
    std::vector<Bone> bones;
    std::vector<glm::mat4> boneTransforms;
};

// Entity render stats
struct EntityRenderStats {
    uint32_t entitiesRendered = 0;
    uint32_t drawCalls = 0;
    uint32_t verticesRendered = 0;
    uint32_t trianglesRendered = 0;
};

class EntityRenderer {
public:
    EntityRenderer();
    ~EntityRenderer();
    
    // No copy
    EntityRenderer(const EntityRenderer&) = delete;
    EntityRenderer& operator=(const EntityRenderer&) = delete;
    
    void init(VulkanDevice* device);
    void cleanup();
    
    // Frame management
    void beginFrame(vk::CommandBuffer cmd, Camera* camera);
    void renderEntities(vk::CommandBuffer cmd, EntityManager* entityManager);
    void endFrame();
    
    // Model management
    void loadModel(const std::string& name, const std::string& path);
    void unloadModel(const std::string& name);
    EntityModel* getModel(const std::string& name);
    
    // Stats
    const EntityRenderStats& getStats() const { return stats; }
    void resetStats();
    
    // Resize handler
    void onResize(uint32_t width, uint32_t height);
    
private:
    void createPipeline();
    void createDescriptorSets();
    void createUniformBuffers();
    void updateUniformBuffer(Camera* camera);
    
    void renderEntity(vk::CommandBuffer cmd, Entity* entity);
    void renderInstanced(vk::CommandBuffer cmd, const std::vector<Entity*>& entities);
    
    VulkanDevice* device = nullptr;
    
    // Pipeline
    vk::Pipeline pipeline;
    vk::PipelineLayout pipelineLayout;
    vk::DescriptorSetLayout descriptorLayout;
    std::vector<vk::DescriptorSet> descriptorSets;
    
    // Uniform buffers
    std::vector<Buffer> uniformBuffers;
    void* uniformBufferMapped = nullptr;
    
    // Instance buffer for instanced rendering
    std::vector<Buffer> instanceBuffers;
    uint32_t maxInstances = 1000;
    
    // Loaded models
    std::unordered_map<std::string, std::unique_ptr<EntityModel>> models;
    
    // Stats
    EntityRenderStats stats;
    
    // Frame data
    uint32_t currentFrame = 0;
    uint32_t frameCount = 2;
    vk::Extent2D extent;
};

} // namespace VoxelForge
