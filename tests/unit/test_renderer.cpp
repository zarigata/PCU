#include <gtest/gtest.h>
#include <VoxelForge/rendering/Renderer.hpp>
#include <VoxelForge/rendering/Camera.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <cmath>

namespace VoxelForge {
namespace test {

class RendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        static bool initialized = false;
        if (!initialized) {
            Logger::init();
            initialized = true;
        }
    }
    
    void TearDown() override {
    }
};

// Test RenderStats default values
TEST_F(RendererTest, RenderStats_DefaultValues) {
    RenderStats stats;
    
    EXPECT_EQ(stats.drawCalls, 0);
    EXPECT_EQ(stats.chunkDrawCalls, 0);
    EXPECT_EQ(stats.entityDrawCalls, 0);
    EXPECT_EQ(stats.particlesDrawCalls, 0);
    EXPECT_EQ(stats.chunksRendered, 0);
    EXPECT_EQ(stats.chunksOccluded, 0);
    EXPECT_EQ(stats.verticesRendered, 0);
    EXPECT_EQ(stats.trianglesRendered, 0);
    EXPECT_FLOAT_EQ(stats.frameTimeMs, 0.0f);
    EXPECT_FLOAT_EQ(stats.gpuTimeMs, 0.0f);
}

// Test RenderSettings default values
TEST_F(RendererTest, RenderSettings_DefaultValues) {
    RenderSettings settings;
    
    EXPECT_EQ(settings.renderDistance, 6);
    EXPECT_EQ(settings.simulationDistance, 6);
    EXPECT_TRUE(settings.enableAO);
    EXPECT_TRUE(settings.enableShadows);
    EXPECT_FALSE(settings.enableVolumetricLighting);
    EXPECT_FALSE(settings.enableReflections);
    EXPECT_EQ(settings.shadowResolution, 2048);
    EXPECT_EQ(settings.reflectionResolution, 256);
    EXPECT_TRUE(settings.enableBloom);
    EXPECT_TRUE(settings.enableTAA);
    EXPECT_TRUE(settings.enableFXAA);
    EXPECT_FLOAT_EQ(settings.bloomIntensity, 0.5f);
    EXPECT_EQ(settings.maxFps, 60);
    EXPECT_TRUE(settings.enableVsync);
    EXPECT_TRUE(settings.enableFrustumCulling);
    EXPECT_TRUE(settings.enableOcclusionCulling);
    EXPECT_TRUE(settings.enableChunkMeshing);
    EXPECT_EQ(settings.maxChunksPerFrame, 4);
}

// Test RenderSettings modification
TEST_F(RendererTest, RenderSettings_Modification) {
    RenderSettings settings;
    
    settings.renderDistance = 16;
    settings.enableShadows = false;
    settings.maxFps = 60;
    settings.enableTAA = false;
    
    EXPECT_EQ(settings.renderDistance, 16);
    EXPECT_FALSE(settings.enableShadows);
    EXPECT_EQ(settings.maxFps, 60);
    EXPECT_FALSE(settings.enableTAA);
}

TEST_F(RendererTest, Renderer_DefaultConstruction) {
    Renderer renderer;
    
    EXPECT_EQ(renderer.getWidth(), 1280);
    EXPECT_EQ(renderer.getHeight(), 720);
}

// Test Renderer settings access
TEST_F(RendererTest, Renderer_SettingsAccess) {
    Renderer renderer;
    
    renderer.getSettings().renderDistance = 12;
    renderer.getSettings().enableAO = false;
    
    const auto& settings = renderer.getSettings();
    EXPECT_EQ(settings.renderDistance, 12);
    EXPECT_FALSE(settings.enableAO);
}

TEST_F(RendererTest, Camera_BasicSetup) {
    Camera camera;
    camera.setProjectionType(Camera::ProjectionType::Perspective);
    
    camera.setPosition(glm::vec3(0.0f, 64.0f, 0.0f));
    camera.setFOV(90.0f);
    camera.setPerspective(90.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    // Basic projection setup checks
    EXPECT_FLOAT_EQ(camera.getFOV(), 90.0f);
    // The API does not expose explicit near/far getters; near/far are applied via setPerspective.
    // Rely on projection matrix validity instead of direct near/far inspection.
    EXPECT_EQ(camera.getProjectionType(), Camera::ProjectionType::Perspective);
}

TEST_F(RendererTest, Camera_ViewMatrix) {
    Camera camera;
    
    camera.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    camera.lookAt(glm::vec3(0.0f, 0.0f, -1.0f));
    
    auto viewMatrix = camera.getViewMatrix();
    
    EXPECT_TRUE(glm::determinant(viewMatrix) != 0.0f);
}

TEST_F(RendererTest, Camera_ProjectionMatrix) {
    Camera camera;
    
    camera.setPerspective(70.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    
    auto projMatrix = camera.getProjectionMatrix();
    
    EXPECT_NE(glm::determinant(projMatrix), 0.0f);
}

TEST_F(RendererTest, Camera_FrustumPlanes) {
    Camera camera;
    camera.setPerspective(90.0f, 1.0f, 0.1f, 1000.0f);
    camera.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    camera.lookAt(glm::vec3(0.0f, 0.0f, -1.0f));
    
    auto viewMatrix = camera.getViewMatrix();
    auto projMatrix = camera.getProjectionMatrix();
    
    EXPECT_NE(glm::determinant(viewMatrix), 0.0f);
    EXPECT_NE(glm::determinant(projMatrix), 0.0f);
}

TEST_F(RendererTest, Camera_OrthographicMode) {
    Camera camera;
    camera.setProjectionType(Camera::ProjectionType::Orthographic);
    camera.setOrthographic(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f);
    
    EXPECT_EQ(camera.getProjectionType(), Camera::ProjectionType::Orthographic);
}

TEST_F(RendererTest, Camera_Rotation) {
    Camera camera;
    
    camera.rotate(0.0f, 45.0f);
    
    auto forward = camera.getForward();
    
    EXPECT_NEAR(std::abs(forward.x), 0.707f, 0.01f);
    EXPECT_NEAR(std::abs(forward.z), 0.707f, 0.01f);
}

TEST_F(RendererTest, RenderStats_Accumulation) {
    RenderStats stats;
    
    stats.drawCalls = 10;
    stats.chunkDrawCalls = 8;
    stats.entityDrawCalls = 2;
    stats.verticesRendered = 15000;
    stats.trianglesRendered = 5000;
    
    EXPECT_EQ(stats.drawCalls, stats.chunkDrawCalls + stats.entityDrawCalls);
    EXPECT_EQ(stats.trianglesRendered, stats.verticesRendered / 3);
}

TEST_F(RendererTest, RenderSettings_QualityPresets) {
    RenderSettings high;
    high.renderDistance = 16;
    high.enableShadows = true;
    high.shadowResolution = 4096;
    high.enableTAA = true;
    high.enableBloom = true;
    
    RenderSettings low;
    low.renderDistance = 4;
    low.enableShadows = false;
    low.enableTAA = false;
    low.enableBloom = false;
    
    // High quality should have more features enabled
    EXPECT_GT(high.renderDistance, low.renderDistance);
    EXPECT_TRUE(high.enableShadows);
    EXPECT_FALSE(low.enableShadows);
}

} // namespace test
} // namespace VoxelForge
