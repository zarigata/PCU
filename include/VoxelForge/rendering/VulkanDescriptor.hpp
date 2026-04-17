#ifndef VOXELFORGE_RENDERING_VULKANDESCRIPTOR_HPP
#define VOXELFORGE_RENDERING_VULKANDESCRIPTOR_HPP

#include <vulkan/vulkan.hpp>
#include <vector>
#include <memory>

namespace VoxelForge {

class VulkanDevice;

class VulkanDescriptorSetLayout {
public:
    VulkanDescriptorSetLayout() = default;
    ~VulkanDescriptorSetLayout() = default;

    void init(vk::Device device, const std::vector<vk::DescriptorSetLayoutBinding>& bindings);
    void cleanup();

    vk::DescriptorSetLayout get() const { return layout; }
    operator vk::DescriptorSetLayout() const { return layout; }

private:
    vk::Device device;
    vk::DescriptorSetLayout layout;
};

class VulkanDescriptorSetLayoutBuilder {
public:
    VulkanDescriptorSetLayoutBuilder& addBinding(
        uint32_t binding,
        vk::DescriptorType type,
        vk::ShaderStageFlags stageFlags,
        uint32_t count = 1
    );

    VulkanDescriptorSetLayout build(vk::Device device);

private:
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
};

class VulkanDescriptorPool {
public:
    VulkanDescriptorPool() = default;
    ~VulkanDescriptorPool() = default;

    void init(vk::Device device, const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets);
    void cleanup();

    std::vector<vk::DescriptorSet> allocate(vk::DescriptorSetLayout layout, uint32_t count);

    vk::DescriptorPool get() const { return pool; }

private:
    vk::Device device;
    vk::DescriptorPool pool;
};

} // namespace VoxelForge

#endif // VOXELFORGE_RENDERING_VULKANDESCRIPTOR_HPP
