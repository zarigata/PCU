/**
 * @file BlockRegistry.cpp
 * @brief Block registry implementation
 */

#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <unordered_set>

namespace VoxelForge {

// ============================================
// Block Registry
// ============================================

BlockRegistry& BlockRegistry::get() {
    static BlockRegistry instance;
    return instance;
}

BlockRegistry::BlockRegistry() {
    blocks.reserve(1024);
    
    // Register air first (ID 0)
    BlockDefinition airDef;
    airDef.id = "minecraft:air";
    airDef.name = "Air";
    airDef.material = Material::Air;
    airDef.opaque = false;
    airDef.solid = false;
    airDef.replaceable = true;
    airDef.renderType = RenderType::Air;
    airDef.collisionShape = VoxelShape::empty();
    airDef.outlineShape = VoxelShape::empty();
    blocks.push_back(std::move(airDef));
    idToIndex["minecraft:air"] = 0;
    
    VF_CORE_INFO("Block registry initialized");
}

BlockRegistry::~BlockRegistry() {
    VF_CORE_INFO("Block registry destroyed with {} blocks", blocks.size());
}

BlockID BlockRegistry::registerBlock(const String& id, BlockDefinition definition) {
    // Check if already registered
    auto it = idToIndex.find(id);
    if (it != idToIndex.end()) {
        VF_CORE_WARN("Block {} already registered with ID {}", id, it->second);
        return it->second;
    }
    
    if (nextId >= MAX_BLOCK_ID) {
        VF_CORE_ERROR("Block registry full! Cannot register {}", id);
        return AIR_BLOCK;
    }
    
    BlockID newId = nextId++;
    definition.id = id;
    
    // Generate state variants
    if (!definition.properties.empty()) {
        generateStateVariants(definition);
    }
    
    blocks.push_back(std::move(definition));
    idToIndex[id] = newId;
    
    // Store states
    blockStates[newId] = blocks.back().stateVariants;
    
    VF_CORE_DEBUG("Registered block: {} -> {}", id, newId);
    return newId;
}

BlockID BlockRegistry::registerBlock(const String& id) {
    BlockDefinition def;
    def.id = id;
    def.name = id;
    return registerBlock(id, std::move(def));
}

void BlockRegistry::generateStateVariants(BlockDefinition& def) {
    // Generate all possible state combinations
    def.stateVariants.clear();
    
    if (def.properties.empty()) {
        def.stateVariants.push_back(BlockState(getBlockId(def.id), 0));
        return;
    }
    
    // Calculate total combinations
    int totalCombinations = 1;
    std::vector<int> valueCounts;
    for (const auto& prop : def.properties) {
        int count = 1;
        if (prop.type == BlockProperty::Type::Bool) {
            count = 2;
        } else if (prop.type == BlockProperty::Type::Int) {
            count = prop.maxValue - prop.minValue + 1;
        } else if (prop.type == BlockProperty::Type::Enum || prop.type == BlockProperty::Type::Direction) {
            count = static_cast<int>(prop.values.size());
        }
        valueCounts.push_back(count);
        totalCombinations *= count;
    }
    
    def.stateVariants.reserve(totalCombinations);
    
    // Generate combinations
    std::vector<int> indices(def.properties.size(), 0);
    
    for (int i = 0; i < totalCombinations; i++) {
        PropertyContainer props;
        for (size_t j = 0; j < def.properties.size(); j++) {
            const auto& prop = def.properties[j];
            if (prop.type == BlockProperty::Type::Bool) {
                props.setBool(prop.name, indices[j] == 1);
            } else if (prop.type == BlockProperty::Type::Int) {
                props.setInt(prop.name, prop.minValue + indices[j]);
            } else {
                props.setEnum(prop.name, prop.values[indices[j]]);
            }
        }
        
        def.stateVariants.push_back(BlockState(getBlockId(def.id), props.toHash()));
        
        // Increment indices
        for (int j = static_cast<int>(def.properties.size()) - 1; j >= 0; j--) {
            indices[j]++;
            if (indices[j] < valueCounts[j]) break;
            indices[j] = 0;
        }
    }
    
    VF_CORE_DEBUG("Generated {} state variants for {}", totalCombinations, def.id);
}

BlockID BlockRegistry::getBlockId(const String& id) const {
    auto it = idToIndex.find(id);
    if (it != idToIndex.end()) {
        return it->second;
    }
    return AIR_BLOCK;
}

const BlockDefinition& BlockRegistry::getDefinition(BlockID id) const {
    static BlockDefinition empty;
    if (id >= blocks.size()) {
        return empty;
    }
    return blocks[id];
}

const BlockDefinition& BlockRegistry::getDefinition(const String& id) const {
    return getDefinition(getBlockId(id));
}

BlockState BlockRegistry::getDefaultState(BlockID id) const {
    if (id >= blocks.size()) {
        return BlockState();
    }
    
    const auto& def = blocks[id];
    if (def.stateVariants.empty()) {
        return BlockState(id, 0);
    }
    return def.stateVariants[0];
}

BlockState BlockRegistry::getDefaultState(const String& id) const {
    return getDefaultState(getBlockId(id));
}

BlockState BlockRegistry::getState(BlockID id, const PropertyContainer& properties) const {
    if (id >= blocks.size()) {
        return BlockState();
    }
    
    // Find matching state
    uint64_t hash = properties.toHash();
    const auto& states = blockStates.at(id);
    for (const auto& state : states) {
        if (state.getPropertyHash() == hash) {
            return state;
        }
    }
    
    // Return default if not found
    return getDefaultState(id);
}

const std::vector<BlockState>& BlockRegistry::getAllStates(BlockID id) const {
    static std::vector<BlockState> empty;
    auto it = blockStates.find(id);
    if (it != blockStates.end()) {
        return it->second;
    }
    return empty;
}

std::vector<BlockState> BlockRegistry::getAllStates() const {
    std::vector<BlockState> allStates;
    for (const auto& [id, states] : blockStates) {
        allStates.insert(allStates.end(), states.begin(), states.end());
    }
    return allStates;
}

void BlockRegistry::registerVanillaBlocks() {
    VF_CORE_INFO("Registering vanilla blocks...");
    
    // Stone
    registerBlock("minecraft:stone", BlockDefinition{
        .id = "minecraft:stone",
        .name = "Stone",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Grass Block
    registerBlock("minecraft:grass_block", BlockDefinition{
        .id = "minecraft:grass_block",
        .name = "Grass Block",
        .material = Material::Dirt,
        .hardness = 0.6f,
        .blastResistance = 0.6f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // Dirt
    registerBlock("minecraft:dirt", BlockDefinition{
        .id = "minecraft:dirt",
        .name = "Dirt",
        .material = Material::Dirt,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRAVEL
    });
    
    // Cobblestone
    registerBlock("minecraft:cobblestone", BlockDefinition{
        .id = "minecraft:cobblestone",
        .name = "Cobblestone",
        .material = Material::Stone,
        .hardness = 2.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Oak Planks
    registerBlock("minecraft:oak_planks", BlockDefinition{
        .id = "minecraft:oak_planks",
        .name = "Oak Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Axe,
        .flammable = true,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Oak Log (with axis property)
    BlockDefinition oakLog;
    oakLog.id = "minecraft:oak_log";
    oakLog.name = "Oak Log";
    oakLog.material = Material::Wood;
    oakLog.hardness = 2.0f;
    oakLog.requiredTool = ToolType::Axe;
    oakLog.flammable = true;
    oakLog.sounds = BlockSoundGroup::WOOD;
    oakLog.properties = {
        {"axis", BlockProperty::Type::Direction, {"y", "x", "z"}, 0}
    };
    oakLog.defaultProperties.setEnum("axis", "y");
    registerBlock("minecraft:oak_log", std::move(oakLog));
    
    // Water
    registerBlock("minecraft:water", BlockDefinition{
        .id = "minecraft:water",
        .name = "Water",
        .material = Material::Water,
        .hardness = 100.0f,
        .blastResistance = 100.0f,
        .opaque = false,
        .solid = false,
        .replaceable = true,
        .renderType = RenderType::Translucent,
        .collisionShape = VoxelShape::empty()
    });
    
    // Lava
    registerBlock("minecraft:lava", BlockDefinition{
        .id = "minecraft:lava",
        .name = "Lava",
        .material = Material::Lava,
        .hardness = 100.0f,
        .blastResistance = 100.0f,
        .opaque = false,
        .solid = false,
        .replaceable = true,
        .renderType = RenderType::Translucent,
        .lightEmission = 15,
        .collisionShape = VoxelShape::empty()
    });
    
    // Sand
    registerBlock("minecraft:sand", BlockDefinition{
        .id = "minecraft:sand",
        .name = "Sand",
        .material = Material::Sand,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::SAND
    });
    
    // Bedrock
    registerBlock("minecraft:bedrock", BlockDefinition{
        .id = "minecraft:bedrock",
        .name = "Bedrock",
        .material = Material::Stone,
        .hardness = -1.0f,  // Unbreakable
        .blastResistance = 3600000.0f
    });
    
    // Iron Ore
    registerBlock("minecraft:iron_ore", BlockDefinition{
        .id = "minecraft:iron_ore",
        .name = "Iron Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Coal Ore
    registerBlock("minecraft:coal_ore", BlockDefinition{
        .id = "minecraft:coal_ore",
        .name = "Coal Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Gold Ore
    registerBlock("minecraft:gold_ore", BlockDefinition{
        .id = "minecraft:gold_ore",
        .name = "Gold Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Diamond Ore
    registerBlock("minecraft:diamond_ore", BlockDefinition{
        .id = "minecraft:diamond_ore",
        .name = "Diamond Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Crafting Table
    registerBlock("minecraft:crafting_table", BlockDefinition{
        .id = "minecraft:crafting_table",
        .name = "Crafting Table",
        .material = Material::Wood,
        .hardness = 2.5f,
        .blastResistance = 2.5f,
        .requiredTool = ToolType::Axe,
        .flammable = true,
        .sounds = BlockSoundGroup::WOOD,
        .hasBlockEntity = false
    });
    
    // Furnace
    registerBlock("minecraft:furnace", BlockDefinition{
        .id = "minecraft:furnace",
        .name = "Furnace",
        .material = Material::Stone,
        .hardness = 3.5f,
        .blastResistance = 3.5f,
        .requiredTool = ToolType::Pickaxe,
        .hasBlockEntity = true,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Chest
    registerBlock("minecraft:chest", BlockDefinition{
        .id = "minecraft:chest",
        .name = "Chest",
        .material = Material::Wood,
        .hardness = 2.5f,
        .blastResistance = 2.5f,
        .flammable = true,
        .hasBlockEntity = true,
        .sounds = BlockSoundGroup::WOOD,
        .collisionShape = VoxelShape::cube(1, 0, 1, 15, 14, 15)
    });
    
    // Torch
    registerBlock("minecraft:torch", BlockDefinition{
        .id = "minecraft:torch",
        .name = "Torch",
        .material = Material::Decoration,
        .hardness = 0.0f,
        .blastResistance = 0.0f,
        .opaque = false,
        .solid = false,
        .renderType = RenderType::Cutout,
        .lightEmission = 14,
        .collisionShape = VoxelShape::empty()
    });
    
    // Glass
    registerBlock("minecraft:glass", BlockDefinition{
        .id = "minecraft:glass",
        .name = "Glass",
        .material = Material::Glass,
        .hardness = 0.3f,
        .blastResistance = 0.3f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .sounds = BlockSoundGroup::GLASS
    });
    
    VF_CORE_INFO("Registered {} vanilla blocks", blocks.size() - 1);
}

// ============================================
// Property Container
// ============================================

void PropertyContainer::setBool(const String& name, bool value) {
    properties[name] = value ? 1 : 0;
}

void PropertyContainer::setInt(const String& name, int value) {
    properties[name] = value;
}

void PropertyContainer::setEnum(const String& name, const String& value) {
    // Hash the enum value
    properties[name] = static_cast<int>(std::hash<String>{}(value));
}

void PropertyContainer::setDirection(const String& name, int direction) {
    properties[name] = direction;
}

bool PropertyContainer::getBool(const String& name) const {
    auto it = properties.find(name);
    return it != properties.end() ? it->second != 0 : false;
}

int PropertyContainer::getInt(const String& name) const {
    auto it = properties.find(name);
    return it != properties.end() ? it->second : 0;
}

String PropertyContainer::getEnum(const String& name) const {
    // This would need the property definition to reverse lookup
    return "";
}

int PropertyContainer::getDirection(const String& name) const {
    return getInt(name);
}

bool PropertyContainer::has(const String& name) const {
    return properties.find(name) != properties.end();
}

uint64_t PropertyContainer::toHash() const {
    uint64_t hash = 0;
    for (const auto& [key, value] : properties) {
        hash ^= std::hash<String>{}(key) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= static_cast<uint64_t>(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
}

// ============================================
// Voxel Shape
// ============================================

VoxelShape VoxelShape::offset(float dx, float dy, float dz) const {
    std::vector<AABB> offsetBoxes;
    for (const auto& box : boxes) {
        offsetBoxes.push_back(AABB(
            box.min.x + dx, box.min.y + dy, box.min.z + dz,
            box.max.x + dx, box.max.y + dy, box.max.z + dz
        ));
    }
    return VoxelShape(offsetBoxes);
}

VoxelShape VoxelShape::combine(const VoxelShape& other) const {
    auto combined = boxes;
    combined.insert(combined.end(), other.boxes.begin(), other.boxes.end());
    return VoxelShape(combined);
}

// ============================================
// Block Sound Groups
// ============================================

namespace BlockSoundGroups {
    BlockSoundGroup STONE = {"block.stone.break", "block.stone.step", "block.stone.place", "block.stone.hit", "block.stone.fall"};
    BlockSoundGroup WOOD = {"block.wood.break", "block.wood.step", "block.wood.place", "block.wood.hit", "block.wood.fall"};
    BlockSoundGroup GRAVEL = {"block.gravel.break", "block.gravel.step", "block.gravel.place", "block.gravel.hit", "block.gravel.fall"};
    BlockSoundGroup GRASS = {"block.grass.break", "block.grass.step", "block.grass.place", "block.grass.hit", "block.grass.fall"};
    BlockSoundGroup SAND = {"block.sand.break", "block.sand.step", "block.sand.place", "block.sand.hit", "block.sand.fall"};
    BlockSoundGroup GLASS = {"block.glass.break", "block.glass.step", "block.glass.place", "block.glass.hit", "block.glass.fall"};
}

} // namespace VoxelForge
