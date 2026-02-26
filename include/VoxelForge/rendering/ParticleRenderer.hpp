/**
 * @file ParticleRenderer.hpp
 * @brief Particle system renderer
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <VoxelForge/rendering/VulkanBuffer.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <unordered_map>

namespace VoxelForge {

class VulkanDevice;
class Camera;

// Particle vertex
struct ParticleVertex {
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec4 color;
    glm::vec3 velocity;
    float size;
    float lifetime;
    float age;
    uint32_t texIndex;
};

// Particle emitter
struct ParticleEmitter {
    glm::vec3 position;
    glm::vec3 direction;
    float spread;
    float initialSpeed;
    float speedVariance;
    glm::vec4 colorStart;
    glm::vec4 colorEnd;
    float sizeStart;
    float sizeEnd;
    float lifetime;
    float lifetimeVariance;
    int emitRate;
    int maxParticles;
    bool loop;
    bool billboard;
    std::string texture;
    
    // State
    int activeParticles = 0;
    bool isEmitting = true;
};

// Particle instance
struct ParticleInstance {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float size;
    float lifetime;
    float age;
    uint32_t texIndex;
    bool active;
};

// Particle system
struct ParticleSystem {
    uint32_t id;
    ParticleEmitter emitter;
    std::vector<ParticleInstance> particles;
    bool active = true;
};

// Particle render settings
struct ParticleRenderSettings {
    int maxParticles = 10000;
    bool enableBlending = true;
    bool enableDepthTest = true;
    bool enableDepthWrite = false;
    float sortBias = 0.0f;
};

class ParticleRenderer {
public:
    ParticleRenderer();
    ~ParticleRenderer();
    
    // No copy
    ParticleRenderer(const ParticleRenderer&) = delete;
    ParticleRenderer& operator=(const ParticleRenderer&) = delete;
    
    void init(VulkanDevice* device);
    void cleanup();
    
    // Frame management
    void beginFrame(vk::CommandBuffer cmd, Camera* camera);
    void render(vk::CommandBuffer cmd);
    void endFrame();
    
    // Particle system management
    uint32_t createSystem(const ParticleEmitter& emitter);
    void destroySystem(uint32_t systemId);
    void updateSystem(uint32_t systemId, const ParticleEmitter& emitter);
    
    // Emission control
    void emit(uint32_t systemId, int count);
    void emitAt(const glm::vec3& position, const std::string& effectName, int count = 1);
    
    // Predefined effects
    void playEffect(const std::string& name, const glm::vec3& position);
    void stopEffect(uint32_t systemId);
    void stopAllEffects();
    
    // Update
    void update(float deltaTime);
    
    // Settings
    ParticleRenderSettings& getSettings() { return settings; }
    
    // Stats
    int getActiveParticleCount() const;
    int getSystemCount() const;
    
private:
    void createPipeline();
    void createBuffers();
    void updateParticles(float deltaTime);
    void uploadParticles();
    
    VulkanDevice* device = nullptr;
    
    vk::Pipeline pipeline;
    vk::PipelineLayout pipelineLayout;
    
    Buffer vertexBuffer;
    Buffer instanceBuffer;
    int maxParticles;
    
    std::unordered_map<uint32_t, ParticleSystem> systems;
    std::vector<ParticleInstance> allParticles;
    uint32_t nextSystemId = 1;
    
    Camera* currentCamera = nullptr;
    ParticleRenderSettings settings;
};

// Predefined particle effects
namespace ParticleEffects {
    ParticleEmitter explosion(const glm::vec3& position, float power = 1.0f);
    ParticleEmitter smoke(const glm::vec3& position);
    ParticleEmitter fire(const glm::vec3& position);
    ParticleEmitter waterSplash(const glm::vec3& position);
    ParticleEmitter lavaSplash(const glm::vec3& position);
    ParticleEmitter blockBreak(const glm::vec3& position, uint32_t blockType);
    ParticleEmitter blockPlace(const glm::vec3& position, uint32_t blockType);
    ParticleEmitter criticalHit(const glm::vec3& position);
    ParticleEmitter magicCrit(const glm::vec3& position);
    ParticleEmitter spell(const glm::vec3& position, const glm::vec4& color);
    ParticleEmitter portal(const glm::vec3& position);
    ParticleEmitter hearts(const glm::vec3& position);
    ParticleEmitter angryVillager(const glm::vec3& position);
    ParticleEmitter happyVillager(const glm::vec3& position);
    ParticleEmitter damage(const glm::vec3& position);
    ParticleEmitter snowballPoof(const glm::vec3& position);
    ParticleEmitter largeSmoke(const glm::vec3& position);
    ParticleEmitter redstoneDust(const glm::vec3& position);
    ParticleEmitter fallingDust(const glm::vec3& position, uint32_t blockType);
    ParticleEmitter totemUndying(const glm::vec3& position);
    ParticleEmitter spit(const glm::vec3& position);
    ParticleEmitter squidInk(const glm::vec3& position);
    ParticleEmitter endRod(const glm::vec3& position);
    ParticleEmitter dragonBreath(const glm::vec3& position);
    ParticleEmitter sweepingEdge(const glm::vec3& position, const glm::vec3& direction);
    ParticleEmitter flash(const glm::vec3& position);
    ParticleEmitter campfireCosySmoke(const glm::vec3& position);
    ParticleEmitter campfireSignalSmoke(const glm::vec3& position);
    ParticleEmitter sneeze(const glm::vec3& position);
    ParticleEmitter waxOn(const glm::vec3& position);
    ParticleEmitter waxOff(const glm::vec3& position);
    ParticleEmitter scrape(const glm::vec3& position);
    ParticleEmitter electricSpark(const glm::vec3& position);
}

} // namespace VoxelForge
