/**
 * @file DimensionRegistry.hpp
 * @brief Dimension registry for managing game dimensions (Overworld, Nether, End)
 */

#pragma once

#include <string>
#include <memory>
#include <unordered_map>

namespace VoxelForge {

/**
 * @brief Represents a game dimension (Overworld, Nether, End, or custom)
 */
class Dimension {
public:
    Dimension(int id, const std::string& name)
        : id(id), name(name) {}

    int getId() const { return id; }
    const std::string& getName() const { return name; }

    int getMinHeight() const { return minHeight; }
    void setMinHeight(int h) { minHeight = h; }

    int getMaxHeight() const { return maxHeight; }
    void setMaxHeight(int h) { maxHeight = h; }

    bool hasSky() const { return hasSkyLight; }
    void setHasSky(bool v) { hasSkyLight = v; }

    bool hasCeiling() const { return hasCeiling_; }
    void setHasCeiling(bool v) { hasCeiling_ = v; }

    bool hasBedrockFloor() const { return bedrockFloor; }
    void setBedrockFloor(bool v) { bedrockFloor = v; }

    bool hasBedrockCeiling() const { return bedrockCeiling; }
    void setBedrockCeiling(bool v) { bedrockCeiling = v; }

private:
    int id;
    std::string name;
    int minHeight = 0;
    int maxHeight = 256;
    bool hasSkyLight = true;
    bool hasCeiling_ = false;
    bool bedrockFloor = true;
    bool bedrockCeiling = false;
};

/**
 * @brief Registry for all game dimensions
 */
class DimensionRegistry {
public:
    static DimensionRegistry& get();

    void registerDimension(std::unique_ptr<Dimension> dimension);
    const Dimension* getDimension(int id) const;

private:
    DimensionRegistry();
    void registerVanillaDimensions();

    std::unordered_map<int, std::unique_ptr<Dimension>> dimensions;
};

} // namespace VoxelForge
