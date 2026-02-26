/**
 * @file ShadowRenderer.cpp
 * @brief Shadow mapping system implementation
 */

#include <VoxelForge/rendering/ShadowRenderer.hpp>
#include <VoxelForge/rendering/Camera.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <cmath>

namespace VoxelForge {

// ============================================================================
// ShadowRenderer Implementation
// ============================================================================

ShadowRenderer::ShadowRenderer() {
    LOG_INFO("ShadowRenderer created");
}

ShadowRenderer::~ShadowRenderer() {
    cleanup();
}

void ShadowRenderer::init(VulkanDevice* vulkanDevice) {
    device = vulkanDevice;
    if (!device) {
        LOG_ERROR("ShadowRenderer: Invalid device");
        return;
    }
    
    cascades.resize(settings.cascadeCount);
    
    createShadowMap();
    createRenderPass();
    createFramebuffer();
    createPipeline();
    createUniformBuffers();
    
    LOG_INFO("ShadowRenderer initialized with {} cascades at {}x{}", 
             settings.cascadeCount, settings.resolution, settings.resolution);
}

void ShadowRenderer::cleanup() {
    if (!device) return;
    
    if (shadowPipeline) {
        device->getDevice().destroyPipeline(shadowPipeline);
    }
    if (shadowPipelineLayout) {
        device->getDevice().destroyPipelineLayout(shadowPipelineLayout);
    }
    if (shadowRenderPass) {
        device->getDevice().destroyRenderPass(shadowRenderPass);
    }
    if (shadowFramebuffer) {
        device->getDevice().destroyFramebuffer(shadowFramebuffer);
    }
    if (shadowMapView) {
        device->getDevice().destroyImageView(shadowMapView);
    }
    if (shadowSampler) {
        device->getDevice().destroySampler(shadowSampler);
    }
    
    uniformBuffers.clear();
    cascades.clear();
    
    LOG_INFO("ShadowRenderer cleaned up");
}

void ShadowRenderer::beginFrame(Camera* camera, const glm::vec3& lightDir) {
    currentCamera = camera;
    lightDirection = glm::normalize(lightDir);
    
    updateCascades(camera, lightDir);
    updateUniformBuffer();
}

void ShadowRenderer::renderShadowPass(vk::CommandBuffer cmd, World* world, ChunkRenderer* chunkRenderer) {
    if (!device || !settings.enabled) return;
    
    // Begin render pass
    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.renderPass = shadowRenderPass;
    renderPassInfo.framebuffer = shadowFramebuffer;
    renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderPassInfo.renderArea.extent = vk::Extent2D(settings.resolution, settings.resolution);
    
    vk::ClearValue clearValue;
    clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;
    
    cmd.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    
    // Bind pipeline
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, shadowPipeline);
    
    // Render scene from light's perspective for each cascade
    for (size_t i = 0; i < cascades.size(); ++i) {
        // Set viewport and scissor for cascade
        vk::Viewport viewport(0.0f, 0.0f, 
                              static_cast<float>(settings.resolution),
                              static_cast<float>(settings.resolution),
                              0.0f, 1.0f);
        cmd.setViewport(0, 1, &viewport);
        
        vk::Rect2D scissor({0, 0}, {settings.resolution, settings.resolution});
        cmd.setScissor(0, 1, &scissor);
        
        // TODO: Bind cascade's viewProj matrix
        // TODO: Render chunks/entities with depth-only pass
    }
    
    cmd.endRenderPass();
}

void ShadowRenderer::endFrame() {
    currentCamera = nullptr;
    currentFrame++;
}

vk::ImageView ShadowRenderer::getShadowMapView() const {
    return shadowMapView;
}

vk::Sampler ShadowRenderer::getShadowSampler() const {
    return shadowSampler;
}

void ShadowRenderer::setResolution(int resolution) {
    settings.resolution = resolution;
    // Recreate shadow map
    if (device) {
        if (shadowMapView) {
            device->getDevice().destroyImageView(shadowMapView);
        }
        if (shadowFramebuffer) {
            device->getDevice().destroyFramebuffer(shadowFramebuffer);
        }
        createShadowMap();
        createFramebuffer();
    }
}

void ShadowRenderer::setMaxDistance(float distance) {
    settings.maxDistance = distance;
}

void ShadowRenderer::setCascadeCount(int count) {
    settings.cascadeCount = count;
    cascades.resize(count);
}

void ShadowRenderer::createPipeline() {
    // TODO: Create shadow rendering pipeline
    // Depth-only pass with appropriate rasterization state
}

void ShadowRenderer::createRenderPass() {
    // TODO: Create render pass for depth-only rendering
}

void ShadowRenderer::createFramebuffer() {
    // TODO: Create framebuffer for shadow map
}

void ShadowRenderer::createShadowMap() {
    // TODO: Create depth image for shadow map
    // Array texture for cascades
}

void ShadowRenderer::createUniformBuffers() {
    // Create uniform buffers for shadow data
    uniformBuffers.resize(frameCount);
    for (auto& buffer : uniformBuffers) {
        // TODO: Create buffer
    }
}

void ShadowRenderer::updateCascades(Camera* camera, const glm::vec3& lightDir) {
    if (!camera) return;
    
    // Calculate cascade splits
    std::vector<float> splits(cascades.size() + 1);
    float nearPlane = camera->getNearPlane();
    float farPlane = std::min(camera->getFarPlane(), settings.maxDistance);
    
    for (size_t i = 0; i <= cascades.size(); ++i) {
        float p = static_cast<float>(i) / static_cast<float>(cascades.size());
        // Use practical split scheme
        float log = nearPlane * std::pow(farPlane / nearPlane, p);
        float uniform = nearPlane + (farPlane - nearPlane) * p;
        splits[i] = settings.splitLambda * log + (1.0f - settings.splitLambda) * uniform;
    }
    
    // Calculate cascade view-projection matrices
    for (size_t i = 0; i < cascades.size(); ++i) {
        cascades[i].nearPlane = splits[i];
        cascades[i].farPlane = splits[i + 1];
        cascades[i].splitDepth = (splits[i + 1] + splits[i]) * 0.5f;
        
        // Calculate frustum corners for this cascade
        glm::mat4 proj = camera->getProjectionMatrix();
        glm::mat4 view = camera->getViewMatrix();
        
        // Get frustum corners in world space
        std::array<glm::vec4, 8> corners;
        float n = splits[i];
        float f = splits[i + 1];
        float aspect = camera->getAspectRatio();
        float fov = camera->getFOV();
        
        float tanHalfFov = std::tan(fov * 0.5f);
        float xn = n * tanHalfFov * aspect;
        float xf = f * tanHalfFov * aspect;
        float yn = n * tanHalfFov;
        float yf = f * tanHalfFov;
        
        corners[0] = glm::vec4(-xn, yn, n, 1.0f);
        corners[1] = glm::vec4(xn, yn, n, 1.0f);
        corners[2] = glm::vec4(xn, -yn, n, 1.0f);
        corners[3] = glm::vec4(-xn, -yn, n, 1.0f);
        corners[4] = glm::vec4(-xf, yf, f, 1.0f);
        corners[5] = glm::vec4(xf, yf, f, 1.0f);
        corners[6] = glm::vec4(xf, -yf, f, 1.0f);
        corners[7] = glm::vec4(-xf, -yf, f, 1.0f);
        
        // Transform to world space
        glm::mat4 invView = glm::inverse(view);
        glm::vec3 center(0.0f);
        for (auto& corner : corners) {
            corner = invView * corner;
            corner /= corner.w;
            center += glm::vec3(corner);
        }
        center /= 8.0f;
        
        // Calculate bounding sphere
        float radius = 0.0f;
        for (const auto& corner : corners) {
            float dist = glm::length(glm::vec3(corner) - center);
            radius = std::max(radius, dist);
        }
        cascades[i].boundingRadius = radius;
        
        // Create light view matrix
        glm::vec3 lightPos = center - lightDir * radius;
        cascades[i].view = glm::lookAt(lightPos, center, glm::vec3(0.0f, 1.0f, 0.0f));
        
        // Create orthographic projection
        float size = radius * 2.0f;
        cascades[i].proj = glm::ortho(-radius, radius, -radius, radius, 
                                       0.0f, radius * 4.0f);
        
        cascades[i].viewProj = cascades[i].proj * cascades[i].view;
    }
}

void ShadowRenderer::updateUniformBuffer() {
    // Update uniform data
    uniformData.lightDirection = lightDirection;
    uniformData.shadowDistance = settings.maxDistance;
    uniformData.cascadeCount = static_cast<int>(cascades.size());
    uniformData.depthBias = settings.depthBias;
    uniformData.slopeBias = settings.slopeBias;
    uniformData.pcfSamples = settings.pcfSamples;
    uniformData.pcfRadius = settings.pcfRadius;
    uniformData.softShadows = settings.softShadows ? 1 : 0;
    uniformData.softness = settings.softness;
    
    for (size_t i = 0; i < cascades.size() && i < 4; ++i) {
        uniformData.cascadeViewProj[i] = cascades[i].viewProj;
        uniformData.cascadeSplits[i] = cascades[i].splitDepth;
    }
    
    // TODO: Write to uniform buffer
}

} // namespace VoxelForge
