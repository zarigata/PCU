/**
 * @file VulkanPipeline.cpp
 * @brief Vulkan pipeline management
 */

#include <VoxelForge/rendering/VulkanPipeline.hpp>
#include <VoxelForge/rendering/VulkanContext.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <fstream>

namespace VoxelForge {

VulkanPipelineBuilder::VulkanPipelineBuilder(vk::Device device) 
    : device_(device) {
    // Set default dynamic states
    dynamicStates_ = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
}

VulkanPipelineBuilder& VulkanPipelineBuilder::setShaders(
    vk::ShaderModule vertexShader, vk::ShaderModule fragmentShader) {
    
    shaderStages_.clear();
    
    vk::PipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    vertStage.stage = vk::ShaderStageFlagBits::eVertex;
    vertStage.module = vertexShader;
    vertStage.pName = "main";
    shaderStages_.push_back(vertStage);
    
    vk::PipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    fragStage.stage = vk::ShaderStageFlagBits::eFragment;
    fragStage.module = fragmentShader;
    fragStage.pName = "main";
    shaderStages_.push_back(fragStage);
    
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::setVertexInput(
    const vk::VertexInputBindingDescription& binding,
    const std::vector<vk::VertexInputAttributeDescription>& attributes) {
    
    vertexBinding_ = binding;
    vertexAttributes_ = attributes;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::setInputTopology(vk::PrimitiveTopology topology) {
    inputAssembly_.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
    inputAssembly_.topology = topology;
    inputAssembly_.primitiveRestartEnable = VK_FALSE;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::setViewport(
    float x, float y, float width, float height, float minDepth, float maxDepth) {
    
    viewport_.x = x;
    viewport_.y = y;
    viewport_.width = width;
    viewport_.height = height;
    viewport_.minDepth = minDepth;
    viewport_.maxDepth = maxDepth;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::setScissor(
    int32_t x, int32_t y, uint32_t width, uint32_t height) {
    
    scissor_.offset = vk::Offset2D{x, y};
    scissor_.extent = vk::Extent2D{width, height};
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::setRasterizer(
    vk::PolygonMode polygonMode, vk::CullModeFlags cullMode, vk::FrontFace frontFace) {
    
    rasterizer_.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
    rasterizer_.depthClampEnable = VK_FALSE;
    rasterizer_.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_.polygonMode = polygonMode;
    rasterizer_.cullMode = cullMode;
    rasterizer_.frontFace = frontFace;
    rasterizer_.depthBiasEnable = VK_FALSE;
    rasterizer_.depthBiasConstantFactor = 0.0f;
    rasterizer_.depthBiasClamp = 0.0f;
    rasterizer_.depthBiasSlopeFactor = 0.0f;
    rasterizer_.lineWidth = 1.0f;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::setMultisampling(vk::SampleCountFlagBits samples) {
    multisampling_.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
    multisampling_.rasterizationSamples = samples;
    multisampling_.sampleShadingEnable = VK_FALSE;
    multisampling_.minSampleShading = 1.0f;
    multisampling_.pSampleMask = nullptr;
    multisampling_.alphaToCoverageEnable = VK_FALSE;
    multisampling_.alphaToOneEnable = VK_FALSE;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::setDepthStencil(bool depthTest, bool depthWrite, vk::CompareOp compareOp) {
    depthStencil_.sType = vk::StructureType::ePipelineDepthStencilStateCreateInfo;
    depthStencil_.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
    depthStencil_.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
    depthStencil_.depthCompareOp = compareOp;
    depthStencil_.depthBoundsTestEnable = VK_FALSE;
    depthStencil_.stencilTestEnable = VK_FALSE;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::setColorBlendAttachment(
    vk::ColorComponentFlags colorWriteMask, bool blendEnable,
    vk::BlendFactor srcColorBlendFactor, vk::BlendFactor dstColorBlendFactor,
    vk::BlendOp colorBlendOp, vk::BlendFactor srcAlphaBlendFactor,
    vk::BlendFactor dstAlphaBlendFactor, vk::BlendOp alphaBlendOp) {
    
    colorBlendAttachment_.colorWriteMask = colorWriteMask;
    colorBlendAttachment_.blendEnable = blendEnable ? VK_TRUE : VK_FALSE;
    colorBlendAttachment_.srcColorBlendFactor = srcColorBlendFactor;
    colorBlendAttachment_.dstColorBlendFactor = dstColorBlendFactor;
    colorBlendAttachment_.colorBlendOp = colorBlendOp;
    colorBlendAttachment_.srcAlphaBlendFactor = srcAlphaBlendFactor;
    colorBlendAttachment_.dstAlphaBlendFactor = dstAlphaBlendFactor;
    colorBlendAttachment_.alphaBlendOp = alphaBlendOp;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::addDescriptorSetLayout(vk::DescriptorSetLayout layout) {
    descriptorSetLayouts_.push_back(layout);
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::setRenderPass(vk::RenderPass renderPass, uint32_t subpass) {
    renderPass_ = renderPass;
    subpass_ = subpass;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::addPushConstantRange(
    vk::ShaderStageFlags stageFlags, uint32_t offset, uint32_t size) {
    
    vk::PushConstantRange range{};
    range.stageFlags = stageFlags;
    range.offset = offset;
    range.size = size;
    pushConstantRanges_.push_back(range);
    return *this;
}

vk::UniquePipeline VulkanPipelineBuilder::build(vk::PipelineCache cache) {
    // Create pipeline layout
    vk::PipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts_.size());
    layoutInfo.pSetLayouts = descriptorSetLayouts_.data();
    layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges_.size());
    layoutInfo.pPushConstantRanges = pushConstantRanges_.data();
    
    pipelineLayout_ = device_.createPipelineLayoutUnique(layoutInfo);
    
    // Viewport state
    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport_;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor_;
    
    // Color blending
    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment_;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    
    // Vertex input state
    vk::PipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &vertexBinding_;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes_.size());
    vertexInput.pVertexAttributeDescriptions = vertexAttributes_.data();
    
    // Dynamic state
    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates_.size());
    dynamicState.pDynamicStates = dynamicStates_.data();
    
    // Create graphics pipeline
    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = vk::StructureType::ePipelineGraphicsPipelineCreateInfo;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages_.size());
    pipelineInfo.pStages = shaderStages_.data();
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly_;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer_;
    pipelineInfo.pMultisampleState = &multisampling_;
    pipelineInfo.pDepthStencilState = &depthStencil_;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout_.get();
    pipelineInfo.renderPass = renderPass_;
    pipelineInfo.subpass = subpass_;
    pipelineInfo.basePipelineHandle = vk::Pipeline{};
    pipelineInfo.basePipelineIndex = -1;
    
    auto result = device_.createGraphicsPipelineUnique(cache, pipelineInfo);
    if (result.result == vk::Result::eSuccess) {
        LOG_DEBUG("Created Vulkan pipeline");
        return std::move(result.value);
    } else {
        LOG_ERROR("Failed to create Vulkan pipeline");
        return vk::UniquePipeline{};
    }
}

vk::PipelineLayout VulkanPipelineBuilder::getPipelineLayout() const {
    return pipelineLayout_.get();
}

// ============================================================================
// Shader Loading
// ============================================================================

vk::UniqueShaderModule VulkanShader::loadFromFile(vk::Device device, const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        LOG_ERROR("Failed to open shader file: {}", filename);
        return vk::UniqueShaderModule{};
    }
    
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();
    
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();
    
    try {
        return device.createShaderModuleUnique(createInfo);
    } catch (const vk::SystemError& e) {
        LOG_ERROR("Failed to create shader module: {}", e.what());
        return vk::UniqueShaderModule{};
    }
}

vk::UniqueShaderModule VulkanShader::loadFromMemory(vk::Device device, const uint32_t* code, size_t size) {
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
    createInfo.codeSize = size;
    createInfo.pCode = code;
    
    try {
        return device.createShaderModuleUnique(createInfo);
    } catch (const vk::SystemError& e) {
        LOG_ERROR("Failed to create shader module: {}", e.what());
        return vk::UniqueShaderModule{};
    }
}

} // namespace VoxelForge
