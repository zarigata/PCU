#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

namespace VoxelForge {

class VulkanDevice; // forward declaration

// Lightweight stub of a Vulkan pipeline builder used for compilation.
class VulkanPipelineBuilder {
public:
    explicit VulkanPipelineBuilder(VulkanDevice* device) : device_(device) {}

    VulkanPipelineBuilder& setVertexInput(const std::vector<vk::VertexInputBindingDescription>& bindings,
                                        const std::vector<vk::VertexInputAttributeDescription>& attributes) {
        return *this;
    }

    VulkanPipelineBuilder& addShaderStage(vk::ShaderStageFlagBits stage, const char* /*name*/) {
        return *this;
    }

    VulkanPipelineBuilder& setInputAssembly(vk::PrimitiveTopology topology) {
        (void)topology; return *this;
    }

    VulkanPipelineBuilder& setViewport(uint32_t width, uint32_t height) {
        (void)width; (void)height; return *this;
    }

    VulkanPipelineBuilder& setRasterizer(vk::PolygonMode polygonMode, vk::CullModeFlagBits cullMode) {
        (void)polygonMode; (void)cullMode; return *this;
    }

    VulkanPipelineBuilder& setMultisampling(vk::SampleCountFlagBits samples) {
        (void)samples; return *this;
    }

    VulkanPipelineBuilder& setDepthStencil(bool depthTest, bool depthWrite, vk::CompareOp compareOp) {
        (void)depthTest; (void)depthWrite; (void)compareOp; return *this;
    }

    VulkanPipelineBuilder& addColorBlendAttachment() { return *this; }

    VulkanPipelineBuilder& addColorBlendAttachment(vk::BlendOp blendOp,
                                                    vk::BlendFactor srcFactor,
                                                    vk::BlendFactor dstFactor) {
        (void)blendOp; (void)srcFactor; (void)dstFactor; return *this;
    }

    VulkanPipelineBuilder& setLayout(vk::PipelineLayout layout) {
        (void)layout; return *this;
    }

    VulkanPipelineBuilder& setRenderPass(void* /*renderPass*/) { return *this; }

    vk::Pipeline build() {
        // Return a default/null pipeline handle as a stub
        return vk::Pipeline{};
    }

private:
    VulkanDevice* device_;
};

} // namespace VoxelForge
