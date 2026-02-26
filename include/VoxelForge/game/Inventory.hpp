/**
 * @file Inventory.hpp
 * @brief Inventory system for players and containers
 */

#pragma once

#include <VoxelForge/game/Item.hpp>
#include <array>
#include <functional>
#include <optional>

namespace VoxelForge {

// Slot index type
using SlotIndex = uint8_t;

// Inventory change event
struct InventoryChange {
    SlotIndex slot;
    ItemStack oldStack;
    ItemStack newStack;
};

// Inventory base class
class Inventory {
public:
    explicit Inventory(size_t size);
    virtual ~Inventory() = default;
    
    // Size
    size_t getSize() const { return slots.size(); }
    
    // Slot access
    ItemStack& getSlot(SlotIndex slot);
    const ItemStack& getSlot(SlotIndex slot) const;
    void setSlot(SlotIndex slot, const ItemStack& stack);
    
    // Item operations
    bool addItem(const ItemStack& stack);
    ItemStack removeItem(SlotIndex slot, ItemCount count = 64);
    ItemStack removeItem(const ItemStack& stack);
    
    // Find operations
    std::optional<SlotIndex> findFirst(const ItemStack& stack) const;
    std::optional<SlotIndex> findFirstEmpty() const;
    std::vector<SlotIndex> findAll(const ItemStack& stack) const;
    
    // Count
    ItemCount countItem(ItemID item) const;
    ItemCount countItem(const std::string& itemId) const;
    
    // Check
    bool canInsert(const ItemStack& stack) const;
    bool hasItem(ItemID item, ItemCount count = 1) const;
    bool isEmpty() const;
    bool isFull() const;
    
    // Clear
    void clear();
    
    // Swap
    void swap(SlotIndex a, SlotIndex b);
    
    // Change callback
    using ChangeCallback = std::function<void(const InventoryChange&)>;
    void setChangeCallback(ChangeCallback callback) { on_change = callback; }
    
    // Iterator support
    auto begin() { return slots.begin(); }
    auto end() { return slots.end(); }
    auto begin() const { return slots.begin(); }
    auto end() const { return slots.end(); }
    
protected:
    std::vector<ItemStack> slots;
    ChangeCallback on_change;
    
    void notifyChange(SlotIndex slot, const ItemStack& oldStack, const ItemStack& newStack);
};

// Player inventory (36 slots + armor + offhand)
class PlayerInventory : public Inventory {
public:
    // Slot indices
    static constexpr SlotIndex HOTBAR_START = 0;
    static constexpr SlotIndex HOTBAR_END = 8;
    static constexpr SlotIndex MAIN_START = 9;
    static constexpr SlotIndex MAIN_END = 35;
    static constexpr SlotIndex HELMET = 36;
    static constexpr SlotIndex CHESTPLATE = 37;
    static constexpr SlotIndex LEGGINGS = 38;
    static constexpr SlotIndex BOOTS = 39;
    static constexpr SlotIndex OFFHAND = 40;
    static constexpr SlotIndex TOTAL_SIZE = 41;
    
    PlayerInventory();
    
    // Hotbar
    SlotIndex getSelectedSlot() const { return selectedSlot; }
    void setSelectedSlot(SlotIndex slot);
    ItemStack& getSelectedStack();
    const ItemStack& getSelectedStack() const;
    
    // Armor
    ItemStack& getHelmet() { return getSlot(HELMET); }
    ItemStack& getChestplate() { return getSlot(CHESTPLATE); }
    ItemStack& getLeggings() { return getSlot(LEGGINGS); }
    ItemStack& getBoots() { return getSlot(BOOTS); }
    ItemStack& getOffhand() { return getSlot(OFFHAND); }
    
    const ItemStack& getHelmet() const { return getSlot(HELMET); }
    const ItemStack& getChestplate() const { return getSlot(CHESTPLATE); }
    const ItemStack& getLeggings() const { return getSlot(LEGGINGS); }
    const ItemStack& getBoots() const { return getSlot(BOOTS); }
    const ItemStack& getOffhand() const { return getSlot(OFFHAND); }
    
    // Crafting (temporary)
    static constexpr SlotIndex CRAFTING_START = 100;
    static constexpr SlotIndex CRAFTING_RESULT = 104;
    
    ItemStack& getCraftingSlot(SlotIndex index) { return craftingGrid[index]; }
    const ItemStack& getCraftingSlot(SlotIndex index) const { return craftingGrid[index]; }
    ItemStack& getCraftingResult() { return craftingResult; }
    const ItemStack& getCraftingResult() const { return craftingResult; }
    
    void setCraftingSlot(SlotIndex index, const ItemStack& stack);
    void setCraftingResult(const ItemStack& stack);
    
    // Armor stats
    int getTotalArmor() const;
    float getTotalArmorToughness() const;
    
private:
    SlotIndex selectedSlot = 0;
    std::array<ItemStack, 4> craftingGrid;
    ItemStack craftingResult;
};

// Chest inventory
class ChestInventory : public Inventory {
public:
    explicit ChestInventory(bool isDouble = false);
    
    static constexpr size_t SINGLE_SIZE = 27;
    static constexpr size_t DOUBLE_SIZE = 54;
    
    bool isDoubleChest() const { return isDouble; }
    
private:
    bool isDouble;
};

// Creative inventory (all items)
class CreativeInventory : public Inventory {
public:
    CreativeInventory();
    
    void populate();
    void filterByCategory(ItemCategory category);
    void search(const std::string& query);
};

} // namespace VoxelForge
