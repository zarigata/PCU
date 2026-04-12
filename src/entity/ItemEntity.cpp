/**
 * @file ItemEntity.cpp
 * @brief Item entity implementation (dropped items)
 */

#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/entity/Player.hpp>
#include <VoxelForge/game/Item.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <cmath>

namespace VoxelForge {

ItemEntitySystem::ItemEntitySystem() {
    VF_INFO("ItemEntitySystem created");
}

void ItemEntitySystem::update(ECSWorld& world, float deltaTime) {
    auto view = world.view<ItemEntityComponent, EntityBaseComponent>();
    
    for (EntityID entity : view) {
        auto* item = world.getComponent<ItemEntityComponent>(entity);
        auto* base = world.getComponent<EntityBaseComponent>(entity);
        
        if (!item || !base) continue;
        if (!base->isAlive) continue;
        
        // Update pickup delay
        if (item->pickupDelay > 0) {
            item->pickupDelay--;
        }
        
        // Update lifespan
        item->lifespan--;
        if (item->lifespan <= 0) {
            base->isAlive = false;
            VF_INFO("Item {} despawned", entity);
            continue;
        }
        
        // Update hover animation
        item->hoverStart += deltaTime * 2.0f;
        
        // TODO: Physics update
        // - Apply gravity
        // - Apply drag
        // - Check collision
        // - Merge with nearby items of same type
    }
}

bool ItemEntitySystem::canPickup(const ItemEntityComponent& item, EntityID player) {
    return item.pickupDelay <= 0;
}

void ItemEntitySystem::onPickup(ECSWorld& world, EntityID itemEntity, ItemEntityComponent& item, 
                                 EntityID player, PlayerComponent& playerComp) {
    // Try to add item to inventory
    int remaining = addToInventory(playerComp.inventory, *item.stack);
    
    if (remaining <= 0) {
        // All items picked up
        world.getComponent<EntityBaseComponent>(itemEntity)->isAlive = false;
        VF_INFO("Player picked up item {}", item.stack->getCount());
    } else {
        // Partial pickup
        item.stack->setCount(remaining);
        VF_INFO("Player picked up partial item, {} remaining", remaining);
    }
}

int ItemEntitySystem::addToInventory(PlayerInventory& inventory, ItemStack& stack) {
    // First, try to merge with existing stacks
    for (size_t i = 0; i < inventory.getSize(); i++) {
        ItemStack& slot = inventory.getSlot(i);
        if (!slot.isEmpty() && slot.getItem() == stack.getItem() && slot.getCount() < 64) {
            int space = 64 - slot.getCount();
            int toAdd = std::min(space, stack.getCount());
            slot.add(toAdd);
            stack.remove(toAdd);
            
            if (stack.getCount() <= 0) {
                return 0;
            }
        }
    }
    
    // Then, try to find empty slot
    for (size_t i = 0; i < inventory.getSize(); i++) {
        if (inventory.getSlot(i).isEmpty()) {
            inventory.setSlot(i, stack);
            return 0;
        }
    }
    
    return stack.getCount();
}

void ItemEntitySystem::mergeItems(ECSWorld& world, EntityID entity1, ItemEntityComponent& item1,
                                   EntityID entity2, ItemEntityComponent& item2) {
    // Merge item2 into item1 if same type
    if (item1.stack->getItem() == item2.stack->getItem()) {
        int space = 64 - item1.stack->getCount();
        int toMerge = std::min(space, item2.stack->getCount());
        item1.stack->add(toMerge);
        item2.stack->remove(toMerge);
        
        if (item2.stack->getCount() <= 0) {
            world.getComponent<EntityBaseComponent>(entity2)->isAlive = false;
        }
    }
}

// ============================================================================
// Item Entity Factory Functions
// ============================================================================

namespace ItemEntityFactory {

EntityID createItem(ECSWorld& world, const Vec3& position, const ItemStack& stack) {
    return EntityFactory::createItem(world, position, stack);
}

EntityID createItemWithMotion(ECSWorld& world, const Vec3& position, const Vec3& velocity, 
                            const ItemStack& stack) {
    EntityID entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Item;
    base.isAlive = true;
    
    auto& item = world.addComponent<ItemEntityComponent>(entity);
    item.stack = std::make_shared<ItemStack>(stack);
    item.pickupDelay = 10;
    item.lifespan = 6000;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.25f;
    collision.height = 0.25f;
    
    // TODO: Set velocity in movement component
    
    return entity;
}

EntityID createExperienceOrb(ECSWorld& world, const Vec3& position, int value) {
    EntityID entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Item; // Or could have its own type
    base.isAlive = true;
    
    // Experience orbs use a special component
    // For simplicity, reusing ItemEntityComponent
    auto& item = world.addComponent<ItemEntityComponent>(entity);
    item.stack = std::make_shared<ItemStack>(0, value); // Item ID 0 for XP
    item.pickupDelay = 10;
    item.lifespan = 6000;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.5f;
    collision.height = 0.5f;
    
    return entity;
}

std::vector<EntityID> createExperienceOrbs(ECSWorld& world, const Vec3& position, int totalValue) {
    std::vector<EntityID> orbs;
    
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
