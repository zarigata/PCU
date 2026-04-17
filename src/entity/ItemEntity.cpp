/**
 * @file ItemEntity.cpp
 * @brief Item entity implementation (dropped items)
 */

#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/game/Item.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <cmath>
#include <algorithm>

namespace VoxelForge {

// Since the systems are currently missing from the codebase, we stub them
// to allow the project to build. These will be restored when found or rewritten.
class ItemEntitySystem : public System {
public:
    ItemEntitySystem() {
        VF_INFO("ItemEntitySystem created");
    }
    
    void update(ECSWorld& world, float deltaTime) {
        auto view = world.view<ItemEntityComponent, EntityBaseComponent>();
        for (auto entity : view) {
            auto* item = world.getComponent<ItemEntityComponent>(entity);
            auto* base = world.getComponent<EntityBaseComponent>(entity);
            if (!item || !base || !base->isAlive) continue;
            if (item->pickupDelay > 0) item->pickupDelay--;
            item->lifespan--;
            if (item->lifespan <= 0) {
                base->isAlive = false;
                VF_INFO("Item {} despawned", entity);
            }
            item->hoverStart += deltaTime * 2.0f;
        }
    }

    bool canPickup(const ItemEntityComponent& item, EntityID player) {
        return item.pickupDelay <= 0;
    }

    void onPickup(ECSWorld& world, EntityID itemEntity, ItemEntityComponent& item, 
                  EntityID player, struct InventoryComponent& inventory) {
        int remaining = addToInventory(inventory, *item.stack);
        if (remaining <= 0) {
            auto* base = world.getComponent<EntityBaseComponent>(itemEntity);
            if (base) base->isAlive = false;
            VF_INFO("Player picked up item {}", item.stack->getCount());
        } else {
            item.stack->setCount(remaining);
            VF_INFO("Player picked up partial item, {} remaining", remaining);
        }
    }

    int addToInventory(struct InventoryComponent& inventory, ItemStack& stack) {
        // Implementation would go here
        return 0;
    }

    void mergeItems(ECSWorld& world, EntityID entity1, ItemEntityComponent& item1,
                    EntityID entity2, ItemEntityComponent& item2) {
        if (item1.stack->getItem() == item2.stack->getItem()) {
            // Merge logic
        }
    }
};

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
    
    return entity;
}

EntityID createExperienceOrb(ECSWorld& world, const Vec3& position, int value) {
    EntityID entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Item;
    base.isAlive = true;
    
    auto& item = world.addComponent<ItemEntityComponent>(entity);
    // Experience orbs use count to store XP value, item type is INVALID
    item.stack = std::make_shared<ItemStack>();
    item.stack->setCount(value);
    item.pickupDelay = 10;
    item.lifespan = 6000;
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.5f;
    collision.height = 0.5f;
    
    return entity;
}

std::vector<EntityID> createExperienceOrbs(ECSWorld& world, const Vec3& position, int totalValue) {
    std::vector<EntityID> orbs;
    while (totalValue > 0) {
        int orbValue = std::min(totalValue, 50);
        orbs.push_back(createExperienceOrb(world, position, orbValue));
        totalValue -= orbValue;
    }
    return orbs;
}

} // namespace ItemEntityFactory

} // namespace VoxelForge

