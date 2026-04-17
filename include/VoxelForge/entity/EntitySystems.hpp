/**
 * @file EntitySystems.hpp
 * @brief Declarations for entity-specific systems
 */

#pragma once

#include <VoxelForge/core/ECS.hpp>
#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/entity/Player.hpp>  // For DamageType
#include <VoxelForge/game/Item.hpp>
#include <VoxelForge/world/World.hpp>
#include <vector>

namespace VoxelForge {

class MobAISystem : public System {
public:
    MobAISystem();
    void update(ECSWorld& world, float deltaTime);

    void updateIdle(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateWander(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateFollow(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateFlee(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateAttack(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateEat(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateSleep(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateBreed(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateWork(AIComponent& ai, MovementComponent& movement, float deltaTime);
};

class ProjectileSystem : public System {
public:
    ProjectileSystem();
    void update(ECSWorld& world, float deltaTime);

    void onHitBlock(ECSWorld& world, EntityID entity, ProjectileComponent& projectile, const BlockPos& pos);
    void onHitEntity(ECSWorld& world, EntityID projectileEntity, ProjectileComponent& projectile, 
                      EntityID target, LivingComponent& targetLiving);
};

class LivingEntitySystem : public System {
public:
    LivingEntitySystem();
    void update(ECSWorld& world, float deltaTime);

    void applyEffectModifiers(LivingComponent& living, float deltaTime);
    void onDeath(ECSWorld& world, EntityID entity, LivingComponent& living, EntityBaseComponent& base);
    float calculateDamage(LivingComponent& living, float baseDamage, DamageType type);
    void heal(LivingComponent& living, float amount);
    void damage(ECSWorld& world, EntityID entity, LivingComponent& living, float amount, DamageType type);
};

class ItemEntitySystem : public System {
public:
    ItemEntitySystem();
    void update(ECSWorld& world, float deltaTime);

    bool canPickup(const ItemEntityComponent& item, EntityID player);
    void onPickup(ECSWorld& world, EntityID itemEntity, ItemEntityComponent& item, 
                  EntityID player, struct InventoryComponent& inventory);
    int addToInventory(struct InventoryComponent& inventory, ItemStack& stack);
    void mergeItems(ECSWorld& world, EntityID entity1, ItemEntityComponent& item1,
                    EntityID entity2, ItemEntityComponent& item2);
};

} // namespace VoxelForge

