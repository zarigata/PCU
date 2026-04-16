/**
 * @file SmeltingSystem.hpp
 * @brief Smelting system for furnaces, blast furnaces, and smokers
 *
 * Manages smelting operations including fuel tracking, cook progress,
 * and recipe matching for all furnace-type blocks.
 */

#pragma once

#include <VoxelForge/game/Inventory.hpp>
#include <VoxelForge/game/RecipeRegistry.hpp>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <string>

namespace VoxelForge {

// Fuel registry - maps items to burn time (in ticks)
class FuelRegistry {
public:
    static FuelRegistry& get();

    /// Register an item as fuel with burn time in ticks (20 ticks = 1 second)
    void registerFuel(const std::string& itemId, int burnTicks);
    void unregisterFuel(const std::string& itemId);

    /// Returns burn time in ticks, or 0 if the item is not a fuel
    int getBurnTime(const std::string& itemId) const;
    int getBurnTime(ItemID item) const;

    bool isFuel(const std::string& itemId) const;
    bool isFuel(ItemID item) const;

    /// Register all vanilla fuels
    void registerVanillaFuels();

    size_t getFuelCount() const { return fuels.size(); }

private:
    FuelRegistry() = default;

    std::unordered_map<std::string, int> fuels;  // itemId -> burn ticks
};

/// Furnace type
enum class FurnaceType {
    Normal,        // Standard furnace
    Blast,         // Blast furnace (2x speed, ores only)
    Smoker         // Smoker (2x speed, food only)
};

/// State of a single smelting operation
struct SmeltingState {
    bool isBurning = false;
    int burnTimeRemaining = 0;     // Ticks of fuel left
    int burnTimeTotal = 0;         // Total burn time of current fuel
    int cookTime = 0;              // Ticks spent cooking current item
    int cookTimeTotal = 200;       // Ticks needed to cook (200 for normal, 100 for blast/smoker)
};

/// Callback when smelting completes
using SmeltCompleteCallback = std::function<void(SlotIndex inputSlot, SlotIndex fuelSlot, SlotIndex outputSlot, const ItemStack& result)>;

/// A single furnace instance (one per furnace block in the world)
class FurnaceInstance {
public:
    FurnaceInstance(FurnaceType type = FurnaceType::Normal);

    // --- Inventory slots ---
    static constexpr SlotIndex SLOT_INPUT = 0;
    static constexpr SlotIndex SLOT_FUEL  = 1;
    static constexpr SlotIndex SLOT_OUTPUT = 2;
    static constexpr size_t SLOT_COUNT = 3;

    // Access
    Inventory& getInventory() { return inventory; }
    const Inventory& getInventory() const { return inventory; }
    ItemStack& getInput()  { return inventory.getSlot(SLOT_INPUT); }
    ItemStack& getFuel()   { return inventory.getSlot(SLOT_FUEL); }
    ItemStack& getOutput() { return inventory.getSlot(SLOT_OUTPUT); }

    const ItemStack& getInput()  const { return inventory.getSlot(SLOT_INPUT); }
    const ItemStack& getFuel()   const { return inventory.getSlot(SLOT_FUEL); }
    const ItemStack& getOutput() const { return inventory.getSlot(SLOT_OUTPUT); }

    // State
    const SmeltingState& getState() const { return state; }
    FurnaceType getFurnaceType() const { return type; }

    /// Main tick - call once per game tick
    void tick();

    /// Check if a recipe exists for the current input
    bool canSmelt() const;

    /// Set a callback for when smelting completes
    void setOnSmeltComplete(SmeltCompleteCallback cb) { onSmeltComplete = std::move(cb); }

    // Experience
    float getExperience() const { return accumulatedXP; }
    float consumeExperience();

private:
    /// Attempt to consume fuel from the fuel slot
    void burnFuel();

    /// Attempt to smelt the current input into the output
    void smelt();

    /// Find matching cooking recipe for the current input
    const CookingRecipe* findRecipe() const;

    /// Get the recipe type this furnace accepts
    RecipeType getRecipeType() const;

    /// Get the cook time for this furnace type
    int getCookTimeTotal() const;

    Inventory inventory;
    FurnaceType type;
    SmeltingState state;
    float accumulatedXP = 0.0f;
    SmeltCompleteCallback onSmeltComplete;
};

/// Global smelting manager (manages all furnace instances in the world)
class SmeltingSystem {
public:
    static SmeltingSystem& get();

    /// Create a furnace instance, returns its ID
    uint64_t createFurnace(FurnaceType type = FurnaceType::Normal);

    /// Destroy a furnace instance
    void destroyFurnace(uint64_t furnaceId);

    /// Get a furnace instance (nullptr if not found)
    FurnaceInstance* getFurnace(uint64_t furnaceId);
    const FurnaceInstance* getFurnace(uint64_t furnaceId) const;

    /// Tick all furnaces
    void tick();

    /// Number of active furnaces
    size_t getFurnaceCount() const;

    /// Number of currently burning furnaces
    size_t getBurningCount() const;

private:
    SmeltingSystem() = default;

    std::unordered_map<uint64_t, std::unique_ptr<FurnaceInstance>> furnaces;
    uint64_t nextId = 1;
};

} // namespace VoxelForge
