/**
 * @file PostProcessor.hpp
 * @brief Post-processing effects system
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <VoxelForge/rendering/VulkanBuffer.hpp>
#include <VoxelForge/rendering/VulkanImage.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace VoxelForge {

class VulkanDevice;
class Camera;

// Post-processing effect type
enum class EffectType {
    Tonemap,
    Bloom,
    FXAA,
    SMAA,
    TAA,
    MotionBlur,
    DepthOfField,
    ChromaticAberration,
    Vignette,
    ColorGrading,
    Exposure,
    GammaCorrection,
    Fog,
    Underwater,
    Nether,
    End,
    DamageFlash,
    Poison,
    Wither,
    Freeze,
    Blindness,
    Darkness
};

// Tonemap operator
enum class TonemapOperator {
    None,
    Reinhard,
    ReinhardLuma,
    ACES,
    ACESApprox,
    Filmic,
    Uchimura,
    Lottes,
    Uncharted2
};

// Post-processing settings
struct PostSettings {
    // Exposure
    float exposure = 1.0f;
    float autoExposure = true;
    float minExposure = 0.5f;
    float maxExposure = 2.0f;
    float exposureSpeed = 1.0f;
    
    // Tonemap
    TonemapOperator tonemap = TonemapOperator::ACES;
    float tonemapStrength = 1.0f;
    
    // Bloom
    bool bloomEnabled = true;
    float bloomThreshold = 1.0f;
    float bloomIntensity = 0.5f;
    float bloomRadius = 1.0f;
    int bloomMipLevels = 5;
    
    // FXAA
    bool fxaaEnabled = true;
    float fxaaSubpixel = 0.75f;
    float fxaaEdgeThreshold = 0.125f;
    float fxaaEdgeThresholdMin = 0.0625f;
    
    // TAA
    bool taaEnabled = false;
    float taaFeedback = 0.95f;
    
    // Motion blur
    bool motionBlurEnabled = false;
    float motionBlurIntensity = 0.5f;
    int motionBlurSamples = 8;
    
    // Depth of field
    bool dofEnabled = false;
    float dofFocusDistance = 10.0f;
    float dofFocusRange = 5.0f;
    float dofBlurRadius = 5.0f;
    
    // Chromatic aberration
    bool chromaticAberrationEnabled = false;
    float chromaticAberrationStrength = 0.005f;
    
    // Vignette
    bool vignetteEnabled = false;
    float vignetteIntensity = 0.5f;
    float vignetteSmoothness = 0.5f;
    glm::vec2 vignetteCenter = glm::vec2(0.5f);
    
    // Color grading
    glm::vec3 colorFilter = glm::vec3(1.0f);
    float saturation = 1.0f;
    float contrast = 1.0f;
    float brightness = 0.0f;
    glm::vec3 lift = glm::vec3(0.0f);
    glm::vec3 gamma = glm::vec3(1.0f);
    glm::vec3 gain = glm::vec3(1.0f);
    
    // Gamma
    float gamma = 2.2f;
};

// Effect uniform data
struct PostUniformData {
    // Resolution
    alignas(8) glm::vec2 resolution;
    alignas(8) glm::vec2 invResolution;
    
    // Exposure
    alignas(4) float exposure;
    alignas(4) float prevExposure;
    
    // Bloom
    alignas(4) float bloomThreshold;
    alignas(4) float bloomIntensity;
    alignas(4) float bloomRadius;
    
    // TAA
    alignas(16) glm::mat4 prevViewProj;
    alignas(4) float taaFeedback;
    
    // Motion blur
    alignas(4) float motionBlurIntensity;
    alignas(4) int motionBlurSamples;
    
    // DOF
    alignas(4) float dofFocusDistance;
    alignas(4) float dofFocusRange;
    alignas(4) float dofBlurRadius;
    
    // Effects
    alignas(4) float chromaticAberration;
    alignas(4) float vignetteIntensity;
    alignas(4) float vignetteSmoothness;
    alignas(8) glm::vec2 vignetteCenter;
    
    // Color grading
    alignas(16) glm::vec3 colorFilter;
    alignas(4) float saturation;
    alignas(4) float contrast;
    alignas(4) float brightness;
    alignas(16) glm::vec3 lift;
    alignas(16) glm::vec3 gamma;
    alignas(16) glm::vec3 gain;
    
    // Gamma
    alignas(4) float gamma;
    
    // Time
    alignas(4) float time;
    alignas(4) float deltaTime;
};

// Post-processing pass
struct PostPass {
    std::string name;
    vk::Pipeline pipeline;
    vk::PipelineLayout layout;
    std::vector<vk::DescriptorSet> descriptorSets;
    std::vector<Buffer> uniformBuffers;
    Image outputImage;
    vk::ImageView outputView;
    vk::Framebuffer framebuffer;
    vk::RenderPass renderPass;
};

class PostProcessor {
public:
    PostProcessor();
    ~PostProcessor();
    
    // No copy
    PostProcessor(const PostProcessor&) = delete;
    PostProcessor& operator=(const PostProcessor&) = delete;
    
    void init(VulkanDevice* device, vk::Extent2D extent);
    void cleanup();
    
    // Resize
    void onResize(vk::Extent2D newExtent);
    
    // Frame management
    void beginFrame(vk::CommandBuffer cmd, vk::ImageView colorInput, vk::ImageView depthInput);
    void process(vk::CommandBuffer cmd);
    void endFrame();
    
    // Output
    vk::ImageView getOutputView() const;
    vk::Image getOutputImage() const;
    
    // Settings
    PostSettings& getSettings() { return settings; }
    const PostSettings& getSettings() const { return settings; }
    void setSettings(const PostSettings& newSettings);
    
    // Dimension-specific presets
    void applyOverworldPreset();
    void applyNetherPreset();
    void applyEndPreset();
    
    // Status effects
    void applyDamageFlash(float intensity = 1.0f);
    void applyPoisonEffect(float intensity = 0.5f);
    void applyWitherEffect(float intensity = 0.5f);
    void applyFreezeEffect(float intensity = 0.5f);
    void applyBlindnessEffect(float intensity = 1.0f);
    void applyDarknessEffect(float intensity = 0.5f);
    void clearAllEffects();
    
    // Effect toggle
    void enableEffect(EffectType effect);
    void disableEffect(EffectType effect);
    bool isEffectEnabled(EffectType effect) const;
    
    // Update
    void update(float deltaTime);
    
private:
    void createRenderPasses();
    void createPipelines();
    void createBuffers();
    void createImages();
    void createDescriptorSets();
    
    void processTonemap(vk::CommandBuffer cmd);
    void processBloom(vk::CommandBuffer cmd);
    void processFXAA(vk::CommandBuffer cmd);
    void processTAA(vk::CommandBuffer cmd);
    void processMotionBlur(vk::CommandBuffer cmd);
    void processDOF(vk::CommandBuffer cmd);
    void processColorGrading(vk::CommandBuffer cmd);
    void processEffects(vk::CommandBuffer cmd);
    
    void updateUniformBuffer();
    
    VulkanDevice* device = nullptr;
    vk::Extent2D extent;
    
    // Passes
    std::unordered_map<std::string, PostPass> passes;
    
    // Input/Output
    vk::ImageView colorInput;
    vk::ImageView depthInput;
    Image outputImage;
    vk::ImageView outputView;
    
    // Bloom mip chain
    std::vector<Image> bloomMips;
    std::vector<vk::ImageView> bloomMipViews;
    
    // History for TAA
    Image historyImage;
    vk::ImageView historyView;
    glm::mat4 prevViewProj;
    
    // Settings
    PostSettings settings;
    PostUniformData uniformData;
    
    // Active effects
    std::unordered_map<EffectType, float> activeEffects;
    
    // Auto exposure
    float currentExposure = 1.0f;
    
    uint32_t currentFrame = 0;
    uint32_t frameCount = 2;
};

} // namespace VoxelForge
