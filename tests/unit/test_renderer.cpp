/**
 * @file test_renderer.cpp
 * @brief Unit tests for renderer components
 */

#include <gtest/gtest.h>
#include <VoxelForge/rendering/Renderer.hpp>
#include <VoxelForge/rendering/Camera.hpp>
#include <VoxelForge/core/Logger.hpp>

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
    
    EXPECT_EQ(settings.renderDistance, 8);
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
    EXPECT_EQ(settings.maxFps, 120);
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

// Test Renderer default construction
TEST_F(RendererTest, Renderer_DefaultConstruction) {
    Renderer renderer;
    
    // Renderer should be created but not initialized
    // getWidth/getHeight return default values
    EXPECT_EQ(renderer.getWidth(), 1280);
    EXPECT_EQ(renderer.getHeight(), 720);
}

// Test Renderer settings access
TEST_F(RendererTest, Renderer_SettingsAccess) {
    Renderer renderer;
    
    // Modify settings
    renderer.getSettings().renderDistance = 12;
    renderer.getSettings().enableAO = false;
    
    // Verify const access
    const auto& settings = renderer.getSettings();
    EXPECT_EQ(settings.renderDistance, 12);
    EXPECT_FALSE(settings.enableAO);
}

// Test max frames in flight constant
TEST_F(RendererTest, Renderer_MaxFramesInFlight) {
    // Should be 2 for double buffering
    EXPECT_EQ(Renderer::MAX_FRAMES_IN_FLIGHT, 2);
}

// Test Camera integration with renderer
TEST_F(RendererTest, Camera_BasicSetup) {
    Camera camera(CameraType::Perspective);
    
    camera.setPosition(glm::vec3(0.0f, 64.0f, 0.0f));
    camera.setFOV(90.0f);
    camera.setNearPlane(0.1f);
    camera.setFarPlane(1000.0f);
    
    EXPECT_FLOAT_EQ(camera.getFOV(), 90.0f);
    EXPECT_FLOAT_EQ(camera.getNearPlane(), 0.1f);
    EXPECT_FLOAT_EQ(camera.getFarPlane(), 1000.0f);
}

// Test Camera view matrix
TEST_F(RendererTest, Camera_ViewMatrix) {
    Camera camera(CameraType::Perspective);
    
    camera.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    camera.lookAt(glm::vec3(0.0f, 0.0f, -1.0f));
    
    auto viewMatrix = camera.getViewMatrix();
    
    // View matrix should be valid
    EXPECT_TRUE(glm::isIdentity(viewMatrix, 0.0001f) || 
                glm::determinant(viewMatrix) != 0.0f);
}

// Test Camera projection matrix
TEST_F(RendererTest, Camera_ProjectionMatrix) {
    Camera camera(CameraType::Perspective);
    
    camera.setFOV(70.0f);
    camera.setAspectRatio(16.0f / 9.0f);
    camera.setNearPlane(0.1f);
    camera.setFarPlane(1000.0f);
    
    auto projMatrix = camera.getProjectionMatrix();
    
    // Projection matrix should be valid
    EXPECT_NE(glm::determinant(projMatrix), 0.0f);
}

// Test Camera frustum planes
TEST_F(RendererTest, Camera_FrustumPlanes) {
    Camera camera(CameraType::Perspective);
    
    camera.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    camera.setFOV(90.0f);
    camera.setAspectRatio(1.0f);
    camera.updateFrustum();
    
    // Test point inside frustum
    bool inside = camera.isPointInFrustum(glm::vec3(0.0f, 0.0f, -10.0f));
    EXPECT_TRUE(inside);
    
    // Test point behind camera
    bool behind = camera.isPointInFrustum(glm::vec3(0.0f, 0.0f, 10.0f));
    EXPECT_FALSE(behind);
}

// Test Camera orthographic mode
TEST_F(RendererTest, Camera_OrthographicMode) {
    Camera camera(CameraType::Orthographic);
    
    camera.setOrthographicSize(10.0f);
    
    EXPECT_EQ(camera.getType(), CameraType::Orthographic);
}

// Test Camera rotation
TEST_F(RendererTest, Camera_Rotation) {
    Camera camera(CameraType::Perspective);
    
    camera.rotate(45.0f, 0.0f);  // Yaw 45 degrees
    
    auto forward = camera.getForward();
    
    // Forward should point in rotated direction
    EXPECT_NEAR(forward.x, -0.707f, 0.01f);
    EXPECT_NEAR(forward.z, -0.707f, 0.01f);
}

// Test RenderStats accumulation
TEST_F(RendererTest, RenderStats_Accumulation) {
    RenderStats stats;
    
    stats.drawCalls = 10;
    stats.chunkDrawCalls = 8;
    stats.entityDrawCalls = 2;
    stats.verticesRendered = 10000;
    stats.trianglesRendered = 5000;
    
    EXPECT_EQ(stats.drawCalls, stats.chunkDrawCalls + stats.entityDrawCalls);
    EXPECT_EQ(stats.trianglesRendered, stats.verticesRendered / 3);
}

// Test render quality settings
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
