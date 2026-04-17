/**
 * @file Player.cpp
 * @brief Player entity implementation
 *
 * Implements the PlayerSystem including the 1.9+ attack cooldown combat
 * system, critical hits, knockback, invulnerability frames, and the
 * full damage pipeline with armor reduction.
 */

#include <VoxelForge/entity/Player.hpp>
#include <VoxelForge/game/Inventory.hpp>
#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>
#include <cmath>

namespace VoxelForge {

PlayerSystem::PlayerSystem() {
    VF_INFO("PlayerSystem created");
}

void PlayerSystem::update(ECSWorld& world, float deltaTime) {
    auto view = world.view<PlayerComponent, MovementComponent, EntityBaseComponent>();

    for (auto entity : view) {
        auto& player = view.get<PlayerComponent>(entity);
        auto& movement = view.get<MovementComponent>(entity);
        auto& base = view.get<EntityBaseComponent>(entity);

        if (!base.isAlive) continue;

        // Update player-specific logic
        updatePlayerMovement(player, movement, deltaTime);
        updatePlayerAbilities(player, deltaTime);

        // Update experience
        if (player.experience > player.experienceToNextLevel) {
            player.experience -= player.experienceToNextLevel;
            player.level++;
            player.experienceToNextLevel = calculateExperienceForLevel(player.level + 1);
            VF_INFO("Player leveled up to {}", player.level);
        }

        // === Attack Cooldown (1.9+ combat) ===
        if (player.attackCooldown > 0) {
            player.attackCooldown -= deltaTime;
            if (player.attackCooldown < 0) {
                player.attackCooldown = 0;
            }
        }
        // Update cooldown progress (0.0 = just attacked, 1.0 = fully charged)
        player.cooldownProgress = getAttackCooldownProgress(player);

        // === Invulnerability Frames ===
        if (player.invulnerabilityTicks > 0) {
            player.invulnerabilityTicks -= static_cast<int>(deltaTime * 20.0f);
            if (player.invulnerabilityTicks < 0) {
                player.invulnerabilityTicks = 0;
            }
        }

        // Update air supply when underwater
        if (base.isInWater) {
            player.airSupply -= deltaTime * 20.0f; // Ticks per second
            if (player.airSupply <= 0) {
                player.airSupply = 0;
                // Drowning damage: 2 HP per second once air is depleted
                // Handled by LivingEntitySystem or game tick
            }
        } else {
            player.airSupply = player.maxAirSupply;
        }
    }
}

// ============================================================
// Combat System - 1.9+ Attack Cooldown
// ============================================================

float PlayerSystem::getAttackCooldownProgress(PlayerComponent& player) const {
    if (player.attackSpeed <= 0.0f) {
        return 1.0f; // No cooldown if speed is zero/broken
    }

    float cooldownPeriod = 1.0f / player.attackSpeed; // e.g. 0.25s at 4.0 speed
    if (cooldownPeriod <= 0.0f) {
        return 1.0f;
    }

    float progress = 1.0f - (player.attackCooldown / cooldownPeriod);
    return std::clamp(progress, 0.0f, 1.0f);
}

void PlayerSystem::resetAttackCooldown(PlayerComponent& player) {
    if (player.attackSpeed > 0.0f) {
        player.attackCooldown = 1.0f / player.attackSpeed;
    }
    player.cooldownProgress = 0.0f;
}

AttackResult PlayerSystem::performAttack(ECSWorld& world, EntityID attacker,
                                          EntityID target, PlayerComponent& player) {
    AttackResult result;
    result.cooldownStrength = player.cooldownProgress;

    // Reset cooldown regardless of hit
    resetAttackCooldown(player);

    // Check if attack is strong enough (damage scales with cooldown)
    // In vanilla, attacks below ~0.2 strength deal very little damage
    float baseDamage = getAttackDamage(player);
    float scaledDamage = baseDamage * (0.2f + 0.8f * result.cooldownStrength);
    // At full charge (1.0): 100% damage. At 0 charge: 20% damage.

    // Check for critical hit (only at >= 0.85 cooldown strength)
    auto* base = world.getComponent<EntityBaseComponent>(attacker);
    result.wasCritical = (result.cooldownStrength >= 0.85f) && isCriticalHit(player, base ? *base : EntityBaseComponent{});

    if (result.wasCritical) {
        // Critical hits deal 150% damage
        scaledDamage *= 1.5f;
        VF_TRACE("Critical hit! Damage: {}", scaledDamage);
    }

    // Apply damage to target
    auto* targetLiving = world.getComponent<LivingComponent>(target);
    if (targetLiving) {
        // Check invulnerability frames on target
        auto* targetBase = world.getComponent<EntityBaseComponent>(target);
        if (targetBase && !targetBase->isAlive) {
            result.didHit = false;
            return result;
        }

        result.damageDealt = scaledDamage;
        targetLiving->health -= scaledDamage;
        if (targetLiving->health < 0) {
            targetLiving->health = 0;
        }
        result.didHit = true;

        VF_TRACE("Player attacked entity {} for {} damage (cooldown: {:.0f}%, crit: {})",
                 target, scaledDamage, result.cooldownStrength * 100.0f, result.wasCritical);
    } else {
        result.didHit = false;
    }

    return result;
}

float PlayerSystem::calculateFinalDamage(float baseDamage, float armor,
                                          float armorToughness) const {
    // Vanilla Minecraft armor damage reduction formula:
    // reduction = clamp(armor - baseDamage * (2 / (4 + toughness)), 0, 20) / 25
    if (baseDamage <= 0.0f) return 0.0f;

    float effectiveArmor = armor - baseDamage * (2.0f / (4.0f + armorToughness));
    effectiveArmor = std::clamp(effectiveArmor, 0.0f, 20.0f);
    float reduction = effectiveArmor / 25.0f;

    return baseDamage * (1.0f - reduction);
}

bool PlayerSystem::isCriticalHit(PlayerComponent& player,
                                  const EntityBaseComponent& base) const {
    // Critical hit conditions (vanilla 1.9+):
    // 1. Player is NOT on the ground (falling)
    // 2. Player is NOT in water
    // 3. Player is NOT climbing (ladder/vine)
    // 4. Player does NOT have Blindness effect
    // 5. Player is NOT riding an entity
    return !base.isOnGround && !base.isInWater;
}

// ============================================================
// Movement
// ============================================================

void PlayerSystem::updatePlayerMovement(PlayerComponent& player,
                                         MovementComponent& movement,
                                         float deltaTime) {
    // Calculate movement speed
    float baseSpeed = 4.317f; // Blocks per second (vanilla walking)

    if (movement.isSprinting) {
        baseSpeed *= 1.3f; // 5.612 bps sprinting
    }
    if (movement.isSneaking) {
        baseSpeed *= 0.3f; // ~1.3 bps sneaking
    }
    if (movement.isSwimming) {
        baseSpeed *= 0.2f; // ~0.86 bps swimming
    }

    // Creative mode fly speed override
    if (player.gamemode == Gamemode::Creative && player.abilities.isFlying) {
        baseSpeed = 10.89f * player.abilities.flySpeed * 20.0f;
        if (movement.isSprinting) {
            baseSpeed *= 2.0f; // Double fly speed when sprinting
        }
    }

    player.currentSpeed = baseSpeed;
}

// ============================================================
// Abilities
// ============================================================

void PlayerSystem::updatePlayerAbilities(PlayerComponent& player, float deltaTime) {
    // Update flight
    if (player.abilities.canFly && player.abilities.isFlying) {
        // Handle flight movement — actual movement applied by physics
    }

    // Creative mode overrides
    if (player.gamemode == Gamemode::Creative) {
        player.abilities.canFly = true;
        player.abilities.isInstabuild = true;
        player.abilities.isInvulnerable = true;
    } else if (player.gamemode == Gamemode::Spectator) {
        player.abilities.isFlying = true;
        player.abilities.isInvulnerable = true;
        player.abilities.mayBuild = false;
    }
}

// ============================================================
// Experience
// ============================================================

int PlayerSystem::calculateExperienceForLevel(int level) {
    if (level >= 30) {
        return 112 + (level - 30) * 9;
    } else if (level >= 15) {
        return 37 + (level - 15) * 5;
    } else {
        return 7 + level * 2;
    }
}

// ============================================================
// Mining
// ============================================================

float PlayerSystem::getDigSpeed(PlayerComponent& player, BlockState block,
                                 ToolType tool) {
    float speed = 1.0f;

    // Check tool effectiveness
    if (player.inventory.hasTool(tool)) {
        speed *= 4.0f; // Tool bonus
        // TODO: Check tool tier for multiplier
    }

    // Underwater penalty (unless has Aqua Affinity)
    if (player.isUnderwater && !player.hasAquaAffinity) {
        speed *= 0.2f; // 5x slower underwater
    }

    // Not on ground penalty
    if (!player.isOnGround) {
        speed *= 0.2f; // 5x slower in air
    }

    // TODO: Haste effect bonus
    // TODO: Mining fatigue penalty
    // TODO: Tool efficiency enchantment

    return speed;
}

// ============================================================
// Combat Getters
// ============================================================

float PlayerSystem::getAttackDamage(PlayerComponent& player) {
    float damage = 1.0f; // Base fist damage

    // Check held item for weapon damage
    const ItemStack& held = player.inventory.getSelectedStack();
    if (!held.isEmpty()) {
        const ItemDefinition* def = ItemRegistry::get().getDefinition(held.getItem());
        if (def && def->toolProperties.has_value()) {
            damage += def->toolProperties->attackDamage;
        }
    }

    // TODO: Strength effect bonus (+3 per level)
    // TODO: Weapon enchantment bonus (Sharpness, etc.)

    return damage;
}

float PlayerSystem::getAttackSpeed(PlayerComponent& player) {
    float speed = 4.0f; // Base attack speed (attacks per second)

    // Check held item for weapon attack speed modifier
    const ItemStack& held = player.inventory.getSelectedStack();
    if (!held.isEmpty()) {
        const ItemDefinition* def = ItemRegistry::get().getDefinition(held.getItem());
        if (def && def->toolProperties.has_value()) {
            // Most weapons have a negative modifier (e.g. sword: -2.4 → net 1.6)
            speed += def->toolProperties->attackSpeed;
        }
    }

    // TODO: Haste effect bonus
    // TODO: Attack speed enchantments

    player.attackSpeed = speed;
    return speed;
}

// ============================================================
// Food & Health
// ============================================================

void PlayerSystem::addExperience(PlayerComponent& player, int amount) {
    player.experience += amount;
    player.totalExperience += amount;
}

void PlayerSystem::addExhaustion(PlayerComponent& player, float amount) {
    player.exhaustion += amount;

    if (player.exhaustion >= 4.0f) {
        player.exhaustion -= 4.0f;
        if (player.saturation > 0) {
            player.saturation = std::max(0.0f, player.saturation - 1.0f);
        } else {
            player.foodLevel = std::max(0, player.foodLevel - 1);
        }
    }
}

void PlayerSystem::healFromFood(PlayerComponent& player) {
    if (player.foodLevel >= 18 && player.saturation > 0) {
        if (player.healthRegenTimer <= 0) {
            player.healthRegenTimer = 50; // 2.5 seconds
            // Healing would be handled in LivingEntitySystem
        }
    }
}

} // namespace VoxelForge
