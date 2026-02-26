/**
 * @file EntityRenderer.cpp
 * @brief Entity rendering system implementation
 */

#include "EntityRenderer.hpp"
#include <VoxelForge/rendering/VulkanDevice.hpp>
#include <VoxelForge/entity/EntityManager.hpp>
#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace VoxelForge {

EntityRenderer::EntityRenderer() = default;

EntityRenderer::~EntityRenderer() {
    cleanup();
}

void EntityRenderer::init(VulkanDevice* device) {
    this->device = device;
    
    createDescriptorSets();
    createUniformBuffers();
    createPipeline();
    
    // Create instance buffers
    instanceBuffers.resize(frameCount);
    for (uint32_t i = 0; i < frameCount; i++) {
        vk::DeviceSize size = maxInstances * sizeof(EntityInstanceData);
        instanceBuffers[i] = VulkanBuffer::createStorageBuffer(
            device->getDevice(),
            device->getPhysicalDevice(),
            size
        );
        instanceBuffers[i].map(device->getDevice());
    }
    
    Logger::info("EntityRenderer initialized");
}

void EntityRenderer::cleanup() {
    if (!device) return;
    
    auto vkDevice = device->getDevice();
    vkDevice.waitIdle();
    
    // Clean up models
    for (auto& [name, model] : models) {
        if (model->vertexBuffer.buffer) {
            VulkanBuffer::destroyBuffer(vkDevice, model->vertexBuffer);
        }
        if (model->indexBuffer.buffer) {
            VulkanBuffer::destroyBuffer(vkDevice, model->indexBuffer);
        }
    }
    models.clear();
    
    // Clean up uniform buffers
    for (auto& buffer : uniformBuffers) {
        VulkanBuffer::destroyBuffer(vkDevice, buffer);
    }
    uniformBuffers.clear();
    
    // Clean up instance buffers
    for (auto& buffer : instanceBuffers) {
        VulkanBuffer::destroyBuffer(vkDevice, buffer);
    }
    instanceBuffers.clear();
    
    // Clean up pipeline
    if (pipeline) {
        vkDevice.destroyPipeline(pipeline);
        pipeline = nullptr;
    }
    if (pipelineLayout) {
        vkDevice.destroyPipelineLayout(pipelineLayout);
        pipelineLayout = nullptr;
    }
    if (descriptorLayout) {
        vkDevice.destroyDescriptorSetLayout(descriptorLayout);
        descriptorLayout = nullptr;
    }
    
    device = nullptr;
}

void EntityRenderer::createDescriptorSets() {
    // Create descriptor set layout
    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        {0, vk::DescriptorType::eUniformBuffer, 1, 
         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
        {1, vk::DescriptorType::eStorageBuffer, 1,
         vk::ShaderStageFlagBits::eVertex},
        {2, vk::DescriptorType::eCombinedImageSampler, 1,
         vk::ShaderStageFlagBits::eFragment}
    };
    
    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    descriptorLayout = device->getDevice().createDescriptorSetLayout(layoutInfo);
    
    // Create descriptor pool
    vk::DescriptorPoolSize poolSizes[] = {
        {vk::DescriptorType::eUniformBuffer, frameCount},
        {vk::DescriptorType::eStorageBuffer, frameCount},
        {vk::DescriptorType::eCombinedImageSampler, frameCount}
    };
    
    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.poolSizeCount = 3;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = frameCount;
    
    auto pool = device->getDevice().createDescriptorPool(poolInfo);
    
    // Allocate descriptor sets
    std::vector<vk::DescriptorSetLayout> layouts(frameCount, descriptorLayout);
    
    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = frameCount;
    allocInfo.pSetLayouts = layouts.data();
    
    descriptorSets = device->getDevice().allocateDescriptorSets(allocInfo);
    
    device->getDevice().destroyDescriptorPool(pool);
}

void EntityRenderer::createUniformBuffers() {
    uniformBuffers.resize(frameCount);
    
    for (uint32_t i = 0; i < frameCount; i++) {
        uniformBuffers[i] = VulkanBuffer::createUniformBuffer(
            device->getDevice(),
            device->getPhysicalDevice(),
            sizeof(EntityUniformData)
        );
        uniformBuffers[i].map(device->getDevice());
    }
}

void EntityRenderer::createPipeline() {
    // Create pipeline layout
    vk::PipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorLayout;
    
    pipelineLayout = device->getDevice().createPipelineLayout(layoutInfo);
    
    // Create pipeline using builder
    VulkanPipelineBuilder builder(device);
    
    builder.setVertexInput(
        {EntityVertex::getBindingDescription()},
        EntityVertex::getAttributeDescriptions()
    );
    
    builder.addShaderStage(vk::ShaderStageFlagBits::eVertex, "entity.vert");
    builder.addShaderStage(vk::ShaderStageFlagBits::eFragment, "entity.frag");
    
    builder.setInputAssembly(vk::PrimitiveTopology::eTriangleList);
    builder.setViewport(extent.width, extent.height);
    builder.setRasterizer(vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack);
    builder.setMultisampling(vk::SampleCountFlagBits::e1);
    builder.setDepthStencil(true, true, vk::CompareOp::eLess);
    builder.addColorBlendAttachment(vk::BlendOp::eAdd, vk::BlendFactor::eSrcAlpha, 
                                    vk::BlendFactor::eOneMinusSrcAlpha);
    
    builder.setLayout(pipelineLayout);
    
    // TODO: Set render pass and build
    // pipeline = builder.build();
}

void EntityRenderer::beginFrame(vk::CommandBuffer cmd, Camera* camera) {
    currentFrame = (currentFrame + 1) % frameCount;
    updateUniformBuffer(camera);
    resetStats();
}

void EntityRenderer::renderEntities(vk::CommandBuffer cmd, EntityManager* entityManager) {
    if (!entityManager) return;
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout,
                          0, 1, &descriptorSets[currentFrame], 0, nullptr);
    
    // TODO: Iterate through entities and render
    // For now, just use individual rendering
    
    // auto entities = entityManager->getAllEntities();
    // for (auto* entity : entities) {
    //     renderEntity(cmd, entity);
    // }
}

void EntityRenderer::endFrame() {
    // Nothing to do
}

void EntityRenderer::updateUniformBuffer(Camera* camera) {
    if (!camera) return;
    
    EntityUniformData data{};
    data.viewProj = camera->getViewProjection();
    data.view = camera->getView();
    data.projection = camera->getProjection();
    data.cameraPos = camera->getPosition();
    data.time = 0.0f;  // TODO: Get actual time
    data.lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
    data.lightIntensity = 1.0f;
    data.enableShadows = 1;
    
    uniformBuffers[currentFrame].writeToBuffer(device->getDevice(), &data, sizeof(data));
}

void EntityRenderer::renderEntity(vk::CommandBuffer cmd, Entity* entity) {
    if (!entity) return;
    
    // Get model matrix from entity
    // glm::mat4 model = entity->getTransform().getModelMatrix();
    glm::mat4 model = glm::mat4(1.0f);  // TODO: Get from entity
    
    cmd.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex,
                     0, sizeof(glm::mat4), glm::value_ptr(model));
    
    // Get model mesh
    // std::string modelName = entity->getModelName();
    // EntityModel* model = getModel(modelName);
    // if (!model) return;
    
    // Bind and draw
    // cmd.bindVertexBuffers(0, 1, &model->vertexBuffer.buffer, vk::DeviceSize{0});
    // cmd.bindIndexBuffer(model->indexBuffer.buffer, 0, vk::IndexType::eUint32);
    // cmd.drawIndexed(model->indexCount, 1, 0, 0, 0);
    
    stats.entitiesRendered++;
    stats.drawCalls++;
}

void EntityRenderer::renderInstanced(vk::CommandBuffer cmd, const std::vector<Entity*>& entities) {
    if (entities.empty()) return;
    
    // Collect instance data
    std::vector<EntityInstanceData> instanceData;
    instanceData.reserve(entities.size());
    
    for (auto* entity : entities) {
        EntityInstanceData data{};
        // data.model = entity->getTransform().getModelMatrix();
        data.model = glm::mat4(1.0f);
        data.color = glm::vec4(1.0f);
        data.texIndex = 0;
        data.entityId = 0;  // entity->getId();
        instanceData.push_back(data);
    }
    
    // Upload instance data
    memcpy(instanceBuffers[currentFrame].mapped, instanceData.data(),
           instanceData.size() * sizeof(EntityInstanceData));
    
    // Draw instanced
    // TODO: Implement instanced rendering
}

void EntityRenderer::loadModel(const std::string& name, const std::string& path) {
    auto model = std::make_unique<EntityModel>();
    model->name = name;
    
    // TODO: Load model from file (OBJ, GLTF, etc.)
    // For now, create a simple cube
    
    // Cube vertices
    std::vector<EntityVertex> vertices = {
        // Front face
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
        // Back, top, bottom, right, left faces...
        // ... (abbreviated for space)
    };
    
    std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,  // Front
        // ... other faces
    };
    
    model->vertexCount = static_cast<uint32_t>(vertices.size());
    model->indexCount = static_cast<uint32_t>(indices.size());
    
    // Create buffers
    model->vertexBuffer = VulkanBuffer::createVertexBuffer(
        device->getDevice(),
        device->getPhysicalDevice(),
        device->getGraphicsQueue(),
        device->getQueueFamilies().graphicsFamily.value(),
        vertices.data(),
        vertices.size() * sizeof(EntityVertex)
    );
    
    model->indexBuffer = VulkanBuffer::createIndexBuffer(
        device->getDevice(),
        device->getPhysicalDevice(),
        device->getGraphicsQueue(),
        device->getQueueFamilies().graphicsFamily.value(),
        indices.data(),
        indices.size() * sizeof(uint32_t)
    );
    
    models[name] = std::move(model);
    Logger::debug("Loaded entity model: {}", name);
}

void EntityRenderer::unloadModel(const std::string& name) {
    auto it = models.find(name);
    if (it != models.end()) {
        if (it->second->vertexBuffer.buffer) {
            VulkanBuffer::destroyBuffer(device->getDevice(), it->second->vertexBuffer);
        }
        if (it->second->indexBuffer.buffer) {
            VulkanBuffer::destroyBuffer(device->getDevice(), it->second->indexBuffer);
        }
        models.erase(it);
    }
}

EntityModel* EntityRenderer::getModel(const std::string& name) {
    auto it = models.find(name);
    if (it != models.end()) {
        return it->second.get();
    }
    return nullptr;
}

void EntityRenderer::resetStats() {
    stats = {};
}

void EntityRenderer::onResize(uint32_t width, uint32_t height) {
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
