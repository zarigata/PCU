/**
 * @file Renderer.hpp
 * @brief Main renderer class
 */

#pragma once

#include <string>
#include <array>
#include <unordered_map>
#include <unordered_set>

#include <VoxelForge/rendering/VulkanContext.hpp>
#include <VoxelForge/rendering/VulkanPipeline.hpp>
#include <VoxelForge/rendering/Camera.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace VoxelForge {

class World;
class Entity;
struct AsyncMeshResult;

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
    int renderDistance = 6;
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
    int maxFps = 60;
    bool enableVsync = true;
    bool enableFrustumCulling = true;
    bool enableOcclusionCulling = true;
    bool enableChunkMeshing = true;
    int maxChunksPerFrame = 4;
};

struct UIMenuState {
    int selected = 0;
    int hovered = -1;
    float fov = 70.0f;
    float sensitivity = 0.3f;
    int renderDistance = 6;
    int maxFPS = 60;
    bool invertY = false;
    bool invertX = false;
    bool vsync = true;
    bool flyMode = true;
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
    
    // Clear color (sky)
    void setClearColor(float r, float g, float b, float a = 1.0f);
    
    // Chunk rendering
    void initChunkRendering();
    void cleanupChunkRendering();
    void renderWorldChunks(World* world, Camera* camera, float fogR, float fogG, float fogB);
    void generateAndUploadChunks(World* world, const glm::vec3& cameraPos);
    void uploadMeshData(int chunkX, int chunkZ, const std::vector<float>& verts, const std::vector<uint32_t>& indices);
    void uploadMeshDataBatch(const std::vector<AsyncMeshResult>& meshes);
    void submitUploadBatch(const std::vector<AsyncMeshResult>& meshes);
    void waitForPendingUpload();
    void evictDistantMeshes(const glm::vec3& cameraPos);
    void invalidateChunkMesh(int chunkX, int chunkZ);
    std::unordered_set<uint64_t> getChunkMeshKeys() const;
    void drawCrosshair();
    void drawHotbar(int selectedSlot, const std::array<uint32_t, 9>& hotbarBlocks);
    void drawPauseMenu(const UIMenuState& menu);
    void drawClickToPlay();
    void drawText(const char* text, float x, float y, uint32_t color, float scale = 1.0f);
    void drawDebugOverlay(float fps, float frameTime, int chunks, int drawCalls, float pitch, float yaw);
    void resetUIBatch();
    void drawClouds(Camera* camera, float gameTime);

    // UI rendering (host-visible buffers, dedicated UI pipeline)
    void initUIRendering();
    void cleanupUIRendering();
    
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
    void recreateSwapChain();
    void createRenderPass();
    void createDepthResources();
    void createFramebuffers();
    void createCommandBuffers();
    void createSyncObjects();
    
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& modes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    
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
    
    // Depth buffer
    vk::Image depthImage;
    vk::DeviceMemory depthImageMemory;
    vk::ImageView depthImageView;
    vk::Format depthFormat;
    
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
    uint32_t currentImageIndex = 0;
    
    // Dimensions
    int width = 1280;
    int height = 720;
    bool framebufferResized = false;
    
    // Stats and settings
    RenderStats stats;
    RenderSettings settings;
    
    std::array<float, 4> clearColorValue = {0.0f, 0.0f, 0.2f, 1.0f};
    
    struct ChunkGPUMesh {
        vk::Buffer vertexBuffer = VK_NULL_HANDLE;
        vk::DeviceMemory vertexMemory = VK_NULL_HANDLE;
        vk::DeviceSize vertexCapacity = 0;
        vk::Buffer indexBuffer = VK_NULL_HANDLE;
        vk::DeviceMemory indexMemory = VK_NULL_HANDLE;
        vk::DeviceSize indexCapacity = 0;
        uint32_t indexCount = 0;
        glm::ivec3 chunkPos{};
        bool valid = false;
    };
    
    vk::Pipeline chunkPipeline;
    vk::PipelineLayout chunkPipelineLayout;
    vk::ShaderModule chunkVertShader = VK_NULL_HANDLE;
    vk::ShaderModule chunkFragShader = VK_NULL_HANDLE;
    std::unordered_map<uint64_t, ChunkGPUMesh> chunkMeshes;
    bool chunkPipelineReady = false;

    struct PendingUpload {
        vk::Fence fence;
        vk::CommandBuffer cmdBuf;
        vk::Buffer stagingBuf;
        vk::DeviceMemory stagingMem;
        std::vector<std::pair<uint64_t, ChunkGPUMesh>> pendingMeshes;
        bool active = false;
    } pendingUpload_;

    struct PooledBuffer {
        vk::Buffer vertexBuffer;
        vk::DeviceMemory vertexMemory;
        vk::DeviceSize vertexCapacity;
        vk::Buffer indexBuffer;
        vk::DeviceMemory indexMemory;
        vk::DeviceSize indexCapacity;
    };
    std::vector<PooledBuffer> bufferPool_;
    static constexpr size_t MAX_POOL_SIZE = 64;

    void recycleBuffers(ChunkGPUMesh& mesh);
    bool acquireFromPool(vk::DeviceSize vsize, vk::DeviceSize isize, ChunkGPUMesh& out);
    
    vk::ShaderModule compileShader(const std::string& source, int stage);
    void createChunkBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props, vk::Buffer& buf, vk::DeviceMemory& mem);
    void uploadChunkMesh(const glm::ivec3& pos, const std::vector<float>& verts, const std::vector<uint32_t>& indices);
    static uint64_t posKey(int x, int y, int z);

    // UI rendering resources (host-visible, persistent buffers + UI pipeline)
    vk::Pipeline uiPipeline = VK_NULL_HANDLE;
    vk::PipelineLayout uiPipelineLayout = VK_NULL_HANDLE;
    vk::ShaderModule uiVertShader = VK_NULL_HANDLE;
    vk::ShaderModule uiFragShader = VK_NULL_HANDLE;
    vk::Buffer uiVertexBuffer = VK_NULL_HANDLE;
    vk::DeviceMemory uiVertexMemory = VK_NULL_HANDLE;
    vk::Buffer uiIndexBuffer = VK_NULL_HANDLE;
    vk::DeviceMemory uiIndexMemory = VK_NULL_HANDLE;
    vk::DeviceSize uiVertexBufferSize = 256 * 1024;
    vk::DeviceSize uiIndexBufferSize = 256 * 1024;
    void* uiVertexMapped = nullptr;
    void* uiIndexMapped = nullptr;
    uint32_t uiBatchV = 0;
    uint32_t uiBatchI = 0;
    bool uiPipelineReady = false;
    
    // Frame data
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
};

// Global renderer
Renderer& GetRenderer();
void InitRenderer(GLFWwindow* window);
void ShutdownRenderer();

} // namespace VoxelForge
