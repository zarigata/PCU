/**
 * @file SmeltingSystem.cpp
 * @brief Smelting system implementation for VoxelForge
 *
 * Manages furnace smelting logic: fuel burning, cook progress, recipe matching,
 * and output handling for Normal / Blast / Smoker furnace types.
 */

#include <VoxelForge/game/SmeltingSystem.hpp>
#include <VoxelForge/game/Item.hpp>
#include <VoxelForge/game/RecipeRegistry.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>

namespace VoxelForge {

// ============================================================================
// FuelRegistry
// ============================================================================

FuelRegistry& FuelRegistry::get() {
    static FuelRegistry instance;
    return instance;
}

void FuelRegistry::registerFuel(const std::string& itemId, int burnTicks) {
    if (burnTicks <= 0) return;
    fuels[itemId] = burnTicks;
    SPDLOG_DEBUG("FuelRegistry: registered fuel '{}' -> {} ticks", itemId, burnTicks);
}

void FuelRegistry::unregisterFuel(const std::string& itemId) {
    fuels.erase(itemId);
}

int FuelRegistry::getBurnTime(const std::string& itemId) const {
    auto it = fuels.find(itemId);
    return it != fuels.end() ? it->second : 0;
}

int FuelRegistry::getBurnTime(ItemID item) const {
    if (item == INVALID_ITEM) return 0;
    const ItemDefinition* def = ItemRegistry::get().getDefinition(item);
    if (!def) return 0;
    return getBurnTime(def->registryName);
}

bool FuelRegistry::isFuel(const std::string& itemId) const {
    return fuels.count(itemId) > 0;
}

bool FuelRegistry::isFuel(ItemID item) const {
    return getBurnTime(item) > 0;
}

void FuelRegistry::registerVanillaFuels() {
    // Coal & charcoal
    registerFuel("poorcraftultra:coal", 1600);
    registerFuel("poorcraftultra:charcoal", 1600);

    // Wood items (planks, logs, sticks, etc.)
    registerFuel("poorcraftultra:stick", 100);
    registerFuel("poorcraftultra:oak_planks", 300);
    registerFuel("poorcraftultra:spruce_planks", 300);
    registerFuel("poorcraftultra:birch_planks", 300);
    registerFuel("poorcraftultra:jungle_planks", 300);
    registerFuel("poorcraftultra:acacia_planks", 300);
    registerFuel("poorcraftultra:dark_oak_planks", 300);
    registerFuel("poorcraftultra:oak_log", 300);
    registerFuel("poorcraftultra:spruce_log", 300);
    registerFuel("poorcraftultra:birch_log", 300);
    registerFuel("poorcraftultra:jungle_log", 300);
    registerFuel("poorcraftultra:acacia_log", 300);
    registerFuel("poorcraftultra:dark_oak_log", 300);
    // Stripped logs
    registerFuel("poorcraftultra:stripped_oak_log", 300);
    registerFuel("poorcraftultra:stripped_spruce_log", 300);
    registerFuel("poorcraftultra:stripped_birch_log", 300);

    // Wooden tools/weapons (burn for 200)
    registerFuel("poorcraftultra:wooden_pickaxe", 200);
    registerFuel("poorcraftultra:wooden_axe", 200);
    registerFuel("poorcraftultra:wooden_shovel", 200);
    registerFuel("poorcraftultra:wooden_sword", 200);
    registerFuel("poorcraftultra:wooden_hoe", 200);

    // Coal block
    registerFuel("poorcraftultra:coal_block", 16000);

    // Blaze rod
    registerFuel("poorcraftultra:blaze_rod", 2400);

    // Lava bucket
    registerFuel("poorcraftultra:lava_bucket", 20000);

    // Bamboo
    registerFuel("poorcraftultra:bamboo", 50);

    // Dried kelp block
    registerFuel("poorcraftultra:dried_kelp_block", 4000);

    // Carpet
    registerFuel("poorcraftultra:white_carpet", 67);
    registerFuel("poorcraftultra:red_carpet", 67);

    // Wooden slabs, stairs, trapdoors, etc.
    registerFuel("poorcraftultra:oak_slab", 150);
    registerFuel("poorcraftultra:wooden_pressure_plate", 300);

    SPDLOG_INFO("FuelRegistry: registered {} vanilla fuels", fuels.size());
}

// ============================================================================
// FurnaceInstance
// ============================================================================

FurnaceInstance::FurnaceInstance(FurnaceType type)
    : inventory(SLOT_COUNT), type(type) {
    state.cookTimeTotal = getCookTimeTotal();
}

void FurnaceInstance::tick() {
    // 1. If burning, decrement burn time
    if (state.burnTimeRemaining > 0) {
        state.burnTimeRemaining--;
        state.isBurning = true;
    } else {
        state.isBurning = false;
    }

    // 2. Check if we can smelt
    bool canSmeltNow = canSmelt();

    if (!canSmeltNow) {
        // If not smelting, reset cook progress
        if (state.cookTime > 0) {
            state.cookTime = 0;
        }
        // Try to burn fuel if there's input but no burn time
        if (!state.isBurning && !getInput().isEmpty()) {
            burnFuel();
        }
        return;
    }

    // 3. If not burning, try to consume fuel
    if (!state.isBurning) {
        burnFuel();
        if (!state.isBurning) {
            // No fuel available, cannot cook
            return;
        }
    }

    // 4. Increment cook time
    state.cookTime++;

    // 5. If cook time reached, produce output
    if (state.cookTime >= state.cookTimeTotal) {
        smelt();
        state.cookTime = 0;
    }
}

bool FurnaceInstance::canSmelt() const {
    const ItemStack& input = getInput();
    if (input.isEmpty()) return false;

    // Find a matching recipe
    const CookingRecipe* recipe = findRecipe();
    if (!recipe) return false;

    // Check output slot
    const ItemStack& output = getOutput();
    ItemStack result = recipe->getResult();
    if (output.isEmpty()) return true;

    // Output slot has items - check if result can stack
    if (output.getItem() != result.getItem()) return false;
    if (output.getCount() + result.getCount() > output.getMaxStackSize()) return false;

    return true;
}

void FurnaceInstance::burnFuel() {
    ItemStack& fuel = getFuel();
    if (fuel.isEmpty()) return;

    int burnTime = FuelRegistry::get().getBurnTime(fuel.getItem());
    if (burnTime <= 0) return;

    state.burnTimeRemaining = burnTime;
    state.burnTimeTotal = burnTime;
    state.isBurning = true;

    // Consume one fuel item
    fuel.remove(1);
}

void FurnaceInstance::smelt() {
    const CookingRecipe* recipe = findRecipe();
    if (!recipe) return;

    ItemStack& input = getInput();
    ItemStack& output = getOutput();
    ItemStack result = recipe->getResult();

    if (output.isEmpty()) {
        output = result;
    } else {
        output.add(result.getCount());
    }

    // Consume one input item
    input.remove(1);

    // Accumulate XP (from recipe's experience field)
    // Cooking recipes typically grant 0.35 XP for smelting, 0.7 for blast/smoker
    float xp = 0.35f;
    if (type == FurnaceType::Blast || type == FurnaceType::Smoker) {
        xp = 0.7f;
    }
    accumulatedXP += xp;

    if (onSmeltComplete) {
        onSmeltComplete(SLOT_INPUT, SLOT_FUEL, SLOT_OUTPUT, result);
    }
}

const CookingRecipe* FurnaceInstance::findRecipe() const {
    const ItemStack& input = getInput();
    if (input.isEmpty()) return nullptr;

    // Get all smelting-type recipes that match this input
    RecipeType recipeType = getRecipeType();
    auto recipes = RecipeRegistry::getInstance().getRecipesByType(recipeType);

    std::vector<ItemStack> inputs = {input};
    for (const auto& recipe : recipes) {
        auto* cooking = static_cast<const CookingRecipe*>(recipe);
        if (cooking && cooking->matches(inputs)) {
            return cooking;
        }
    }

    return nullptr;
}

RecipeType FurnaceInstance::getRecipeType() const {
    switch (type) {
        case FurnaceType::Blast:   return RecipeType::Blasting;
        case FurnaceType::Smoker:  return RecipeType::Smoking;
        case FurnaceType::Normal:
        default:                   return RecipeType::Smelting;
    }
}

int FurnaceInstance::getCookTimeTotal() const {
    switch (type) {
        case FurnaceType::Blast:   return 100;  // 2x faster
        case FurnaceType::Smoker:  return 100;  // 2x faster
        case FurnaceType::Normal:
        default:                   return 200;  // 10 seconds at 20 tps
    }
}

float FurnaceInstance::consumeExperience() {
    float xp = accumulatedXP;
    accumulatedXP = 0.0f;
    return xp;
}

// ============================================================================
// SmeltingSystem (global manager)
// ============================================================================

SmeltingSystem& SmeltingSystem::get() {
    static SmeltingSystem instance;
    return instance;
}

uint64_t SmeltingSystem::createFurnace(FurnaceType type) {
    uint64_t id = nextId++;
    furnaces[id] = std::make_unique<FurnaceInstance>(type);
    SPDLOG_DEBUG("SmeltingSystem: created furnace {} (type={})", id, static_cast<int>(type));
    return id;
}

void SmeltingSystem::destroyFurnace(uint64_t furnaceId) {
    furnaces.erase(furnaceId);
}

FurnaceInstance* SmeltingSystem::getFurnace(uint64_t furnaceId) {
    auto it = furnaces.find(furnaceId);
    return it != furnaces.end() ? it->second.get() : nullptr;
}

const FurnaceInstance* SmeltingSystem::getFurnace(uint64_t furnaceId) const {
    auto it = furnaces.find(furnaceId);
    return it != furnaces.end() ? it->second.get() : nullptr;
}

void SmeltingSystem::tick() {
    for (auto& [id, furnace] : furnaces) {
        furnace->tick();
    }
}

size_t SmeltingSystem::getFurnaceCount() const {
    return furnaces.size();
}

size_t SmeltingSystem::getBurningCount() const {
    size_t count = 0;
    for (const auto& [id, furnace] : furnaces) {
        if (furnace->getState().isBurning) count++;
    }
    return count;
}

} // namespace VoxelForge
