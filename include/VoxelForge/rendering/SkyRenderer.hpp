/**
 * @file SkyRenderer.hpp
 * @brief Sky and atmosphere rendering
 */

#pragma once

#include <string>

#include <vulkan/vulkan.hpp>
#include <VoxelForge/rendering/VulkanBuffer.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace VoxelForge {

class VulkanDevice;
class Camera;

// Sky colors
struct SkyColors {
    glm::vec3 zenithColor = glm::vec3(0.5f, 0.7f, 1.0f);      // Top of sky
    glm::vec3 horizonColor = glm::vec3(0.8f, 0.9f, 1.0f);     // Horizon
    glm::vec3 fogColor = glm::vec3(0.75f, 0.85f, 1.0f);       // Fog
    
    // Sun
    glm::vec3 sunColor = glm::vec3(1.0f, 0.95f, 0.8f);
    float sunIntensity = 1.0f;
    float sunSize = 30.0f;
    
    // Moon
    glm::vec3 moonColor = glm::vec3(0.9f, 0.9f, 1.0f);
    float moonIntensity = 0.3f;
    float moonSize = 20.0f;
    
    // Stars
    glm::vec3 starColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float starIntensity = 0.5f;
    float starTwinkle = 0.3f;
    
    // Clouds
    glm::vec3 cloudColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float cloudDensity = 0.5f;
    float cloudSpeed = 1.0f;
};

// Weather settings
struct WeatherSettings {
    float rain = 0.0f;          // 0.0 to 1.0
    float thunder = 0.0f;       // 0.0 to 1.0
    float snow = 0.0f;          // 0.0 to 1.0
    float lightningFlash = 0.0f;
    float fogDensity = 0.0f;
    glm::vec3 fogColor = glm::vec3(0.6f, 0.6f, 0.65f);
};

// Celestial body
struct CelestialBody {
    glm::vec3 direction;
    float angle;
    float size;
    glm::vec4 color;
    uint32_t textureIndex;
    bool visible;
};

// Time of day
enum class TimeOfDay {
    Dawn,
    Day,
    Dusk,
    Night
};

// Sky uniform data
struct SkyUniformData {
    alignas(16) glm::mat4 viewProj;
    alignas(16) glm::vec3 cameraPos;
    alignas(4) float time;
    alignas(16) glm::vec3 zenithColor;
    alignas(16) glm::vec3 horizonColor;
    alignas(16) glm::vec3 sunDirection;
    alignas(16) glm::vec3 moonDirection;
    alignas(16) glm::vec4 sunColorSize;
    alignas(16) glm::vec4 moonColorSize;
    alignas(4) float rain;
    alignas(4) float thunder;
    alignas(4) float fogStart;
    alignas(4) float fogEnd;
    alignas(16) glm::vec4 fogColor;
};

class SkyRenderer {
public:
    SkyRenderer();
    ~SkyRenderer();
    
    // No copy
    SkyRenderer(const SkyRenderer&) = delete;
    SkyRenderer& operator=(const SkyRenderer&) = delete;
    
    void init(VulkanDevice* device);
    void cleanup();
    
    // Frame management
    void beginFrame(vk::CommandBuffer cmd, Camera* camera);
    void render(vk::CommandBuffer cmd);
    void renderSunMoon(vk::CommandBuffer cmd);
    void renderStars(vk::CommandBuffer cmd);
    void renderClouds(vk::CommandBuffer cmd);
    void renderWeather(vk::CommandBuffer cmd);
    void endFrame();
    
    // Time management
    void setTimeOfDay(float time);  // 0.0 to 24000 (game ticks)
    float getTimeOfDay() const { return dayTime; }
    TimeOfDay getTimeOfDayEnum() const;
    
    // Weather
    void setWeather(const WeatherSettings& weather);
    const WeatherSettings& getWeather() const { return weather; }
    void triggerLightning();
    
    // Sky colors
    void setSkyColors(const SkyColors& colors);
    const SkyColors& getSkyColors() const { return colors; }
    
    // Dimension-specific
    void setOverworldSky();
    void setNetherSky();
    void setEndSky();
    void setCustomSky(const SkyColors& colors, const std::string& texturePath = "");
    
    // Fog
    void setFog(float start, float end, const glm::vec3& color);
    
    // Clouds
    void setCloudHeight(float height) { cloudHeight = height; }
    void setCloudTexture(const std::string& path);
    
    // Update
    void update(float deltaTime);
    
private:
    void createPipeline();
    void createBuffers();
    void createTextures();
    void updateUniformBuffer();
    void calculateCelestialPositions();
    void interpolateSkyColors();
    
    VulkanDevice* device = nullptr;
    
    // Pipelines
    vk::Pipeline skyPipeline;
    vk::PipelineLayout skyPipelineLayout;
    vk::Pipeline celestialPipeline;
    vk::PipelineLayout celestialPipelineLayout;
    vk::Pipeline cloudPipeline;
    vk::PipelineLayout cloudPipelineLayout;
    
    // Buffers
    std::vector<Buffer> uniformBuffers;
    
    // Geometry
    Buffer skyboxVertexBuffer;
    Buffer skyboxIndexBuffer;
    Buffer quadVertexBuffer;
    
    // State
    float dayTime = 6000.0f;    // Noon
    float daySpeed = 1.0f;
    SkyColors colors;
    WeatherSettings weather;
    float cloudHeight = 192.0f;
    float cloudOffset = 0.0f;
    
    // Celestial bodies
    CelestialBody sun;
    CelestialBody moon;
    
    // Textures
    uint32_t sunTexture = 0;
    uint32_t moonTexture = 0;
    uint32_t starTexture = 0;
    uint32_t cloudTexture = 0;
    
    Camera* currentCamera = nullptr;
    uint32_t currentFrame = 0;
};

} // namespace VoxelForge
