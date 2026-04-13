/**
 * @file MiningSystem.cpp
 * @brief Block breaking / mining system implementation
 */

#include <VoxelForge/game/MiningSystem.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>
#include <cmath>

namespace VoxelForge {

// ============================================================================
// Constants
// ============================================================================

// Tool tier levels (must match Feature Matrix tier system)
static constexpr int TIER_HAND      = 0;
static constexpr int TIER_WOOD      = 1;
static constexpr int TIER_STONE     = 2;
static constexpr int TIER_IRON      = 3;
static constexpr int TIER_DIAMOND   = 4;
static constexpr int TIER_NETHERITE = 5;

// Base mining speed multipliers per tool material
static constexpr float SPEED_HAND      = 1.0f;
static constexpr float SPEED_WOOD      = 2.0f;
static constexpr float SPEED_STONE     = 4.0f;
static constexpr float SPEED_IRON      = 6.0f;
static constexpr float SPEED_DIAMOND   = 8.0f;
static constexpr float SPEED_NETHERITE = 9.0f;

// Mining speed modifiers per tool type against its intended material
static constexpr float TOOL_MATCH_MULTIPLIER = 1.0f;
static constexpr float TOOL_MISMATCH_MULTIPLIER = 0.3f; // Using wrong tool type

// Creative mode instant break time
static constexpr float CREATIVE_BREAK_TIME = 0.0f;

// Minimum hardness for "instant mine" with correct tool
static constexpr float INSTANT_MINE_HARDNESS = 0.0f;

// ============================================================================
// Mining Lifecycle
// ============================================================================

bool MiningSystem::startMining(Player& player, const BlockPos& target, const glm::vec3& faceNormal) {
    // (Player is an Entity — we'll look up the PlayerComponent via ECS in a full integration.
    //  For now, the public API accepts Player& for future wiring.)
    (void)player;

    progress_ = MiningProgress{};
    progress_.targetBlock = target;
    progress_.normal = faceNormal;
    progress_.active = true;
    progress_.progress = 0.0f;

    VF_TRACE("MiningSystem: started mining at ({}, {}, {})", target.x, target.y, target.z);
    return true;
}

std::optional<MiningResult> MiningSystem::tick(Player& player, World& world, float deltaTime) {
    if (!progress_.active) return std::nullopt;

    // Validate target block still exists and hasn't changed
    if (!validateTarget(world)) {
        VF_TRACE("MiningSystem: target block changed, cancelling");
        cancelMining();
        return std::nullopt;
    }

    BlockState block = world.getBlock(progress_.targetBlock);
    if (block.isAir()) {
        cancelMining();
        return std::nullopt;
    }

    // For now, get tool from the player's component via the entity system.
    // In the full integration this will use ECS lookup. We compute speed
    // from the block hardness as a baseline.
    float breakTime = getBreakTime(block, ItemStack{});

    // TODO: When ECS-wired, fetch held tool from PlayerComponent::inventory
    // auto& inv = player.getComponent<PlayerComponent>().inventory;
    // ItemStack& tool = inv.getSelectedStack();
    // breakTime = getBreakTime(block, tool);

    if (breakTime <= 0.0f) {
        // Instant break (creative or zero-hardness block)
        progress_.progress = 1.0f;
    } else {
        progress_.speed = 1.0f / breakTime;
        progress_.progress += progress_.speed * deltaTime;
    }

    // Block broken
    if (progress_.progress >= 1.0f) {
        MiningResult result;
        result.success = true;
        result.blockPos = progress_.targetBlock;
        result.brokenBlock = block;
        result.timeToBreak = (progress_.speed > 0.0f) ? (1.0f / progress_.speed) : 0.0f;
        result.drops = calculateDrops(block, ItemStack{}, MiningEnchantments{});
        result.xpDrop = calculateXPDrop(block);
        result.toolBroken = false;

        // Set block to air
        world.setBlock(progress_.targetBlock, BlockState{});

        VF_INFO("MiningSystem: broke block at ({}, {}, {}), time={:.3f}s, drops={}",
                progress_.targetBlock.x, progress_.targetBlock.y, progress_.targetBlock.z,
                result.timeToBreak, result.drops.size());

        progress_.active = false;
        progress_.progress = 0.0f;
        return result;
    }

    return std::nullopt;
}

void MiningSystem::cancelMining() {
    if (progress_.active) {
        VF_TRACE("MiningSystem: mining cancelled at ({}, {}, {})",
                 progress_.targetBlock.x, progress_.targetBlock.y, progress_.targetBlock.z);
    }
    progress_ = MiningProgress{};
}

bool MiningSystem::validateTarget(World& world) const {
    BlockState current = world.getBlock(progress_.targetBlock);
    if (progress_.originalBlockId < 0) {
        // First tick: record the block
        const_cast<MiningSystem*>(this)->progress_.originalBlockId =
            static_cast<int>(current.getBlockId());
        return true;
    }
    return current.getBlockId() == static_cast<BlockID>(progress_.originalBlockId);
}

// ============================================================================
// Static Helpers
// ============================================================================

float MiningSystem::calculateMiningSpeed(const ItemStack& tool, const BlockState& block) {
    if (tool.isEmpty()) {
        return SPEED_HAND;
    }

    const ItemDefinition* def = ItemRegistry::get().getDefinition(tool.getItem());
    if (!def || !def->toolProperties.has_value()) {
        return SPEED_HAND;
    }

    const auto& toolProps = def->toolProperties.value();
    float baseSpeed = SPEED_HAND;

    switch (static_cast<int>(toolProps.level)) {
        case 1: baseSpeed = SPEED_WOOD; break;
        case 2: baseSpeed = SPEED_STONE; break;
        case 3: baseSpeed = SPEED_IRON; break;
        case 4: baseSpeed = SPEED_DIAMOND; break;
        case 5: baseSpeed = SPEED_NETHERITE; break;
        default: baseSpeed = SPEED_HAND; break;
    }

    // Check tool type vs block material
    const auto& blockDef = block.getDefinition();
    bool toolMatches = false;

    switch (toolProps.type) {
        case ToolProperties::Type::Pickaxe:
            toolMatches = (blockDef.material == Material::Stone ||
                          blockDef.material == Material::Metal ||
                          blockDef.material == Material::Ice);
            break;
        case ToolProperties::Type::Axe:
            toolMatches = (blockDef.material == Material::Wood ||
                          blockDef.material == Material::NetherWood ||
                          blockDef.material == Material::Plant);
            break;
        case ToolProperties::Type::Shovel:
            toolMatches = (blockDef.material == Material::Dirt ||
                          blockDef.material == Material::Sand ||
                          blockDef.material == Material::Snow ||
                          blockDef.material == Material::Clay);
            break;
        case ToolProperties::Type::Hoe:
            toolMatches = false; // Hoe doesn't speed up mining
            break;
        case ToolProperties::Type::Sword:
            toolMatches = (blockDef.material == Material::Plant ||
                          blockDef.material == Material::Vegetable ||
                          blockDef.material == Material::Leaves);
            break;
        case ToolProperties::Type::Shears:
            toolMatches = (blockDef.material == Material::Leaves ||
                          blockDef.material == Material::Wool);
            break;
        default:
            break;
    }

    float multiplier = toolMatches ? TOOL_MATCH_MULTIPLIER : TOOL_MISMATCH_MULTIPLIER;
    float speed = baseSpeed * multiplier;

    // Apply tool's miningSpeed property
    speed *= toolProps.miningSpeed;

    return speed;
}

bool MiningSystem::canHarvest(const ItemStack& tool, const BlockState& block) {
    const auto& blockDef = block.getDefinition();

    // Blocks that don't require a tool can always be harvested
    if (!blockDef.requiresTool) {
        return true;
    }

    // Check tool tier
    int toolTier = getToolTierLevel(tool);
    int requiredTier = getRequiredTierLevel(block);

    if (toolTier < requiredTier) {
        return false;
    }

    // Check tool type matches
    if (tool.isEmpty()) return false;

    const ItemDefinition* def = ItemRegistry::get().getDefinition(tool.getItem());
    if (!def || !def->toolProperties.has_value()) return false;

    const auto& toolProps = def->toolProperties.value();

    switch (blockDef.requiredTool) {
        case ToolType::Pickaxe:
            return toolProps.type == ToolProperties::Type::Pickaxe;
        case ToolType::Axe:
            return toolProps.type == ToolProperties::Type::Axe;
        case ToolType::Shovel:
            return toolProps.type == ToolProperties::Type::Shovel;
        case ToolType::Hoe:
            return toolProps.type == ToolProperties::Type::Hoe;
        case ToolType::Sword:
            return toolProps.type == ToolProperties::Type::Sword;
        case ToolType::Shears:
            return toolProps.type == ToolProperties::Type::Shears;
        default:
            return true;
    }
}

float MiningSystem::getBreakTime(const BlockState& block, const ItemStack& tool) {
    const auto& blockDef = block.getDefinition();
    float hardness = blockDef.hardness;

    // Air and unbreakable blocks
    if (block.isAir()) return 0.0f;
    if (hardness < 0.0f) return -1.0f; // Indestructible (bedrock)

    // Zero-hardness blocks break instantly
    if (hardness == 0.0f) return 0.0f;

    float speed = calculateMiningSpeed(tool, block);
    bool canHarvestBlock = canHarvest(tool, block);

    // Vanilla formula: break time = hardness * 1.5 / speed (if can harvest)
    //                  break time = hardness * 5.0 / speed (if cannot harvest)
    float divisor = canHarvestBlock ? speed : (speed * 0.3f);
    if (divisor <= 0.0f) divisor = 0.001f;

    float breakTime = (hardness * (canHarvestBlock ? 1.5f : 5.0f)) / divisor;

    // Apply efficiency enchantment bonus
    MiningEnchantments enchants = getEnchantments(tool);
    if (enchants.efficiency > 0 && canHarvestBlock) {
        breakTime /= (1.0f + efficiencyBonus(enchants.efficiency));
    }

    return std::max(breakTime, 0.0f); // seconds
}

std::vector<ItemStack> MiningSystem::calculateDrops(const BlockState& block,
                                                     const ItemStack& tool,
                                                     const MiningEnchantments& enchants) {
    std::vector<ItemStack> drops;

    // If silk touch, drop the block itself
    if (enchants.silkTouch > 0) {
        // Create an item stack from the block
        // Block ID is used as a simple item mapping
        ItemStack drop(block.getBlockId(), 1);
        drops.push_back(drop);
        return drops;
    }

    // Check if tool can harvest the block
    if (!canHarvest(tool, block)) {
        // No drops if can't harvest (e.g., stone with bare hands)
        return drops;
    }

    // Basic drop: 1 of the block item
    int count = 1;

    // Fortune bonus
    if (enchants.fortune > 0) {
        // Simple fortune formula: random(0, fortune+1) bonus items
        // Using a deterministic approximation for server-side consistency
        int bonus = enchants.fortune; // Simplified: always give max fortune bonus
        count += bonus;
    }

    ItemStack drop(block.getBlockId(), static_cast<ItemCount>(std::min(count, 64)));
    drops.push_back(drop);
    return drops;
}

bool MiningSystem::shouldDamageTool(const ItemStack& tool, const BlockState& block) {
    (void)block;
    if (tool.isEmpty()) return false;

    const ItemDefinition* def = ItemRegistry::get().getDefinition(tool.getItem());
    if (!def || !def->toolProperties.has_value()) return false;

    return true; // Tools always take damage when mining
}

bool MiningSystem::applyDurabilityDamage(ItemStack& tool, int amount) {
    if (tool.isEmpty()) return false;

    // Check unbreaking enchantment
    MiningEnchantments enchants = getEnchantments(tool);
    if (unbreakingRoll(enchants.unbreaking)) {
        return false; // Damage was prevented
    }

    // Reduce durability via NBT
    auto& nbt = tool.getNBT();
    int currentDurability = 0;
    auto it = nbt.integers.find("durability");
    if (it != nbt.integers.end()) {
        currentDurability = it->second;
    }

    currentDurability += amount;

    // Check if tool has max durability
    const ItemDefinition* def = ItemRegistry::get().getDefinition(tool.getItem());
    int maxDurability = 0;
    if (def && def->toolProperties.has_value()) {
        maxDurability = def->toolProperties->durability;
    }

    if (maxDurability > 0 && currentDurability >= maxDurability) {
        // Tool broke
        tool.setCount(0); // Destroy the item
        VF_TRACE("MiningSystem: tool broke after {} damage", currentDurability);
        return true;
    }

    nbt.integers["durability"] = currentDurability;
    return false;
}

MiningEnchantments MiningSystem::getEnchantments(const ItemStack& tool) {
    MiningEnchantments enchants{};
    if (tool.isEmpty()) return enchants;

    const auto& nbt = tool.getNBT();
    auto it = nbt.integers.find("enchant_efficiency");
    if (it != nbt.integers.end()) enchants.efficiency = it->second;

    it = nbt.integers.find("enchant_fortune");
    if (it != nbt.integers.end()) enchants.fortune = it->second;

    it = nbt.integers.find("enchant_silk_touch");
    if (it != nbt.integers.end()) enchants.silkTouch = it->second;

    it = nbt.integers.find("enchant_unbreaking");
    if (it != nbt.integers.end()) enchants.unbreaking = it->second;

    return enchants;
}

bool MiningSystem::unbreakingRoll(int unbreakingLevel) {
    if (unbreakingLevel <= 0) return false;
    // Vanilla: chance to negate = level / (level + 1)
    // For deterministic behavior we use a simple threshold
    // In production, this should use the RNG system
    return (unbreakingLevel >= 3); // Simplified: max unbreaking always negates
}

int MiningSystem::getToolTierLevel(const ItemStack& tool) {
    if (tool.isEmpty()) return TIER_HAND;

    const ItemDefinition* def = ItemRegistry::get().getDefinition(tool.getItem());
    if (!def || !def->toolProperties.has_value()) return TIER_HAND;

    return def->toolProperties->level;
}

int MiningSystem::getRequiredTierLevel(const BlockState& block) {
    const auto& blockDef = block.getDefinition();

    switch (blockDef.minimumTier) {
        case ToolTier::None:      return TIER_HAND;
        case ToolTier::Wood:      return TIER_WOOD;
        case ToolTier::Stone:     return TIER_STONE;
        case ToolTier::Iron:      return TIER_IRON;
        case ToolTier::Diamond:   return TIER_DIAMOND;
        case ToolTier::Netherite: return TIER_NETHERITE;
    }
    return TIER_HAND;
}

int MiningSystem::calculateXPDrop(const BlockState& block) {
    const auto& blockDef = block.getDefinition();

    // Ore blocks drop XP based on material
    switch (blockDef.material) {
        case Material::Metal:
            return 1; // Most ores drop 1-3 XP, simplified to 1
        default:
            return 0;
    }
}

float MiningSystem::efficiencyBonus(int level) {
    // Vanilla: efficiency adds level^2 + 1 to mining speed
    return static_cast<float>(level * level + 1);
}

} // namespace VoxelForge
