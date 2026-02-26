/**
 * @file Player.cpp
 * @brief Player entity implementation
 */

#include <VoxelForge/entity/Player.hpp>
#include <VoxelForge/game/Inventory.hpp>
#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

PlayerSystem::PlayerSystem() {
    LOG_INFO("PlayerSystem created");
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
            LOG_INFO("Player leveled up to {}", player.level);
        }
        
        // Update cooldowns
        if (player.attackCooldown > 0) {
            player.attackCooldown -= deltaTime;
        }
        
        // Update air supply when underwater
        if (base.isInWater) {
            player.airSupply -= deltaTime * 20.0f; // Ticks per second
            if (player.airSupply <= 0) {
                player.airSupply = 0;
                // Drowning damage would be handled here
            }
        } else {
            player.airSupply = player.maxAirSupply;
        }
    }
}

void PlayerSystem::updatePlayerMovement(PlayerComponent& player, MovementComponent& movement, float deltaTime) {
    // Calculate movement speed
    float baseSpeed = 4.317f; // Blocks per second
    
    if (movement.isSprinting) {
        baseSpeed *= 1.3f;
    }
    if (movement.isSneaking) {
        baseSpeed *= 0.3f;
    }
    if (movement.isSwimming) {
        baseSpeed *= 0.2f;
    }
    
    player.currentSpeed = baseSpeed;
}

void PlayerSystem::updatePlayerAbilities(PlayerComponent& player, float deltaTime) {
    // Update flight
    if (player.abilities.canFly && player.abilities.isFlying) {
        // Handle flight movement
    }
    
    // Update invulnerability
    if (player.invulnerabilityTicks > 0) {
        player.invulnerabilityTicks -= static_cast<int>(deltaTime * 20.0f);
    }
}

int PlayerSystem::calculateExperienceForLevel(int level) {
    if (level >= 30) {
        return 112 + (level - 30) * 9;
    } else if (level >= 15) {
        return 37 + (level - 15) * 5;
    } else {
        return 7 + level * 2;
    }
}

float PlayerSystem::getDigSpeed(PlayerComponent& player, BlockState block, ToolType tool) {
    float speed = 1.0f;
    
    // Check tool effectiveness
    if (player.inventory.hasTool(tool)) {
        speed *= 4.0f; // Tool bonus
        
        // Check tool tier
        // TODO: Get actual tool tier from inventory
    }
    
    // Check underwater penalty
    if (player.isUnderwater && !player.hasAquaAffinity) {
        speed *= 0.2f;
    }
    
    // Check on ground bonus
    if (!player.isOnGround) {
        speed *= 0.2f;
    }
    
    // Check haste effect
    // TODO: Check for haste effect
    
    // Check mining fatigue
    // TODO: Check for mining fatigue effect
    
    return speed;
}

float PlayerSystem::getAttackDamage(PlayerComponent& player) {
    float damage = 1.0f; // Base fist damage
    
    // Check held item
    // TODO: Get weapon damage from inventory
    
    // Check strength effect
    // TODO: Check for strength effect
    
    return damage;
}

float PlayerSystem::getAttackSpeed(PlayerComponent& player) {
    float speed = 4.0f; // Base attack speed
    
    // Check held item
    // TODO: Get weapon attack speed from inventory
    
    return speed;
}

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
            // Regenerate health
            player.healthRegenTimer = 50; // 2.5 seconds
            // Healing would be handled in LivingEntitySystem
        }
    }
}

} // namespace VoxelForge
