/**
 * @file CraftingSystem.cpp
 * @brief Crafting system implementation
 */

#include <VoxelForge/game/CraftingSystem.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>

namespace VoxelForge {

// ============================================================================
// Recipe Implementation
// ============================================================================

bool Recipe::matches(const std::vector<ItemStack>& ingredients) const {
    if (ingredients.size() != this->ingredients.size()) {
        return false;
    }
    
    for (size_t i = 0; i < ingredients.size(); i++) {
        if (!ingredients[i].canStackWith(this->ingredients[i])) {
            return false;
        }
        if (ingredients[i].count < this->ingredients[i].count) {
            return false;
        }
    }
    
    return true;
}

bool Recipe::matchesShapeless(const std::vector<ItemStack>& ingredients) const {
    std::vector<bool> used(ingredients.size(), false);
    
    for (const auto& required : this->ingredients) {
        bool found = false;
        
        for (size_t i = 0; i < ingredients.size(); i++) {
            if (used[i]) continue;
            
            if (ingredients[i].canStackWith(required) && 
                ingredients[i].count >= required.count) {
                used[i] = true;
                found = true;
                break;
            }
        }
        
        if (!found) return false;
    }
    
    return true;
}

// ============================================================================
// Crafting System Implementation
// ============================================================================

CraftingSystem::CraftingSystem() {
    LOG_INFO("CraftingSystem created");
    registerVanillaRecipes();
}

CraftingSystem::~CraftingSystem() {
    LOG_INFO("CraftingSystem destroyed");
}

void CraftingSystem::registerRecipe(const Recipe& recipe) {
    recipes_.push_back(recipe);
    LOG_DEBUG("Registered recipe: {} -> {}", recipe.name, recipe.result.item);
}

const Recipe* CraftingSystem::findRecipe(const std::vector<ItemStack>& ingredients, 
                                          RecipeType type) const {
    for (const auto& recipe : recipes_) {
        if (recipe.type != type) continue;
        
        if (recipe.shapeless) {
            if (recipe.matchesShapeless(ingredients)) {
                return &recipe;
            }
        } else {
            if (recipe.matches(ingredients)) {
                return &recipe;
            }
        }
    }
    
    return nullptr;
}

std::vector<const Recipe*> CraftingSystem::findAllRecipes(const std::vector<ItemStack>& ingredients,
                                                           RecipeType type) const {
    std::vector<const Recipe*> matching;
    
    for (const auto& recipe : recipes_) {
        if (recipe.type != type) continue;
        
        bool canCraft = true;
        
        if (recipe.shapeless) {
            canCraft = recipe.matchesShapeless(ingredients);
        } else {
            canCraft = recipe.matches(ingredients);
        }
        
        if (canCraft) {
            matching.push_back(&recipe);
        }
    }
    
    return matching;
}

bool CraftingSystem::canCraft(const Recipe& recipe, 
                               const std::vector<ItemStack>& available) const {
    std::vector<ItemStack> remaining = available;
    
    for (const auto& ingredient : recipe.ingredients) {
        bool found = false;
        
        for (auto& stack : remaining) {
            if (stack.canStackWith(ingredient) && stack.count >= ingredient.count) {
                stack.count -= ingredient.count;
                found = true;
                break;
            }
        }
        
        if (!found) return false;
    }
    
    return true;
}

ItemStack CraftingSystem::craft(const Recipe& recipe, std::vector<ItemStack>& ingredients) const {
    // Remove ingredients
    for (const auto& ingredient : recipe.ingredients) {
        for (auto& stack : ingredients) {
            if (stack.canStackWith(ingredient)) {
                int toRemove = std::min(ingredient.count, stack.count);
                stack.count -= toRemove;
                if (stack.count <= 0) {
                    stack = ItemStack();
                }
                break;
            }
        }
    }
    
    // Return result
    return recipe.result;
}

void CraftingSystem::registerVanillaRecipes() {
    LOG_INFO("Registering vanilla recipes...");
    
    // Wood planks from logs
    Recipe planks;
    planks.name = "oak_planks";
    planks.type = RecipeType::Crafting;
    planks.shapeless = false;
    planks.ingredients = {{1, 0}}; // Oak log
    planks.result = {4, 1}; // 4 oak planks
    planks.width = 1;
    planks.height = 1;
    registerRecipe(planks);
    
    // Sticks
    Recipe sticks;
    sticks.name = "sticks";
    sticks.type = RecipeType::Crafting;
    sticks.shapeless = false;
    sticks.ingredients = {{1, 1}, {1, 1}}; // 2 planks vertically
    sticks.result = {4, 2}; // 4 sticks
    sticks.width = 1;
    sticks.height = 2;
    registerRecipe(sticks);
    
    // Crafting table
    Recipe craftingTable;
    craftingTable.name = "crafting_table";
    craftingTable.type = RecipeType::Crafting;
    craftingTable.shapeless = false;
    craftingTable.ingredients = {
        {1, 1}, {1, 1},
        {1, 1}, {1, 1}
    }; // 4 planks in square
    craftingTable.result = {1, 3};
    craftingTable.width = 2;
    craftingTable.height = 2;
    registerRecipe(craftingTable);
    
    // Furnace
    Recipe furnace;
    furnace.name = "furnace";
    furnace.type = RecipeType::Crafting;
    furnace.shapeless = false;
    furnace.ingredients = {
        {1, 4}, {1, 4}, {1, 4},
        {1, 4}, {0, 0}, {1, 4},
        {1, 4}, {1, 4}, {1, 4}
    }; // 8 cobblestone with hole
    furnace.result = {1, 5};
    furnace.width = 3;
    furnace.height = 3;
    registerRecipe(furnace);
    
    // Chest
    Recipe chest;
    chest.name = "chest";
    chest.type = RecipeType::Crafting;
    chest.shapeless = false;
    chest.ingredients = {
        {1, 1}, {1, 1}, {1, 1},
        {1, 1}, {0, 0}, {1, 1},
        {1, 1}, {1, 1}, {1, 1}
    }; // 8 planks with hole
    chest.result = {1, 6};
    chest.width = 3;
    chest.height = 3;
    registerRecipe(chest);
    
    // Wooden pickaxe
    Recipe woodenPickaxe;
    woodenPickaxe.name = "wooden_pickaxe";
    woodenPickaxe.type = RecipeType::Crafting;
    woodenPickaxe.shapeless = false;
    woodenPickaxe.ingredients = {
        {1, 1}, {1, 1}, {1, 1},
        {0, 0}, {1, 2}, {0, 0},
        {0, 0}, {1, 2}, {0, 0}
    };
    woodenPickaxe.result = {1, 10};
    woodenPickaxe.width = 3;
    woodenPickaxe.height = 3;
    registerRecipe(woodenPickaxe);
    
    // Wooden axe
    Recipe woodenAxe;
    woodenAxe.name = "wooden_axe";
    woodenAxe.type = RecipeType::Crafting;
    woodenAxe.shapeless = false;
    woodenAxe.ingredients = {
        {1, 1}, {1, 1},
        {1, 1}, {1, 2},
        {0, 0}, {1, 2}
    };
    woodenAxe.result = {1, 11};
    woodenAxe.width = 2;
    woodenAxe.height = 3;
    registerRecipe(woodenAxe);
    
    // Wooden sword
    Recipe woodenSword;
    woodenSword.name = "wooden_sword";
    woodenSword.type = RecipeType::Crafting;
    woodenSword.shapeless = false;
    woodenSword.ingredients = {
        {1, 1},
        {1, 1},
        {1, 2}
    };
    woodenSword.result = {1, 12};
    woodenSword.width = 1;
    woodenSword.height = 3;
    registerRecipe(woodenSword);
    
    // Wooden shovel
    Recipe woodenShovel;
    woodenShovel.name = "wooden_shovel";
    woodenShovel.type = RecipeType::Crafting;
    woodenShovel.shapeless = false;
    woodenShovel.ingredients = {
        {1, 1},
        {1, 2},
        {1, 2}
    };
    woodenShovel.result = {1, 13};
    woodenShovel.width = 1;
    woodenShovel.height = 3;
    registerRecipe(woodenShovel);
    
    // Wooden hoe
    Recipe woodenHoe;
    woodenHoe.name = "wooden_hoe";
    woodenHoe.type = RecipeType::Crafting;
    woodenHoe.shapeless = false;
    woodenHoe.ingredients = {
        {1, 1}, {1, 1},
        {0, 0}, {1, 2},
        {0, 0}, {1, 2}
    };
    woodenHoe.result = {1, 14};
    woodenHoe.width = 2;
    woodenHoe.height = 3;
    registerRecipe(woodenHoe);
    
    // Torch
    Recipe torch;
    torch.name = "torch";
    torch.type = RecipeType::Crafting;
    torch.shapeless = false;
    torch.ingredients = {
        {1, 15}, // Coal
        {1, 2}   // Stick
    };
    torch.result = {4, 16};
    torch.width = 1;
    torch.height = 2;
    registerRecipe(torch);
    
    // Bread (shapeless)
    Recipe bread;
    bread.name = "bread";
    bread.type = RecipeType::Crafting;
    bread.shapeless = true;
    bread.ingredients = {{3, 20}}; // 3 wheat
    bread.result = {1, 17};
    registerRecipe(bread);
    
    LOG_INFO("Registered {} vanilla recipes", recipes_.size());
}

// ============================================================================
// Smelting System Implementation
// ============================================================================

SmeltingSystem::SmeltingSystem() {
    LOG_INFO("SmeltingSystem created");
    registerVanillaSmeltingRecipes();
}

SmeltingSystem::~SmeltingSystem() {
    LOG_INFO("SmeltingSystem destroyed");
}

void SmeltingSystem::registerSmeltingRecipe(const SmeltingRecipe& recipe) {
    smeltingRecipes_.push_back(recipe);
}

const SmeltingRecipe* SmeltingSystem::findSmeltingRecipe(ItemId input) const {
    for (const auto& recipe : smeltingRecipes_) {
        if (recipe.input == input) {
            return &recipe;
        }
    }
    return nullptr;
}

float SmeltingSystem::getSmeltingTime(ItemId input) const {
    auto* recipe = findSmeltingRecipe(input);
    return recipe ? recipe->cookTime : 10.0f; // Default 10 seconds
}

int SmeltingSystem::getFuelValue(ItemId fuel) const {
    // Coal = 8 items, charcoal = 8, stick = 0.5, etc.
    static const std::unordered_map<ItemId, int> fuelValues = {
        {100, 8},    // Coal
        {101, 8},    // Charcoal
        {102, 80},   // Coal block
        {103, 1},    // Stick
        {104, 1.5},  // Wood planks
        {105, 15},   // Blaze rod
        {106, 100},  // Lava bucket
    };
    
    auto it = fuelValues.find(fuel);
    return it != fuelValues.end() ? it->second : 0;
}

bool SmeltingSystem::isFuel(ItemId item) const {
    return getFuelValue(item) > 0;
}

void SmeltingSystem::registerVanillaSmeltingRecipes() {
    LOG_INFO("Registering vanilla smelting recipes...");
    
    // Iron ore -> Iron ingot
    registerSmeltingRecipe({200, 300, 10.0f, 0.7f});
    
    // Gold ore -> Gold ingot
    registerSmeltingRecipe({201, 301, 10.0f, 1.0f});
    
    // Copper ore -> Copper ingot
    registerSmeltingRecipe({202, 302, 10.0f, 0.7f});
    
    // Sand -> Glass
    registerSmeltingRecipe({203, 303, 10.0f, 0.1f});
    
    // Cobblestone -> Stone
    registerSmeltingRecipe({204, 4, 10.0f, 0.1f});
    
    // Stone -> Smooth stone
    registerSmeltingRecipe({4, 305, 10.0f, 0.1f});
    
    // Clay -> Terracotta
    registerSmeltingRecipe({205, 306, 10.0f, 0.35f});
    
    // Raw porkchop -> Cooked porkchop
    registerSmeltingRecipe({400, 401, 10.0f, 0.35f});
    
    // Raw beef -> Steak
    registerSmeltingRecipe({402, 403, 10.0f, 0.35f});
    
    // Raw chicken -> Cooked chicken
    registerSmeltingRecipe({404, 405, 10.0f, 0.35f});
    
    // Potato -> Baked potato
    registerSmeltingRecipe({500, 501, 10.0f, 0.35f});
    
    LOG_INFO("Registered {} smelting recipes", smeltingRecipes_.size());
}

} // namespace VoxelForge
