/**
 * @file Player.hpp
 * @brief Player entity component and system
 *
 * Implements the player component with full combat mechanics including
 * the 1.9+ attack cooldown system, critical hits, knockback, and
 * invulnerability frames.
 */

#pragma once

#include <VoxelForge/core/ECS.hpp>
#include <VoxelForge/game/Inventory.hpp>
#include <VoxelForge/game/Item.hpp>
#include <VoxelForge/entity/Entity.hpp>
#include <cstdint>

namespace VoxelForge {

// Damage types for combat calculations
enum class DamageType : uint8_t {
    Physical,    // Melee / projectile
    Fire,        // Fire / lava
    Magic,       // Potions / instant damage
    Fall,        // Fall damage
    Drowning,    // Water drowning
    Starvation,  // Hunger
    Void,        // Below world
    Explosion,   // TNT / creeper
    Thorns,      // Thorns enchantment
    Wither,      // Wither effect
    Suffocation, // Inside block
    Cramming,    // Too many entities
    Freeze,      // Powder snow
    Generic      // /kill, etc.
};

// Gamemode enum
enum class Gamemode : uint8_t {
    Survival,
    Creative,
    Adventure,
    Spectator
};

// Player abilities
struct PlayerAbilities {
    bool isFlying = false;
    bool canFly = false;
    bool isInvulnerable = false;
    bool isInstabuild = false;     // Instant block break (creative)
    bool mayBuild = true;          // Can place/break blocks
    float flySpeed = 0.05f;
    float walkSpeed = 0.1f;
};

// Attack result returned after performing an attack
struct AttackResult {
    float damageDealt = 0.0f;
    bool wasCritical = false;
    float cooldownStrength = 0.0f;  // 0.0 - 1.0, how "charged" the attack was
    bool didHit = false;
};

/**
 * PlayerComponent - ECS component attached to player entities.
 *
 * Contains all player-specific state including health, hunger, experience,
 * combat state, and game mode.
 */
struct PlayerComponent {
    // === Health & Status ===
    float health = 20.0f;
    float maxHealth = 20.0f;
    float absorption = 0.0f;       // Absorption hearts
    float armor = 0.0f;
    float armorToughness = 0.0f;

    // === Hunger ===
    int foodLevel = 20;             // 0-20
    float saturation = 5.0f;       // Saturation level
    float exhaustion = 0.0f;       // Exhaustion accumulator
    int healthRegenTimer = 0;       // Ticks until next regen

    // === Experience ===
    int level = 0;
    float experience = 0.0f;       // Progress toward next level (0.0 - 1.0)
    int totalExperience = 0;
    int experienceToNextLevel = 7;

    // === Combat State (1.9+ Attack Cooldown) ===
    float attackCooldown = 0.0f;            // Seconds remaining in cooldown
    float lastAttackTime = 0.0f;            // Timestamp of last attack
    float attackSpeed = 4.0f;               // Attacks per second (base)
    float attackDamage = 1.0f;              // Base attack damage (fist)
    float cooldownProgress = 0.0f;          // 0.0 - 1.0, current charge level
    int invulnerabilityTicks = 0;           // i-frames remaining (20 ticks = 1s)
    float lastDamageTaken = 0.0f;           // For rendering damage tilt

    // === Movement State ===
    float currentSpeed = 4.317f;    // Blocks per second
    bool isUnderwater = false;
    bool isOnGround = true;
    bool hasAquaAffinity = false;   // From helmet enchant

    // === Air ===
    float airSupply = 300.0f;       // In ticks (300 = 15 seconds)
    float maxAirSupply = 300.0f;

    // === Gamemode & Abilities ===
    Gamemode gamemode = Gamemode::Survival;
    PlayerAbilities abilities;

    // === Inventory ===
    PlayerInventory inventory;

    // === Spawn ===
    Vec3 spawnPosition = Vec3(0.0f);
    bool spawnForced = false;       // True if spawn was manually set
};

/**
 * PlayerSystem - ECS system that updates all player entities.
 *
 * Handles:
 * - Movement speed calculation
 * - Attack cooldown ticking (1.9+ combat)
 * - Experience leveling
 * - Air supply / drowning
 * - Food exhaustion and health regeneration
 * - Invulnerability frame countdown
 *
 * Combat API:
 * - getAttackCooldownProgress(): Returns 0.0-1.0 charge level
 * - performAttack(): Executes attack with cooldown-based damage scaling
 * - calculateFinalDamage(): Full damage pipeline (armor, enchantments, potions)
 */
class PlayerSystem {
public:
    PlayerSystem();

    /**
     * Update all player entities. Call once per frame.
     */
    void update(ECSWorld& world, float deltaTime);

    // === Combat API ===

    /**
     * Get the current attack cooldown progress for a player.
     * Returns 0.0 (just attacked) to 1.0 (fully charged).
     *
     * This drives the 1.9+ combat mechanic: damage is scaled by
     * this value, and only attacks at >= 0.85 charge get the
     * sweep attack bonus for swords.
     */
    float getAttackCooldownProgress(PlayerComponent& player) const;

    /**
     * Reset the attack cooldown (called when player attacks).
     * The cooldown duration is 1.0 / attackSpeed seconds.
     */
    void resetAttackCooldown(PlayerComponent& player);

    /**
     * Perform an attack from a player onto a target entity.
     * Applies cooldown scaling, critical hit check, knockback.
     * Returns an AttackResult with details.
     */
    AttackResult performAttack(ECSWorld& world, EntityID attacker,
                               EntityID target, PlayerComponent& player);

    /**
     * Calculate final damage after armor, toughness, enchantments, and effects.
     * Uses the vanilla Minecraft damage formula:
     *   damage = damage * (1 - min(20, max(armor / 5, armor - damage * (2/4+toughness))) / 25)
     */
    float calculateFinalDamage(float baseDamage, float armor,
                               float armorToughness) const;

    /**
     * Check if an attack should be a critical hit.
     * Critical hits occur when the player is falling (not on ground,
     * not in water, not climbing, and not affected by Blindness).
     */
    bool isCriticalHit(PlayerComponent& player,
                       const EntityBaseComponent& base) const;

    // === Movement ===
    void updatePlayerMovement(PlayerComponent& player,
                              MovementComponent& movement, float deltaTime);

    // === Abilities ===
    void updatePlayerAbilities(PlayerComponent& player, float deltaTime);

    // === Experience ===
    static int calculateExperienceForLevel(int level);

    // === Mining ===
    float getDigSpeed(PlayerComponent& player, BlockState block, ToolType tool);

    // === Combat Getters ===
    float getAttackDamage(PlayerComponent& player);
    float getAttackSpeed(PlayerComponent& player);

    // === Food & Health ===
    void addExperience(PlayerComponent& player, int amount);
    void addExhaustion(PlayerComponent& player, float amount);
    void healFromFood(PlayerComponent& player);
};

} // namespace VoxelForge
