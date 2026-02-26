/**
 * @file ShadowRenderer.hpp
 * @brief Shadow mapping system
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <VoxelForge/rendering/VulkanBuffer.hpp>
#include <VoxelForge/rendering/VulkanImage.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <array>

namespace VoxelForge {

class VulkanDevice;
class Camera;
class World;
class ChunkRenderer;

// Shadow cascade
struct ShadowCascade {
    glm::mat4 viewProj;
    glm::mat4 view;
    glm::mat4 proj;
    float splitDepth;
    float nearPlane;
    float farPlane;
    float boundingRadius;
};

// Shadow settings
struct ShadowSettings {
    bool enabled = true;
    int resolution = 2048;
    int cascadeCount = 4;
    float maxDistance = 200.0f;
    float splitLambda = 0.5f;       // Cascade split factor
    float depthBias = 0.0005f;
    float slopeBias = 2.0f;
    bool pcfEnabled = true;
    int pcfSamples = 16;
    float pcfRadius = 2.0f;
    bool vsmEnabled = false;        // Variance shadow mapping
    bool softShadows = true;
    float softness = 1.0f;
    bool cascadeBlending = true;
    float blendDistance = 10.0f;
};

// Shadow uniform data
struct ShadowUniformData {
    alignas(16) std::array<glm::mat4, 4> cascadeViewProj;
    alignas(16) glm::vec3 lightDirection;
    alignas(4) float shadowDistance;
    alignas(16) std::array<float, 4> cascadeSplits;
    alignas(16) std::array<glm::vec4, 4> cascadeBounds;
    alignas(4) int cascadeCount;
    alignas(4) float depthBias;
    alignas(4) float slopeBias;
    alignas(4) int pcfSamples;
    alignas(4) float pcfRadius;
    alignas(4) int softShadows;
    alignas(4) float softness;
};

class ShadowRenderer {
public:
    ShadowRenderer();
    ~ShadowRenderer();
    
    // No copy
    ShadowRenderer(const ShadowRenderer&) = delete;
    ShadowRenderer& operator=(const ShadowRenderer&) = delete;
    
    void init(VulkanDevice* device);
    void cleanup();
    
    // Frame management
    void beginFrame(Camera* camera, const glm::vec3& lightDir);
    void renderShadowPass(vk::CommandBuffer cmd, World* world, ChunkRenderer* chunkRenderer);
    void endFrame();
    
    // Shadow map access
    vk::ImageView getShadowMapView() const;
    vk::Sampler getShadowSampler() const;
    const ShadowUniformData& getUniformData() const { return uniformData; }
    
    // Cascades
    const std::vector<ShadowCascade>& getCascades() const { return cascades; }
    int getCascadeCount() const { return settings.cascadeCount; }
    
    // Settings
    ShadowSettings& getSettings() { return settings; }
    const ShadowSettings& getSettings() const { return settings; }
    void setResolution(int resolution);
    void setMaxDistance(float distance);
    void setCascadeCount(int count);
    
    // Debug
    void renderDebugCascadeOverlays(vk::CommandBuffer cmd);
    bool isDebugEnabled() const { return debugEnabled; }
    void setDebugEnabled(bool enabled) { debugEnabled = enabled; }
    
private:
    void createPipeline();
    void createRenderPass();
    void createFramebuffer();
    void createShadowMap();
    void createUniformBuffers();
    void updateCascades(Camera* camera, const glm::vec3& lightDir);
    void updateUniformBuffer();
    
    VulkanDevice* device = nullptr;
    
    // Render pass
    vk::RenderPass shadowRenderPass;
    vk::Framebuffer shadowFramebuffer;
    
    // Pipeline
    vk::Pipeline shadowPipeline;
    vk::PipelineLayout shadowPipelineLayout;
    
    // Shadow map
    Image shadowMapImage;
    vk::ImageView shadowMapView;
    vk::Sampler shadowSampler;
    vk::Format shadowFormat = vk::Format::eD32Sfloat;
    
    // Cascades
    std::vector<ShadowCascade> cascades;
    
    // Uniform buffers
    std::vector<Buffer> uniformBuffers;
    ShadowUniformData uniformData;
    
    // Settings
    ShadowSettings settings;
    glm::vec3 lightDirection;
    
    // Debug
    bool debugEnabled = false;
    vk::Pipeline debugPipeline;
    vk::PipelineLayout debugPipelineLayout;
    
    Camera* currentCamera = nullptr;
    uint32_t currentFrame = 0;
    uint32_t frameCount = 2;
};

} // namespace VoxelForge
