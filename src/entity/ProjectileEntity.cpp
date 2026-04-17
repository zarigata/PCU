/**
 * @file ProjectileEntity.cpp
 * @brief Projectile entity implementation
 */

#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/entity/EntitySystems.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <cmath>

namespace VoxelForge {

ProjectileSystem::ProjectileSystem() {
    VF_INFO("ProjectileSystem created");
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

void ProjectileSystem::onHitBlock(ECSWorld& world, EntityID entity, ProjectileComponent& projectile, const BlockPos& pos) {
    projectile.inGround = true;
    VF_INFO("Projectile {} hit block at ({}, {}, {})", entity, pos.x, pos.y, pos.z);
}

void ProjectileSystem::onHitEntity(ECSWorld& world, EntityID projectileEntity, ProjectileComponent& projectile, 
                                     EntityID target, LivingComponent& targetLiving) {
    // Apply damage
    targetLiving.health -= projectile.damage;
    
    VF_INFO("Projectile {} hit entity {} for {} damage", 
               projectileEntity, target, projectile.damage);
    
    // Handle piercing
    if (projectile.pierce && projectile.pierceLevel > 0) {
        projectile.pierceLevel--;
    } else {
        // Destroy projectile
        auto* base = world.getComponent<EntityBaseComponent>(projectileEntity);
        if (base) base->isAlive = false;
    }
}

// ============================================================================
// Projectile Factory Functions
// ============================================================================

namespace ProjectileFactory {

EntityID createArrow(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner) {
    return EntityFactory::createArrow(world, position, velocity, owner);
}

EntityID createSpectralArrow(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner) {
    EntityID entity = world.createEntity();
    
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

EntityID createSnowball(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner) {
    return EntityFactory::createSnowball(world, position, velocity, owner);
}

EntityID createEgg(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner) {
    EntityID entity = world.createEntity();
    
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

EntityID createEnderPearl(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner) {
    EntityID entity = world.createEntity();
    
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

EntityID createFireball(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner) {
    EntityID entity = world.createEntity();
    
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

EntityID createSmallFireball(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner) {
    EntityID entity = world.createEntity();
    
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

EntityID createDragonFireball(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner) {
    EntityID entity = world.createEntity();
    
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

EntityID createWitherSkull(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner) {
    EntityID entity = world.createEntity();
    
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

EntityID createShulkerBullet(ECSWorld& world, const Vec3& position, EntityID owner, EntityID target) {
    EntityID entity = world.createEntity();
    
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

EntityID createFishingBobber(ECSWorld& world, const Vec3& position, EntityID owner) {
    EntityID entity = world.createEntity();
    
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

EntityID createLlamaSpit(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner) {
    EntityID entity = world.createEntity();
    
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

EntityID createTrident(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner) {
    EntityID entity = world.createEntity();
    
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

