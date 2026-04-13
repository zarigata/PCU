/**
 * @file ChunkRenderer.cpp
 * @brief Chunk rendering system implementation
 */

#include <VoxelForge/rendering/ChunkRenderer.hpp>
#include <VoxelForge/rendering/VulkanDevice.hpp>
#include <VoxelForge/rendering/TextureAtlas.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/world/ChunkMesher.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace VoxelForge {

// Vulkan vertex binding/attribute descriptions for ChunkVertex
vk::VertexInputBindingDescription getChunkVertexBindingDescription() {
    return {0, sizeof(ChunkVertex), vk::VertexInputRate::eVertex};
}

std::vector<vk::VertexInputAttributeDescription> getChunkVertexAttributeDescriptions() {
    return {
        {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(ChunkVertex, x)},        // Position
        {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(ChunkVertex, nx)},       // Normal
        {2, 0, vk::Format::eR32G32Sfloat, offsetof(ChunkVertex, u)},           // UV
        {3, 0, vk::Format::eR32Sfloat, offsetof(ChunkVertex, ao)},             // Ambient occlusion
        {4, 0, vk::Format::eR32Uint, offsetof(ChunkVertex, light)},            // Packed light
        {5, 0, vk::Format::eR32Uint, offsetof(ChunkVertex, color)}             // Vertex color
    };
}

ChunkRenderer::ChunkRenderer() = default;

ChunkRenderer::~ChunkRenderer() {
    cleanup();
}

void ChunkRenderer::init(VulkanDevice* device) {
    this->device = device;
    
    // Create command pool
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.queueFamilyIndex = device->getQueueFamilies().graphicsFamily.value();
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPool = device->getDevice().createCommandPool(poolInfo);
    
    createDescriptorSets();
    createUniformBuffers();
    createPipeline();
    
    // Create staging buffer
    stagingBuffer = std::make_unique<VulkanRingBuffer>(
        device->getDevice(),
        device->getPhysicalDevice(),
        16 * 1024 * 1024  // 16 MB staging
    );
    
    VF_INFO("ChunkRenderer initialized");
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
    
    // descriptorLayout.reset(); // TODO: Implement VulkanDescriptor
    stagingBuffer.reset();
    device = nullptr;
}

void ChunkRenderer::createDescriptorSets() {
    // TODO: Implement VulkanDescriptorSetLayout and VulkanDescriptorPool
    VF_WARN("createDescriptorSets: Not implemented - VulkanDescriptor classes missing");
    // Stub implementation to allow compilation
    descriptorSets.resize(frameCount);
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
    // TODO: Implement VulkanPipelineBuilder and VulkanPipeline
    VF_WARN("createPipeline: Not implemented - VulkanPipeline classes missing");
    
    // Create basic pipeline layout (without descriptor sets for now)
    vk::PipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.setLayoutCount = 0;
    
    // Push constants
    vk::PushConstantRange pushRange{};
    pushRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
    pushRange.offset = 0;
    pushRange.size = sizeof(glm::mat4);  // Model matrix per chunk
    
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;
    
    pipelineLayout = device->getDevice().createPipelineLayout(layoutInfo);
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
    data.viewProj = camera->getViewProjectionMatrix();
    data.view = camera->getViewMatrix();
    data.projection = camera->getProjectionMatrix();
    data.cameraPos = camera->getPosition();
    data.time = 0.0f;  // TODO: Get actual time
    data.fogStart = 50.0f;
    data.fogEnd = static_cast<float>(settings.renderDistance) * 16.0f;
    data.fogColor = glm::vec4(0.6f, 0.8f, 1.0f, 1.0f);
    data.renderDistance = settings.renderDistance;
    
    uniformBuffers[currentFrame].writeToBuffer(device->getDevice(), &data, sizeof(data));
}

void ChunkRenderer::uploadChunkMesh(ChunkMeshData* mesh, const glm::ivec3& chunkPos) {
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

void ChunkRenderer::updateChunkMesh(ChunkMeshData* mesh, const glm::ivec3& chunkPos) {
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
            center += glm::vec3(v.x, v.y, v.z);
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
            commandPool,
            mesh->vertices.data(),
            vertexSize
        );
        
        // Create index buffer
        vk::DeviceSize indexSize = mesh->indices.size() * sizeof(uint32_t);
        gpuMesh.indexBuffer = VulkanBuffer::createIndexBuffer(
            device->getDevice(),
            device->getPhysicalDevice(),
            device->getGraphicsQueue(),
            commandPool,
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
