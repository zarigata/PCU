/**
 * @file VulkanDescriptor.cpp
 * @brief Vulkan descriptor management
 */

#include <VoxelForge/rendering/VulkanDescriptor.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

// ============================================================================
// Descriptor Set Layout Builder
// ============================================================================

VulkanDescriptorSetLayoutBuilder::VulkanDescriptorSetLayoutBuilder(vk::Device device)
    : device_(device) {
}

VulkanDescriptorSetLayoutBuilder& VulkanDescriptorSetLayoutBuilder::addBinding(
    uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stageFlags,
    uint32_t count) {
    
    vk::DescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = count;
    layoutBinding.stageFlags = stageFlags;
    layoutBinding.pImmutableSamplers = nullptr;
    
    bindings_.push_back(layoutBinding);
    return *this;
}

vk::UniqueDescriptorSetLayout VulkanDescriptorSetLayoutBuilder::build() {
    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings_.size());
    layoutInfo.pBindings = bindings_.data();
    
    try {
        return device_.createDescriptorSetLayoutUnique(layoutInfo);
    } catch (const vk::SystemError& e) {
        LOG_ERROR("Failed to create descriptor set layout: {}", e.what());
        return vk::UniqueDescriptorSetLayout{};
    }
}

// ============================================================================
// Descriptor Pool Builder
// ============================================================================

VulkanDescriptorPoolBuilder::VulkanDescriptorPoolBuilder(vk::Device device)
    : device_(device)
    , maxSets_(1000) {
}

VulkanDescriptorPoolBuilder& VulkanDescriptorPoolBuilder::addPoolSize(
    vk::DescriptorType type, uint32_t count) {
    
    vk::DescriptorPoolSize poolSize{};
    poolSize.type = type;
    poolSize.descriptorCount = count;
    poolSizes_.push_back(poolSize);
    return *this;
}

VulkanDescriptorPoolBuilder& VulkanDescriptorPoolBuilder::setMaxSets(uint32_t count) {
    maxSets_ = count;
    return *this;
}

VulkanDescriptorPoolBuilder& VulkanDescriptorPoolBuilder::setPoolFlags(vk::DescriptorPoolCreateFlags flags) {
    poolFlags_ = flags;
    return *this;
}

vk::UniqueDescriptorPool VulkanDescriptorPoolBuilder::build() {
    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = vk::StructureType::eDescriptorPoolCreateInfo;
    poolInfo.flags = poolFlags_;
    poolInfo.maxSets = maxSets_;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes_.size());
    poolInfo.pPoolSizes = poolSizes_.data();
    
    try {
        return device_.createDescriptorPoolUnique(poolInfo);
    } catch (const vk::SystemError& e) {
        LOG_ERROR("Failed to create descriptor pool: {}", e.what());
        return vk::UniqueDescriptorPool{};
    }
}

// ============================================================================
// Descriptor Writer
// ============================================================================

VulkanDescriptorWriter::VulkanDescriptorWriter(
    vk::DescriptorSetLayout layout, vk::DescriptorPool pool)
    : layout_(layout)
    , pool_(pool) {
}

VulkanDescriptorWriter& VulkanDescriptorWriter::writeBuffer(
    uint32_t binding, vk::DescriptorType type, const vk::DescriptorBufferInfo& bufferInfo) {
    
    vk::WriteDescriptorSet write{};
    write.sType = vk::StructureType::eWriteDescriptorSet;
    write.dstSet = vk::DescriptorSet{}; // Set later
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorType = type;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfos_.emplace_back(bufferInfo);
    
    writes_.push_back(write);
    return *this;
}

VulkanDescriptorWriter& VulkanDescriptorWriter::writeImage(
    uint32_t binding, vk::DescriptorType type, const vk::DescriptorImageInfo& imageInfo) {
    
    vk::WriteDescriptorSet write{};
    write.sType = vk::StructureType::eWriteDescriptorSet;
    write.dstSet = vk::DescriptorSet{}; // Set later
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorType = type;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfos_.emplace_back(imageInfo);
    
    writes_.push_back(write);
    return *this;
}

vk::DescriptorSet VulkanDescriptorWriter::build(vk::Device device) {
    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eDescriptorSetAllocateInfo;
    allocInfo.descriptorPool = pool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout_;
    
    vk::DescriptorSet descriptorSet;
    try {
        descriptorSet = device.allocateDescriptorSets(allocInfo)[0];
    } catch (const vk::SystemError& e) {
        LOG_ERROR("Failed to allocate descriptor set: {}", e.what());
        return vk::DescriptorSet{};
    }
    
    // Update all writes with the allocated set
    for (auto& write : writes_) {
        write.dstSet = descriptorSet;
    }
    
    device.updateDescriptorSets(
        static_cast<uint32_t>(writes_.size()),
        writes_.data(),
        0, nullptr
    );
    
    return descriptorSet;
}

void VulkanDescriptorWriter::overwrite(vk::Device device, vk::DescriptorSet set) {
    for (auto& write : writes_) {
        write.dstSet = set;
    }
    
    device.updateDescriptorSets(
        static_cast<uint32_t>(writes_.size()),
        writes_.data(),
        0, nullptr
    );
}

// ============================================================================
// Descriptor Set Manager
// ============================================================================

DescriptorSetManager::DescriptorSetManager(vk::Device device)
    : device_(device) {
    LOG_INFO("DescriptorSetManager created");
}

DescriptorSetManager::~DescriptorSetManager() {
    pools_.clear();
    layouts_.clear();
    LOG_INFO("DescriptorSetManager destroyed");
}

void DescriptorSetManager::createLayout(const std::string& name,
    const std::vector<vk::DescriptorSetLayoutBinding>& bindings) {
    
    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    layouts_[name] = device_.createDescriptorSetLayoutUnique(layoutInfo);
    LOG_DEBUG("Created descriptor set layout: {}", name);
}

vk::DescriptorSetLayout DescriptorSetManager::getLayout(const std::string& name) const {
    auto it = layouts_.find(name);
    if (it != layouts_.end()) {
        return it->second.get();
    }
    LOG_WARN("Descriptor set layout not found: {}", name);
    return vk::DescriptorSetLayout{};
}

void DescriptorSetManager::createPool(const std::string& name,
    const std::vector<vk::DescriptorPoolSize>& poolSizes, uint32_t maxSets) {
    
    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = vk::StructureType::eDescriptorPoolCreateInfo;
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    
    pools_[name] = device_.createDescriptorPoolUnique(poolInfo);
    LOG_DEBUG("Created descriptor pool: {}", name);
}

vk::DescriptorPool DescriptorSetManager::getPool(const std::string& name) const {
    auto it = pools_.find(name);
    if (it != pools_.end()) {
        return it->second.get();
    }
    LOG_WARN("Descriptor pool not found: {}", name);
    return vk::DescriptorPool{};
}

vk::DescriptorSet DescriptorSetManager::allocate(const std::string& layoutName,
    const std::string& poolName) {
    
    auto layout = getLayout(layoutName);
    auto pool = getPool(poolName);
    
    if (!layout || !pool) {
        LOG_ERROR("Failed to allocate descriptor set: layout or pool not found");
        return vk::DescriptorSet{};
    }
    
    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eDescriptorSetAllocateInfo;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;
    
    try {
        return device_.allocateDescriptorSets(allocInfo)[0];
    } catch (const vk::SystemError& e) {
        LOG_ERROR("Failed to allocate descriptor set: {}", e.what());
        return vk::DescriptorSet{};
    }
}

void DescriptorSetManager::free(vk::DescriptorSet set, const std::string& poolName) {
    auto pool = getPool(poolName);
    if (pool) {
        device_.freeDescriptorSets(pool, 1, &set);
    }
}

} // namespace VoxelForge
