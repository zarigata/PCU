/**
 * @file ParticleRenderer.cpp
 * @brief Particle system renderer implementation
 */

#include <VoxelForge/rendering/ParticleRenderer.hpp>
#include <VoxelForge/rendering/Camera.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>
#include <random>

namespace VoxelForge {

// ============================================================================
// ParticleRenderer Implementation
// ============================================================================

ParticleRenderer::ParticleRenderer() {
    LOG_INFO("ParticleRenderer created");
}

ParticleRenderer::~ParticleRenderer() {
    cleanup();
}

void ParticleRenderer::init(VulkanDevice* vulkanDevice) {
    device = vulkanDevice;
    if (!device) {
        LOG_ERROR("ParticleRenderer: Invalid device");
        return;
    }
    
    maxParticles = settings.maxParticles;
    
    createBuffers();
    createPipeline();
    
    LOG_INFO("ParticleRenderer initialized with max {} particles", maxParticles);
}

void ParticleRenderer::cleanup() {
    if (!device) return;
    
    // Cleanup pipelines
    if (pipeline) {
        device->getDevice().destroyPipeline(pipeline);
        pipeline = vk::Pipeline();
    }
    if (pipelineLayout) {
        device->getDevice().destroyPipelineLayout(pipelineLayout);
        pipelineLayout = vk::PipelineLayout();
    }
    
    // Cleanup buffers
    // Note: VulkanBuffer handles its own cleanup
    
    systems.clear();
    allParticles.clear();
    
    LOG_INFO("ParticleRenderer cleaned up");
}

void ParticleRenderer::beginFrame(vk::CommandBuffer cmd, Camera* camera) {
    currentCamera = camera;
}

void ParticleRenderer::render(vk::CommandBuffer cmd) {
    if (!device || allParticles.empty()) return;
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    
    // Draw particles
    vk::DeviceSize offset = 0;
    cmd.bindVertexBuffers(0, 1, &vertexBuffer.buffer, &offset);
    
    uint32_t particleCount = std::min(static_cast<uint32_t>(allParticles.size()), 
                                       static_cast<uint32_t>(maxParticles));
    cmd.draw(4, particleCount, 0, 0);
}

void ParticleRenderer::endFrame() {
    currentCamera = nullptr;
}

uint32_t ParticleRenderer::createSystem(const ParticleEmitter& emitter) {
    uint32_t id = nextSystemId++;
    
    ParticleSystem system;
    system.id = id;
    system.emitter = emitter;
    system.particles.resize(emitter.maxParticles);
    system.active = true;
    
    systems[id] = system;
    
    LOG_DEBUG("Created particle system {} with {} max particles", id, emitter.maxParticles);
    return id;
}

void ParticleRenderer::destroySystem(uint32_t systemId) {
    systems.erase(systemId);
}

void ParticleRenderer::updateSystem(uint32_t systemId, const ParticleEmitter& emitter) {
    auto it = systems.find(systemId);
    if (it != systems.end()) {
        it->second.emitter = emitter;
    }
}

void ParticleRenderer::emit(uint32_t systemId, int count) {
    auto it = systems.find(systemId);
    if (it == systems.end()) return;
    
    auto& system = it->second;
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    std::uniform_real_distribution<float> dis01(0.0f, 1.0f);
    
    for (int i = 0; i < count; ++i) {
        // Find inactive particle
        for (auto& particle : system.particles) {
            if (!particle.active) {
                particle.active = true;
                particle.position = system.emitter.position;
                
                // Random velocity based on spread
                glm::vec3 randomDir(
                    dis(gen) * system.emitter.spread,
                    dis(gen) * system.emitter.spread,
                    dis(gen) * system.emitter.spread
                );
                particle.velocity = (system.emitter.direction + randomDir) * 
                    (system.emitter.initialSpeed + dis(gen) * system.emitter.speedVariance);
                
                particle.color = system.emitter.colorStart;
                particle.size = system.emitter.sizeStart;
                particle.lifetime = system.emitter.lifetime + 
                    dis(gen) * system.emitter.lifetimeVariance;
                particle.age = 0.0f;
                
                system.activeParticles++;
                break;
            }
        }
    }
}

void ParticleRenderer::emitAt(const glm::vec3& position, const std::string& effectName, int count) {
    // Create effect from predefined
    ParticleEmitter emitter;
    
    if (effectName == "explosion") {
        emitter = ParticleEffects::explosion(position);
    } else if (effectName == "smoke") {
        emitter = ParticleEffects::smoke(position);
    } else if (effectName == "fire") {
        emitter = ParticleEffects::fire(position);
    } else if (effectName == "splash") {
        emitter = ParticleEffects::waterSplash(position);
    } else {
        emitter.position = position;
        emitter.maxParticles = count * 10;
    }
    
    uint32_t systemId = createSystem(emitter);
    emit(systemId, count);
}

void ParticleRenderer::playEffect(const std::string& name, const glm::vec3& position) {
    emitAt(position, name, 10);
}

void ParticleRenderer::stopEffect(uint32_t systemId) {
    auto it = systems.find(systemId);
    if (it != systems.end()) {
        it->second.active = false;
        it->second.emitter.isEmitting = false;
    }
}

void ParticleRenderer::stopAllEffects() {
    for (auto& [id, system] : systems) {
        system.active = false;
        system.emitter.isEmitting = false;
    }
}

void ParticleRenderer::update(float deltaTime) {
    allParticles.clear();
    
    for (auto& [id, system] : systems) {
        if (!system.active) continue;
        
        // Emit new particles
        if (system.emitter.isEmitting && system.emitter.emitRate > 0) {
            int toEmit = static_cast<int>(system.emitter.emitRate * deltaTime);
            emit(id, toEmit);
        }
        
        // Update existing particles
        for (auto& particle : system.particles) {
            if (!particle.active) continue;
            
            particle.age += deltaTime;
            
            if (particle.age >= particle.lifetime) {
                particle.active = false;
                system.activeParticles--;
                continue;
            }
            
            float t = particle.age / particle.lifetime;
            
            // Update position
            particle.position += particle.velocity * deltaTime;
            particle.velocity.y -= 9.8f * deltaTime; // Gravity
            
            // Update color (lerp)
            particle.color = glm::mix(system.emitter.colorStart, 
                                       system.emitter.colorEnd, t);
            
            // Update size
            particle.size = glm::mix(system.emitter.sizeStart, 
                                      system.emitter.sizeEnd, t);
            
            allParticles.push_back(particle);
        }
    }
}

int ParticleRenderer::getActiveParticleCount() const {
    return static_cast<int>(allParticles.size());
}

int ParticleRenderer::getSystemCount() const {
    return static_cast<int>(systems.size());
}

void ParticleRenderer::createPipeline() {
    // TODO: Create Vulkan pipeline for particle rendering
}

void ParticleRenderer::createBuffers() {
    // TODO: Create vertex and instance buffers
}

// ============================================================================
// Particle Effects Implementation
// ============================================================================

namespace ParticleEffects {

ParticleEmitter explosion(const glm::vec3& position, float power) {
    ParticleEmitter emitter;
    emitter.position = position;
    emitter.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    emitter.spread = 1.0f * power;
    emitter.initialSpeed = 5.0f * power;
    emitter.speedVariance = 2.0f * power;
    emitter.colorStart = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);
    emitter.colorEnd = glm::vec4(1.0f, 0.2f, 0.0f, 0.0f);
    emitter.sizeStart = 0.5f * power;
    emitter.sizeEnd = 0.1f;
    emitter.lifetime = 0.5f;
    emitter.lifetimeVariance = 0.2f;
    emitter.emitRate = 100;
    emitter.maxParticles = 200;
    emitter.loop = false;
    emitter.billboard = true;
    emitter.texture = "particle/explosion";
    return emitter;
}

ParticleEmitter smoke(const glm::vec3& position) {
    ParticleEmitter emitter;
    emitter.position = position;
    emitter.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    emitter.spread = 0.3f;
    emitter.initialSpeed = 1.0f;
    emitter.speedVariance = 0.5f;
    emitter.colorStart = glm::vec4(0.3f, 0.3f, 0.3f, 0.8f);
    emitter.colorEnd = glm::vec4(0.5f, 0.5f, 0.5f, 0.0f);
    emitter.sizeStart = 0.2f;
    emitter.sizeEnd = 0.8f;
    emitter.lifetime = 2.0f;
    emitter.lifetimeVariance = 1.0f;
    emitter.emitRate = 10;
    emitter.maxParticles = 100;
    emitter.loop = true;
    emitter.billboard = true;
    emitter.texture = "particle/smoke";
    return emitter;
}

ParticleEmitter fire(const glm::vec3& position) {
    ParticleEmitter emitter;
    emitter.position = position;
    emitter.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    emitter.spread = 0.2f;
    emitter.initialSpeed = 2.0f;
    emitter.speedVariance = 1.0f;
    emitter.colorStart = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);
    emitter.colorEnd = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    emitter.sizeStart = 0.3f;
    emitter.sizeEnd = 0.1f;
    emitter.lifetime = 0.5f;
    emitter.lifetimeVariance = 0.2f;
    emitter.emitRate = 50;
    emitter.maxParticles = 100;
    emitter.loop = true;
    emitter.billboard = true;
    emitter.texture = "particle/flame";
    return emitter;
}

ParticleEmitter waterSplash(const glm::vec3& position) {
    ParticleEmitter emitter;
    emitter.position = position;
    emitter.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    emitter.spread = 0.5f;
    emitter.initialSpeed = 3.0f;
    emitter.speedVariance = 1.0f;
    emitter.colorStart = glm::vec4(0.3f, 0.5f, 1.0f, 0.8f);
    emitter.colorEnd = glm::vec4(0.3f, 0.5f, 1.0f, 0.0f);
    emitter.sizeStart = 0.1f;
    emitter.sizeEnd = 0.05f;
    emitter.lifetime = 0.5f;
    emitter.lifetimeVariance = 0.2f;
    emitter.emitRate = 50;
    emitter.maxParticles = 100;
    emitter.loop = false;
    emitter.billboard = true;
    emitter.texture = "particle/splash";
    return emitter;
}

ParticleEmitter lavaSplash(const glm::vec3& position) {
    ParticleEmitter emitter;
    emitter.position = position;
    emitter.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    emitter.spread = 0.3f;
    emitter.initialSpeed = 2.0f;
    emitter.speedVariance = 0.5f;
    emitter.colorStart = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
    emitter.colorEnd = glm::vec4(0.5f, 0.0f, 0.0f, 0.0f);
    emitter.sizeStart = 0.2f;
    emitter.sizeEnd = 0.1f;
    emitter.lifetime = 1.0f;
    emitter.lifetimeVariance = 0.3f;
    emitter.emitRate = 30;
    emitter.maxParticles = 60;
    emitter.loop = false;
    emitter.billboard = true;
    emitter.texture = "particle/lava";
    return emitter;
}

ParticleEmitter blockBreak(const glm::vec3& position, uint32_t blockType) {
    ParticleEmitter emitter;
    emitter.position = position + glm::vec3(0.5f);
    emitter.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    emitter.spread = 1.0f;
    emitter.initialSpeed = 2.0f;
    emitter.speedVariance = 1.0f;
    emitter.colorStart = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
    emitter.colorEnd = glm::vec4(0.6f, 0.6f, 0.6f, 0.0f);
    emitter.sizeStart = 0.1f;
    emitter.sizeEnd = 0.05f;
    emitter.lifetime = 0.5f;
    emitter.lifetimeVariance = 0.2f;
    emitter.emitRate = 30;
    emitter.maxParticles = 50;
    emitter.loop = false;
    emitter.billboard = true;
    emitter.texture = "particle/blockcrack_" + std::to_string(blockType);
    return emitter;
}

ParticleEmitter blockPlace(const glm::vec3& position, uint32_t blockType) {
    ParticleEmitter emitter;
    emitter.position = position + glm::vec3(0.5f);
    emitter.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    emitter.spread = 0.5f;
    emitter.initialSpeed = 0.5f;
    emitter.speedVariance = 0.2f;
    emitter.colorStart = glm::vec4(0.6f, 0.6f, 0.6f, 0.8f);
    emitter.colorEnd = glm::vec4(0.6f, 0.6f, 0.6f, 0.0f);
    emitter.sizeStart = 0.05f;
    emitter.sizeEnd = 0.02f;
    emitter.lifetime = 0.3f;
    emitter.lifetimeVariance = 0.1f;
    emitter.emitRate = 20;
    emitter.maxParticles = 30;
    emitter.loop = false;
    emitter.billboard = true;
    emitter.texture = "particle/blockcrack_" + std::to_string(blockType);
    return emitter;
}

ParticleEmitter criticalHit(const glm::vec3& position) {
    ParticleEmitter emitter;
    emitter.position = position;
    emitter.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    emitter.spread = 0.3f;
    emitter.initialSpeed = 3.0f;
    emitter.speedVariance = 0.5f;
    emitter.colorStart = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    emitter.colorEnd = glm::vec4(1.0f, 0.5f, 0.0f, 0.0f);
    emitter.sizeStart = 0.15f;
    emitter.sizeEnd = 0.05f;
    emitter.lifetime = 0.3f;
    emitter.lifetimeVariance = 0.1f;
    emitter.emitRate = 50;
    emitter.maxParticles = 50;
    emitter.loop = false;
    emitter.billboard = true;
    emitter.texture = "particle/crit";
    return emitter;
}

ParticleEmitter magicCrit(const glm::vec3& position) {
    ParticleEmitter emitter;
    emitter.position = position;
    emitter.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    emitter.spread = 0.5f;
    emitter.initialSpeed = 4.0f;
    emitter.speedVariance = 1.0f;
    emitter.colorStart = glm::vec4(0.5f, 0.0f, 1.0f, 1.0f);
    emitter.colorEnd = glm::vec4(0.0f, 0.5f, 1.0f, 0.0f);
    emitter.sizeStart = 0.2f;
    emitter.sizeEnd = 0.05f;
    emitter.lifetime = 0.5f;
    emitter.lifetimeVariance = 0.2f;
    emitter.emitRate = 30;
    emitter.maxParticles = 60;
    emitter.loop = false;
    emitter.billboard = true;
    emitter.texture = "particle/magic_crit";
    return emitter;
}

ParticleEmitter spell(const glm::vec3& position, const glm::vec4& color) {
    ParticleEmitter emitter;
    emitter.position = position;
    emitter.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    emitter.spread = 0.3f;
    emitter.initialSpeed = 2.0f;
    emitter.speedVariance = 0.5f;
    emitter.colorStart = color;
    emitter.colorEnd = glm::vec4(color.r, color.g, color.b, 0.0f);
    emitter.sizeStart = 0.15f;
    emitter.sizeEnd = 0.05f;
    emitter.lifetime = 0.5f;
    emitter.lifetimeVariance = 0.2f;
    emitter.emitRate = 20;
    emitter.maxParticles = 40;
    emitter.loop = false;
    emitter.billboard = true;
    emitter.texture = "particle/spell";
    return emitter;
}

ParticleEmitter portal(const glm::vec3& position) {
    ParticleEmitter emitter;
    emitter.position = position;
    emitter.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    emitter.spread = 0.5f;
    emitter.initialSpeed = 1.0f;
    emitter.speedVariance = 0.3f;
    emitter.colorStart = glm::vec4(0.5f, 0.0f, 1.0f, 1.0f);
    emitter.colorEnd = glm::vec4(0.2f, 0.0f, 0.5f, 0.0f);
    emitter.sizeStart = 0.2f;
    emitter.sizeEnd = 0.1f;
    emitter.lifetime = 1.0f;
    emitter.lifetimeVariance = 0.5f;
    emitter.emitRate = 15;
    emitter.maxParticles = 50;
    emitter.loop = true;
    emitter.billboard = true;
    emitter.texture = "particle/portal";
    return emitter;
}

ParticleEmitter hearts(const glm::vec3& position) {
    ParticleEmitter emitter;
    emitter.position = position + glm::vec3(0.0f, 1.0f, 0.0f);
    emitter.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    emitter.spread = 0.3f;
    emitter.initialSpeed = 0.5f;
    emitter.speedVariance = 0.2f;
    emitter.colorStart = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);
    emitter.colorEnd = glm::vec4(1.0f, 0.2f, 0.2f, 0.0f);
    emitter.sizeStart = 0.2f;
    emitter.sizeEnd = 0.1f;
    emitter.lifetime = 0.5f;
    emitter.lifetimeVariance = 0.1f;
    emitter.emitRate = 10;
    emitter.maxParticles = 10;
    emitter.loop = false;
    emitter.billboard = true;
    emitter.texture = "particle/heart";
    return emitter;
}

// Additional effect implementations...
ParticleEmitter angryVillager(const glm::vec3& position) { return smoke(position); }
ParticleEmitter happyVillager(const glm::vec3& position) { return hearts(position); }
ParticleEmitter damage(const glm::vec3& position) { return criticalHit(position); }
ParticleEmitter snowballPoof(const glm::vec3& position) { return smoke(position); }
ParticleEmitter largeSmoke(const glm::vec3& position) { return smoke(position); }
ParticleEmitter redstoneDust(const glm::vec3& position) { return spell(position, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); }
ParticleEmitter fallingDust(const glm::vec3& position, uint32_t blockType) { return smoke(position); }
ParticleEmitter totemUndying(const glm::vec3& position) { return spell(position, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)); }
ParticleEmitter spit(const glm::vec3& position) { return smoke(position); }
ParticleEmitter squidInk(const glm::vec3& position) { return spell(position, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)); }
ParticleEmitter endRod(const glm::vec3& position) { return portal(position); }
ParticleEmitter dragonBreath(const glm::vec3& position) { return fire(position); }
ParticleEmitter sweepingEdge(const glm::vec3& position, const glm::vec3& direction) { return criticalHit(position); }
ParticleEmitter flash(const glm::vec3& position) { return explosion(position, 0.5f); }
ParticleEmitter campfireCosySmoke(const glm::vec3& position) { return smoke(position); }
ParticleEmitter campfireSignalSmoke(const glm::vec3& position) { return smoke(position); }
ParticleEmitter sneeze(const glm::vec3& position) { return smoke(position); }
ParticleEmitter waxOn(const glm::vec3& position) { return spell(position, glm::vec4(1.0f, 1.0f, 0.8f, 1.0f)); }
ParticleEmitter waxOff(const glm::vec3& position) { return spell(position, glm::vec4(0.8f, 0.8f, 0.8f, 1.0f)); }
ParticleEmitter scrape(const glm::vec3& position) { return smoke(position); }
ParticleEmitter electricSpark(const glm::vec3& position) { return spell(position, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)); }

} // namespace ParticleEffects

} // namespace VoxelForge
