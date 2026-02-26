/**
 * @file Inventory.cpp
 * @brief Inventory system implementation
 */

#include <VoxelForge/game/Inventory.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>

namespace VoxelForge {

// ============================================================================
// Inventory Component Implementation
// ============================================================================

InventoryComponent::InventoryComponent() {
    mainInventory.resize(36); // 3 rows of 9 slots
    armor.resize(4);
    offhand.resize(1);
}

int InventoryComponent::getSlot(SlotType type, int index) const {
    switch (type) {
        case SlotType::Main:
            return index;
        case SlotType::Hotbar:
            return index;
        case SlotType::Armor:
            return 36 + index;
        case SlotType::Offhand:
            return 40;
        default:
            return -1;
    }
}

ItemStack* InventoryComponent::getStack(SlotType type, int index) {
    switch (type) {
        case SlotType::Main:
        case SlotType::Hotbar:
            if (index >= 0 && index < 36) return &mainInventory[index];
            break;
        case SlotType::Armor:
            if (index >= 0 && index < 4) return &armor[index];
            break;
        case SlotType::Offhand:
            return &offhand[0];
    }
    return nullptr;
}

const ItemStack* InventoryComponent::getStack(SlotType type, int index) const {
    return const_cast<InventoryComponent*>(this)->getStack(type, index);
}

bool InventoryComponent::setStack(SlotType type, int index, const ItemStack& stack) {
    auto* slot = getStack(type, index);
    if (slot) {
        *slot = stack;
        return true;
    }
    return false;
}

int InventoryComponent::addItem(const ItemStack& stack) {
    ItemStack remaining = stack;
    
    // First, try to merge with existing stacks
    for (auto& slot : mainInventory) {
        if (!slot.isEmpty() && slot.canStackWith(remaining)) {
            int space = slot.getMaxStackSize() - slot.count;
            int toAdd = std::min(space, remaining.count);
            slot.count += toAdd;
            remaining.count -= toAdd;
            
            if (remaining.isEmpty()) {
                return 0;
            }
        }
    }
    
    // Then, find empty slots
    for (auto& slot : mainInventory) {
        if (slot.isEmpty()) {
            slot = remaining;
            return 0;
        }
    }
    
    return remaining.count;
}

int InventoryComponent::removeItem(ItemId item, int count) {
    int removed = 0;
    
    for (auto& slot : mainInventory) {
        if (slot.item == item && !slot.isEmpty()) {
            int toRemove = std::min(count - removed, slot.count);
            slot.count -= toRemove;
            removed += toRemove;
            
            if (removed >= count) {
                break;
            }
        }
    }
    
    return removed;
}

int InventoryComponent::countItem(ItemId item) const {
    int total = 0;
    
    for (const auto& slot : mainInventory) {
        if (slot.item == item) {
            total += slot.count;
        }
    }
    
    return total;
}

bool InventoryComponent::hasItem(ItemId item, int minCount) const {
    return countItem(item) >= minCount;
}

void InventoryComponent::swapSlots(int slot1, int slot2) {
    std::swap(mainInventory[slot1], mainInventory[slot2]);
}

void InventoryComponent::dropSlot(int slot) {
    mainInventory[slot] = ItemStack();
}

void InventoryComponent::dropAll() {
    for (auto& slot : mainInventory) {
        slot = ItemStack();
    }
    for (auto& slot : armor) {
        slot = ItemStack();
    }
    offhand[0] = ItemStack();
}

int InventoryComponent::getFirstEmptySlot() const {
    for (int i = 0; i < 36; i++) {
        if (mainInventory[i].isEmpty()) {
            return i;
        }
    }
    return -1;
}

bool InventoryComponent::isFull() const {
    for (const auto& slot : mainInventory) {
        if (slot.isEmpty()) {
            return false;
        }
    }
    return true;
}

bool InventoryComponent::isEmpty() const {
    for (const auto& slot : mainInventory) {
        if (!slot.isEmpty()) return false;
    }
    for (const auto& slot : armor) {
        if (!slot.isEmpty()) return false;
    }
    return offhand[0].isEmpty();
}

// ============================================================================
// Inventory System Implementation
// ============================================================================

InventorySystem::InventorySystem() {
    LOG_INFO("InventorySystem created");
}

void InventorySystem::update(ECSWorld& world, float deltaTime) {
    // Update inventory animations, etc.
}

bool InventorySystem::canPlaceItem(const InventoryComponent& inventory, 
                                    SlotType type, int index, const ItemStack& stack) {
    auto* existing = const_cast<InventoryComponent&>(inventory).getStack(type, index);
    if (!existing) return false;
    
    if (existing->isEmpty()) {
        return true;
    }
    
    if (existing->canStackWith(stack)) {
        return existing->count + stack.count <= existing->getMaxStackSize();
    }
    
    return false;
}

bool InventorySystem::placeItem(InventoryComponent& inventory, 
                                 SlotType type, int index, ItemStack& stack) {
    auto* existing = inventory.getStack(type, index);
    if (!existing) return false;
    
    if (existing->isEmpty()) {
        *existing = stack;
        stack = ItemStack();
        return true;
    }
    
    if (existing->canStackWith(stack)) {
        int space = existing->getMaxStackSize() - existing->count;
        int toAdd = std::min(space, stack.count);
        existing->count += toAdd;
        stack.count -= toAdd;
        return true;
    }
    
    return false;
}

ItemStack InventorySystem::takeItem(InventoryComponent& inventory, 
                                     SlotType type, int index, int count) {
    auto* existing = inventory.getStack(type, index);
    if (!existing || existing->isEmpty()) {
        return ItemStack();
    }
    
    ItemStack result;
    result.item = existing->item;
    result.metadata = existing->metadata;
    result.count = std::min(count, existing->count);
    
    existing->count -= result.count;
    if (existing->count <= 0) {
        *existing = ItemStack();
    }
    
    return result;
}

ItemStack InventorySystem::swapItems(InventoryComponent& inventory, 
                                      SlotType type, int index, const ItemStack& stack) {
    auto* existing = inventory.getStack(type, index);
    if (!existing) return stack;
    
    ItemStack result = *existing;
    *existing = stack;
    return result;
}

int InventorySystem::insertItems(InventoryComponent& inventory, ItemStack stack) {
    return inventory.addItem(stack);
}

ItemStack InventorySystem::extractItems(InventoryComponent& inventory, 
                                         ItemId item, int count) {
    ItemStack result;
    result.item = item;
    result.count = 0;
    
    for (auto& slot : inventory.mainInventory) {
        if (slot.item == item && !slot.isEmpty()) {
            int toTake = std::min(count - result.count, slot.count);
            result.count += toTake;
            slot.count -= toTake;
            
            if (slot.count <= 0) {
                slot = ItemStack();
            }
            
            if (result.count >= count) {
                break;
            }
        }
    }
    
    return result;
}

void InventorySystem::sortInventory(InventoryComponent& inventory) {
    // Sort by item type, then by count
    std::sort(inventory.mainInventory.begin(), inventory.mainInventory.end(),
        [](const ItemStack& a, const ItemStack& b) {
            if (a.isEmpty()) return false;
            if (b.isEmpty()) return true;
            if (a.item != b.item) return a.item < b.item;
            return a.count > b.count;
        });
}

// ============================================================================
// Chest Inventory Implementation
// ============================================================================

ChestInventory::ChestInventory(int size) : size_(size) {
    slots.resize(size);
}

ItemStack* ChestInventory::getSlot(int index) {
    if (index >= 0 && index < size_) {
        return &slots[index];
    }
    return nullptr;
}

const ItemStack* ChestInventory::getSlot(int index) const {
    if (index >= 0 && index < size_) {
        return &slots[index];
    }
    return nullptr;
}

bool ChestInventory::setSlot(int index, const ItemStack& stack) {
    if (index >= 0 && index < size_) {
        slots[index] = stack;
        return true;
    }
    return false;
}

int ChestInventory::addItem(const ItemStack& stack) {
    ItemStack remaining = stack;
    
    for (auto& slot : slots) {
        if (!slot.isEmpty() && slot.canStackWith(remaining)) {
            int space = slot.getMaxStackSize() - slot.count;
            int toAdd = std::min(space, remaining.count);
            slot.count += toAdd;
            remaining.count -= toAdd;
            
            if (remaining.isEmpty()) return 0;
        }
    }
    
    for (auto& slot : slots) {
        if (slot.isEmpty()) {
            slot = remaining;
            return 0;
        }
    }
    
    return remaining.count;
}

bool ChestInventory::isEmpty() const {
    for (const auto& slot : slots) {
        if (!slot.isEmpty()) return false;
    }
    return true;
}

bool ChestInventory::isFull() const {
    for (const auto& slot : slots) {
        if (slot.isEmpty()) return false;
    }
    return true;
}

void ChestInventory::clear() {
    for (auto& slot : slots) {
        slot = ItemStack();
    }
}

} // namespace VoxelForge
