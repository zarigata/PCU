/**
 * @file EntityBase.cpp
 * @brief Base entity implementation
 */

#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

// ============================================================================
// LivingComponent Implementation
// ============================================================================

bool LivingComponent::hasEffect(ActiveEffect::Type type) const {
    for (const auto& effect : effects) {
        if (effect.type == type && effect.duration > 0) {
            return true;
        }
    }
    return false;
}

void LivingComponent::addEffect(ActiveEffect::Type type, int duration, int amplifier) {
    // Check if effect already exists
    for (auto& effect : effects) {
        if (effect.type == type) {
            // Upgrade if new amplifier is higher
            if (amplifier >= effect.amplifier) {
                effect.duration = duration;
                effect.amplifier = amplifier;
            }
            return;
        }
    }
    
    // Add new effect
    effects.push_back({type, duration, amplifier});
}

void LivingComponent::removeEffect(ActiveEffect::Type type) {
    effects.erase(
        std::remove_if(effects.begin(), effects.end(),
            [type](const ActiveEffect& e) { return e.type == type; }),
        effects.end()
    );
}

void LivingComponent::tickEffects() {
    // Decrease duration and remove expired effects
    for (auto it = effects.begin(); it != effects.end();) {
        it->duration--;
        if (it->duration <= 0) {
            it = effects.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// EntityFactory Implementation
// ============================================================================

namespace EntityFactory {

Entity createPlayer(ECSWorld& world, const Vec3& position) {
    Entity entity = world.createEntity();
    
    // Add base component
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Player;
    base.isAlive = true;
    
    // Add living component
    auto& living = world.addComponent<LivingComponent>(entity);
    living.health = 20.0f;
    living.maxHealth = 20.0f;
    
    // Add movement component
    world.addComponent<MovementComponent>(entity);
    
    // Add collision component
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.6f;
    collision.height = 1.8f;
    collision.eyeHeight = 1.62f;
    
    LOG_DEBUG("Created player entity {}", entity);
    return entity;
}

Entity createItem(ECSWorld& world, const Vec3& position, const ItemStack& stack) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Item;
    base.isAlive = true;
    
    auto& item = world.addComponent<ItemEntityComponent>(entity);
    item.stack = stack;
    item.pickupDelay = 10; // Half second before can be picked up
    item.lifespan = 6000;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.25f;
    collision.height = 0.25f;
    collision.eyeHeight = 0.125f;
    
    return entity;
}

Entity createZombie(ECSWorld& world, const Vec3& position) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Monster;
    base.isAlive = true;
    
    auto& living = world.addComponent<LivingComponent>(entity);
    living.health = 20.0f;
    living.maxHealth = 20.0f;
    
    world.addComponent<MovementComponent>(entity);
    
    auto& ai = world.addComponent<AIComponent>(entity);
    ai.currentState = AIComponent::State::Idle;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.6f;
    collision.height = 1.95f;
    collision.eyeHeight = 1.74f;
    
    LOG_DEBUG("Created zombie entity {}", entity);
    return entity;
}

Entity createSkeleton(ECSWorld& world, const Vec3& position) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Monster;
    base.isAlive = true;
    
    auto& living = world.addComponent<LivingComponent>(entity);
    living.health = 20.0f;
    living.maxHealth = 20.0f;
    
    world.addComponent<MovementComponent>(entity);
    world.addComponent<AIComponent>(entity);
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.6f;
    collision.height = 1.99f;
    collision.eyeHeight = 1.74f;
    
    return entity;
}

Entity createCreeper(ECSWorld& world, const Vec3& position) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Monster;
    base.isAlive = true;
    
    auto& living = world.addComponent<LivingComponent>(entity);
    living.health = 20.0f;
    living.maxHealth = 20.0f;
    
    world.addComponent<MovementComponent>(entity);
    world.addComponent<AIComponent>(entity);
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.6f;
    collision.height = 1.7f;
    collision.eyeHeight = 1.545f;
    
    return entity;
}

Entity createCow(ECSWorld& world, const Vec3& position) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Animal;
    base.isAlive = true;
    
    auto& living = world.addComponent<LivingComponent>(entity);
    living.health = 10.0f;
    living.maxHealth = 10.0f;
    
    world.addComponent<MovementComponent>(entity);
    
    auto& age = world.addComponent<AgeComponent>(entity);
    age.age = 0;
    age.isBaby = false;
    
    auto& ai = world.addComponent<AIComponent>(entity);
    ai.currentState = AIComponent::State::Idle;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.9f;
    collision.height = 1.4f;
    collision.eyeHeight = 1.3f;
    
    return entity;
}

Entity createPig(ECSWorld& world, const Vec3& position) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Animal;
    base.isAlive = true;
    
    auto& living = world.addComponent<LivingComponent>(entity);
    living.health = 10.0f;
    living.maxHealth = 10.0f;
    
    world.addComponent<MovementComponent>(entity);
    world.addComponent<AgeComponent>(entity);
    world.addComponent<AIComponent>(entity);
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.9f;
    collision.height = 0.9f;
    collision.eyeHeight = 0.8f;
    
    return entity;
}

Entity createSheep(ECSWorld& world, const Vec3& position) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Animal;
    base.isAlive = true;
    
    auto& living = world.addComponent<LivingComponent>(entity);
    living.health = 8.0f;
    living.maxHealth = 8.0f;
    
    world.addComponent<MovementComponent>(entity);
    world.addComponent<AgeComponent>(entity);
    world.addComponent<AIComponent>(entity);
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.9f;
    collision.height = 1.3f;
    collision.eyeHeight = 1.1f;
    
    return entity;
}

Entity createChicken(ECSWorld& world, const Vec3& position) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Animal;
    base.isAlive = true;
    
    auto& living = world.addComponent<LivingComponent>(entity);
    living.health = 4.0f;
    living.maxHealth = 4.0f;
    
    world.addComponent<MovementComponent>(entity);
    world.addComponent<AgeComponent>(entity);
    world.addComponent<AIComponent>(entity);
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.4f;
    collision.height = 0.7f;
    collision.eyeHeight = 0.6f;
    
    return entity;
}

Entity createArrow(ECSWorld& world, const Vec3& position, const Vec3& velocity, Entity owner) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Projectile;
    base.isAlive = true;
    
    auto& projectile = world.addComponent<ProjectileComponent>(entity);
    projectile.owner = owner;
    projectile.damage = 2.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.5f;
    collision.height = 0.5f;
    
    return entity;
}

Entity createSnowball(ECSWorld& world, const Vec3& position, const Vec3& velocity, Entity owner) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Projectile;
    base.isAlive = true;
    
    auto& projectile = world.addComponent<ProjectileComponent>(entity);
    projectile.owner = owner;
    projectile.damage = 0.0f; // Snowballs don't do damage (except to blazes)
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.25f;
    collision.height = 0.25f;
    
    return entity;
}

} // namespace EntityFactory

} // namespace VoxelForge
