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
    airDef.id = "poorcraftultra:air";
    airDef.name = "Air";
    airDef.material = Material::Air;
    airDef.opaque = false;
    airDef.solid = false;
    airDef.replaceable = true;
    airDef.renderType = RenderType::Air;
    airDef.collisionShape = VoxelShape::empty();
    airDef.outlineShape = VoxelShape::empty();
    blocks.push_back(std::move(airDef));
    idToIndex["poorcraftultra:air"] = 0;
    
    VF_CORE_INFO("Block registry initialized");
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
    
    idToIndex[id] = newId;
    
    // Generate state variants
    if (!definition.properties.empty()) {
        generateStateVariants(definition);
    }
    
    blocks.push_back(std::move(definition));
    
    // Store states
    blockStates[newId] = blocks.back().stateVariants;
    
    spdlog::debug("Registered block: {} -> {}", id, newId);
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
    
    spdlog::debug("Generated {} state variants for {}", totalCombinations, def.id);
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
    registerBlock("poorcraftultra:stone", BlockDefinition{
        .id = "poorcraftultra:stone",
        .name = "Stone",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Grass Block
    registerBlock("poorcraftultra:grass_block", BlockDefinition{
        .id = "poorcraftultra:grass_block",
        .name = "Grass Block",
        .material = Material::Dirt,
        .hardness = 0.6f,
        .blastResistance = 0.6f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // Dirt
    registerBlock("poorcraftultra:dirt", BlockDefinition{
        .id = "poorcraftultra:dirt",
        .name = "Dirt",
        .material = Material::Dirt,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRAVEL
    });
    
    // Cobblestone
    registerBlock("poorcraftultra:cobblestone", BlockDefinition{
        .id = "poorcraftultra:cobblestone",
        .name = "Cobblestone",
        .material = Material::Stone,
        .hardness = 2.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Oak Planks
    registerBlock("poorcraftultra:oak_planks", BlockDefinition{
        .id = "poorcraftultra:oak_planks",
        .name = "Oak Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .flammable = true,
        .requiredTool = ToolType::Axe,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Oak Log (with axis property)
    BlockDefinition oakLog;
    oakLog.id = "poorcraftultra:oak_log";
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
    registerBlock("poorcraftultra:oak_log", std::move(oakLog));
    
    // Birch Log (with axis property)
    BlockDefinition birchLog;
    birchLog.id = "poorcraftultra:birch_log";
    birchLog.name = "Birch Log";
    birchLog.material = Material::Wood;
    birchLog.hardness = 2.0f;
    birchLog.requiredTool = ToolType::Axe;
    birchLog.flammable = true;
    birchLog.sounds = BlockSoundGroup::WOOD;
    birchLog.properties = {
        {"axis", BlockProperty::Type::Direction, {"y", "x", "z"}, 0}
    };
    birchLog.defaultProperties.setEnum("axis", "y");
    registerBlock("poorcraftultra:birch_log", std::move(birchLog));
    
    // Spruce Log (with axis property)
    BlockDefinition spruceLog;
    spruceLog.id = "poorcraftultra:spruce_log";
    spruceLog.name = "Spruce Log";
    spruceLog.material = Material::Wood;
    spruceLog.hardness = 2.0f;
    spruceLog.requiredTool = ToolType::Axe;
    spruceLog.flammable = true;
    spruceLog.sounds = BlockSoundGroup::WOOD;
    spruceLog.properties = {
        {"axis", BlockProperty::Type::Direction, {"y", "x", "z"}, 0}
    };
    spruceLog.defaultProperties.setEnum("axis", "y");
    registerBlock("poorcraftultra:spruce_log", std::move(spruceLog));
    
    // Dark Oak Log (with axis property)
    BlockDefinition darkOakLog;
    darkOakLog.id = "poorcraftultra:dark_oak_log";
    darkOakLog.name = "Dark Oak Log";
    darkOakLog.material = Material::Wood;
    darkOakLog.hardness = 2.0f;
    darkOakLog.requiredTool = ToolType::Axe;
    darkOakLog.flammable = true;
    darkOakLog.sounds = BlockSoundGroup::WOOD;
    darkOakLog.properties = {
        {"axis", BlockProperty::Type::Direction, {"y", "x", "z"}, 0}
    };
    darkOakLog.defaultProperties.setEnum("axis", "y");
    registerBlock("poorcraftultra:dark_oak_log", std::move(darkOakLog));
    
    // Acacia Log (with axis property)
    BlockDefinition acaciaLog;
    acaciaLog.id = "poorcraftultra:acacia_log";
    acaciaLog.name = "Acacia Log";
    acaciaLog.material = Material::Wood;
    acaciaLog.hardness = 2.0f;
    acaciaLog.requiredTool = ToolType::Axe;
    acaciaLog.flammable = true;
    acaciaLog.sounds = BlockSoundGroup::WOOD;
    acaciaLog.properties = {
        {"axis", BlockProperty::Type::Direction, {"y", "x", "z"}, 0}
    };
    acaciaLog.defaultProperties.setEnum("axis", "y");
    registerBlock("poorcraftultra:acacia_log", std::move(acaciaLog));
    
    // Water
    registerBlock("poorcraftultra:water", BlockDefinition{
        .id = "poorcraftultra:water",
        .name = "Water",
        .material = Material::Water,
        .hardness = 100.0f,
        .blastResistance = 100.0f,
        .solid = false,
        .opaque = false,
        .replaceable = true,
        .renderType = RenderType::Translucent,
        .collisionShape = VoxelShape::empty()
    });
    
    // Lava
    registerBlock("poorcraftultra:lava", BlockDefinition{
        .id = "poorcraftultra:lava",
        .name = "Lava",
        .material = Material::Lava,
        .hardness = 100.0f,
        .blastResistance = 100.0f,
        .solid = false,
        .opaque = false,
        .replaceable = true,
        .renderType = RenderType::Translucent,
        .lightEmission = 15,
        .collisionShape = VoxelShape::empty()
    });
    
    // Sand
    registerBlock("poorcraftultra:sand", BlockDefinition{
        .id = "poorcraftultra:sand",
        .name = "Sand",
        .material = Material::Sand,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::SAND
    });
    
    // Bedrock
    registerBlock("poorcraftultra:bedrock", BlockDefinition{
        .id = "poorcraftultra:bedrock",
        .name = "Bedrock",
        .material = Material::Stone,
        .hardness = -1.0f,  // Unbreakable
        .blastResistance = 3600000.0f
    });
    
    // Iron Ore
    registerBlock("poorcraftultra:iron_ore", BlockDefinition{
        .id = "poorcraftultra:iron_ore",
        .name = "Iron Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Coal Ore
    registerBlock("poorcraftultra:coal_ore", BlockDefinition{
        .id = "poorcraftultra:coal_ore",
        .name = "Coal Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Gold Ore
    registerBlock("poorcraftultra:gold_ore", BlockDefinition{
        .id = "poorcraftultra:gold_ore",
        .name = "Gold Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Diamond Ore
    registerBlock("poorcraftultra:diamond_ore", BlockDefinition{
        .id = "poorcraftultra:diamond_ore",
        .name = "Diamond Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Crafting Table
    registerBlock("poorcraftultra:crafting_table", BlockDefinition{
        .id = "poorcraftultra:crafting_table",
        .name = "Crafting Table",
        .material = Material::Wood,
        .hardness = 2.5f,
        .blastResistance = 2.5f,
        .flammable = true,
        .hasBlockEntity = false,
        .requiredTool = ToolType::Axe,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Furnace
    registerBlock("poorcraftultra:furnace", BlockDefinition{
        .id = "poorcraftultra:furnace",
        .name = "Furnace",
        .material = Material::Stone,
        .hardness = 3.5f,
        .blastResistance = 3.5f,
        .hasBlockEntity = true,
        .requiredTool = ToolType::Pickaxe,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Chest
    registerBlock("poorcraftultra:chest", BlockDefinition{
        .id = "poorcraftultra:chest",
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
    registerBlock("poorcraftultra:torch", BlockDefinition{
        .id = "poorcraftultra:torch",
        .name = "Torch",
        .material = Material::Decoration,
        .hardness = 0.0f,
        .blastResistance = 0.0f,
        .solid = false,
        .opaque = false,
        .renderType = RenderType::Cutout,
        .lightEmission = 14,
        .collisionShape = VoxelShape::empty()
    });
    
    // Glass
    registerBlock("poorcraftultra:glass", BlockDefinition{
        .id = "poorcraftultra:glass",
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
    registerBlock("poorcraftultra:granite", BlockDefinition{
        .id = "poorcraftultra:granite",
        .name = "Granite",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Polished Granite
    registerBlock("poorcraftultra:polished_granite", BlockDefinition{
        .id = "poorcraftultra:polished_granite",
        .name = "Polished Granite",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Diorite
    registerBlock("poorcraftultra:diorite", BlockDefinition{
        .id = "poorcraftultra:diorite",
        .name = "Diorite",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Polished Diorite
    registerBlock("poorcraftultra:polished_diorite", BlockDefinition{
        .id = "poorcraftultra:polished_diorite",
        .name = "Polished Diorite",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Andesite
    registerBlock("poorcraftultra:andesite", BlockDefinition{
        .id = "poorcraftultra:andesite",
        .name = "Andesite",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Polished Andesite
    registerBlock("poorcraftultra:polished_andesite", BlockDefinition{
        .id = "poorcraftultra:polished_andesite",
        .name = "Polished Andesite",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Deepslate
    registerBlock("poorcraftultra:deepslate", BlockDefinition{
        .id = "poorcraftultra:deepslate",
        .name = "Deepslate",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Cobbled Deepslate
    registerBlock("poorcraftultra:cobbled_deepslate", BlockDefinition{
        .id = "poorcraftultra:cobbled_deepslate",
        .name = "Cobbled Deepslate",
        .material = Material::Stone,
        .hardness = 3.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Tuff
    registerBlock("poorcraftultra:tuff", BlockDefinition{
        .id = "poorcraftultra:tuff",
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
    registerBlock("poorcraftultra:stone_bricks", BlockDefinition{
        .id = "poorcraftultra:stone_bricks",
        .name = "Stone Bricks",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Cracked Stone Bricks
    registerBlock("poorcraftultra:cracked_stone_bricks", BlockDefinition{
        .id = "poorcraftultra:cracked_stone_bricks",
        .name = "Cracked Stone Bricks",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Mossy Stone Bricks
    registerBlock("poorcraftultra:mossy_stone_bricks", BlockDefinition{
        .id = "poorcraftultra:mossy_stone_bricks",
        .name = "Mossy Stone Bricks",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Chiseled Stone Bricks
    registerBlock("poorcraftultra:chiseled_stone_bricks", BlockDefinition{
        .id = "poorcraftultra:chiseled_stone_bricks",
        .name = "Chiseled Stone Bricks",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Bricks
    registerBlock("poorcraftultra:bricks", BlockDefinition{
        .id = "poorcraftultra:bricks",
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
    registerBlock("poorcraftultra:copper_ore", BlockDefinition{
        .id = "poorcraftultra:copper_ore",
        .name = "Copper Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Deepslate Copper Ore
    registerBlock("poorcraftultra:deepslate_copper_ore", BlockDefinition{
        .id = "poorcraftultra:deepslate_copper_ore",
        .name = "Deepslate Copper Ore",
        .material = Material::Stone,
        .hardness = 4.5f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Emerald Ore
    registerBlock("poorcraftultra:emerald_ore", BlockDefinition{
        .id = "poorcraftultra:emerald_ore",
        .name = "Emerald Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Deepslate Emerald Ore
    registerBlock("poorcraftultra:deepslate_emerald_ore", BlockDefinition{
        .id = "poorcraftultra:deepslate_emerald_ore",
        .name = "Deepslate Emerald Ore",
        .material = Material::Stone,
        .hardness = 4.5f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Redstone Ore
    registerBlock("poorcraftultra:redstone_ore", BlockDefinition{
        .id = "poorcraftultra:redstone_ore",
        .name = "Redstone Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .lightEmission = 7,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Deepslate Redstone Ore
    registerBlock("poorcraftultra:deepslate_redstone_ore", BlockDefinition{
        .id = "poorcraftultra:deepslate_redstone_ore",
        .name = "Deepslate Redstone Ore",
        .material = Material::Stone,
        .hardness = 4.5f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .lightEmission = 7,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Lapis Ore
    registerBlock("poorcraftultra:lapis_ore", BlockDefinition{
        .id = "poorcraftultra:lapis_ore",
        .name = "Lapis Ore",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Deepslate Lapis Ore
    registerBlock("poorcraftultra:deepslate_lapis_ore", BlockDefinition{
        .id = "poorcraftultra:deepslate_lapis_ore",
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
    registerBlock("poorcraftultra:coal_block", BlockDefinition{
        .id = "poorcraftultra:coal_block",
        .name = "Block of Coal",
        .material = Material::Stone,
        .hardness = 5.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Iron Block
    registerBlock("poorcraftultra:iron_block", BlockDefinition{
        .id = "poorcraftultra:iron_block",
        .name = "Block of Iron",
        .material = Material::Metal,
        .hardness = 5.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Gold Block
    registerBlock("poorcraftultra:gold_block", BlockDefinition{
        .id = "poorcraftultra:gold_block",
        .name = "Block of Gold",
        .material = Material::Metal,
        .hardness = 3.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Diamond Block
    registerBlock("poorcraftultra:diamond_block", BlockDefinition{
        .id = "poorcraftultra:diamond_block",
        .name = "Block of Diamond",
        .material = Material::Metal,
        .hardness = 5.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Emerald Block
    registerBlock("poorcraftultra:emerald_block", BlockDefinition{
        .id = "poorcraftultra:emerald_block",
        .name = "Block of Emerald",
        .material = Material::Metal,
        .hardness = 5.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Iron,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Copper Block
    registerBlock("poorcraftultra:copper_block", BlockDefinition{
        .id = "poorcraftultra:copper_block",
        .name = "Block of Copper",
        .material = Material::Metal,
        .hardness = 3.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Lapis Block
    registerBlock("poorcraftultra:lapis_block", BlockDefinition{
        .id = "poorcraftultra:lapis_block",
        .name = "Block of Lapis Lazuli",
        .material = Material::Metal,
        .hardness = 3.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Stone,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Redstone Block
    registerBlock("poorcraftultra:redstone_block", BlockDefinition{
        .id = "poorcraftultra:redstone_block",
        .name = "Block of Redstone",
        .material = Material::Metal,
        .hardness = 5.0f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .lightEmission = 15,
        .sounds = BlockSoundGroup::METAL
    });
    
    // ============================================
    // WOOD VARIANTS
    // ============================================
    
    // Spruce Planks
    registerBlock("poorcraftultra:spruce_planks", BlockDefinition{
        .id = "poorcraftultra:spruce_planks",
        .name = "Spruce Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .flammable = true,
        .requiredTool = ToolType::Axe,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Birch Planks
    registerBlock("poorcraftultra:birch_planks", BlockDefinition{
        .id = "poorcraftultra:birch_planks",
        .name = "Birch Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .flammable = true,
        .requiredTool = ToolType::Axe,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Jungle Planks
    registerBlock("poorcraftultra:jungle_planks", BlockDefinition{
        .id = "poorcraftultra:jungle_planks",
        .name = "Jungle Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .flammable = true,
        .requiredTool = ToolType::Axe,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Acacia Planks
    registerBlock("poorcraftultra:acacia_planks", BlockDefinition{
        .id = "poorcraftultra:acacia_planks",
        .name = "Acacia Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .flammable = true,
        .requiredTool = ToolType::Axe,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Dark Oak Planks
    registerBlock("poorcraftultra:dark_oak_planks", BlockDefinition{
        .id = "poorcraftultra:dark_oak_planks",
        .name = "Dark Oak Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .flammable = true,
        .requiredTool = ToolType::Axe,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Mangrove Planks
    registerBlock("poorcraftultra:mangrove_planks", BlockDefinition{
        .id = "poorcraftultra:mangrove_planks",
        .name = "Mangrove Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .flammable = true,
        .requiredTool = ToolType::Axe,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Cherry Planks
    registerBlock("poorcraftultra:cherry_planks", BlockDefinition{
        .id = "poorcraftultra:cherry_planks",
        .name = "Cherry Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .flammable = true,
        .requiredTool = ToolType::Axe,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Bamboo Planks
    registerBlock("poorcraftultra:bamboo_planks", BlockDefinition{
        .id = "poorcraftultra:bamboo_planks",
        .name = "Bamboo Planks",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .flammable = true,
        .requiredTool = ToolType::Axe,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Crimson Planks
    registerBlock("poorcraftultra:crimson_planks", BlockDefinition{
        .id = "poorcraftultra:crimson_planks",
        .name = "Crimson Planks",
        .material = Material::NetherWood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .flammable = false,
        .requiredTool = ToolType::Axe,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Warped Planks
    registerBlock("poorcraftultra:warped_planks", BlockDefinition{
        .id = "poorcraftultra:warped_planks",
        .name = "Warped Planks",
        .material = Material::NetherWood,
        .hardness = 2.0f,
        .blastResistance = 3.0f,
        .flammable = false,
        .requiredTool = ToolType::Axe,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // ============================================
    // LEAVES
    // ============================================
    
    // Oak Leaves
    registerBlock("poorcraftultra:oak_leaves", BlockDefinition{
        .id = "poorcraftultra:oak_leaves",
        .name = "Oak Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .flammable = true,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // Spruce Leaves
    registerBlock("poorcraftultra:spruce_leaves", BlockDefinition{
        .id = "poorcraftultra:spruce_leaves",
        .name = "Spruce Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .flammable = true,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // Birch Leaves
    registerBlock("poorcraftultra:birch_leaves", BlockDefinition{
        .id = "poorcraftultra:birch_leaves",
        .name = "Birch Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .flammable = true,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // Jungle Leaves
    registerBlock("poorcraftultra:jungle_leaves", BlockDefinition{
        .id = "poorcraftultra:jungle_leaves",
        .name = "Jungle Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .flammable = true,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // Acacia Leaves
    registerBlock("poorcraftultra:acacia_leaves", BlockDefinition{
        .id = "poorcraftultra:acacia_leaves",
        .name = "Acacia Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .flammable = true,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // Dark Oak Leaves
    registerBlock("poorcraftultra:dark_oak_leaves", BlockDefinition{
        .id = "poorcraftultra:dark_oak_leaves",
        .name = "Dark Oak Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .flammable = true,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // Mangrove Leaves
    registerBlock("poorcraftultra:mangrove_leaves", BlockDefinition{
        .id = "poorcraftultra:mangrove_leaves",
        .name = "Mangrove Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .flammable = true,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // Cherry Leaves
    registerBlock("poorcraftultra:cherry_leaves", BlockDefinition{
        .id = "poorcraftultra:cherry_leaves",
        .name = "Cherry Leaves",
        .material = Material::Leaves,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .flammable = true,
        .opaque = false,
        .renderType = RenderType::CutoutMipped,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // ============================================
    // SOILS & GROUND
    // ============================================
    
    // Gravel
    registerBlock("poorcraftultra:gravel", BlockDefinition{
        .id = "poorcraftultra:gravel",
        .name = "Gravel",
        .material = Material::Sand,
        .hardness = 0.6f,
        .blastResistance = 0.6f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRAVEL
    });
    
    // Coarse Dirt
    registerBlock("poorcraftultra:coarse_dirt", BlockDefinition{
        .id = "poorcraftultra:coarse_dirt",
        .name = "Coarse Dirt",
        .material = Material::Dirt,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRAVEL
    });
    
    // Podzol
    registerBlock("poorcraftultra:podzol", BlockDefinition{
        .id = "poorcraftultra:podzol",
        .name = "Podzol",
        .material = Material::Dirt,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // Rooted Dirt
    registerBlock("poorcraftultra:rooted_dirt", BlockDefinition{
        .id = "poorcraftultra:rooted_dirt",
        .name = "Rooted Dirt",
        .material = Material::Dirt,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRAVEL
    });
    
    // Moss Block
    registerBlock("poorcraftultra:moss_block", BlockDefinition{
        .id = "poorcraftultra:moss_block",
        .name = "Moss Block",
        .material = Material::Dirt,
        .hardness = 0.1f,
        .blastResistance = 0.1f,
        .requiredTool = ToolType::Hoe,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // Mycelium
    registerBlock("poorcraftultra:mycelium", BlockDefinition{
        .id = "poorcraftultra:mycelium",
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
    registerBlock("poorcraftultra:red_sand", BlockDefinition{
        .id = "poorcraftultra:red_sand",
        .name = "Red Sand",
        .material = Material::Sand,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::SAND
    });
    
    // Sandstone
    registerBlock("poorcraftultra:sandstone", BlockDefinition{
        .id = "poorcraftultra:sandstone",
        .name = "Sandstone",
        .material = Material::Stone,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Red Sandstone
    registerBlock("poorcraftultra:red_sandstone", BlockDefinition{
        .id = "poorcraftultra:red_sandstone",
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
    registerBlock("poorcraftultra:netherrack", BlockDefinition{
        .id = "poorcraftultra:netherrack",
        .name = "Netherrack",
        .material = Material::Stone,
        .hardness = 0.4f,
        .blastResistance = 0.4f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::NETHERRACK
    });
    
    // Soul Sand
    registerBlock("poorcraftultra:soul_sand", BlockDefinition{
        .id = "poorcraftultra:soul_sand",
        .name = "Soul Sand",
        .material = Material::Sand,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::SAND
    });
    
    // Soul Soil
    registerBlock("poorcraftultra:soul_soil", BlockDefinition{
        .id = "poorcraftultra:soul_soil",
        .name = "Soul Soil",
        .material = Material::Dirt,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::SAND
    });
    
    // Basalt
    registerBlock("poorcraftultra:basalt", BlockDefinition{
        .id = "poorcraftultra:basalt",
        .name = "Basalt",
        .material = Material::Stone,
        .hardness = 1.25f,
        .blastResistance = 4.2f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Blackstone
    registerBlock("poorcraftultra:blackstone", BlockDefinition{
        .id = "poorcraftultra:blackstone",
        .name = "Blackstone",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Glowstone
    registerBlock("poorcraftultra:glowstone", BlockDefinition{
        .id = "poorcraftultra:glowstone",
        .name = "Glowstone",
        .material = Material::Glass,
        .hardness = 0.3f,
        .blastResistance = 0.3f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .lightEmission = 15,
        .sounds = BlockSoundGroup::GLASS
    });
    
    // ============================================
    // END BLOCKS
    // ============================================
    
    // End Stone
    registerBlock("poorcraftultra:end_stone", BlockDefinition{
        .id = "poorcraftultra:end_stone",
        .name = "End Stone",
        .material = Material::Stone,
        .hardness = 3.0f,
        .blastResistance = 9.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Obsidian
    registerBlock("poorcraftultra:obsidian", BlockDefinition{
        .id = "poorcraftultra:obsidian",
        .name = "Obsidian",
        .material = Material::Stone,
        .hardness = 50.0f,
        .blastResistance = 1200.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Diamond,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Crying Obsidian
    registerBlock("poorcraftultra:crying_obsidian", BlockDefinition{
        .id = "poorcraftultra:crying_obsidian",
        .name = "Crying Obsidian",
        .material = Material::Stone,
        .hardness = 50.0f,
        .blastResistance = 1200.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Diamond,
        .lightEmission = 10,
        .sounds = BlockSoundGroup::STONE
    });
    
    // End Portal Frame
    registerBlock("poorcraftultra:end_portal_frame", BlockDefinition{
        .id = "poorcraftultra:end_portal_frame",
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
    registerBlock("poorcraftultra:white_wool", BlockDefinition{
        .id = "poorcraftultra:white_wool",
        .name = "White Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Orange Wool
    registerBlock("poorcraftultra:orange_wool", BlockDefinition{
        .id = "poorcraftultra:orange_wool",
        .name = "Orange Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Magenta Wool
    registerBlock("poorcraftultra:magenta_wool", BlockDefinition{
        .id = "poorcraftultra:magenta_wool",
        .name = "Magenta Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Light Blue Wool
    registerBlock("poorcraftultra:light_blue_wool", BlockDefinition{
        .id = "poorcraftultra:light_blue_wool",
        .name = "Light Blue Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Yellow Wool
    registerBlock("poorcraftultra:yellow_wool", BlockDefinition{
        .id = "poorcraftultra:yellow_wool",
        .name = "Yellow Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Lime Wool
    registerBlock("poorcraftultra:lime_wool", BlockDefinition{
        .id = "poorcraftultra:lime_wool",
        .name = "Lime Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Pink Wool
    registerBlock("poorcraftultra:pink_wool", BlockDefinition{
        .id = "poorcraftultra:pink_wool",
        .name = "Pink Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Gray Wool
    registerBlock("poorcraftultra:gray_wool", BlockDefinition{
        .id = "poorcraftultra:gray_wool",
        .name = "Gray Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Light Gray Wool
    registerBlock("poorcraftultra:light_gray_wool", BlockDefinition{
        .id = "poorcraftultra:light_gray_wool",
        .name = "Light Gray Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Cyan Wool
    registerBlock("poorcraftultra:cyan_wool", BlockDefinition{
        .id = "poorcraftultra:cyan_wool",
        .name = "Cyan Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Purple Wool
    registerBlock("poorcraftultra:purple_wool", BlockDefinition{
        .id = "poorcraftultra:purple_wool",
        .name = "Purple Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Blue Wool
    registerBlock("poorcraftultra:blue_wool", BlockDefinition{
        .id = "poorcraftultra:blue_wool",
        .name = "Blue Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Brown Wool
    registerBlock("poorcraftultra:brown_wool", BlockDefinition{
        .id = "poorcraftultra:brown_wool",
        .name = "Brown Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Green Wool
    registerBlock("poorcraftultra:green_wool", BlockDefinition{
        .id = "poorcraftultra:green_wool",
        .name = "Green Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Red Wool
    registerBlock("poorcraftultra:red_wool", BlockDefinition{
        .id = "poorcraftultra:red_wool",
        .name = "Red Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // Black Wool
    registerBlock("poorcraftultra:black_wool", BlockDefinition{
        .id = "poorcraftultra:black_wool",
        .name = "Black Wool",
        .material = Material::Wool,
        .hardness = 0.8f,
        .blastResistance = 0.8f,
        .flammable = true,
        .requiredTool = ToolType::Shears,
        .sounds = BlockSoundGroup::WOOL
    });
    
    // ============================================
    // LIGHT SOURCES
    // ============================================
    
    // Lantern
    registerBlock("poorcraftultra:lantern", BlockDefinition{
        .id = "poorcraftultra:lantern",
        .name = "Lantern",
        .material = Material::Metal,
        .hardness = 3.5f,
        .blastResistance = 3.5f,
        .opaque = false,
        .requiredTool = ToolType::Pickaxe,
        .renderType = RenderType::Cutout,
        .lightEmission = 15,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Soul Lantern
    registerBlock("poorcraftultra:soul_lantern", BlockDefinition{
        .id = "poorcraftultra:soul_lantern",
        .name = "Soul Lantern",
        .material = Material::Metal,
        .hardness = 3.5f,
        .blastResistance = 3.5f,
        .opaque = false,
        .requiredTool = ToolType::Pickaxe,
        .renderType = RenderType::Cutout,
        .lightEmission = 10,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Campfire
    registerBlock("poorcraftultra:campfire", BlockDefinition{
        .id = "poorcraftultra:campfire",
        .name = "Campfire",
        .material = Material::Wood,
        .hardness = 2.0f,
        .blastResistance = 2.0f,
        .opaque = false,
        .renderType = RenderType::Cutout,
        .lightEmission = 15,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Sea Lantern
    registerBlock("poorcraftultra:sea_lantern", BlockDefinition{
        .id = "poorcraftultra:sea_lantern",
        .name = "Sea Lantern",
        .material = Material::Glass,
        .hardness = 0.3f,
        .blastResistance = 0.3f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .lightEmission = 15,
        .sounds = BlockSoundGroup::GLASS
    });
    
    // Jack o'Lantern
    registerBlock("poorcraftultra:jack_o_lantern", BlockDefinition{
        .id = "poorcraftultra:jack_o_lantern",
        .name = "Jack o'Lantern",
        .material = Material::Vegetable,
        .hardness = 1.0f,
        .blastResistance = 1.0f,
        .lightEmission = 15,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Shroomlight
    registerBlock("poorcraftultra:shroomlight", BlockDefinition{
        .id = "poorcraftultra:shroomlight",
        .name = "Shroomlight",
        .material = Material::Vegetable,
        .hardness = 1.0f,
        .blastResistance = 1.0f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .lightEmission = 15,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // ============================================
    // UTILITY BLOCKS
    // ============================================
    
    // Anvil
    registerBlock("poorcraftultra:anvil", BlockDefinition{
        .id = "poorcraftultra:anvil",
        .name = "Anvil",
        .material = Material::Metal,
        .hardness = 5.0f,
        .blastResistance = 1200.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Enchanting Table
    registerBlock("poorcraftultra:enchanting_table", BlockDefinition{
        .id = "poorcraftultra:enchanting_table",
        .name = "Enchanting Table",
        .material = Material::Stone,
        .hardness = 5.0f,
        .blastResistance = 1200.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .lightEmission = 7,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Brewing Stand
    registerBlock("poorcraftultra:brewing_stand", BlockDefinition{
        .id = "poorcraftultra:brewing_stand",
        .name = "Brewing Stand",
        .material = Material::Metal,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .opaque = false,
        .renderType = RenderType::Cutout,
        .lightEmission = 1,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Cauldron
    registerBlock("poorcraftultra:cauldron", BlockDefinition{
        .id = "poorcraftultra:cauldron",
        .name = "Cauldron",
        .material = Material::Metal,
        .hardness = 2.0f,
        .blastResistance = 2.0f,
        .opaque = false,
        .requiredTool = ToolType::Pickaxe,
        .renderType = RenderType::Cutout,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Bell
    registerBlock("poorcraftultra:bell", BlockDefinition{
        .id = "poorcraftultra:bell",
        .name = "Bell",
        .material = Material::Metal,
        .hardness = 5.0f,
        .blastResistance = 5.0f,
        .opaque = false,
        .requiredTool = ToolType::Pickaxe,
        .renderType = RenderType::Cutout,
        .sounds = BlockSoundGroup::METAL
    });
    
    // Beehive
    registerBlock("poorcraftultra:beehive", BlockDefinition{
        .id = "poorcraftultra:beehive",
        .name = "Beehive",
        .material = Material::Wood,
        .hardness = 0.6f,
        .blastResistance = 0.6f,
        .flammable = true,
        .requiredTool = ToolType::Axe,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // ============================================
    // REDSTONE COMPONENTS
    // ============================================
    
    // Redstone Lamp
    registerBlock("poorcraftultra:redstone_lamp", BlockDefinition{
        .id = "poorcraftultra:redstone_lamp",
        .name = "Redstone Lamp",
        .material = Material::Stone,
        .hardness = 0.3f,
        .blastResistance = 0.3f,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Piston
    registerBlock("poorcraftultra:piston", BlockDefinition{
        .id = "poorcraftultra:piston",
        .name = "Piston",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 1.5f,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Sticky Piston
    registerBlock("poorcraftultra:sticky_piston", BlockDefinition{
        .id = "poorcraftultra:sticky_piston",
        .name = "Sticky Piston",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 1.5f,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Dispenser
    registerBlock("poorcraftultra:dispenser", BlockDefinition{
        .id = "poorcraftultra:dispenser",
        .name = "Dispenser",
        .material = Material::Stone,
        .hardness = 3.5f,
        .blastResistance = 3.5f,
        .requiredTool = ToolType::Pickaxe,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Dropper
    registerBlock("poorcraftultra:dropper", BlockDefinition{
        .id = "poorcraftultra:dropper",
        .name = "Dropper",
        .material = Material::Stone,
        .hardness = 3.5f,
        .blastResistance = 3.5f,
        .requiredTool = ToolType::Pickaxe,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Observer
    registerBlock("poorcraftultra:observer", BlockDefinition{
        .id = "poorcraftultra:observer",
        .name = "Observer",
        .material = Material::Stone,
        .hardness = 3.5f,
        .blastResistance = 3.5f,
        .requiredTool = ToolType::Pickaxe,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Hopper
    registerBlock("poorcraftultra:hopper", BlockDefinition{
        .id = "poorcraftultra:hopper",
        .name = "Hopper",
        .material = Material::Metal,
        .hardness = 3.0f,
        .blastResistance = 3.0f,
        .opaque = false,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .renderType = RenderType::Cutout,
        .sounds = BlockSoundGroup::METAL
    });
    
    // ============================================
    // DECORATIVE & MISC
    // ============================================
    
    // Bookshelf
    registerBlock("poorcraftultra:bookshelf", BlockDefinition{
        .id = "poorcraftultra:bookshelf",
        .name = "Bookshelf",
        .material = Material::Wood,
        .hardness = 1.5f,
        .blastResistance = 1.5f,
        .flammable = true,
        .requiredTool = ToolType::Axe,
        .sounds = BlockSoundGroup::WOOD
    });
    
    // Ladder
    registerBlock("poorcraftultra:ladder", BlockDefinition{
        .id = "poorcraftultra:ladder",
        .name = "Ladder",
        .material = Material::Decoration,
        .hardness = 0.4f,
        .blastResistance = 0.4f,
        .opaque = false,
        .renderType = RenderType::Cutout,
        .sounds = BlockSoundGroup::LADDER
    });
    
    // Snow Block
    registerBlock("poorcraftultra:snow_block", BlockDefinition{
        .id = "poorcraftultra:snow_block",
        .name = "Snow Block",
        .material = Material::Snow,
        .hardness = 0.2f,
        .blastResistance = 0.2f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::SNOW
    });
    
    // Ice
    registerBlock("poorcraftultra:ice", BlockDefinition{
        .id = "poorcraftultra:ice",
        .name = "Ice",
        .material = Material::Ice,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .sounds = BlockSoundGroup::GLASS
    });
    
    // Packed Ice
    registerBlock("poorcraftultra:packed_ice", BlockDefinition{
        .id = "poorcraftultra:packed_ice",
        .name = "Packed Ice",
        .material = Material::Ice,
        .hardness = 0.5f,
        .blastResistance = 0.5f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .sounds = BlockSoundGroup::GLASS
    });
    
    // Blue Ice
    registerBlock("poorcraftultra:blue_ice", BlockDefinition{
        .id = "poorcraftultra:blue_ice",
        .name = "Blue Ice",
        .material = Material::Ice,
        .hardness = 2.8f,
        .blastResistance = 2.8f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .sounds = BlockSoundGroup::GLASS
    });
    
    // Clay
    registerBlock("poorcraftultra:clay", BlockDefinition{
        .id = "poorcraftultra:clay",
        .name = "Clay",
        .material = Material::Dirt,
        .hardness = 0.6f,
        .blastResistance = 0.6f,
        .requiredTool = ToolType::Shovel,
        .sounds = BlockSoundGroup::GRAVEL
    });
    
    // Sponge
    registerBlock("poorcraftultra:sponge", BlockDefinition{
        .id = "poorcraftultra:sponge",
        .name = "Sponge",
        .material = Material::Dirt,
        .hardness = 0.6f,
        .blastResistance = 0.6f,
        .sounds = BlockSoundGroup::GRASS
    });
    
    // ============================================
    // FLUIDS (level property)
    // ============================================
    
    // Water (source)
    registerBlock("poorcraftultra:water", BlockDefinition{
        .id = "poorcraftultra:water",
        .name = "Water",
        .material = Material::Water,
        .hardness = 100.0f,
        .blastResistance = 100.0f,
        .solid = false,
        .opaque = false,
        .replaceable = true,
        .renderType = RenderType::Translucent,
        .lightEmission = 0,
        .lightOpacity = 3,
        .collisionShape = VoxelShape::empty()
    });
    
    // Water (flowing)
    registerBlock("poorcraftultra:flowing_water", BlockDefinition{
        .id = "poorcraftultra:flowing_water",
        .name = "Water (Flowing)",
        .material = Material::Water,
        .hardness = 100.0f,
        .blastResistance = 100.0f,
        .solid = false,
        .opaque = false,
        .replaceable = true,
        .renderType = RenderType::Translucent,
        .lightEmission = 0,
        .lightOpacity = 3,
        .collisionShape = VoxelShape::empty(),
        .properties = {{
            .name = "level",
            .type = BlockProperty::Type::Int,
            .defaultValue = 1,
            .minValue = 1,
            .maxValue = 7
        }}
    });
    
    // Lava (source)
    registerBlock("poorcraftultra:lava", BlockDefinition{
        .id = "poorcraftultra:lava",
        .name = "Lava",
        .material = Material::Lava,
        .hardness = 100.0f,
        .blastResistance = 100.0f,
        .solid = false,
        .opaque = false,
        .replaceable = true,
        .renderType = RenderType::Translucent,
        .lightEmission = 15,
        .lightOpacity = 255,
        .collisionShape = VoxelShape::empty()
    });
    
    // Lava (flowing)
    registerBlock("poorcraftultra:flowing_lava", BlockDefinition{
        .id = "poorcraftultra:flowing_lava",
        .name = "Lava (Flowing)",
        .material = Material::Lava,
        .hardness = 100.0f,
        .blastResistance = 100.0f,
        .solid = false,
        .opaque = false,
        .replaceable = true,
        .renderType = RenderType::Translucent,
        .lightEmission = 15,
        .lightOpacity = 255,
        .collisionShape = VoxelShape::empty(),
        .properties = {{
            .name = "level",
            .type = BlockProperty::Type::Int,
            .defaultValue = 1,
            .minValue = 1,
            .maxValue = 7
        }}
    });
    
    // Bubble Column (water with bubbles)
    registerBlock("poorcraftultra:bubble_column", BlockDefinition{
        .id = "poorcraftultra:bubble_column",
        .name = "Bubble Column",
        .material = Material::Water,
        .hardness = 100.0f,
        .blastResistance = 100.0f,
        .solid = false,
        .opaque = false,
        .replaceable = true,
        .renderType = RenderType::Translucent,
        .collisionShape = VoxelShape::empty()
    });
    
    // ============================================
    // FARMING
    // ============================================
    
    // Farmland
    registerBlock("poorcraftultra:farmland", BlockDefinition{
        .id = "poorcraftultra:farmland",
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
    registerBlock("poorcraftultra:terracotta", BlockDefinition{
        .id = "poorcraftultra:terracotta",
        .name = "Terracotta",
        .material = Material::Stone,
        .hardness = 1.25f,
        .blastResistance = 4.2f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // White Concrete
    registerBlock("poorcraftultra:white_concrete", BlockDefinition{
        .id = "poorcraftultra:white_concrete",
        .name = "White Concrete",
        .material = Material::Stone,
        .hardness = 1.8f,
        .blastResistance = 1.8f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Black Concrete
    registerBlock("poorcraftultra:black_concrete", BlockDefinition{
        .id = "poorcraftultra:black_concrete",
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
    registerBlock("poorcraftultra:prismarine", BlockDefinition{
        .id = "poorcraftultra:prismarine",
        .name = "Prismarine",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Prismarine Bricks
    registerBlock("poorcraftultra:prismarine_bricks", BlockDefinition{
        .id = "poorcraftultra:prismarine_bricks",
        .name = "Prismarine Bricks",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Dark Prismarine
    registerBlock("poorcraftultra:dark_prismarine", BlockDefinition{
        .id = "poorcraftultra:dark_prismarine",
        .name = "Dark Prismarine",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 6.0f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::STONE
    });
    
    // Sea Pickle
    registerBlock("poorcraftultra:sea_pickle", BlockDefinition{
        .id = "poorcraftultra:sea_pickle",
        .name = "Sea Pickle",
        .material = Material::Decoration,
        .hardness = 0.0f,
        .blastResistance = 0.0f,
        .solid = false,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .lightEmission = 6
    });
    
    // ============================================
    // AMETHYST
    // ============================================
    
    // Amethyst Block
    registerBlock("poorcraftultra:amethyst_block", BlockDefinition{
        .id = "poorcraftultra:amethyst_block",
        .name = "Block of Amethyst",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 1.5f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::AMETHYST
    });
    
    // Budding Amethyst
    registerBlock("poorcraftultra:budding_amethyst", BlockDefinition{
        .id = "poorcraftultra:budding_amethyst",
        .name = "Budding Amethyst",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 1.5f,
        .requiredTool = ToolType::Pickaxe,
        .minimumTier = ToolTier::Wood,
        .sounds = BlockSoundGroup::AMETHYST
    });
    
    // Calcite
    registerBlock("poorcraftultra:calcite", BlockDefinition{
        .id = "poorcraftultra:calcite",
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
    registerBlock("poorcraftultra:sculk", BlockDefinition{
        .id = "poorcraftultra:sculk",
        .name = "Sculk",
        .material = Material::Stone,
        .hardness = 0.6f,
        .blastResistance = 0.6f,
        .sounds = BlockSoundGroup::SCULK
    });
    
    // Sculk Sensor
    registerBlock("poorcraftultra:sculk_sensor", BlockDefinition{
        .id = "poorcraftultra:sculk_sensor",
        .name = "Sculk Sensor",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 1.5f,
        .opaque = false,
        .renderType = RenderType::Translucent,
        .sounds = BlockSoundGroup::SCULK
    });
    
    // Sculk Shrieker
    registerBlock("poorcraftultra:sculk_shrieker", BlockDefinition{
        .id = "poorcraftultra:sculk_shrieker",
        .name = "Sculk Shrieker",
        .material = Material::Stone,
        .hardness = 1.5f,
        .blastResistance = 1.5f,
        .sounds = BlockSoundGroup::SCULK
    });
    
    // Reinforced Deepslate
    registerBlock("poorcraftultra:reinforced_deepslate", BlockDefinition{
        .id = "poorcraftultra:reinforced_deepslate",
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
