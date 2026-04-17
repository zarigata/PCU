/**
 * @file Inventory.cpp
 * @brief Inventory system implementation (adapter to new Inventory.hpp API)
 */

#include <VoxelForge/game/Inventory.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>

namespace VoxelForge {

// Base Inventory implementation
Inventory::Inventory(size_t size) {
    slots.resize(size);
}

ItemStack& Inventory::getSlot(SlotIndex slot) {
    return slots.at(slot);
}

const ItemStack& Inventory::getSlot(SlotIndex slot) const {
    return slots.at(slot);
}

void Inventory::setSlot(SlotIndex slot, const ItemStack& stack) {
    slots.at(slot) = stack;
}

bool Inventory::addItem(const ItemStack& stack) {
    if (stack.isEmpty()) return true;
    ItemStack remaining = stack;
    // Try to merge with existing stacks
    for (auto& slot : slots) {
        if (!slot.isEmpty() && slot.canStackWith(remaining)) {
            int space = slot.getMaxStackSize() - static_cast<int>(slot.getCount());
            int toAdd = std::min(space, static_cast<int>(remaining.getCount()));
            slot.setCount(slot.getCount() + toAdd);
            remaining.setCount(remaining.getCount() - toAdd);
            if (remaining.isEmpty()) return true;
        }
    }
    // Put into first empty slot
    for (auto& slot : slots) {
        if (slot.isEmpty()) {
            slot = remaining;
            return true;
        }
    }
    return false;
}

ItemStack Inventory::removeItem(SlotIndex slot, ItemCount count) {
    if (slot < slots.size()) {
        ItemStack& s = slots[slot];
        if (s.isEmpty()) return ItemStack();
        ItemCount rem = std::min(count, s.getCount());
        ItemStack result(s.getItem(), rem);
        s.setCount(s.getCount() - rem);
        if (s.getCount() == 0) s = ItemStack();
        return result;
    }
    return ItemStack();
}

ItemStack Inventory::removeItem(const ItemStack& stack) {
    for (auto it = slots.begin(); it != slots.end(); ++it) {
        if (!it->isEmpty() && it->getItem() == stack.getItem()) {
            int toRemove = std::min(static_cast<int>(stack.getCount()), static_cast<int>(it->getCount()));
            ItemStack result(it->getItem(), toRemove);
            it->setCount(it->getCount() - toRemove);
            if (it->getCount() == 0) *it = ItemStack();
            return result;
        }
    }
    return ItemStack();
}

std::optional<SlotIndex> Inventory::findFirst(const ItemStack& stack) const {
    for (SlotIndex i = 0; i < slots.size(); ++i) {
        if (!slots[i].isEmpty() && slots[i].getItem() == stack.getItem() && slots[i].getCount() >= stack.getCount()) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<SlotIndex> Inventory::findFirstEmpty() const {
    for (SlotIndex i = 0; i < slots.size(); ++i) {
        if (slots[i].isEmpty()) return i;
    }
    return std::nullopt;
}

std::vector<SlotIndex> Inventory::findAll(const ItemStack& stack) const {
    std::vector<SlotIndex> result;
    for (SlotIndex i = 0; i < slots.size(); ++i) {
        if (!slots[i].isEmpty() && slots[i].getItem() == stack.getItem()) {
            result.push_back(i);
        }
    }
    return result;
}

ItemCount Inventory::countItem(ItemID item) const {
    ItemCount total = 0;
    for (const auto& slot : slots) {
        if (!slot.isEmpty() && slot.getItem() == item) total += slot.getCount();
    }
    return total;
}

ItemCount Inventory::countItem(const std::string& itemId) const {
    ItemID id = ItemRegistry::get().getItemId(itemId);
    return countItem(id);
}

bool Inventory::canInsert(const ItemStack& stack) const {
    for (const auto& slot : slots) {
        if (slot.isEmpty()) return true;
        if (slot.canStackWith(stack)) {
            return slot.getCount() < slot.getMaxStackSize();
        }
    }
    return false;
}

bool Inventory::hasItem(ItemID item, ItemCount count) const {
    return countItem(item) >= count;
}

bool Inventory::isEmpty() const {
    for (const auto& slot : slots) if (!slot.isEmpty()) return false;
    return true;
}

bool Inventory::isFull() const {
    for (const auto& slot : slots) if (slot.isEmpty()) return false;
    return true;
}

void Inventory::clear() {
    for (auto& slot : slots) slot = ItemStack();
}

void Inventory::swap(SlotIndex a, SlotIndex b) {
    std::swap(slots[a], slots[b]);
}

// Lightweight stubs for modding/loader API compatibility (no-op)
ChestInventory::ChestInventory(bool isDouble) : Inventory(isDouble ? 54 : 27) {}
CreativeInventory::CreativeInventory() : Inventory(128) { populate(); }
void CreativeInventory::populate() {}
void CreativeInventory::filterByCategory(ItemCategory) {}
void CreativeInventory::search(const std::string&) {}

} // namespace VoxelForge
