#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>

namespace VoxelForge {

class VulkanDevice;

class VulkanPipelineBuilder {
public:
    explicit VulkanPipelineBuilder(vk::Device device);
    explicit VulkanPipelineBuilder(VulkanDevice*) : VulkanPipelineBuilder(vk::Device{}) {}

    VulkanPipelineBuilder& setShaders(vk::ShaderModule vertexShader, vk::ShaderModule fragmentShader);

    VulkanPipelineBuilder& setVertexInput(
        const vk::VertexInputBindingDescription& binding,
        const std::vector<vk::VertexInputAttributeDescription>& attributes);

    VulkanPipelineBuilder& setInputTopology(vk::PrimitiveTopology topology);

    VulkanPipelineBuilder& setViewport(float x, float y, float width, float height,
                                        float minDepth = 0.0f, float maxDepth = 1.0f);

    VulkanPipelineBuilder& setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

    VulkanPipelineBuilder& setRasterizer(vk::PolygonMode polygonMode,
                                          vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack,
                                          vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise);

    VulkanPipelineBuilder& setMultisampling(vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1);

    VulkanPipelineBuilder& setDepthStencil(bool depthTest, bool depthWrite,
                                            vk::CompareOp compareOp = vk::CompareOp::eLess);

    VulkanPipelineBuilder& setColorBlendAttachment(
        vk::ColorComponentFlags colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        bool blendEnable = false,
        vk::BlendFactor srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        vk::BlendFactor dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        vk::BlendOp colorBlendOp = vk::BlendOp::eAdd,
        vk::BlendFactor srcAlphaBlendFactor = vk::BlendFactor::eOne,
        vk::BlendFactor dstAlphaBlendFactor = vk::BlendFactor::eZero,
        vk::BlendOp alphaBlendOp = vk::BlendOp::eAdd);

    VulkanPipelineBuilder& addDescriptorSetLayout(vk::DescriptorSetLayout layout);

    VulkanPipelineBuilder& setRenderPass(vk::RenderPass renderPass, uint32_t subpass = 0);

    VulkanPipelineBuilder& addPushConstantRange(vk::ShaderStageFlags stageFlags,
                                                 uint32_t offset, uint32_t size);

    vk::UniquePipeline build(vk::PipelineCache cache = vk::PipelineCache{});
    vk::PipelineLayout getPipelineLayout() const;

    VulkanPipelineBuilder& setInputAssembly(vk::PrimitiveTopology topology) {
        return setInputTopology(topology);
    }
    VulkanPipelineBuilder& setViewport(uint32_t w, uint32_t h) {
        return setViewport(0.0f, 0.0f, (float)w, (float)h);
    }
    VulkanPipelineBuilder& addShaderStage(vk::ShaderStageFlagBits, const char*) { return *this; }
    VulkanPipelineBuilder& addColorBlendAttachment() { return setColorBlendAttachment(); }
    VulkanPipelineBuilder& addColorBlendAttachment(vk::BlendOp, vk::BlendFactor, vk::BlendFactor) { return *this; }
    VulkanPipelineBuilder& setLayout(vk::PipelineLayout) { return *this; }

private:
    vk::Device device_;

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages_;
    vk::VertexInputBindingDescription vertexBinding_;
    std::vector<vk::VertexInputAttributeDescription> vertexAttributes_;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly_;
    vk::Viewport viewport_;
    vk::Rect2D scissor_;
    vk::PipelineRasterizationStateCreateInfo rasterizer_;
    vk::PipelineMultisampleStateCreateInfo multisampling_;
    vk::PipelineDepthStencilStateCreateInfo depthStencil_;
    vk::PipelineColorBlendAttachmentState colorBlendAttachment_;
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts_;
    std::vector<vk::PushConstantRange> pushConstantRanges_;
    std::vector<vk::DynamicState> dynamicStates_;
    vk::RenderPass renderPass_;
    uint32_t subpass_ = 0;
    vk::UniquePipelineLayout pipelineLayout_;
};

namespace VulkanShader {
    vk::UniqueShaderModule loadFromFile(vk::Device device, const std::string& filename);
    vk::UniqueShaderModule loadFromMemory(vk::Device device, const uint32_t* code, size_t size);
}

} // namespace VoxelForge
