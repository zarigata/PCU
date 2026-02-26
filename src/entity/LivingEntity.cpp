/**
 * @file LivingEntity.cpp
 * @brief Living entity implementation
 */

#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

LivingEntitySystem::LivingEntitySystem() {
    LOG_INFO("LivingEntitySystem created");
}

void LivingEntitySystem::update(ECSWorld& world, float deltaTime) {
    // Get all entities with LivingComponent
    auto view = world.view<LivingComponent, EntityBaseComponent>();
    
    for (auto entity : view) {
        auto& living = view.get<LivingComponent>(entity);
        auto& base = view.get<EntityBaseComponent>(entity);
        
        if (!base.isAlive) continue;
        
        // Tick effects
        living.tickEffects();
        
        // Apply effect modifiers
        applyEffectModifiers(living, deltaTime);
        
        // Check for death
        if (living.health <= 0) {
            onDeath(world, entity, living, base);
        }
    }
}

void LivingEntitySystem::applyEffectModifiers(LivingComponent& living, float deltaTime) {
    // Apply regeneration
    if (living.hasEffect(LivingComponent::ActiveEffect::Type::Regeneration)) {
        // Heal over time
        living.health = std::min(living.health + 0.1f * deltaTime, living.maxHealth);
    }
    
    // Apply poison
    if (living.hasEffect(LivingComponent::ActiveEffect::Type::Poison)) {
        // Damage over time (can't kill)
        if (living.health > 1.0f) {
            living.health = std::max(living.health - 0.05f * deltaTime, 1.0f);
        }
    }
    
    // Apply wither
    if (living.hasEffect(LivingComponent::ActiveEffect::Type::Wither)) {
        living.health -= 0.05f * deltaTime;
    }
}

void LivingEntitySystem::onDeath(ECSWorld& world, Entity entity, LivingComponent& living, EntityBaseComponent& base) {
    base.isAlive = false;
    LOG_DEBUG("Entity {} died", entity);
    
    // TODO: Spawn death particles
    // TODO: Drop items
    // TODO: Trigger death events
}

float LivingEntitySystem::calculateDamage(LivingComponent& living, float baseDamage, DamageType type) {
    float damage = baseDamage;
    
    // Apply armor reduction for physical damage
    if (type == DamageType::Physical) {
        float armorReduction = living.armor * 0.04f; // 4% per armor point
        damage *= (1.0f - armorReduction);
    }
    
    // Apply resistance effect
    if (living.hasEffect(LivingComponent::ActiveEffect::Type::Resistance)) {
        // Find amplifier
        int amplifier = 0;
        for (const auto& effect : living.effects) {
            if (effect.type == LivingComponent::ActiveEffect::Type::Resistance) {
                amplifier = effect.amplifier;
                break;
            }
        }
        damage *= (1.0f - (amplifier + 1) * 0.2f); // 20% per level
    }
    
    return damage;
}

void LivingEntitySystem::heal(LivingComponent& living, float amount) {
    float oldHealth = living.health;
    living.health = std::min(living.health + amount, living.maxHealth);
    
    float healed = living.health - oldHealth;
    if (healed > 0) {
        LOG_DEBUG("Healed for {} HP", healed);
    }
}

void LivingEntitySystem::damage(ECSWorld& world, Entity entity, LivingComponent& living, float amount, DamageType type) {
    float actualDamage = calculateDamage(living, amount, type);
    living.health -= actualDamage;
    
    LOG_DEBUG("Entity {} took {} damage (type: {})", entity, actualDamage, static_cast<int>(type));
    
    if (living.health <= 0) {
        living.health = 0;
    }
}

} // namespace VoxelForge
