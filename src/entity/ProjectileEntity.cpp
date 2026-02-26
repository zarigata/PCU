/**
 * @file ProjectileEntity.cpp
 * @brief Projectile entity implementation
 */

#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <cmath>

namespace VoxelForge {

ProjectileSystem::ProjectileSystem() {
    LOG_INFO("ProjectileSystem created");
}

void ProjectileSystem::update(ECSWorld& world, float deltaTime) {
    auto view = world.view<ProjectileComponent, EntityBaseComponent>();
    
    for (auto entity : view) {
        auto& projectile = view.get<ProjectileComponent>(entity);
        auto& base = view.get<EntityBaseComponent>(entity);
        
        if (!base.isAlive) continue;
        
        // Update in-ground time
        if (projectile.inGround) {
            projectile.inGroundTime++;
            
            // Despawn after 1 minute in ground
            if (projectile.inGroundTime > 1200) {
                base.isAlive = false;
            }
            continue;
        }
        
        // TODO: Physics update
        // - Apply gravity
        // - Check collision
        // - Handle piercing
    }
}

void ProjectileSystem::onHitBlock(ECSWorld& world, Entity entity, ProjectileComponent& projectile, const BlockPos& pos) {
    projectile.inGround = true;
    LOG_DEBUG("Projectile {} hit block at ({}, {}, {})", entity, pos.x, pos.y, pos.z);
}

void ProjectileSystem::onHitEntity(ECSWorld& world, Entity projectileEntity, ProjectileComponent& projectile, 
                                    Entity target, LivingComponent& targetLiving) {
    // Apply damage
    targetLiving.health -= projectile.damage;
    
    LOG_DEBUG("Projectile {} hit entity {} for {} damage", 
              projectileEntity, target, projectile.damage);
    
    // Handle piercing
    if (projectile.pierce && projectile.pierceLevel > 0) {
        projectile.pierceLevel--;
    } else {
        // Destroy projectile
        world.getComponent<EntityBaseComponent>(projectileEntity).isAlive = false;
    }
}

// ============================================================================
// Projectile Factory Functions
// ============================================================================

namespace ProjectileFactory {

Entity createArrow(ECSWorld& world, const Vec3& position, const Vec3& velocity, Entity owner) {
    return EntityFactory::createArrow(world, position, velocity, owner);
}

Entity createSpectralArrow(ECSWorld& world, const Vec3& position, const Vec3& velocity, Entity owner) {
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
    return EntityFactory::createSnowball(world, position, velocity, owner);
}

Entity createEgg(ECSWorld& world, const Vec3& position, const Vec3& velocity, Entity owner) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Projectile;
    base.isAlive = true;
    
    auto& projectile = world.addComponent<ProjectileComponent>(entity);
    projectile.owner = owner;
    projectile.damage = 0.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.25f;
    collision.height = 0.25f;
    
    return entity;
}

Entity createEnderPearl(ECSWorld& world, const Vec3& position, const Vec3& velocity, Entity owner) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Projectile;
    base.isAlive = true;
    
    auto& projectile = world.addComponent<ProjectileComponent>(entity);
    projectile.owner = owner;
    projectile.damage = 5.0f; // Fall damage to thrower
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.25f;
    collision.height = 0.25f;
    
    return entity;
}

Entity createFireball(ECSWorld& world, const Vec3& position, const Vec3& velocity, Entity owner) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Projectile;
    base.isAlive = true;
    
    auto& projectile = world.addComponent<ProjectileComponent>(entity);
    projectile.owner = owner;
    projectile.damage = 6.0f; // Plus fire damage
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 1.0f;
    collision.height = 1.0f;
    
    return entity;
}

Entity createSmallFireball(ECSWorld& world, const Vec3& position, const Vec3& velocity, Entity owner) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Projectile;
    base.isAlive = true;
    
    auto& projectile = world.addComponent<ProjectileComponent>(entity);
    projectile.owner = owner;
    projectile.damage = 5.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.31f;
    collision.height = 0.31f;
    
    return entity;
}

Entity createDragonFireball(ECSWorld& world, const Vec3& position, const Vec3& velocity, Entity owner) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Projectile;
    base.isAlive = true;
    
    auto& projectile = world.addComponent<ProjectileComponent>(entity);
    projectile.owner = owner;
    projectile.damage = 12.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 1.0f;
    collision.height = 1.0f;
    
    return entity;
}

Entity createWitherSkull(ECSWorld& world, const Vec3& position, const Vec3& velocity, Entity owner) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Projectile;
    base.isAlive = true;
    
    auto& projectile = world.addComponent<ProjectileComponent>(entity);
    projectile.owner = owner;
    projectile.damage = 8.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.31f;
    collision.height = 0.31f;
    
    return entity;
}

Entity createShulkerBullet(ECSWorld& world, const Vec3& position, Entity owner, Entity target) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Projectile;
    base.isAlive = true;
    
    auto& projectile = world.addComponent<ProjectileComponent>(entity);
    projectile.owner = owner;
    projectile.damage = 4.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.31f;
    collision.height = 0.31f;
    
    return entity;
}

Entity createFishingBobber(ECSWorld& world, const Vec3& position, Entity owner) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Projectile;
    base.isAlive = true;
    
    auto& projectile = world.addComponent<ProjectileComponent>(entity);
    projectile.owner = owner;
    projectile.damage = 0.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.25f;
    collision.height = 0.25f;
    
    return entity;
}

Entity createLlamaSpit(ECSWorld& world, const Vec3& position, const Vec3& velocity, Entity owner) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Projectile;
    base.isAlive = true;
    
    auto& projectile = world.addComponent<ProjectileComponent>(entity);
    projectile.owner = owner;
    projectile.damage = 1.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.31f;
    collision.height = 0.31f;
    
    return entity;
}

Entity createTrident(ECSWorld& world, const Vec3& position, const Vec3& velocity, Entity owner) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Projectile;
    base.isAlive = true;
    
    auto& projectile = world.addComponent<ProjectileComponent>(entity);
    projectile.owner = owner;
    projectile.damage = 8.0f;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.5f;
    collision.height = 0.5f;
    
    return entity;
}

} // namespace ProjectileFactory

} // namespace VoxelForge
