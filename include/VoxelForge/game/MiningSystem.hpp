/**
 * @file MiningSystem.hpp
 * @brief Block breaking / mining system for VoxelForge
 *
 * Implements the mining mechanics from section 5.2 of the Feature Matrix:
 * - Block breaking with mining speed based on tool + block hardness
 * - Tool requirements (tier system: wood < stone < iron < diamond < netherite)
 * - Durability loss on tool items
 * - Fortune / Silk Touch / Efficiency enchantment hooks
 * - Unbreaking enchantment hook
 *
 * Usage:
 *   MiningSystem mining;
 *   mining.startMining(player, targetPos, face);
 *   mining.tick(player, world, deltaTime);
 *   // ... when progress >= 1.0f, the block is broken automatically
 */

#pragma once

#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/world/BlockRegistry.hpp>
#include <VoxelForge/game/Item.hpp>
#include <VoxelForge/game/Inventory.hpp>
#include <cstdint>
#include <optional>

namespace VoxelForge {

class World;
class Player;

// ============================================
// Mining Progress State
// ============================================

struct MiningProgress {
    BlockPos targetBlock;           // Block being mined
    glm::vec3 normal;               // Face normal hit
    float progress = 0.0f;          // 0.0 to 1.0
    float speed = 0.0f;             // Calculated mining speed (blocks/sec)
    bool active = false;            // Is mining in progress?
    int originalBlockId = -1;       // Block ID when mining started (detects changes)
};

// ============================================
// Mining Result
// ============================================

struct MiningResult {
    bool success = false;
    BlockPos blockPos;
    BlockState brokenBlock;
    std::vector<ItemStack> drops;
    int xpDrop = 0;                 // Experience dropped
    float timeToBreak = 0.0f;       // How long it took in seconds
    bool toolBroken = false;        // Whether the tool broke during mining
};

// ============================================
// Enchantments (simplified, hook for future system)
// ============================================

struct MiningEnchantments {
    int efficiency = 0;     // Level 0-5
    int fortune = 0;        // Level 0-3
    int silkTouch = 0;      // Level 0-1
    int unbreaking = 0;     // Level 0-3
};

// ============================================
// Mining System
// ============================================

class MiningSystem {
public:
    MiningSystem() = default;
    ~MiningSystem() = default;

    // --- Mining lifecycle ---

    /// Begin mining a block. Returns false if the block cannot be mined.
    bool startMining(Player& player, const BlockPos& target, const glm::vec3& faceNormal);

    /// Continue mining (call every tick/frame). Returns a result when the block breaks.
    std::optional<MiningResult> tick(Player& player, World& world, float deltaTime);

    /// Abort the current mining operation.
    void cancelMining();

    /// Check if a block is currently being mined.
    bool isMining() const { return progress_.active; }

    /// Get current mining progress (0-1).
    float getProgress() const { return progress_.progress; }

    /// Get current mining speed in blocks per second.
    float getMiningSpeed() const { return progress_.speed; }

    /// Get the block currently being mined.
    const MiningProgress& getMiningProgress() const { return progress_; }

    // --- Static helpers (can be used without an instance) ---

    /// Calculate the mining speed multiplier from the held tool.
    /// Returns 1.0f for hands, higher for tools.
    static float calculateMiningSpeed(const ItemStack& tool, const BlockState& block);

    /// Check if the given tool can mine the block (tier check).
    static bool canHarvest(const ItemStack& tool, const BlockState& block);

    /// Get the break time in seconds for a block with a given tool.
    static float getBreakTime(const BlockState& block, const ItemStack& tool);

    /// Calculate drops for a broken block.
    static std::vector<ItemStack> calculateDrops(const BlockState& block,
                                                  const ItemStack& tool,
                                                  const MiningEnchantments& enchants);

    /// Check if a tool takes durability damage for mining a block.
    static bool shouldDamageTool(const ItemStack& tool, const BlockState& block);

    /// Apply durability damage to an item stack. Returns true if the tool broke.
    static bool applyDurabilityDamage(ItemStack& tool, int amount = 1);

    /// Read enchantments from item NBT (simplified).
    static MiningEnchantments getEnchantments(const ItemStack& tool);

    /// Apply Unbreaking: returns true if damage should be skipped.
    static bool unbreakingRoll(int unbreakingLevel);

    /// Get the tool tier level (0=hand, 1=wood, 2=stone, 3=iron, 4=diamond, 5=netherite).
    static int getToolTierLevel(const ItemStack& tool);

    /// Get the minimum tier required to harvest a block.
    static int getRequiredTierLevel(const BlockState& block);

    /// Calculate XP drop from a block (simplified for ores).
    static int calculateXPDrop(const BlockState& block);

private:
    MiningProgress progress_;

    // Speed adjustment for efficiency enchantment
    static float efficiencyBonus(int level);

    // Check block hasn't changed since mining started
    bool validateTarget(World& world) const;
};

} // namespace VoxelForge
