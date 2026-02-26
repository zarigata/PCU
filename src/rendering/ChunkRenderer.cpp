/**
 * @file ChunkRenderer.cpp
 * @brief Chunk rendering system implementation
 */

#include "ChunkRenderer.hpp"
#include <VoxelForge/rendering/VulkanDevice.hpp>
#include <VoxelForge/rendering/TextureAtlas.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/world/ChunkMesher.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace VoxelForge {

ChunkRenderer::ChunkRenderer() = default;

ChunkRenderer::~ChunkRenderer() {
    cleanup();
}

void ChunkRenderer::init(VulkanDevice* device) {
    this->device = device;
    
    createDescriptorSets();
    createUniformBuffers();
    createPipeline();
    
    // Create staging buffer
    stagingBuffer = std::make_unique<VulkanRingBuffer>(
        device->getDevice(),
        device->getPhysicalDevice(),
        16 * 1024 * 1024  // 16 MB staging
    );
    
    Logger::info("ChunkRenderer initialized");
}

void ChunkRenderer::cleanup() {
    if (!device) return;
    
    auto vkDevice = device->getDevice();
    vkDevice.waitIdle();
    
    // Clean up chunk meshes
    for (auto& [pos, mesh] : chunkMeshes) {
        if (mesh.vertexBuffer.buffer) {
            VulkanBuffer::destroyBuffer(vkDevice, mesh.vertexBuffer);
        }
        if (mesh.indexBuffer.buffer) {
            VulkanBuffer::destroyBuffer(vkDevice, mesh.indexBuffer);
        }
    }
    chunkMeshes.clear();
    
    // Clean up uniform buffers
    for (auto& buffer : uniformBuffers) {
        VulkanBuffer::destroyBuffer(vkDevice, buffer);
    }
    uniformBuffers.clear();
    
    // Clean up pipeline
    if (pipeline) {
        vkDevice.destroyPipeline(pipeline);
        pipeline = nullptr;
    }
    if (pipelineLayout) {
        vkDevice.destroyPipelineLayout(pipelineLayout);
        pipelineLayout = nullptr;
    }
    
    descriptorLayout.reset();
    stagingBuffer.reset();
    device = nullptr;
}

void ChunkRenderer::createDescriptorSets() {
    // Create descriptor set layout
    VulkanDescriptorSetLayoutBuilder builder;
    builder.addBinding(0, vk::DescriptorType::eUniformBuffer, 
                       vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 1);
    builder.addBinding(1, vk::DescriptorType::eCombinedImageSampler,
                       vk::ShaderStageFlagBits::eFragment, 1);
    
    descriptorLayout = std::make_unique<VulkanDescriptorSetLayout>();
    descriptorLayout->init(device->getDevice(), builder);
    
    // Create descriptor pool
    vk::DescriptorPoolSize poolSizes[] = {
        {vk::DescriptorType::eUniformBuffer, frameCount},
        {vk::DescriptorType::eCombinedImageSampler, frameCount}
    };
    
    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = frameCount;
    
    auto pool = device->getDevice().createDescriptorPool(poolInfo);
    
    // Allocate descriptor sets
    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorLayout->getLayout();
    
    descriptorSets.resize(frameCount);
    for (uint32_t i = 0; i < frameCount; i++) {
        descriptorSets[i] = device->getDevice().allocateDescriptorSets(allocInfo)[0];
    }
    
    device->getDevice().destroyDescriptorPool(pool);
}

void ChunkRenderer::createUniformBuffers() {
    uniformBuffers.resize(frameCount);
    
    for (uint32_t i = 0; i < frameCount; i++) {
        uniformBuffers[i] = VulkanBuffer::createUniformBuffer(
            device->getDevice(),
            device->getPhysicalDevice(),
            sizeof(ChunkUniformData)
        );
        uniformBuffers[i].map(device->getDevice());
    }
}

void ChunkRenderer::createPipeline() {
    // Create pipeline layout
    vk::PipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.setLayoutCount = 1;
    auto layout = descriptorLayout->getLayout();
    layoutInfo.pSetLayouts = &layout;
    
    // Push constants
    vk::PushConstantRange pushRange{};
    pushRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
    pushRange.offset = 0;
    pushRange.size = sizeof(glm::mat4);  // Model matrix per chunk
    
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;
    
    pipelineLayout = device->getDevice().createPipelineLayout(layoutInfo);
    
    // Create pipeline using builder
    VulkanPipelineBuilder builder(device);
    
    builder.setVertexInput(
        {ChunkVertex::getBindingDescription()},
        ChunkVertex::getAttributeDescriptions()
    );
    
    builder.addShaderStage(vk::ShaderStageFlagBits::eVertex, "chunk.vert");
    builder.addShaderStage(vk::ShaderStageFlagBits::eFragment, "chunk.frag");
    
    builder.setInputAssembly(vk::PrimitiveTopology::eTriangleList);
    builder.setViewport(extent.width, extent.height);
    builder.setRasterizer(vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack);
    builder.setMultisampling(vk::SampleCountFlagBits::e1);
    builder.setDepthStencil(true, true, vk::CompareOp::eLess);
    builder.addColorBlendAttachment();
    
    builder.setLayout(pipelineLayout);
    
    // TODO: Set render pass
    // builder.setRenderPass(renderPass);
    
    // pipeline = builder.build();
}

void ChunkRenderer::beginFrame(vk::CommandBuffer cmd, Camera* camera) {
    currentFrame = (currentFrame + 1) % frameCount;
    updateUniformBuffer(camera);
    uploadPendingMeshes();
    resetStats();
}

void ChunkRenderer::renderChunks(vk::CommandBuffer cmd, World* world) {
    if (!world) return;
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout,
                          0, 1, &descriptorSets[currentFrame], 0, nullptr);
    
    // Render each visible chunk
    for (auto& [pos, mesh] : chunkMeshes) {
        if (!mesh.valid) continue;
        
        // Frustum culling
        if (settings.enableFrustumCulling) {
            // TODO: Get camera from context
            // if (!isChunkVisible(mesh, camera)) {
            //     stats.chunksCulled++;
            //     continue;
            // }
        }
        
        // Push model matrix
        glm::mat4 model = glm::translate(glm::mat4(1.0f), 
            glm::vec3(pos.x * 16, pos.y * 16, pos.z * 16));
        cmd.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex,
                         0, sizeof(glm::mat4), glm::value_ptr(model));
        
        // Bind buffers and draw
        cmd.bindVertexBuffers(0, 1, &mesh.vertexBuffer.buffer, 
                             vk::DeviceSize{0});
        cmd.bindIndexBuffer(mesh.indexBuffer.buffer, 0, vk::IndexType::eUint32);
        cmd.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
        
        // Update stats
        stats.chunksRendered++;
        stats.drawCalls++;
        stats.verticesRendered += mesh.vertexCount;
        stats.trianglesRendered += mesh.indexCount / 3;
    }
}

void ChunkRenderer::endFrame() {
    // Nothing to do
}

void ChunkRenderer::updateUniformBuffer(Camera* camera) {
    if (!camera) return;
    
    ChunkUniformData data{};
    data.viewProj = camera->getViewProjection();
    data.view = camera->getView();
    data.projection = camera->getProjection();
    data.cameraPos = camera->getPosition();
    data.time = 0.0f;  // TODO: Get actual time
    data.fogStart = 50.0f;
    data.fogEnd = static_cast<float>(settings.renderDistance) * 16.0f;
    data.fogColor = glm::vec4(0.6f, 0.8f, 1.0f, 1.0f);
    data.renderDistance = settings.renderDistance;
    
    uniformBuffers[currentFrame].writeToBuffer(device->getDevice(), &data, sizeof(data));
}

void ChunkRenderer::uploadChunkMesh(ChunkMesh* mesh, const glm::ivec3& chunkPos) {
    if (!mesh) return;
    
    pendingUploads.push_back({mesh, chunkPos});
}

void ChunkRenderer::removeChunkMesh(const glm::ivec3& chunkPos) {
    auto it = chunkMeshes.find(chunkPos);
    if (it != chunkMeshes.end()) {
        if (it->second.vertexBuffer.buffer) {
            VulkanBuffer::destroyBuffer(device->getDevice(), it->second.vertexBuffer);
        }
        if (it->second.indexBuffer.buffer) {
            VulkanBuffer::destroyBuffer(device->getDevice(), it->second.indexBuffer);
        }
        chunkMeshes.erase(it);
    }
}

void ChunkRenderer::updateChunkMesh(ChunkMesh* mesh, const glm::ivec3& chunkPos) {
    removeChunkMesh(chunkPos);
    uploadChunkMesh(mesh, chunkPos);
}

void ChunkRenderer::uploadPendingMeshes() {
    int uploaded = 0;
    
    for (auto& [mesh, pos] : pendingUploads) {
        if (uploaded >= settings.maxChunksPerFrame) break;
        
        if (!mesh || mesh->vertices.empty()) continue;
        
        ChunkMeshGPU gpuMesh{};
        gpuMesh.chunkPos = pos;
        gpuMesh.vertexCount = static_cast<uint32_t>(mesh->vertices.size());
        gpuMesh.indexCount = static_cast<uint32_t>(mesh->indices.size());
        gpuMesh.valid = true;
        
        // Calculate bounding sphere
        glm::vec3 center(0.0f);
        for (const auto& v : mesh->vertices) {
            center += v.position;
        }
        center /= static_cast<float>(mesh->vertices.size());
        gpuMesh.center = center + glm::vec3(pos.x * 16 + 8, pos.y * 16 + 8, pos.z * 16 + 8);
        gpuMesh.radius = 16.0f * glm::sqrt(3.0f);  // Diagonal of chunk
        
        // Create vertex buffer
        vk::DeviceSize vertexSize = mesh->vertices.size() * sizeof(ChunkVertex);
        gpuMesh.vertexBuffer = VulkanBuffer::createVertexBuffer(
            device->getDevice(),
            device->getPhysicalDevice(),
            device->getGraphicsQueue(),
            device->getQueueFamilies().graphicsFamily.value(),
            mesh->vertices.data(),
            vertexSize
        );
        
        // Create index buffer
        vk::DeviceSize indexSize = mesh->indices.size() * sizeof(uint32_t);
        gpuMesh.indexBuffer = VulkanBuffer::createIndexBuffer(
            device->getDevice(),
            device->getPhysicalDevice(),
            device->getGraphicsQueue(),
            device->getQueueFamilies().graphicsFamily.value(),
            mesh->indices.data(),
            indexSize
        );
        
        chunkMeshes[pos] = std::move(gpuMesh);
        uploaded++;
        stats.chunksUploaded++;
    }
    
    pendingUploads.clear();
}

bool ChunkRenderer::isChunkVisible(const ChunkMeshGPU& mesh, const Camera* camera) const {
    // Simple sphere frustum test
    if (!camera) return true;
    
    // TODO: Implement proper frustum culling
    // For now, just check distance
    float dist = glm::distance(mesh.center, camera->getPosition());
    return dist < (settings.renderDistance * 16.0f + mesh.radius);
}

void ChunkRenderer::resetStats() {
    stats = {};
}

void ChunkRenderer::onResize(uint32_t width, uint32_t height) {
    extent.width = width;
    extent.height = height;
    
    // Recreate pipeline with new viewport
    if (pipeline) {
        device->getDevice().destroyPipeline(pipeline);
        pipeline = nullptr;
        createPipeline();
    }
}

} // namespace VoxelForge
