/**
 * @file Item.hpp
 * @brief Item system for VoxelForge
 */

#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include <functional>

namespace VoxelForge {

// Forward declarations
class World;
class Player;
class Entity;

using ItemID = uint16_t;
using ItemCount = uint8_t;
constexpr ItemID INVALID_ITEM = 0;
constexpr ItemID MAX_ITEM_ID = 65535;
constexpr ItemCount MAX_STACK_SIZE = 64;

// Item categories
enum class ItemCategory {
    Misc,
    BuildingBlocks,
    NaturalBlocks,
    Combat,
    Tools,
    Food,
    Redstone,
    Transportation,
    Decoration,
    Brewing
};

// Item rarity
enum class Rarity {
    Common,
    Uncommon,
    Rare,
    Epic,
    Legendary
};

// Food properties
struct FoodProperties {
    int nutrition = 0;
    float saturation = 0.0f;
    bool isMeat = false;
    bool fast = false;
    bool alwaysEdible = false;
};

// Tool properties
struct ToolProperties {
    enum class Type { None, Pickaxe, Axe, Shovel, Hoe, Sword, Shears } type = Type::None;
    int level = 0;           // Mining level (0=wood, 1=stone, 2=iron, 3=diamond, 4=netherite)
    int durability = 0;
    float miningSpeed = 1.0f;
    float attackDamage = 0.0f;
    float attackSpeed = 0.0f;
    int enchantability = 0;
};

// Armor properties
struct ArmorProperties {
    enum class Slot { Head, Chest, Legs, Feet } slot = Slot::Head;
    int defense = 0;
    float toughness = 0.0f;
    float knockbackResistance = 0.0f;
    int durability = 0;
    int enchantability = 0;
    ItemID repairItem = INVALID_ITEM;
};

// Item definition
struct ItemDefinition {
    ItemID id = INVALID_ITEM;
    std::string registryName;
    std::string displayName;
    std::string description;
    
    ItemCategory category = ItemCategory::Misc;
    Rarity rarity = Rarity::Common;
    
    int maxStackSize = MAX_STACK_SIZE;
    
    std::optional<FoodProperties> foodProperties;
    std::optional<ToolProperties> toolProperties;
    std::optional<ArmorProperties> armorProperties;
    
    // Behavior callbacks (for modding)
    std::function<bool(Player&, World&)> onUse;
    std::function<bool(Player&, Entity&, const struct RaycastHit&)> onEntityHit;
    std::function<bool(Player&, const BlockPos&, BlockState)> onBlockBreak;
};

// ItemStack - combines item and count
class ItemStack {
public:
    ItemStack();
    ItemStack(ItemID item, ItemCount count = 1);
    ItemStack(const std::string& itemId, ItemCount count = 1);
    
    bool isEmpty() const { return item == INVALID_ITEM || count == 0; }
    ItemID getItem() const { return item; }
    ItemCount getCount() const { return count; }
    ItemCount getMaxStackSize() const;
    
    void setCount(ItemCount c) { count = c; }
    ItemCount add(ItemCount amount);
    ItemCount remove(ItemCount amount);
    
    bool canStackWith(const ItemStack& other) const;
    
    // Operators
    bool operator==(const ItemStack& other) const;
    bool operator!=(const ItemStack& other) const;
    
    // NBT data (simplified)
    struct NBTData {
        std::unordered_map<std::string, int> integers;
        std::unordered_map<std::string, float> floats;
        std::unordered_map<std::string, std::string> strings;
    };
    
    NBTData& getNBT() { return nbt; }
    const NBTData& getNBT() const { return nbt; }
    
private:
    ItemID item = INVALID_ITEM;
    ItemCount count = 0;
    NBTData nbt;
};

// Item registry
class ItemRegistry {
public:
    static ItemRegistry& get();
    
    ItemID registerItem(const std::string& id, ItemDefinition definition);
    ItemID getItemId(const std::string& id) const;
    const ItemDefinition* getDefinition(ItemID id) const;
    const ItemDefinition* getDefinition(const std::string& id) const;
    
    size_t getItemCount() const { return items.size(); }
    
    // Iteration
    auto begin() const { return items.begin(); }
    auto end() const { return items.end(); }
    
    // Register vanilla items
    void registerVanillaItems();
    
private:
    ItemRegistry();
    
    std::vector<ItemDefinition> items;
    std::unordered_map<std::string, ItemID> nameToId;
    ItemID nextId = 1;
};

} // namespace VoxelForge
