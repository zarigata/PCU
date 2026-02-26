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
    
    // ============================================
    // STONE VARIANTS
    // ============================================
    
    // Granite
    registerBlock("minecraft:granite", BlockDefinition{
        .id = "minecraft:granite",
        .name = "Granite",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Polished Granite
    registerBlock("minecraft:polished_granite", BlockDefinition{
        .id = "minecraft:polished_granite",
        .name = "Polished Granite",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Diorite
    registerBlock("minecraft:diorite", BlockDefinition{
        .id = "minecraft:diorite",
        .name = "Diorite",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Polished Diorite
    registerBlock("minecraft:polished_diorite", BlockDefinition{
        .id = "minecraft:polished_diorite",
        .name = "Polished Diorite",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Andesite
    registerBlock("minecraft:andesite", BlockDefinition{
        .id = "minecraft:andesite",
        .name = "Andesite",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Polished Andesite
    registerBlock("minecraft:polished_andesite", BlockDefinition{
        .id = "minecraft:polished_andesite",
        .name = "Polished Andesite",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Deepslate
    registerBlock("minecraft:deepslate", BlockDefinition{
        .id = "minecraft:deepslate",
        .name = "Deepslate",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Cobbled Deepslate
    registerBlock("minecraft:cobbled_deepslate", BlockDefinition{
        .id = "minecraft:cobbled_deepslate",
        .name = "Cobbled Deepslate",
        .material = Material::Stone,
        .hardness = 3.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Tuff
    registerBlock("minecraft:tuff", BlockDefinition{
        .id = "minecraft:tuff",
        .name = "Tuff",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // ============================================
    // STONE BRICKS & BRICKS
    // ============================================
    
    // Stone Bricks
    registerBlock("minecraft:stone_bricks", BlockDefinition{
        .id = "minecraft:stone_bricks",
        .name = "Stone Bricks",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Cracked Stone Bricks
    registerBlock("minecraft:cracked_stone_bricks", BlockDefinition{
        .id = "minecraft:cracked_stone_bricks",
        .name = "Cracked Stone Bricks",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Mossy Stone Bricks
    registerBlock("minecraft:mossy_stone_bricks", BlockDefinition{
        .id = "minecraft:mossy_stone_bricks",
        .name = "Mossy Stone Bricks",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Chiseled Stone Bricks
    registerBlock("minecraft:chiseled_stone_bricks", BlockDefinition{
        .id = "minecraft:chiseled_stone_bricks",
        .name = "Chiseled Stone Bricks",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Bricks
    registerBlock("minecraft:bricks", BlockDefinition{
        .id = "minecraft:bricks",
        .name = "Bricks",
        .material = Material::Stone,
        .hardness = 2.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // ============================================
    // MORE ORES
    // ============================================
    
    // Copper Ore
    registerBlock("minecraft:copper_ore", BlockDefinition{
        .id = "minecraft:copper_ore",
        .name = "Copper Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Deepslate Copper Ore
    registerBlock("minecraft:deepslate_copper_ore", BlockDefinition{
        .id = "minecraft:deepslate_copper_ore",
        .name = "Deepslate Copper Ore",
        .material = Material::Stone,
        .hardness = 4.5f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Emerald Ore
    registerBlock("minecraft:emerald_ore", BlockDefinition{
        .id = "minecraft:emerald_ore",
        .name = "Emerald Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Deepslate Emerald Ore
    registerBlock("minecraft:deepslate_emerald_ore", BlockDefinition{
        .id = "minecraft:deepslate_emerald_ore",
        .name = "Deepslate Emerald Ore",
        .material = Material::Stone,
        .hardness = 4.5f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Redstone Ore
    registerBlock("minecraft:redstone_ore", BlockDefinition{
        .id = "minecraft:redstone_ore",
        .name = "Redstone Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::STONE,
        .lightEmission = 7
    });
    
    // Deepslate Redstone Ore
    registerBlock("minecraft:deepslate_redstone_ore", BlockDefinition{
        .id = "minecraft:deepslate_redstone_ore",
        .name = "Deepslate Redstone Ore",
        .material = Material::Stone,
        .hardness = 4.5f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::STONE,
        .lightEmission = 7
    });
    
    // Lapis Ore
    registerBlock("minecraft:lapis_ore", BlockDefinition{
        .id = "minecraft:lapis_ore",
        .name = "Lapis Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Deepslate Lapis Ore
    registerBlock("minecraft:deepslate_lapis_ore", BlockDefinition{
        .id = "minecraft:deepslate_lapis_ore",
        .name = "Deepslate Lapis Ore",
        .material = Material::Stone,
        .hardness = 4.5f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::STONE
    });
    
    // ============================================
    // METAL BLOCKS
    // ============================================
    
    // Coal Block
    registerBlock("minecraft:coal_block", BlockDefinition{
        .id = "minecraft:coal_block",
        .name = "Block of Coal",
        .material = Material::Stone,
        .hardness = 5.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Iron Block
    registerBlock("minecraft:iron_block", BlockDefinition{
        .id = "minecraft:iron_block",
        .name = "Block of Iron",
        .material = Material::Metal,
        .hardness = 5.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Gold Block
    registerBlock("minecraft:gold_block", BlockDefinition{
        .id = "minecraft:gold_block",
        .name = "Block of Gold",
        .material = Material::Metal,
        .hardness = 3.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Diamond Block
    registerBlock("minecraft:diamond_block", BlockDefinition{
        .id = "minecraft:diamond_block",
        .name = "Block of Diamond",
        .material = Material::Metal,
        .hardness = 5.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Emerald Block
    registerBlock("minecraft:emerald_block", BlockDefinition{
        .id = "minecraft:emerald_block",
        .name = "Block of Emerald",
        .material = Material::Metal,
        .hardness = 5.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Copper Block
    registerBlock("minecraft:copper_block", BlockDefinition{
        .id = "minecraft:copper_block",
        .name = "Block of Copper",
        .material = Material::Metal,
        .hardness = 3.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Lapis Block
    registerBlock("minecraft:lapis_block", BlockDefinition{
        .id = "minecraft:lapis_block",
        .name = "Block of Lapis Lazuli",
        .material = Material::Metal,
        .hardness = 3.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Redstone Block
    registerBlock("minecraft:redstone_block", BlockDefinition{
        .id = "minecraft:redstone_block",
        .name = "Block of Redstone",
        .material = Material::Metal,
        .hardness = 5.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::METAL,
        .lightEmission = 15
    });
    
    // ============================================
    // WOOD VARIANTS
    // ============================================
    
    // Spruce Planks
    registerBlock("minecraft:spruce_planks", BlockDefinition{
        .id = "minecraft:spruce_planks",
        .name = "Spruce Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Axe,
        .flammable = true,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Birch Planks
    registerBlock("minecraft:birch_planks", BlockDefinition{
        .id = "minecraft:birch_planks",
        .name = "Birch Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Axe,
        .flammable = true,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Jungle Planks
    registerBlock("minecraft:jungle_planks", BlockDefinition{
        .id = "minecraft:jungle_planks",
        .name = "Jungle Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Axe,
        .flammable = true,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Acacia Planks
    registerBlock("minecraft:acacia_planks", BlockDefinition{
        .id = "minecraft:acacia_planks",
        .name = "Acacia Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Axe,
        .flammable = true,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Dark Oak Planks
    registerBlock("minecraft:dark_oak_planks", BlockDefinition{
        .id = "minecraft:dark_oak_planks",
        .name = "Dark Oak Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Axe,
        .flammable = true,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Mangrove Planks
    registerBlock("minecraft:mangrove_planks", BlockDefinition{
        .id = "minecraft:mangrove_planks",
        .name = "Mangrove Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Axe,
        .flammable = true,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Cherry Planks
    registerBlock("minecraft:cherry_planks", BlockDefinition{
        .id = "minecraft:cherry_planks",
        .name = "Cherry Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Axe,
        .flammable = true,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Bamboo Planks
    registerBlock("minecraft:bamboo_planks", BlockDefinition{
        .id = "minecraft:bamboo_planks",
        .name = "Bamboo Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Axe,
        .flammable = true,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Crimson Planks
    registerBlock("minecraft:crimson_planks", BlockDefinition{
        .id = "minecraft:crimson_planks",
        .name = "Crimson Planks",
        .material = Material::NetherWood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Axe,
        .flammable = false,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Warped Planks
    registerBlock("minecraft:warped_planks", BlockDefinition{
        .id = "minecraft:warped_planks",
        .name = "Warped Planks",
        .material = Material::NetherWood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Axe,
        .flammable = false,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // ============================================
    // LEAVES
    // ============================================
    
    // Oak Leaves
    registerBlock("minecraft:oak_leaves", BlockDefinition{
        .id = "minecraft:oak_leaves",
        .name = "Oak Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS,
        .flammable = true
    });
    
    // Spruce Leaves
    registerBlock("minecraft:spruce_leaves", BlockDefinition{
        .id = "minecraft:spruce_leaves",
        .name = "Spruce Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS,
        .flammable = true
    });
    
    // Birch Leaves
    registerBlock("minecraft:birch_leaves", BlockDefinition{
        .id = "minecraft:birch_leaves",
        .name = "Birch Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS,
        .flammable = true
    });
    
    // Jungle Leaves
    registerBlock("minecraft:jungle_leaves", BlockDefinition{
        .id = "minecraft:jungle_leaves",
        .name = "Jungle Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS,
        .flammable = true
    });
    
    // Acacia Leaves
    registerBlock("minecraft:acacia_leaves", BlockDefinition{
        .id = "minecraft:acacia_leaves",
        .name = "Acacia Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS,
        .flammable = true
    });
    
    // Dark Oak Leaves
    registerBlock("minecraft:dark_oak_leaves", BlockDefinition{
        .id = "minecraft:dark_oak_leaves",
        .name = "Dark Oak Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS,
        .flammable = true
    });
    
    // Mangrove Leaves
    registerBlock("minecraft:mangrove_leaves", BlockDefinition{
        .id = "minecraft:mangrove_leaves",
        .name = "Mangrove Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS,
        .flammable = true
    });
    
    // Cherry Leaves
    registerBlock("minecraft:cherry_leaves", BlockDefinition{
        .id = "minecraft:cherry_leaves",
        .name = "Cherry Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS,
        .flammable = true
    });
    
    // ============================================
    // SOILS & GROUND
    // ============================================
    
    // Gravel
    registerBlock("minecraft:gravel", BlockDefinition{
        .id = "minecraft:gravel",
        .name = "Gravel",
        .material = Material::Sand,
        .hardness = 0.6f,
        .blastResistance = 0.6f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRAVEL
    });
    
    // Coarse Dirt
    registerBlock("minecraft:coarse_dirt", BlockDefinition{
        .id = "minecraft:coarse_dirt",
        .name = "Coarse Dirt",
        .material = Material::Dirt,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRAVEL
    });
    
    // Podzol
    registerBlock("minecraft:podzol", BlockDefinition{
        .id = "minecraft:podzol",
        .name = "Podzol",
        .material = Material::Dirt,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // Rooted Dirt
    registerBlock("minecraft:rooted_dirt", BlockDefinition{
        .id = "minecraft:rooted_dirt",
        .name = "Rooted Dirt",
        .material = Material::Dirt,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRAVEL
    });
    
    // Moss Block
    registerBlock("minecraft:moss_block", BlockDefinition{
        .id = "minecraft:moss_block",
        .name = "Moss Block",
        .material = Material::Dirt,
        .hardness = 0.1f,
        .blastResistance = 0.1f,
        .requiredTool = ToolType::Hoe,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // Mycelium
    registerBlock("minecraft:mycelium", BlockDefinition{
        .id = "minecraft:mycelium",
        .name = "Mycelium",
        .material = Material::Dirt,
        .hardness = 0.6f,
        .blastResistance = 0.6f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // ============================================
    // SAND VARIANTS
    // ============================================
    
    // Red Sand
    registerBlock("minecraft:red_sand", BlockDefinition{
        .id = "minecraft:red_sand",
        .name = "Red Sand",
        .material = Material::Sand,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::SAND
    });
    
    // Sandstone
    registerBlock("minecraft:sandstone", BlockDefinition{
        .id = "minecraft:sandstone",
        .name = "Sandstone",
        .material = Material::Stone,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Red Sandstone
    registerBlock("minecraft:red_sandstone", BlockDefinition{
        .id = "minecraft:red_sandstone",
        .name = "Red Sandstone",
        .material = Material::Stone,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // ============================================
    // NETHER BLOCKS
    // ============================================
    
    // Netherrack
    registerBlock("minecraft:netherrack", BlockDefinition{
        .id = "minecraft:netherrack",
        .name = "Netherrack",
        .material = Material::Stone,
        .hardness = 0.4f,
        .blastResistance = 0.4f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::NETHERRACK
    });
    
    // Soul Sand
    registerBlock("minecraft:soul_sand", BlockDefinition{
        .id = "minecraft:soul_sand",
        .name = "Soul Sand",
        .material = Material::Sand,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::SAND
    });
    
    // Soul Soil
    registerBlock("minecraft:soul_soil", BlockDefinition{
        .id = "minecraft:soul_soil",
        .name = "Soul Soil",
        .material = Material::Dirt,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::SAND
    });
    
    // Basalt
    registerBlock("minecraft:basalt", BlockDefinition{
        .id = "minecraft:basalt",
        .name = "Basalt",
        .material = Material::Stone,
        .hardness = 1.25f,
        .blastResistance = 4.2f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Blackstone
    registerBlock("minecraft:blackstone", BlockDefinition{
        .id = "minecraft:blackstone",
        .name = "Blackstone",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Glowstone
    registerBlock("minecraft:glowstone", BlockDefinition{
        .id = "minecraft:glowstone",
        .name = "Glowstone",
        .material = Material::Glass,
        .hardness = 0.3f,
        .blastResistance = 0.3f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .sounds = BlockSoundGroup::GLASS,
        .lightEmission = 15
    });
    
    // ============================================
    // END BLOCKS
    // ============================================
    
    // End Stone
    registerBlock("minecraft:end_stone", BlockDefinition{
        .id = "minecraft:end_stone",
        .name = "End Stone",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 9.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Obsidian
    registerBlock("minecraft:obsidian", BlockDefinition{
        .id = "minecraft:obsidian",
        .name = "Obsidian",
        .material = Material::Stone,
        .hardness = 50.0f,
        .blastResistance = 1200.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Diamond,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Crying Obsidian
    registerBlock("minecraft:crying_obsidian", BlockDefinition{
        .id = "minecraft:crying_obsidian",
        .name = "Crying Obsidian",
        .material = Material::Stone,
        .hardness = 50.0f,
        .blastResistance = 1200.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Diamond,
        .sounds = BlockSoundGroup::STONE,
        .lightEmission = 10
    });
    
    // End Portal Frame
    registerBlock("minecraft:end_portal_frame", BlockDefinition{
        .id = "minecraft:end_portal_frame",
        .name = "End Portal Frame",
        .material = Material::Stone,
        .hardness = -1.0f,
        .blastResistance = 3600000.0f,
        .sounds = BlockSoundGroup::STONE
    });
    
    // ============================================
    // WOOL
    // ============================================
    
    // White Wool
    registerBlock("minecraft:white_wool", BlockDefinition{
        .id = "minecraft:white_wool",
        .name = "White Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Orange Wool
    registerBlock("minecraft:orange_wool", BlockDefinition{
        .id = "minecraft:orange_wool",
        .name = "Orange Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Magenta Wool
    registerBlock("minecraft:magenta_wool", BlockDefinition{
        .id = "minecraft:magenta_wool",
        .name = "Magenta Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Light Blue Wool
    registerBlock("minecraft:light_blue_wool", BlockDefinition{
        .id = "minecraft:light_blue_wool",
        .name = "Light Blue Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Yellow Wool
    registerBlock("minecraft:yellow_wool", BlockDefinition{
        .id = "minecraft:yellow_wool",
        .name = "Yellow Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Lime Wool
    registerBlock("minecraft:lime_wool", BlockDefinition{
        .id = "minecraft:lime_wool",
        .name = "Lime Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Pink Wool
    registerBlock("minecraft:pink_wool", BlockDefinition{
        .id = "minecraft:pink_wool",
        .name = "Pink Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Gray Wool
    registerBlock("minecraft:gray_wool", BlockDefinition{
        .id = "minecraft:gray_wool",
        .name = "Gray Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Light Gray Wool
    registerBlock("minecraft:light_gray_wool", BlockDefinition{
        .id = "minecraft:light_gray_wool",
        .name = "Light Gray Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Cyan Wool
    registerBlock("minecraft:cyan_wool", BlockDefinition{
        .id = "minecraft:cyan_wool",
        .name = "Cyan Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Purple Wool
    registerBlock("minecraft:purple_wool", BlockDefinition{
        .id = "minecraft:purple_wool",
        .name = "Purple Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Blue Wool
    registerBlock("minecraft:blue_wool", BlockDefinition{
        .id = "minecraft:blue_wool",
        .name = "Blue Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Brown Wool
    registerBlock("minecraft:brown_wool", BlockDefinition{
        .id = "minecraft:brown_wool",
        .name = "Brown Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Green Wool
    registerBlock("minecraft:green_wool", BlockDefinition{
        .id = "minecraft:green_wool",
        .name = "Green Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Red Wool
    registerBlock("minecraft:red_wool", BlockDefinition{
        .id = "minecraft:red_wool",
        .name = "Red Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // Black Wool
    registerBlock("minecraft:black_wool", BlockDefinition{
        .id = "minecraft:black_wool",
        .name = "Black Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL,
        .flammable = true
    });
    
    // ============================================
    // LIGHT SOURCES
    // ============================================
    
    // Lantern
    registerBlock("minecraft:lantern", BlockDefinition{
        .id = "minecraft:lantern",
        .name = "Lantern",
        .material = Material::Metal,
        .hardness = 3.5f,
        .blastResistance = 3.5f,
        .opaque = false,
        .renderType = RenderType::Cutout,
        .requiredTool = ToolType::Pickaxe,
        .sounds = BlockSoundGroup::METAL,
        .lightEmission = 15
    });
    
    // Soul Lantern
    registerBlock("minecraft:soul_lantern", BlockDefinition{
        .id = "minecraft:soul_lantern",
        .name = "Soul Lantern",
        .material = Material::Metal,
        .hardness = 3.5f,
        .blastResistance = 3.5f,
        .opaque = false,
        .renderType = RenderType::Cutout,
        .requiredTool = ToolType::Pickaxe,
        .sounds = BlockSoundGroup::METAL,
        .lightEmission = 10
    });
    
    // Campfire
    registerBlock("minecraft:campfire", BlockDefinition{
        .id = "minecraft:campfire",
        .name = "Campfire",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 2.0f,
        .opaque = false,
        .renderType = RenderType::Cutout,
        .sounds = BlockSoundGroup::WOOD,
        .lightEmission = 15
    });
    
    // Sea Lantern
    registerBlock("minecraft:sea_lantern", BlockDefinition{
        .id = "minecraft:sea_lantern",
        .name = "Sea Lantern",
        .material = Material::Glass,
        .hardness = 0.3f,
        .blastResistance = 0.3f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .sounds = BlockSoundGroup::GLASS,
        .lightEmission = 15
    });
    
    // Jack o'Lantern
    registerBlock("minecraft:jack_o_lantern", BlockDefinition{
        .id = "minecraft:jack_o_lantern",
        .name = "Jack o'Lantern",
        .material = Material::Vegetable,
        .hardness = 1.0f,
        .blastResistance = 1.0f,
        .sounds = BlockSoundGroup::WOOD,
        .lightEmission = 15
    });
    
    // Shroomlight
    registerBlock("minecraft:shroomlight", BlockDefinition{
        .id = "minecraft:shroomlight",
        .name = "Shroomlight",
        .material = Material::Vegetable,
        .hardness = 1.0f,
        .blastResistance = 1.0f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .sounds = BlockSoundGroup::WOOD,
        .lightEmission = 15
    });
    
    // ============================================
    // UTILITY BLOCKS
    // ============================================
    
    // Anvil
    registerBlock("minecraft:anvil", BlockDefinition{
        .id = "minecraft:anvil",
        .name = "Anvil",
        .material = Material::Metal,
        .hardness = 5.0f,
        .blastResistance = 1200.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Enchanting Table
    registerBlock("minecraft:enchanting_table", BlockDefinition{
        .id = "minecraft:enchanting_table",
        .name = "Enchanting Table",
        .material = Material::Stone,
        .hardness = 5.0f,
        .blastResistance = 1200.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE,
        .lightEmission = 7
    });
    
    // Brewing Stand
    registerBlock("minecraft:brewing_stand", BlockDefinition{
        .id = "minecraft:brewing_stand",
        .name = "Brewing Stand",
        .material = Material::Metal,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .opaque = false,
        .renderType = RenderType::Cutout,
        .lightEmission = 1
        .sounds = BlockSoundGroup::METAL
    });
    
    // Cauldron
    registerBlock("minecraft:cauldron", BlockDefinition{
        .id = "minecraft:cauldron",
        .name = "Cauldron",
        .material = Material::Metal,
        .hardness = 2.0f,
        .blastResistance = 2.0f,
        .opaque = false,
        .renderType = RenderType::Cutout,
        .requiredTool = ToolType::Pickaxe,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Bell
    registerBlock("minecraft:bell", BlockDefinition{
        .id = "minecraft:bell",
        .name = "Bell",
        .material = Material::Metal,
        .hardness = 5.0f,
        .blastResistance = 5.0f,
        .opaque = false,
        .renderType = RenderType::Cutout,
        .requiredTool = ToolType::Pickaxe,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Beehive
    registerBlock("minecraft:beehive", BlockDefinition{
        .id = "minecraft:beehive",
        .name = "Beehive",
        .material = Material::Wood,
        .hardness = 0.6f,
        .blastResistance = 0.6f,
        .requiredTool = ToolType::Axe,
        .flammable = true,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // ============================================
    // REDSTONE COMPONENTS
    // ============================================
    
    // Redstone Lamp
    registerBlock("minecraft:redstone_lamp", BlockDefinition{
        .id = "minecraft:redstone_lamp",
        .name = "Redstone Lamp",
        .material = Material::Stone,
        .hardness = 0.3f,
        .blastResistance = 0.3f,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Piston
    registerBlock("minecraft:piston", BlockDefinition{
        .id = "minecraft:piston",
        .name = "Piston",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 1.5f,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Sticky Piston
    registerBlock("minecraft:sticky_piston", BlockDefinition{
        .id = "minecraft:sticky_piston",
        .name = "Sticky Piston",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 1.5f,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Dispenser
    registerBlock("minecraft:dispenser", BlockDefinition{
        .id = "minecraft:dispenser",
        .name = "Dispenser",
        .material = Material::Stone,
        .hardness = 3.5f,
        .blastResistance = 3.5f,
        .requiredTool = ToolType::Pickaxe,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Dropper
    registerBlock("minecraft:dropper", BlockDefinition{
        .id = "minecraft:dropper",
        .name = "Dropper",
        .material = Material::Stone,
        .hardness = 3.5f,
        .blastResistance = 3.5f,
        .requiredTool = ToolType::Pickaxe,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Observer
    registerBlock("minecraft:observer", BlockDefinition{
        .id = "minecraft:observer",
        .name = "Observer",
        .material = Material::Stone,
        .hardness = 3.5f,
        .blastResistance = 3.5f,
        .requiredTool = ToolType::Pickaxe,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Hopper
    registerBlock("minecraft:hopper", BlockDefinition{
        .id = "minecraft:hopper",
        .name = "Hopper",
        .material = Material::Metal,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .opaque = false,
        .renderType = RenderType::Cutout,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::METAL
    });
    
    // ============================================
    // DECORATIVE & MISC
    // ============================================
    
    // Bookshelf
    registerBlock("minecraft:bookshelf", BlockDefinition{
        .id = "minecraft:bookshelf",
        .name = "Bookshelf",
        .material = Material::Wood,
        .hardness = 1.5f,
        .blastResistance = 1.5f,
        .requiredTool = ToolType::Axe,
        .flammable = true,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Ladder
    registerBlock("minecraft:ladder", BlockDefinition{
        .id = "minecraft:ladder",
        .name = "Ladder",
        .material = Material::Decoration,
        .hardness = 0.4f,
        .blastResistance = 0.4f,
        .opaque = false,
        .renderType = RenderType::Cutout,
        .sounds = BlockSoundGroup::LADDER
    });
    
    // Snow Block
    registerBlock("minecraft:snow_block", BlockDefinition{
        .id = "minecraft:snow_block",
        .name = "Snow Block",
        .material = Material::Snow,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::SNOW
    });
    
    // Ice
    registerBlock("minecraft:ice", BlockDefinition{
        .id = "minecraft:ice",
        .name = "Ice",
        .material = Material::Ice,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .sounds = BlockSoundGroup::GLASS
    });
    
    // Packed Ice
    registerBlock("minecraft:packed_ice", BlockDefinition{
        .id = "minecraft:packed_ice",
        .name = "Packed Ice",
        .material = Material::Ice,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .sounds = BlockSoundGroup::GLASS
    });
    
    // Blue Ice
    registerBlock("minecraft:blue_ice", BlockDefinition{
        .id = "minecraft:blue_ice",
        .name = "Blue Ice",
        .material = Material::Ice,
        .hardness = 2.8f,
        .blastResistance = 2.8f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .sounds = BlockSoundGroup::GLASS
    });
    
    // Clay
    registerBlock("minecraft:clay", BlockDefinition{
        .id = "minecraft:clay",
        .name = "Clay",
        .material = Material::Dirt,
        .hardness = 0.6f,
        .blastResistance = 0.6f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRAVEL
    });
    
    // Sponge
    registerBlock("minecraft:sponge", BlockDefinition{
        .id = "minecraft:sponge",
        .name = "Sponge",
        .material = Material::Dirt,
        .hardness = 0.6f,
        .blastResistance = 0.6f,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // ============================================
    // FLUIDS (level property)
    // ============================================
    
    // Bubble Column (water with bubbles)
    registerBlock("minecraft:bubble_column", BlockDefinition{
        .id = "minecraft:bubble_column",
        .name = "Bubble Column",
        .material = Material::Water,
        .hardness = 100.0f,
        .blastResistance = 100.0f,
        .opaque = false,
        .solid = false,
        .replaceable = true,
        .renderType = RenderType::Translucent,
        .collisionShape = VoxelShape::empty()
    });
    
    // ============================================
    // FARMING
    // ============================================
    
    // Farmland
    registerBlock("minecraft:farmland", BlockDefinition{
        .id = "minecraft:farmland",
        .name = "Farmland",
        .material = Material::Dirt,
        .hardness = 0.6f,
        .blastResistance = 0.6f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRAVEL
    });
    
    // ============================================
    // TERRACOTTA & CONCRETE
    // ============================================
    
    // Terracotta
    registerBlock("minecraft:terracotta", BlockDefinition{
        .id = "minecraft:terracotta",
        .name = "Terracotta",
        .material = Material::Stone,
        .hardness = 1.25f,
        .blastResistance = 4.2f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // White Concrete
    registerBlock("minecraft:white_concrete", BlockDefinition{
        .id = "minecraft:white_concrete",
        .name = "White Concrete",
        .material = Material::Stone,
        .hardness = 1.8f,
        .blastResistance = 1.8f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Black Concrete
    registerBlock("minecraft:black_concrete", BlockDefinition{
        .id = "minecraft:black_concrete",
        .name = "Black Concrete",
        .material = Material::Stone,
        .hardness = 1.8f,
        .blastResistance = 1.8f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // ============================================
    // PRISMARINE
    // ============================================
    
    // Prismarine
    registerBlock("minecraft:prismarine", BlockDefinition{
        .id = "minecraft:prismarine",
        .name = "Prismarine",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Prismarine Bricks
    registerBlock("minecraft:prismarine_bricks", BlockDefinition{
        .id = "minecraft:prismarine_bricks",
        .name = "Prismarine Bricks",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Dark Prismarine
    registerBlock("minecraft:dark_prismarine", BlockDefinition{
        .id = "minecraft:dark_prismarine",
        .name = "Dark Prismarine",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Sea Pickle
    registerBlock("minecraft:sea_pickle", BlockDefinition{
        .id = "minecraft:sea_pickle",
        .name = "Sea Pickle",
        .material = Material::Decoration,
        .hardness = 0.0f,
        .blastResistance = 0.0f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .solid = false,
        .lightEmission = 6
    });
    
    // ============================================
    // AMETHYST
    // ============================================
    
    // Amethyst Block
    registerBlock("minecraft:amethyst_block", BlockDefinition{
        .id = "minecraft:amethyst_block",
        .name = "Block of Amethyst",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 1.5f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::AMETHYST
    });
    
    // Budding Amethyst
    registerBlock("minecraft:budding_amethyst", BlockDefinition{
        .id = "minecraft:budding_amethyst",
        .name = "Budding Amethyst",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 1.5f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::AMETHYST
    });
    
    // Calcite
    registerBlock("minecraft:calcite", BlockDefinition{
        .id = "minecraft:calcite",
        .name = "Calcite",
        .material = Material::Stone,
        .hardness = 0.75f,
        .blastResistance = 0.75f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // ============================================
    // SCULK (1.19+)
    // ============================================
    
    // Sculk
    registerBlock("minecraft:sculk", BlockDefinition{
        .id = "minecraft:sculk",
        .name = "Sculk",
        .material = Material::Stone,
        .hardness = 0.6f,
        .blastResistance = 0.6f,
        .sounds = BlockSoundGroup::SCULK
    });
    
    // Sculk Sensor
    registerBlock("minecraft:sculk_sensor", BlockDefinition{
        .id = "minecraft:sculk_sensor",
        .name = "Sculk Sensor",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 1.5f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .sounds = BlockSoundGroup::SCULK
    });
    
    // Sculk Shrieker
    registerBlock("minecraft:sculk_shrieker", BlockDefinition{
        .id = "minecraft:sculk_shrieker",
        .name = "Sculk Shrieker",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 1.5f,
        .sounds = BlockSoundGroup::SCULK
    });
    
    // Reinforced Deepslate
    registerBlock("minecraft:reinforced_deepslate", BlockDefinition{
        .id = "minecraft:reinforced_deepslate",
        .name = "Reinforced Deepslate",
        .material = Material::Stone,
        .hardness = -1.0f,
        .blastResistance = 3600000.0f,
        .sounds = BlockSoundGroup::STONE
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
    BlockSoundGroup METAL = {"block.metal.break", "block.metal.step", "block.metal.place", "block.metal.hit", "block.metal.fall"
};
    BlockSoundGroup WOOL = {"block.wool.break", "block.wool.step", "block.wool.place", "block.wool.hit", "block.wool.fall"};
    BlockSoundGroup SNOW = {"block.snow.break", "block.snow.step", "block.snow.place", "block.snow.hit", "block.snow.fall"};
    BlockSoundGroup LADDER = {"block.ladder.break", "block.ladder.step", "block.ladder.place", "block.ladder.hit", "block.ladder.fall"};
    BlockSoundGroup NETHERRACK = {"block.netherrack.break", "block.netherrack.step", "block.netherrack.place", "block.netherrack.hit", "block.netherrack.fall"};
    BlockSoundGroup AMETHYST = {"block.amethyst.break", "block.amethyst.step", "block.amethyst.place", "block.amethyst.hit", "block.amethyst.fall"};
    BlockSoundGroup SCULK = {"block.sculk.break", "block.sculk.step", "block.sculk.place", "block.sculk.hit", "block.sculk.fall"};
    BlockSoundGroup CROP = {"block.crop.break", "block.crop.step", "block.crop.place", "block.crop.hit", "block.crop.fall"};
    BlockSoundGroup STEM = {"block.stem.break", "block.stem.step", "block.stem.place", "block.stem.hit", "block.stem.fall"};
}

} // namespace VoxelForge
