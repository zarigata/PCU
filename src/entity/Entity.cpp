/**
 * @file Entity.cpp
 * @brief Entity system implementation
 */

#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>

namespace VoxelForge {

// ============================================
// LivingComponent
// ============================================

bool LivingComponent::hasEffect(ActiveEffect::Type type) const {
    return std::any_of(effects.begin(), effects.end(), 
        [type](const auto& e) { return e.type == type; });
}

void LivingComponent::addEffect(ActiveEffect::Type type, int duration, int amplifier) {
    // Remove existing effect of same type
    removeEffect(type);
    
    effects.push_back({type, duration, amplifier});
}

void LivingComponent::removeEffect(ActiveEffect::Type type) {
    effects.erase(
        std::remove_if(effects.begin(), effects.end(),
            [type](const auto& e) { return e.type == type; }),
        effects.end()
    );
}

void LivingComponent::tickEffects() {
    auto it = effects.begin();
    while (it != effects.end()) {
        it->duration--;
        
        if (it->duration <= 0) {
            it = effects.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================
// Entity Factory
// ============================================

namespace EntityFactory {

Entity createPlayer(ECSWorld& world, const Vec3& position) {
    Entity e = world.createEntity();
    
    // Base entity data
    auto& base = world.addComponent<EntityBaseComponent>(e);
    base.uuid = UUID::generate();
    base.type = EntityType::Player;
    
    // Transform
    auto& transform = world.addComponent<TransformComponent>(e);
    transform.position = position;
    
    // Physics
    auto& physics = world.addComponent<PhysicsComponent>(e);
    physics.bounds = AABB(-0.3f, 0.0f, -0.3f, 0.3f, 1.8f, 0.3f);
    
    // Living
    auto& living = world.addComponent<LivingComponent>(e);
    living.health = 20.0f;
    living.maxHealth = 20.0f;
    
    // Movement
    world.addComponent<MovementComponent>(e);
    
    // Collision
    auto& collision = world.addComponent<CollisionComponent>(e);
    collision.width = 0.6f;
    collision.height = 1.8f;
    collision.eyeHeight = 1.62f;
    
    return e;
}

Entity createItem(ECSWorld& world, const Vec3& position, const ItemStack& stack) {
    Entity e = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(e);
    base.uuid = UUID::generate();
    base.type = EntityType::Item;
    
    auto& transform = world.addComponent<TransformComponent>(e);
    transform.position = position;
    
    auto& physics = world.addComponent<PhysicsComponent>(e);
    physics.bounds = AABB(-0.125f, 0.0f, -0.125f, 0.125f, 0.25f, 0.125f);
    physics.mass = 0.1f;
    
    auto& collision = world.addComponent<CollisionComponent>(e);
    collision.width = 0.25f;
    collision.height = 0.25f;
    
    auto& itemComp = world.addComponent<ItemEntityComponent>(e);
    itemComp.stack = stack;
    itemComp.pickupDelay = 10;
    
    return e;
}

Entity createZombie(ECSWorld& world, const Vec3& position) {
    Entity e = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(e);
    base.uuid = UUID::generate();
    base.type = EntityType::Monster;
    
    auto& transform = world.addComponent<TransformComponent>(e);
    transform.position = position;
    
    auto& physics = world.addComponent<PhysicsComponent>(e);
    physics.bounds = AABB(-0.3f, 0.0f, -0.3f, 0.3f, 1.95f, 0.3f);
    
    auto& living = world.addComponent<LivingComponent>(e);
    living.health = 20.0f;
    living.maxHealth = 20.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(e);
    collision.width = 0.6f;
    collision.height = 1.95f;
    collision.eyeHeight = 1.74f;
    
    world.addComponent<MovementComponent>(e);
    world.addComponent<AIComponent>(e);
    
    return e;
}

Entity createSkeleton(ECSWorld& world, const Vec3& position) {
    Entity e = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(e);
    base.uuid = UUID::generate();
    base.type = EntityType::Monster;
    
    auto& transform = world.addComponent<TransformComponent>(e);
    transform.position = position;
    
    auto& physics = world.addComponent<PhysicsComponent>(e);
    physics.bounds = AABB(-0.3f, 0.0f, -0.3f, 0.3f, 1.99f, 0.3f);
    
    auto& living = world.addComponent<LivingComponent>(e);
    living.health = 20.0f;
    living.maxHealth = 20.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(e);
    collision.width = 0.6f;
    collision.height = 1.99f;
    collision.eyeHeight = 1.74f;
    
    world.addComponent<MovementComponent>(e);
    world.addComponent<AIComponent>(e);
    
    return e;
}

Entity createCreeper(ECSWorld& world, const Vec3& position) {
    Entity e = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(e);
    base.uuid = UUID::generate();
    base.type = EntityType::Monster;
    
    auto& transform = world.addComponent<TransformComponent>(e);
    transform.position = position;
    
    auto& physics = world.addComponent<PhysicsComponent>(e);
    physics.bounds = AABB(-0.3f, 0.0f, -0.3f, 0.3f, 1.7f, 0.3f);
    
    auto& living = world.addComponent<LivingComponent>(e);
    living.health = 20.0f;
    living.maxHealth = 20.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(e);
    collision.width = 0.6f;
    collision.height = 1.7f;
    collision.eyeHeight = 1.5f;
    
    world.addComponent<MovementComponent>(e);
    world.addComponent<AIComponent>(e);
    
    return e;
}

Entity createCow(ECSWorld& world, const Vec3& position) {
    Entity e = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(e);
    base.uuid = UUID::generate();
    base.type = EntityType::Animal;
    
    auto& transform = world.addComponent<TransformComponent>(e);
    transform.position = position;
    
    auto& physics = world.addComponent<PhysicsComponent>(e);
    physics.bounds = AABB(-0.45f, 0.0f, -0.45f, 0.45f, 1.4f, 0.45f);
    
    auto& living = world.addComponent<LivingComponent>(e);
    living.health = 10.0f;
    living.maxHealth = 10.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(e);
    collision.width = 0.9f;
    collision.height = 1.4f;
    collision.eyeHeight = 1.3f;
    
    world.addComponent<MovementComponent>(e);
    world.addComponent<AIComponent>(e);
    world.addComponent<AgeComponent>(e);
    
    return e;
}

Entity createPig(ECSWorld& world, const Vec3& position) {
    Entity e = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(e);
    base.uuid = UUID::generate();
    base.type = EntityType::Animal;
    
    auto& transform = world.addComponent<TransformComponent>(e);
    transform.position = position;
    
    auto& physics = world.addComponent<PhysicsComponent>(e);
    physics.bounds = AABB(-0.45f, 0.0f, -0.45f, 0.45f, 1.0f, 0.45f);
    
    auto& living = world.addComponent<LivingComponent>(e);
    living.health = 10.0f;
    living.maxHealth = 10.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(e);
    collision.width = 0.9f;
    collision.height = 0.9f;
    collision.eyeHeight = 0.8f;
    
    world.addComponent<MovementComponent>(e);
    world.addComponent<AIComponent>(e);
    world.addComponent<AgeComponent>(e);
    
    return e;
}

Entity createSheep(ECSWorld& world, const Vec3& position) {
    Entity e = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(e);
    base.uuid = UUID::generate();
    base.type = EntityType::Animal;
    
    auto& transform = world.addComponent<TransformComponent>(e);
    transform.position = position;
    
    auto& physics = world.addComponent<PhysicsComponent>(e);
    physics.bounds = AABB(-0.45f, 0.0f, -0.45f, 0.45f, 1.3f, 0.45f);
    
    auto& living = world.addComponent<LivingComponent>(e);
    living.health = 8.0f;
    living.maxHealth = 8.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(e);
    collision.width = 0.9f;
    collision.height = 1.3f;
    collision.eyeHeight = 1.1f;
    
    world.addComponent<MovementComponent>(e);
    world.addComponent<AIComponent>(e);
    world.addComponent<AgeComponent>(e);
    
    return e;
}

Entity createChicken(ECSWorld& world, const Vec3& position) {
    Entity e = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(e);
    base.uuid = UUID::generate();
    base.type = EntityType::Animal;
    
    auto& transform = world.addComponent<TransformComponent>(e);
    transform.position = position;
    
    auto& physics = world.addComponent<PhysicsComponent>(e);
    physics.bounds = AABB(-0.2f, 0.0f, -0.2f, 0.2f, 0.7f, 0.2f);
    
    auto& living = world.addComponent<LivingComponent>(e);
    living.health = 4.0f;
    living.maxHealth = 4.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(e);
    collision.width = 0.4f;
    collision.height = 0.7f;
    collision.eyeHeight = 0.6f;
    
    world.addComponent<MovementComponent>(e);
    world.addComponent<AIComponent>(e);
    world.addComponent<AgeComponent>(e);
    
    return e;
}

Entity createArrow(ECSWorld& world, const Vec3& position, const Vec3& velocity, Entity owner) {
    Entity e = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(e);
    base.uuid = UUID::generate();
    base.type = EntityType::Projectile;
    
    auto& transform = world.addComponent<TransformComponent>(e);
    transform.position = position;
    
    auto& vel = world.addComponent<VelocityComponent>(e);
    vel.velocity = velocity;
    
    auto& physics = world.addComponent<PhysicsComponent>(e);
    physics.bounds = AABB(-0.05f, -0.05f, -0.05f, 0.05f, 0.0f, 0.0f);
    physics.isTrigger = true;
    
    auto& collision = world.addComponent<CollisionComponent>(e);
    collision.width = 0.1f;
    collision.height = 0.5f;
    
    auto& proj = world.addComponent<ProjectileComponent>(e);
    proj.owner = owner;
    proj.damage = 2.0f;
    
    return e;
}

Entity createSnowball(ECSWorld& world, const Vec3& position, const Vec3& velocity, Entity owner) {
    Entity e = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(e);
    base.uuid = UUID::generate();
    base.type = EntityType::Projectile;
    
    auto& transform = world.addComponent<TransformComponent>(e);
    transform.position = position;
    
    auto& vel = world.addComponent<VelocityComponent>(e);
    vel.velocity = velocity;
    
    auto& physics = world.addComponent<PhysicsComponent>(e);
    physics.bounds = AABB(-0.125f, -0.125f, -0.125f, 0.125f, 0.0f, 0.125f);
    physics.isTrigger = true;
    
    auto& collision = world.addComponent<CollisionComponent>(e);
    collision.width = 0.25f;
    collision.height = 0.25f;
    
    auto& proj = world.addComponent<ProjectileComponent>(e);
    proj.owner = owner;
    proj.damage = 0.0f;  // Snowball doesn't deal damage
    
    return e;
}

} // namespace EntityFactory

} // namespace VoxelForge
