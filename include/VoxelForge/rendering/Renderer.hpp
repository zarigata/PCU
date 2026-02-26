/**
 * @file Renderer.hpp
 * @brief Main renderer class
 */

#pragma once

#include <VoxelForge/rendering/VulkanContext.hpp>
#include <VoxelForge/rendering/Camera.hpp>
#include <memory>
#include <vector>

namespace VoxelForge {

class World;
class Entity;

// Render statistics
struct RenderStats {
    uint32_t drawCalls = 0;
    uint32_t chunkDrawCalls = 0;
    uint32_t entityDrawCalls = 0;
    uint32_t particlesDrawCalls = 0;
    uint32_t chunksRendered = 0;
    uint32_t chunksOccluded = 0;
    uint32_t verticesRendered = 0;
    uint32_t trianglesRendered = 0;
    float frameTimeMs = 0.0f;
    float gpuTimeMs = 0.0f;
};

// Render settings
struct RenderSettings {
    // Quality
    int renderDistance = 8;
    int simulationDistance = 6;
    bool enableAO = true;
    bool enableShadows = true;
    bool enableVolumetricLighting = false;
    bool enableReflections = false;
    int shadowResolution = 2048;
    int reflectionResolution = 256;
    
    // Post-processing
    bool enableBloom = true;
    bool enableTAA = true;
    bool enableFXAA = true;
    float bloomIntensity = 0.5f;
    
    // Performance
    int maxFps = 120;
    bool enableFrustumCulling = true;
    bool enableOcclusionCulling = true;
    bool enableChunkMeshing = true;
    int maxChunksPerFrame = 4;
};

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    // No copy
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    
    void init(GLFWwindow* window);
    void shutdown();
    
    // Frame management
    void beginFrame();
    void endFrame();
    void render(World* world, Camera* camera);
    
    // Resize
    void onResize(int width, int height);
    
    // Getters
    VulkanContext& getContext() { return *context; }
    const RenderStats& getStats() const { return stats; }
    RenderSettings& getSettings() { return settings; }
    const RenderSettings& getSettings() const { return settings; }
    
    // Width/height
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
    // Screenshot
    void takeScreenshot(const std::string& path);
    
    // Reload shaders
    void reloadShaders();
    
private:
    void createInstance();
    void createDevice();
    void createSwapChain();
    void createRenderPass();
    void createFramebuffers();
    void createCommandBuffers();
    void createSyncObjects();
    
    void renderChunks(World* world, Camera* camera);
    void renderEntities(World* world, Camera* camera);
    void renderParticles(World* world, Camera* camera);
    void renderUI();
    void renderPostProcess();
    
    // Vulkan resources
    std::unique_ptr<VulkanContext> context;
    
    // Swapchain
    vk::SwapchainKHR swapchain;
    std::vector<vk::Image> swapchainImages;
    std::vector<vk::ImageView> swapchainImageViews;
    std::vector<vk::Framebuffer> framebuffers;
    vk::Format swapchainFormat;
    vk::Extent2D swapchainExtent;
    
    // Render pass
    vk::RenderPass renderPass;
    
    // Command buffers
    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;
    
    // Sync objects
    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    std::vector<vk::Fence> inFlightFences;
    uint32_t currentFrame = 0;
    
    // Dimensions
    int width = 1280;
    int height = 720;
    bool framebufferResized = false;
    
    // Stats and settings
    RenderStats stats;
    RenderSettings settings;
    
    // Frame data
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
};

// Global renderer
Renderer& GetRenderer();
void InitRenderer(GLFWwindow* window);
void ShutdownRenderer();

} // namespace VoxelForge
