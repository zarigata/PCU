/**
 * @file DimensionRegistry.cpp
 * @brief Dimension registry implementation
 */

#include <VoxelForge/world/DimensionRegistry.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

DimensionRegistry& DimensionRegistry::get() {
    static DimensionRegistry instance;
    return instance;
}

DimensionRegistry::DimensionRegistry() {
    registerVanillaDimensions();
}

void DimensionRegistry::registerVanillaDimensions() {
    // Overworld
    auto overworld = std::make_unique<Dimension>(0, "minecraft:overworld");
    overworld->setMinHeight(-64);
    overworld->setMaxHeight(320);
    overworld->setHasSky(true);
    overworld->setHasCeiling(false);
    overworld->setBedrockFloor(true);
    overworld->setBedrockCeiling(false);
    registerDimension(std::move(overworld));
    
    // Nether
    auto nether = std::make_unique<Dimension>(-1, "minecraft:the_nether");
    nether->setMinHeight(0);
    nether->setMaxHeight(256);
    nether->setHasSky(false);
    nether->setHasCeiling(true);
    nether->setBedrockFloor(true);
    nether->setBedrockCeiling(true);
    registerDimension(std::move(nether));
    
    // End
    auto end = std::make_unique<Dimension>(1, "minecraft:the_end");
    end->setMinHeight(0);
    end->setMaxHeight(256);
    end->setHasSky(false);
    end->setHasCeiling(false);
    end->setBedrockFloor(false);
    end->setBedrockCeiling(false);
    registerDimension(std::move(end));
    
    LOG_INFO("Registered {} vanilla dimensions", dimensions.size());
}

void DimensionRegistry::registerDimension(std::unique_ptr<Dimension> dimension) {
    int id = dimension->getId();
    dimensions[id] = std::move(dimension);
}

const Dimension* DimensionRegistry::getDimension(int id) const {
    auto it = dimensions.find(id);
    if (it != dimensions.end()) {
        return it->second.get();
    }
    return nullptr;
}

} // namespace VoxelForge
