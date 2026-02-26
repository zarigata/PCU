/**
 * @file ItemEntity.cpp
 * @brief Item entity implementation (dropped items)
 */

#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/game/Item.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <cmath>

namespace VoxelForge {

ItemEntitySystem::ItemEntitySystem() {
    LOG_INFO("ItemEntitySystem created");
}

void ItemEntitySystem::update(ECSWorld& world, float deltaTime) {
    auto view = world.view<ItemEntityComponent, EntityBaseComponent>();
    
    for (auto entity : view) {
        auto& item = view.get<ItemEntityComponent>(entity);
        auto& base = view.get<EntityBaseComponent>(entity);
        
        if (!base.isAlive) continue;
        
        // Update pickup delay
        if (item.pickupDelay > 0) {
            item.pickupDelay--;
        }
        
        // Update lifespan
        item.lifespan--;
        if (item.lifespan <= 0) {
            base.isAlive = false;
            LOG_DEBUG("Item {} despawned", entity);
            continue;
        }
        
        // Update hover animation
        item.hoverStart += deltaTime * 2.0f;
        
        // TODO: Physics update
        // - Apply gravity
        // - Apply drag
        // - Check collision
        // - Merge with nearby items of same type
    }
}

bool ItemEntitySystem::canPickup(const ItemEntityComponent& item, Entity player) {
    return item.pickupDelay <= 0;
}

void ItemEntitySystem::onPickup(ECSWorld& world, Entity itemEntity, ItemEntityComponent& item, 
                                 Entity player, InventoryComponent& inventory) {
    // Try to add item to inventory
    int remaining = addToInventory(inventory, item.stack);
    
    if (remaining <= 0) {
        // All items picked up
        world.getComponent<EntityBaseComponent>(itemEntity).isAlive = false;
        LOG_DEBUG("Player picked up item {}", item.stack.count);
    } else {
        // Partial pickup
        item.stack.count = remaining;
        LOG_DEBUG("Player picked up partial item, {} remaining", remaining);
    }
}

int ItemEntitySystem::addToInventory(InventoryComponent& inventory, ItemStack& stack) {
    // First, try to merge with existing stacks
    for (auto& slot : inventory.mainInventory) {
        if (slot.item == stack.item && slot.count < 64) {
            int space = 64 - slot.count;
            int toAdd = std::min(space, stack.count);
            slot.count += toAdd;
            stack.count -= toAdd;
            
            if (stack.count <= 0) {
                return 0;
            }
        }
    }
    
    // Then, try to find empty slot
    for (auto& slot : inventory.mainInventory) {
        if (slot.isEmpty()) {
            slot = stack;
            return 0;
        }
    }
    
    return stack.count;
}

void ItemEntitySystem::mergeItems(ECSWorld& world, Entity entity1, ItemEntityComponent& item1,
                                   Entity entity2, ItemEntityComponent& item2) {
    // Merge item2 into item1 if same type
    if (item1.stack.item == item2.stack.item) {
        int space = 64 - item1.stack.count;
        int toMerge = std::min(space, item2.stack.count);
        item1.stack.count += toMerge;
        item2.stack.count -= toMerge;
        
        if (item2.stack.count <= 0) {
            world.getComponent<EntityBaseComponent>(entity2).isAlive = false;
        }
    }
}

// ============================================================================
// Item Entity Factory Functions
// ============================================================================

namespace ItemEntityFactory {

Entity createItem(ECSWorld& world, const Vec3& position, const ItemStack& stack) {
    return EntityFactory::createItem(world, position, stack);
}

Entity createItemWithMotion(ECSWorld& world, const Vec3& position, const Vec3& velocity, 
                            const ItemStack& stack) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Item;
    base.isAlive = true;
    
    auto& item = world.addComponent<ItemEntityComponent>(entity);
    item.stack = stack;
    item.pickupDelay = 10;
    item.lifespan = 6000;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.25f;
    collision.height = 0.25f;
    
    // TODO: Set velocity in movement component
    
    return entity;
}

Entity createExperienceOrb(ECSWorld& world, const Vec3& position, int value) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Item; // Or could have its own type
    base.isAlive = true;
    
    // Experience orbs use a special component
    // For simplicity, reusing ItemEntityComponent
    auto& item = world.addComponent<ItemEntityComponent>(entity);
    item.stack.item = 0; // Special marker for XP
    item.stack.count = value;
    item.pickupDelay = 10;
    item.lifespan = 6000;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.5f;
    collision.height = 0.5f;
    
    return entity;
}

std::vector<Entity> createExperienceOrbs(ECSWorld& world, const Vec3& position, int totalValue) {
    std::vector<Entity> orbs;
    
    // Split experience into multiple orbs
    while (totalValue > 0) {
        int orbValue = std::min(totalValue, 50); // Max 50 XP per orb
        orbs.push_back(createExperienceOrb(world, position, orbValue));
        totalValue -= orbValue;
    }
    
    return orbs;
}

} // namespace ItemEntityFactory

} // namespace VoxelForge
