/**
 * @file SkyRenderer.cpp
 * @brief Sky and atmosphere rendering implementation
 */

#include <VoxelForge/rendering/SkyRenderer.hpp>
#include <VoxelForge/rendering/Camera.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <cmath>

namespace VoxelForge {

// ============================================================================
// SkyRenderer Implementation
// ============================================================================

SkyRenderer::SkyRenderer() {
    LOG_INFO("SkyRenderer created");
}

SkyRenderer::~SkyRenderer() {
    cleanup();
}

void SkyRenderer::init(VulkanDevice* vulkanDevice) {
    device = vulkanDevice;
    if (!device) {
        LOG_ERROR("SkyRenderer: Invalid device");
        return;
    }
    
    createBuffers();
    createTextures();
    createPipeline();
    
    // Initialize celestial bodies
    sun.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    sun.size = 30.0f;
    sun.color = glm::vec4(1.0f, 0.95f, 0.8f, 1.0f);
    sun.visible = true;
    
    moon.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    moon.size = 20.0f;
    moon.color = glm::vec4(0.9f, 0.9f, 1.0f, 1.0f);
    moon.visible = true;
    
    setOverworldSky();
    
    LOG_INFO("SkyRenderer initialized");
}

void SkyRenderer::cleanup() {
    if (!device) return;
    
    if (skyPipeline) {
        device->getDevice().destroyPipeline(skyPipeline);
    }
    if (skyPipelineLayout) {
        device->getDevice().destroyPipelineLayout(skyPipelineLayout);
    }
    if (celestialPipeline) {
        device->getDevice().destroyPipeline(celestialPipeline);
    }
    if (celestialPipelineLayout) {
        device->getDevice().destroyPipelineLayout(celestialPipelineLayout);
    }
    if (cloudPipeline) {
        device->getDevice().destroyPipeline(cloudPipeline);
    }
    if (cloudPipelineLayout) {
        device->getDevice().destroyPipelineLayout(cloudPipelineLayout);
    }
    
    LOG_INFO("SkyRenderer cleaned up");
}

void SkyRenderer::beginFrame(vk::CommandBuffer cmd, Camera* camera) {
    currentCamera = camera;
    currentFrame++;
}

void SkyRenderer::render(vk::CommandBuffer cmd) {
    if (!currentCamera) return;
    
    renderSunMoon(cmd);
    renderStars(cmd);
    renderClouds(cmd);
    renderWeather(cmd);
}

void SkyRenderer::renderSunMoon(vk::CommandBuffer cmd) {
    if (!device) return;
    
    // Bind celestial pipeline
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, celestialPipeline);
    
    // Draw sun
    if (sun.visible) {
        // TODO: Draw sun quad
    }
    
    // Draw moon
    if (moon.visible) {
        // TODO: Draw moon quad
    }
}

void SkyRenderer::renderStars(vk::CommandBuffer cmd) {
    if (!device || dayTime > 13000) return; // Stars only visible at night
    
    // TODO: Render starfield
}

void SkyRenderer::renderClouds(vk::CommandBuffer cmd) {
    if (!device) return;
    
    // Bind cloud pipeline
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, cloudPipeline);
    
    // TODO: Draw cloud layer
}

void SkyRenderer::renderWeather(vk::CommandBuffer cmd) {
    if (!device || (weather.rain <= 0.0f && weather.thunder <= 0.0f)) return;
    
    // TODO: Render rain/snow particles
}

void SkyRenderer::endFrame() {
    currentCamera = nullptr;
}

void SkyRenderer::setTimeOfDay(float time) {
    dayTime = time;
    while (dayTime >= 24000.0f) dayTime -= 24000.0f;
    while (dayTime < 0.0f) dayTime += 24000.0f;
    
    calculateCelestialPositions();
    interpolateSkyColors();
}

TimeOfDay SkyRenderer::getTimeOfDayEnum() const {
    if (dayTime >= 0 && dayTime < 6000) return TimeOfDay::Dawn;
    if (dayTime >= 6000 && dayTime < 12000) return TimeOfDay::Day;
    if (dayTime >= 12000 && dayTime < 18000) return TimeOfDay::Dusk;
    return TimeOfDay::Night;
}

void SkyRenderer::setWeather(const WeatherSettings& newWeather) {
    weather = newWeather;
}

void SkyRenderer::triggerLightning() {
    weather.lightningFlash = 1.0f;
    // TODO: Play thunder sound
}

void SkyRenderer::setSkyColors(const SkyColors& newColors) {
    colors = newColors;
}

void SkyRenderer::setOverworldSky() {
    colors = SkyColors();
    cloudHeight = 192.0f;
}

void SkyRenderer::setNetherSky() {
    colors.zenithColor = glm::vec3(0.2f, 0.0f, 0.0f);
    colors.horizonColor = glm::vec3(0.4f, 0.1f, 0.0f);
    colors.fogColor = glm::vec3(0.2f, 0.0f, 0.0f);
    colors.sunIntensity = 0.0f;
    colors.moonIntensity = 0.0f;
    colors.starIntensity = 0.0f;
    cloudHeight = -1000.0f; // No clouds
}

void SkyRenderer::setEndSky() {
    colors.zenithColor = glm::vec3(0.0f, 0.0f, 0.05f);
    colors.horizonColor = glm::vec3(0.1f, 0.1f, 0.15f);
    colors.fogColor = glm::vec3(0.05f, 0.05f, 0.1f);
    colors.sunIntensity = 0.0f;
    colors.moonIntensity = 0.0f;
    colors.starIntensity = 0.0f;
    cloudHeight = -1000.0f; // No clouds
}

void SkyRenderer::setCustomSky(const SkyColors& newColors, const std::string& texturePath) {
    colors = newColors;
    if (!texturePath.empty()) {
        // TODO: Load custom sky texture
    }
}

void SkyRenderer::setFog(float start, float end, const glm::vec3& color) {
    colors.fogColor = color;
    // TODO: Update fog parameters
}

void SkyRenderer::setCloudTexture(const std::string& path) {
    // TODO: Load cloud texture
}

void SkyRenderer::update(float deltaTime) {
    // Update time of day
    dayTime += deltaTime * daySpeed * 20.0f; // 20 ticks per second
    while (dayTime >= 24000.0f) dayTime -= 24000.0f;
    
    // Update cloud offset
    cloudOffset += deltaTime * colors.cloudSpeed * 0.1f;
    
    // Update celestial positions
    calculateCelestialPositions();
    
    // Update lightning flash
    if (weather.lightningFlash > 0.0f) {
        weather.lightningFlash -= deltaTime * 5.0f;
        if (weather.lightningFlash < 0.0f) {
            weather.lightningFlash = 0.0f;
        }
    }
    
    // Interpolate sky colors
    interpolateSkyColors();
    
    updateUniformBuffer();
}

void SkyRenderer::createPipeline() {
    // TODO: Create Vulkan pipelines for sky, celestial, and cloud rendering
}

void SkyRenderer::createBuffers() {
    // TODO: Create vertex buffers for skybox and celestial quads
}

void SkyRenderer::createTextures() {
    // TODO: Load sun, moon, star, and cloud textures
}

void SkyRenderer::updateUniformBuffer() {
    // TODO: Update uniform buffer with current sky data
}

void SkyRenderer::calculateCelestialPositions() {
    // Calculate sun and moon angles based on time of day
    float angle = (dayTime / 24000.0f) * 2.0f * 3.14159f;
    
    // Sun rises at dawn (0), sets at dusk (12000)
    sun.angle = angle;
    sun.direction = glm::vec3(
        std::cos(angle),
        std::sin(angle),
        0.0f
    );
    
    // Moon is opposite to sun
    moon.angle = angle + 3.14159f;
    moon.direction = -sun.direction;
    
    // Visibility based on position
    sun.visible = sun.direction.y > -0.2f;
    moon.visible = moon.direction.y > -0.2f;
}

void SkyRenderer::interpolateSkyColors() {
    // Interpolate sky colors based on time of day
    TimeOfDay timeOfDay = getTimeOfDayEnum();
    
    // Define color presets for different times
    glm::vec3 dawnZenith(0.5f, 0.4f, 0.6f);
    glm::vec3 dawnHorizon(1.0f, 0.6f, 0.3f);
    
    glm::vec3 dayZenith(0.5f, 0.7f, 1.0f);
    glm::vec3 dayHorizon(0.8f, 0.9f, 1.0f);
    
    glm::vec3 duskZenith(0.4f, 0.3f, 0.5f);
    glm::vec3 duskHorizon(1.0f, 0.5f, 0.2f);
    
    glm::vec3 nightZenith(0.0f, 0.0f, 0.1f);
    glm::vec3 nightHorizon(0.1f, 0.1f, 0.2f);
    
    // Calculate interpolation factor
    float t = 0.0f;
    glm::vec3 targetZenith, targetHorizon;
    
    switch (timeOfDay) {
        case TimeOfDay::Dawn:
            t = (dayTime - 0) / 6000.0f;
            targetZenith = glm::mix(dawnZenith, dayZenith, t);
            targetHorizon = glm::mix(dawnHorizon, dayHorizon, t);
            break;
        case TimeOfDay::Day:
            t = (dayTime - 6000) / 6000.0f;
            targetZenith = glm::mix(dayZenith, duskZenith, t);
            targetHorizon = glm::mix(dayHorizon, duskHorizon, t);
            break;
        case TimeOfDay::Dusk:
            t = (dayTime - 12000) / 6000.0f;
            targetZenith = glm::mix(duskZenith, nightZenith, t);
            targetHorizon = glm::mix(duskHorizon, nightHorizon, t);
            break;
        case TimeOfDay::Night:
            t = (dayTime - 18000) / 6000.0f;
            targetZenith = glm::mix(nightZenith, dawnZenith, t);
            targetHorizon = glm::mix(nightHorizon, dawnHorizon, t);
            break;
    }
    
    // Apply weather modification
    if (weather.rain > 0.0f) {
        glm::vec3 rainZenith(0.4f, 0.4f, 0.45f);
        glm::vec3 rainHorizon(0.5f, 0.5f, 0.55f);
        targetZenith = glm::mix(targetZenith, rainZenith, weather.rain);
        targetHorizon = glm::mix(targetHorizon, rainHorizon, weather.rain);
    }
    
    // Lightning flash
    if (weather.lightningFlash > 0.0f) {
        glm::vec3 flashColor(1.0f, 1.0f, 1.0f);
        targetZenith = glm::mix(targetZenith, flashColor, weather.lightningFlash * 0.5f);
        targetHorizon = glm::mix(targetHorizon, flashColor, weather.lightningFlash * 0.5f);
    }
    
    colors.zenithColor = targetZenith;
    colors.horizonColor = targetHorizon;
}

} // namespace VoxelForge
