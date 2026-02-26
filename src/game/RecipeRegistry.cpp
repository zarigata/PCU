/**
 * @file RecipeRegistry.cpp
 * @brief Recipe registry implementation
 */

#include <VoxelForge/game/RecipeRegistry.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>
#include <sstream>

namespace VoxelForge {

// ============================================================================
// Ingredient Implementation
// ============================================================================

bool Ingredient::matches(const ItemId& item) const {
    if (isEmpty()) return false;
    
    for (const auto& i : items) {
        if (i == item) return true;
    }
    return false;
}

// ============================================================================
// Recipe Implementation
// ============================================================================

bool Recipe::matches(const std::vector<ItemStack>& inputs) const {
    if (isShapeless) {
        return matchesShapeless(inputs);
    }
    return matchesShaped(inputs, 3, 3);
}

bool Recipe::matchesShaped(const std::vector<ItemStack>& inputs, int gridWidth, int gridHeight) const {
    if (inputs.empty()) return false;
    
    // Try all possible positions
    for (int startY = 0; startY <= gridHeight - height; ++startY) {
        for (int startX = 0; startX <= gridWidth - width; ++startX) {
            bool match = true;
            
            for (int y = 0; y < height && match; ++y) {
                for (int x = 0; x < width && match; ++x) {
                    int inputIdx = (startY + y) * gridWidth + (startX + x);
                    int patternIdx = y * width + x;
                    
                    const auto& input = inputs[inputIdx];
                    const auto& ingredient = ingredients[patternIdx];
                    
                    if (input.isEmpty() && ingredient.isEmpty()) continue;
                    if (input.isEmpty() != ingredient.isEmpty()) {
                        match = false;
                        continue;
                    }
                    
                    if (!ingredient.matches(input.itemId) || input.count < ingredient.count) {
                        match = false;
                    }
                }
            }
            
            // Check remaining slots are empty
            for (int y = 0; y < gridHeight && match; ++y) {
                for (int x = 0; x < gridWidth && match; ++x) {
                    if (x >= startX && x < startX + width &&
                        y >= startY && y < startY + height) continue;
                    
                    int inputIdx = y * gridWidth + x;
                    if (!inputs[inputIdx].isEmpty()) {
                        match = false;
                    }
                }
            }
            
            if (match) return true;
        }
    }
    
    return false;
}

bool Recipe::matchesShapeless(const std::vector<ItemStack>& inputs) const {
    std::vector<bool> used(inputs.size(), false);
    
    for (const auto& ingredient : ingredients) {
        bool found = false;
        
        for (size_t i = 0; i < inputs.size(); ++i) {
            if (used[i]) continue;
            
            if (ingredient.matches(inputs[i].itemId) && inputs[i].count >= ingredient.count) {
                used[i] = true;
                found = true;
                break;
            }
        }
        
        if (!found) return false;
    }
    
    return true;
}

ItemStack Recipe::getResult() const {
    ItemStack stack;
    stack.itemId = result.item;
    stack.count = result.count;
    return stack;
}

std::vector<ItemStack> Recipe::getRemainingItems(const std::vector<ItemStack>& inputs) const {
    std::vector<ItemStack> remaining = inputs;
    // Return containers (buckets, bottles, etc.)
    for (auto& stack : remaining) {
        // TODO: Handle item containers
    }
    return remaining;
}

nlohmann::json Recipe::toJson() const {
    nlohmann::json j;
    j["type"] = static_cast<int>(type);
    j["id"] = id;
    
    if (!group.empty()) j["group"] = group;
    
    // Ingredients
    nlohmann::json ingrJson = nlohmann::json::array();
    for (const auto& ing : ingredients) {
        nlohmann::json ingJ;
        ingJ["items"] = ing.items;
        ingJ["count"] = ing.count;
        ingJ["isTag"] = ing.isTag;
        ingrJson.push_back(ingJ);
    }
    j["ingredients"] = ingrJson;
    
    // Result
    j["result"]["item"] = result.item;
    j["result"]["count"] = result.count;
    j["result"]["chance"] = result.chance;
    
    if (!isShapeless) {
        j["width"] = width;
        j["height"] = height;
        j["pattern"] = pattern;
    }
    
    if (type == RecipeType::Smelting || type == RecipeType::Blasting ||
        type == RecipeType::Smoking || type == RecipeType::Campfire) {
        j["cookingTime"] = cookingTime;
        j["experience"] = experience;
    }
    
    return j;
}

std::unique_ptr<Recipe> Recipe::fromJson(const nlohmann::json& json) {
    RecipeType type = static_cast<RecipeType>(json.value("type", 0));
    
    std::unique_ptr<Recipe> recipe;
    if (json.value("shapeless", false)) {
        recipe = std::make_unique<ShapelessRecipe>();
    } else {
        recipe = std::make_unique<ShapedRecipe>();
    }
    
    recipe->type = type;
    recipe->id = json.value("id", "");
    recipe->group = json.value("group", "");
    
    // Parse ingredients
    if (json.contains("ingredients")) {
        for (const auto& ingJ : json["ingredients"]) {
            Ingredient ing;
            ing.count = ingJ.value("count", 1);
            ing.isTag = ingJ.value("isTag", false);
            if (ingJ.contains("items")) {
                ing.items = ingJ["items"].get<std::vector<std::string>>();
            }
            recipe->ingredients.push_back(ing);
        }
    }
    
    // Parse result
    if (json.contains("result")) {
        recipe->result.item = json["result"].value("item", "");
        recipe->result.count = json["result"].value("count", 1);
        recipe->result.chance = json["result"].value("chance", 1.0f);
    }
    
    recipe->width = json.value("width", 0);
    recipe->height = json.value("height", 0);
    
    if (json.contains("pattern")) {
        recipe->pattern = json["pattern"].get<std::vector<std::string>>();
    }
    
    recipe->cookingTime = json.value("cookingTime", 10.0f);
    recipe->experience = json.value("experience", 0.0f);
    
    return recipe;
}

// ============================================================================
// ShapedRecipe Implementation
// ============================================================================

bool ShapedRecipe::matches(const std::vector<ItemStack>& inputs) const {
    return matchesShaped(inputs, 3, 3);
}

bool ShapedRecipe::matchesShaped(const std::vector<ItemStack>& inputs, int gridWidth, int gridHeight) const {
    return Recipe::matchesShaped(inputs, gridWidth, gridHeight);
}

// ============================================================================
// ShapelessRecipe Implementation
// ============================================================================

bool ShapelessRecipe::matches(const std::vector<ItemStack>& inputs) const {
    return matchesShapeless(inputs);
}

// ============================================================================
// CookingRecipe Implementation
// ============================================================================

bool CookingRecipe::matches(const std::vector<ItemStack>& inputs) const {
    if (inputs.empty() || ingredients.empty()) return false;
    return ingredients[0].matches(inputs[0].itemId);
}

// ============================================================================
// RecipeRegistry Implementation
// ============================================================================

RecipeRegistry& RecipeRegistry::getInstance() {
    static RecipeRegistry instance;
    return instance;
}

RecipeRegistry::RecipeRegistry() {
    LOG_INFO("RecipeRegistry created");
}

void RecipeRegistry::registerRecipe(std::unique_ptr<Recipe> recipe) {
    if (!recipe || recipe->id.empty()) {
        LOG_ERROR("Cannot register null or empty recipe");
        return;
    }
    
    std::string id = recipe->id;
    ItemId outputItem = recipe->result.item;
    
    recipes[id] = std::move(recipe);
    recipesByType[recipes[id]->type].push_back(id);
    recipesByOutput[outputItem].push_back(id);
    
    // Index by input items
    for (const auto& ing : recipes[id]->ingredients) {
        for (const auto& item : ing.items) {
            if (std::find(recipesByInput[item].begin(), recipesByInput[item].end(), id) == recipesByInput[item].end()) {
                recipesByInput[item].push_back(id);
            }
        }
    }
    
    LOG_DEBUG("Registered recipe: {}", id);
}

void RecipeRegistry::unregisterRecipe(const std::string& id) {
    auto it = recipes.find(id);
    if (it == recipes.end()) return;
    
    // Remove from type index
    auto& typeList = recipesByType[it->second->type];
    typeList.erase(std::remove(typeList.begin(), typeList.end(), id), typeList.end());
    
    // Remove from output index
    auto& outputList = recipesByOutput[it->second->result.item];
    outputList.erase(std::remove(outputList.begin(), outputList.end(), id), outputList.end());
    
    // Remove from input index
    for (const auto& ing : it->second->ingredients) {
        for (const auto& item : ing.items) {
            auto& inputList = recipesByInput[item];
            inputList.erase(std::remove(inputList.begin(), inputList.end(), id), inputList.end());
        }
    }
    
    recipes.erase(it);
    LOG_DEBUG("Unregistered recipe: {}", id);
}

void RecipeRegistry::clear() {
    recipes.clear();
    recipesByType.clear();
    recipesByOutput.clear();
    recipesByInput.clear();
    LOG_INFO("RecipeRegistry cleared");
}

void RecipeRegistry::clearModRecipes(const std::string& modId) {
    std::vector<std::string> toRemove;
    for (const auto& [id, recipe] : recipes) {
        if (id.find(modId + ":") == 0) {
            toRemove.push_back(id);
        }
    }
    for (const auto& id : toRemove) {
        unregisterRecipe(id);
    }
}

const Recipe* RecipeRegistry::getRecipe(const std::string& id) const {
    auto it = recipes.find(id);
    return it != recipes.end() ? it->second.get() : nullptr;
}

std::vector<const Recipe*> RecipeRegistry::getRecipesByType(RecipeType type) const {
    std::vector<const Recipe*> result;
    auto it = recipesByType.find(type);
    if (it != recipesByType.end()) {
        for (const auto& id : it->second) {
            auto recipeIt = recipes.find(id);
            if (recipeIt != recipes.end()) {
                result.push_back(recipeIt->second.get());
            }
        }
    }
    return result;
}

std::vector<const Recipe*> RecipeRegistry::getRecipesByCategory(RecipeCategory category) const {
    std::vector<const Recipe*> result;
    for (const auto& [id, recipe] : recipes) {
        if (recipe->category == category) {
            result.push_back(recipe.get());
        }
    }
    return result;
}

std::vector<const Recipe*> RecipeRegistry::getRecipesForItem(const ItemId& item) const {
    std::vector<const Recipe*> result;
    auto it = recipesByOutput.find(item);
    if (it != recipesByOutput.end()) {
        for (const auto& id : it->second) {
            auto recipeIt = recipes.find(id);
            if (recipeIt != recipes.end()) {
                result.push_back(recipeIt->second.get());
            }
        }
    }
    return result;
}

std::vector<const Recipe*> RecipeRegistry::getRecipesUsingItem(const ItemId& item) const {
    std::vector<const Recipe*> result;
    auto it = recipesByInput.find(item);
    if (it != recipesByInput.end()) {
        for (const auto& id : it->second) {
            auto recipeIt = recipes.find(id);
            if (recipeIt != recipes.end()) {
                result.push_back(recipeIt->second.get());
            }
        }
    }
    return result;
}

std::vector<const Recipe*> RecipeRegistry::findMatchingRecipes(const std::vector<ItemStack>& inputs, 
                                                                 RecipeType type) const {
    std::vector<const Recipe*> result;
    auto typeRecipes = getRecipesByType(type);
    
    for (const auto* recipe : typeRecipes) {
        if (recipe->matches(inputs)) {
            result.push_back(recipe);
        }
    }
    return result;
}

const Recipe* RecipeRegistry::findFirstMatchingRecipe(const std::vector<ItemStack>& inputs,
                                                        RecipeType type) const {
    auto matches = findMatchingRecipes(inputs, type);
    return matches.empty() ? nullptr : matches[0];
}

bool RecipeRegistry::canCraft(const Recipe& recipe, const std::vector<ItemStack>& inventory) const {
    std::unordered_map<ItemId, int> available;
    for (const auto& stack : inventory) {
        if (!stack.isEmpty()) {
            available[stack.itemId] += stack.count;
        }
    }
    
    for (const auto& ing : recipe.ingredients) {
        int needed = ing.count;
        for (const auto& item : ing.items) {
            if (available[item] >= needed) {
                available[item] -= needed;
                needed = 0;
                break;
            } else if (available[item] > 0) {
                needed -= available[item];
                available[item] = 0;
            }
        }
        if (needed > 0) return false;
    }
    return true;
}

void RecipeRegistry::registerVanillaRecipes() {
    LOG_INFO("Registering vanilla recipes...");
    
    // Oak planks from log
    auto planks = std::make_unique<ShapedRecipe>();
    planks->id = "minecraft:oak_planks";
    planks->type = RecipeType::Crafting;
    planks->category = RecipeCategory::Building;
    planks->ingredients = {Ingredient{{"minecraft:oak_log"}, 1}};
    planks->result = {"minecraft:oak_planks", 4};
    planks->width = 1;
    planks->height = 1;
    registerRecipe(std::move(planks));
    
    // Sticks
    auto sticks = std::make_unique<ShapedRecipe>();
    sticks->id = "minecraft:sticks";
    sticks->type = RecipeType::Crafting;
    sticks->category = RecipeCategory::Misc;
    sticks->ingredients = {
        Ingredient{{"minecraft:oak_planks"}, 1},
        Ingredient{{"minecraft:oak_planks"}, 1}
    };
    sticks->result = {"minecraft:stick", 4};
    sticks->width = 1;
    sticks->height = 2;
    sticks->pattern = {"P", "P"};
    sticks->key = {{'P', Ingredient{{"minecraft:oak_planks"}, 1}}};
    registerRecipe(std::move(sticks));
    
    // Crafting table
    auto craftingTable = std::make_unique<ShapedRecipe>();
    craftingTable->id = "minecraft:crafting_table";
    craftingTable->type = RecipeType::Crafting;
    craftingTable->category = RecipeCategory::Building;
    craftingTable->ingredients = {
        Ingredient{{"minecraft:oak_planks"}, 1},
        Ingredient{{"minecraft:oak_planks"}, 1},
        Ingredient{{"minecraft:oak_planks"}, 1},
        Ingredient{{"minecraft:oak_planks"}, 1}
    };
    craftingTable->result = {"minecraft:crafting_table", 1};
    craftingTable->width = 2;
    craftingTable->height = 2;
    craftingTable->pattern = {"PP", "PP"};
    craftingTable->key = {{'P', Ingredient{{"minecraft:oak_planks"}, 1}}};
    registerRecipe(std::move(craftingTable));
    
    // Furnace
    auto furnace = std::make_unique<ShapedRecipe>();
    furnace->id = "minecraft:furnace";
    furnace->type = RecipeType::Crafting;
    furnace->category = RecipeCategory::Misc;
    furnace->ingredients = {
        Ingredient{{"minecraft:cobblestone"}, 1},
        Ingredient{{"minecraft:cobblestone"}, 1},
        Ingredient{{"minecraft:cobblestone"}, 1},
        Ingredient{{"minecraft:cobblestone"}, 1},
        Ingredient{},
        Ingredient{{"minecraft:cobblestone"}, 1},
        Ingredient{{"minecraft:cobblestone"}, 1},
        Ingredient{{"minecraft:cobblestone"}, 1},
        Ingredient{{"minecraft:cobblestone"}, 1}
    };
    furnace->result = {"minecraft:furnace", 1};
    furnace->width = 3;
    furnace->height = 3;
    furnace->pattern = {"CCC", "C C", "CCC"};
    furnace->key = {{'C', Ingredient{{"minecraft:cobblestone"}, 1}}};
    registerRecipe(std::move(furnace));
    
    // Wooden pickaxe
    auto pickaxe = std::make_unique<ShapedRecipe>();
    pickaxe->id = "minecraft:wooden_pickaxe";
    pickaxe->type = RecipeType::Crafting;
    pickaxe->category = RecipeCategory::Equipment;
    pickaxe->ingredients = {
        Ingredient{{"minecraft:oak_planks"}, 1},
        Ingredient{{"minecraft:oak_planks"}, 1},
        Ingredient{{"minecraft:oak_planks"}, 1},
        Ingredient{},
        Ingredient{{"minecraft:stick"}, 1},
        Ingredient{},
        Ingredient{},
        Ingredient{{"minecraft:stick"}, 1},
        Ingredient{}
    };
    pickaxe->result = {"minecraft:wooden_pickaxe", 1};
    pickaxe->width = 3;
    pickaxe->height = 3;
    pickaxe->pattern = {"PPP", " S ", " S "};
    pickaxe->key = {
        {'P', Ingredient{{"minecraft:oak_planks"}, 1}},
        {'S', Ingredient{{"minecraft:stick"}, 1}}
    };
    registerRecipe(std::move(pickaxe));
    
    // Iron ingot (smelting)
    auto ironIngot = std::make_unique<CookingRecipe>();
    ironIngot->id = "minecraft:iron_ingot";
    ironIngot->type = RecipeType::Smelting;
    ironIngot->ingredients = {Ingredient{{"minecraft:iron_ore"}, 1}};
    ironIngot->result = {"minecraft:iron_ingot", 1};
    ironIngot->cookingTime = 10.0f;
    ironIngot->experience = 0.7f;
    registerRecipe(std::move(ironIngot));
    
    LOG_INFO("Registered {} vanilla recipes", recipes.size());
}

void RecipeRegistry::loadFromDirectory(const std::string& path) {
    // TODO: Load recipes from JSON files in directory
    LOG_INFO("Loading recipes from directory: {}", path);
}

void RecipeRegistry::loadFromJson(const nlohmann::json& json) {
    for (const auto& recipeJson : json) {
        auto recipe = Recipe::fromJson(recipeJson);
        if (recipe) {
            registerRecipe(std::move(recipe));
        }
    }
}

nlohmann::json RecipeRegistry::saveToJson() const {
    nlohmann::json j = nlohmann::json::array();
    for (const auto& [id, recipe] : recipes) {
        j.push_back(recipe->toJson());
    }
    return j;
}

size_t RecipeRegistry::getRecipeCountByType(RecipeType type) const {
    auto it = recipesByType.find(type);
    return it != recipesByType.end() ? it->second.size() : 0;
}

} // namespace VoxelForge
