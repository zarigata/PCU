/**
 * @file RecipeRegistry.cpp
 * @brief Full implementation of the recipe registry system
 */

#include <VoxelForge/game/RecipeRegistry.hpp>
#include <VoxelForge/game/Inventory.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>
#include <fstream>

namespace VoxelForge {

// ============================================================================
// Ingredient Implementation
// ============================================================================

bool Ingredient::matches(const ItemId& item) const {
    if (isEmpty()) return false;
    if (isTag) {
        // Tag matching: item must be in the tag list
        for (const auto& tagItem : items) {
            if (tagItem == item) return true;
        }
        return false;
    }
    for (const auto& i : items) {
        if (i == item) return true;
    }
    return false;
}

// ============================================================================
// Recipe Base Implementation
// ============================================================================

bool Recipe::matches(const std::vector<ItemStack>& inputs) const {
    if (isShapeless) {
        return matchesShapeless(inputs);
    }
    return matchesShaped(inputs, width, height);
}

bool Recipe::matchesShaped(const std::vector<ItemStack>& inputs, int gridWidth, int gridHeight) const {
    if (pattern.empty() || key.empty()) {
        // Fallback: direct positional match using ingredients vector
        if (static_cast<int>(inputs.size()) != width * height) return false;
        for (int i = 0; i < width * height; ++i) {
            const auto& input = inputs[i];
            const auto& ing = (i < static_cast<int>(ingredients.size())) ? ingredients[i] : Ingredient{};
            if (ing.isEmpty() && input.isEmpty()) continue;
            if (ing.isEmpty() || input.isEmpty()) return false;
            // Resolve item to string ID for matching
            if (!ing.matches(std::to_string(input.getItem()))) return false;
            if (input.getCount() < ing.count) return false;
        }
        return true;
    }

    // Pattern-based matching with sliding (find smallest bounding box)
    int patMinR = height, patMinC = width, patMaxR = 0, patMaxC = 0;
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            char ch = pattern[r][c];
            if (ch != ' ') {
                patMinR = std::min(patMinR, r);
                patMinC = std::min(patMinC, c);
                patMaxR = std::max(patMaxR, r);
                patMaxC = std::max(patMaxC, c);
            }
        }
    }
    int patH = patMaxR - patMinR + 1;
    int patW = patMaxC - patMinC + 1;

    if (gridWidth < patW || gridHeight < patH) return false;

    // Try all offsets
    for (int dr = 0; dr <= gridHeight - patH; ++dr) {
        for (int dc = 0; dc <= gridWidth - patW; ++dc) {
            bool ok = true;
            for (int r = 0; r < gridHeight && ok; ++r) {
                for (int c = 0; c < gridWidth && ok; ++c) {
                    int pr = r - dr + patMinR;
                    int pc = c - dc + patMinC;
                    const auto& input = inputs[r * gridWidth + c];
                    if (pr >= 0 && pr < height && pc >= 0 && pc < static_cast<int>(pattern[pr].size())) {
                        char ch = pattern[pr][pc];
                        if (ch == ' ') {
                            if (!input.isEmpty()) ok = false;
                        } else {
                            auto it = key.find(ch);
                            if (it == key.end()) { ok = false; continue; }
                            if (input.isEmpty()) { ok = false; continue; }
                            if (!it->second.matches(std::to_string(input.getItem()))) ok = false;
                        }
                    } else {
                        if (!input.isEmpty()) ok = false;
                    }
                }
            }
            if (ok) return true;
        }
    }
    return false;
}

bool Recipe::matchesShapeless(const std::vector<ItemStack>& inputs) const {
    // Count how many non-empty inputs
    std::vector<size_t> nonEmpty;
    for (size_t i = 0; i < inputs.size(); ++i) {
        if (!inputs[i].isEmpty()) nonEmpty.push_back(i);
    }

    std::vector<Ingredient> required = ingredients;
    if (nonEmpty.size() != required.size()) return false;

    // Try all permutations (greedy matching for small recipe sizes)
    std::vector<bool> used(required.size(), false);
    for (size_t ni : nonEmpty) {
        bool found = false;
        for (size_t ri = 0; ri < required.size(); ++ri) {
            if (used[ri]) continue;
            if (required[ri].matches(std::to_string(inputs[ni].getItem())) &&
                inputs[ni].getCount() >= required[ri].count) {
                used[ri] = true;
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

ItemStack Recipe::getResult() const {
    // Convert result item string to ItemStack
    // Try numeric conversion first (for legacy IDs)
    try {
        ItemID id = static_cast<ItemID>(std::stoi(result.item));
        return ItemStack(id, static_cast<ItemCount>(result.count));
    } catch (...) {
        return ItemStack(result.item, static_cast<ItemCount>(result.count));
    }
}

std::vector<ItemStack> Recipe::getRemainingItems(const std::vector<ItemStack>& inputs) const {
    std::vector<ItemStack> remaining = inputs;
    // Default: return copies of inputs (containers like buckets stay)
    for (auto& stack : remaining) {
        // In a full implementation, check for container items
        // For now, consumed items become empty
        stack = ItemStack();
    }
    return remaining;
}

nlohmann::json Recipe::toJson() const {
    nlohmann::json j;
    j["id"] = id;
    j["type"] = static_cast<int>(type);
    j["category"] = static_cast<int>(category);
    j["isShapeless"] = isShapeless;
    j["result"] = {{"item", result.item}, {"count", result.count}};
    if (!isShapeless) {
        j["width"] = width;
        j["height"] = height;
        j["pattern"] = pattern;
        nlohmann::json keyJson;
        for (auto& [ch, ing] : key) {
            nlohmann::json ingJson;
            ingJson["items"] = ing.items;
            ingJson["count"] = ing.count;
            ingJson["isTag"] = ing.isTag;
            keyJson[std::string(1, ch)] = ingJson;
        }
        j["key"] = keyJson;
    } else {
        nlohmann::json ings = nlohmann::json::array();
        for (auto& ing : ingredients) {
            ings.push_back({{"items", ing.items}, {"count", ing.count}, {"isTag", ing.isTag}});
        }
        j["ingredients"] = ings;
    }
    if (cookingTime != 10.0f) j["cookingTime"] = cookingTime;
    if (experience != 0.0f) j["experience"] = experience;
    if (!group.empty()) j["group"] = group;
    return j;
}

std::unique_ptr<Recipe> Recipe::fromJson(const nlohmann::json& json) {
    auto typeVal = json.value("type", 0);
    RecipeType rtype = static_cast<RecipeType>(typeVal);

    std::unique_ptr<Recipe> recipe;
    switch (rtype) {
        case RecipeType::Smelting:
        case RecipeType::Blasting:
        case RecipeType::Smoking:
        case RecipeType::Campfire:
            recipe = std::make_unique<CookingRecipe>(rtype);
            break;
        case RecipeType::Stonecutting:
            recipe = std::make_unique<StonecuttingRecipe>();
            break;
        default:
            if (json.value("isShapeless", false)) {
                recipe = std::make_unique<ShapelessRecipe>();
            } else {
                recipe = std::make_unique<ShapedRecipe>();
            }
            break;
    }

    recipe->id = json.value("id", "");
    recipe->group = json.value("group", "");
    recipe->category = static_cast<RecipeCategory>(json.value("category", 3));

    // Result
    if (json.contains("result")) {
        recipe->result.item = json["result"].value("item", "");
        recipe->result.count = json["result"].value("count", 1);
        recipe->result.chance = json["result"].value("chance", 1.0f);
    }

    // Cooking
    recipe->cookingTime = json.value("cookingTime", 10.0f);
    recipe->experience = json.value("experience", 0.0f);

    // Shaped
    if (!recipe->isShapeless && json.contains("pattern")) {
        recipe->pattern = json["pattern"].get<std::vector<std::string>>();
        recipe->height = static_cast<int>(recipe->pattern.size());
        recipe->width = recipe->pattern.empty() ? 0 : static_cast<int>(recipe->pattern[0].size());
        if (json.contains("key")) {
            for (auto& [keyChar, ingJson] : json["key"].items()) {
                Ingredient ing;
                if (ingJson.contains("items")) ing.items = ingJson["items"].get<std::vector<ItemId>>();
                ing.count = ingJson.value("count", 1);
                ing.isTag = ingJson.value("isTag", false);
                recipe->key[keyChar[0]] = ing;
            }
        }
    }

    // Shapeless ingredients
    if (recipe->isShapeless && json.contains("ingredients")) {
        for (auto& ingJson : json["ingredients"]) {
            Ingredient ing;
            if (ingJson.contains("items")) ing.items = ingJson["items"].get<std::vector<ItemId>>();
            ing.count = ingJson.value("count", 1);
            ing.isTag = ingJson.value("isTag", false);
            recipe->ingredients.push_back(ing);
        }
    }

    return recipe;
}

// ============================================================================
// ShapedRecipe Implementation
// ============================================================================

bool ShapedRecipe::matches(const std::vector<ItemStack>& inputs) const {
    return matchesShaped(inputs, width, height);
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
    if (inputs.empty()) return false;
    // Single input cooking
    if (!ingredients.empty()) {
        return ingredients[0].matches(std::to_string(inputs[0].getItem()));
    }
    return false;
}

// ============================================================================
// StonecuttingRecipe Implementation
// ============================================================================

bool StonecuttingRecipe::matches(const std::vector<ItemStack>& inputs) const {
    if (inputs.empty()) return false;
    if (!ingredients.empty()) {
        return ingredients[0].matches(std::to_string(inputs[0].getItem()));
    }
    return false;
}

// ============================================================================
// RecipeRegistry Implementation
// ============================================================================

RecipeRegistry::RecipeRegistry() {
    VF_INFO("RecipeRegistry initialized");
}

RecipeRegistry& RecipeRegistry::getInstance() {
    static RecipeRegistry instance;
    return instance;
}

void RecipeRegistry::registerRecipe(std::unique_ptr<Recipe> recipe) {
    if (!recipe || recipe->id.empty()) {
        VF_WARN("Attempted to register invalid recipe");
        return;
    }

    std::string rid = recipe->id;
    RecipeType rtype = recipe->type;
    ItemId outputItem = recipe->result.item;

    // Track by input ingredients for fast lookup
    for (const auto& ing : recipe->ingredients) {
        for (const auto& item : ing.items) {
            recipesByInput[item].push_back(rid);
        }
    }
    // Also track pattern-based keys
    for (const auto& [ch, ing] : recipe->key) {
        for (const auto& item : ing.items) {
            recipesByInput[item].push_back(rid);
        }
    }

    recipesByType[rtype].push_back(rid);
    recipesByOutput[outputItem].push_back(rid);
    recipes[rid] = std::move(recipe);

    VF_DEBUG("Registered recipe: {}", rid);
}

void RecipeRegistry::unregisterRecipe(const std::string& id) {
    auto it = recipes.find(id);
    if (it == recipes.end()) return;

    auto& recipe = *it->second;
    RecipeType rtype = recipe.type;

    // Remove from type index
    auto& typeList = recipesByType[rtype];
    typeList.erase(std::remove(typeList.begin(), typeList.end(), id), typeList.end());

    // Remove from output index
    auto& outList = recipesByOutput[recipe.result.item];
    outList.erase(std::remove(outList.begin(), outList.end(), id), outList.end());

    recipes.erase(it);
}

void RecipeRegistry::clear() {
    recipes.clear();
    recipesByType.clear();
    recipesByOutput.clear();
    recipesByInput.clear();
}

void RecipeRegistry::clearModRecipes(const std::string& modId) {
    std::vector<std::string> toRemove;
    for (auto& [id, recipe] : recipes) {
        // Convention: mod recipes have id "modid:recipe_name"
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
            auto rit = recipes.find(id);
            if (rit != recipes.end()) {
                result.push_back(rit->second.get());
            }
        }
    }
    return result;
}

std::vector<const Recipe*> RecipeRegistry::getRecipesByCategory(RecipeCategory category) const {
    std::vector<const Recipe*> result;
    for (auto& [id, recipe] : recipes) {
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
            auto rit = recipes.find(id);
            if (rit != recipes.end()) {
                result.push_back(rit->second.get());
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
            auto rit = recipes.find(id);
            if (rit != recipes.end()) {
                result.push_back(rit->second.get());
            }
        }
    }
    return result;
}

std::vector<const Recipe*> RecipeRegistry::findMatchingRecipes(
    const std::vector<ItemStack>& inputs, RecipeType type) const {
    std::vector<const Recipe*> result;
    auto it = recipesByType.find(type);
    if (it == recipesByType.end()) return result;

    for (const auto& id : it->second) {
        auto rit = recipes.find(id);
        if (rit != recipes.end() && rit->second->matches(inputs)) {
            result.push_back(rit->second.get());
        }
    }
    return result;
}

const Recipe* RecipeRegistry::findFirstMatchingRecipe(
    const std::vector<ItemStack>& inputs, RecipeType type) const {
    auto it = recipesByType.find(type);
    if (it == recipesByType.end()) return nullptr;

    for (const auto& id : it->second) {
        auto rit = recipes.find(id);
        if (rit != recipes.end() && rit->second->matches(inputs)) {
            return rit->second.get();
        }
    }
    return nullptr;
}

bool RecipeRegistry::canCraft(const Recipe& recipe,
                               const std::vector<ItemStack>& inventory) const {
    // Build a map of available item counts
    std::unordered_map<ItemID, int> available;
    for (const auto& stack : inventory) {
        if (!stack.isEmpty()) {
            available[stack.getItem()] += stack.getCount();
        }
    }

    // Check each ingredient
    for (const auto& ing : recipe.ingredients) {
        int needed = ing.count;
        bool found = false;
        for (const auto& itemId : ing.items) {
            try {
                ItemID id = static_cast<ItemID>(std::stoi(itemId));
                auto it = available.find(id);
                if (it != available.end() && it->second >= needed) {
                    found = true;
                    break;
                }
            } catch (...) {}
        }
        if (!found) return false;
    }
    return true;
}

void RecipeRegistry::registerVanillaRecipes() {
    VF_INFO("Registering vanilla crafting recipes...");

    // --- Shaped Recipes ---

    // Oak planks from oak log
    {
        auto r = std::make_unique<ShapedRecipe>();
        r->id = "minecraft:oak_planks";
        r->category = RecipeCategory::Building;
        r->pattern = {"#"};
        r->key = {{'#', Ingredient{{"minecraft:oak_log"}, 1, false}}};
        r->result = {"minecraft:oak_planks", 4};
        r->width = 1; r->height = 1;
        r->ingredients.push_back(*r->key.begin()->second.items.begin() != "" ? r->key.begin()->second : Ingredient{});
        registerRecipe(std::move(r));
    }

    // Sticks
    {
        auto r = std::make_unique<ShapedRecipe>();
        r->id = "minecraft:sticks";
        r->category = RecipeCategory::Misc;
        r->pattern = {"#", "#"};
        r->key = {{'#', Ingredient{{"minecraft:oak_planks"}, 1, false}}};
        r->result = {"minecraft:stick", 4};
        r->width = 1; r->height = 2;
        registerRecipe(std::move(r));
    }

    // Crafting table
    {
        auto r = std::make_unique<ShapedRecipe>();
        r->id = "minecraft:crafting_table";
        r->category = RecipeCategory::Building;
        r->pattern = {"##", "##"};
        r->key = {{'#', Ingredient{{"minecraft:oak_planks"}, 1, false}}};
        r->result = {"minecraft:crafting_table", 1};
        r->width = 2; r->height = 2;
        registerRecipe(std::move(r));
    }

    // Furnace
    {
        auto r = std::make_unique<ShapedRecipe>();
        r->id = "minecraft:furnace";
        r->category = RecipeCategory::Building;
        r->pattern = {"###", "# #", "###"};
        r->key = {{'#', Ingredient{{"minecraft:cobblestone"}, 1, false}}};
        r->result = {"minecraft:furnace", 1};
        r->width = 3; r->height = 3;
        registerRecipe(std::move(r));
    }

    // Chest
    {
        auto r = std::make_unique<ShapedRecipe>();
        r->id = "minecraft:chest";
        r->category = RecipeCategory::Building;
        r->pattern = {"###", "# #", "###"};
        r->key = {{'#', Ingredient{{"minecraft:oak_planks"}, 1, false}}};
        r->result = {"minecraft:chest", 1};
        r->width = 3; r->height = 3;
        registerRecipe(std::move(r));
    }

    // Wooden pickaxe
    {
        auto r = std::make_unique<ShapedRecipe>();
        r->id = "minecraft:wooden_pickaxe";
        r->category = RecipeCategory::Equipment;
        r->pattern = {"###", " | ", " | "};
        r->key = {{'#', Ingredient{{"minecraft:oak_planks"}, 1, false}},
                  {'|', Ingredient{{"minecraft:stick"}, 1, false}}};
        r->result = {"minecraft:wooden_pickaxe", 1};
        r->width = 3; r->height = 3;
        registerRecipe(std::move(r));
    }

    // Wooden axe
    {
        auto r = std::make_unique<ShapedRecipe>();
        r->id = "minecraft:wooden_axe";
        r->category = RecipeCategory::Equipment;
        r->pattern = {"##", "#|", " |"};
        r->key = {{'#', Ingredient{{"minecraft:oak_planks"}, 1, false}},
                  {'|', Ingredient{{"minecraft:stick"}, 1, false}}};
        r->result = {"minecraft:wooden_axe", 1};
        r->width = 2; r->height = 3;
        registerRecipe(std::move(r));
    }

    // Wooden sword
    {
        auto r = std::make_unique<ShapedRecipe>();
        r->id = "minecraft:wooden_sword";
        r->category = RecipeCategory::Equipment;
        r->pattern = {"#", "#", "|"};
        r->key = {{'#', Ingredient{{"minecraft:oak_planks"}, 1, false}},
                  {'|', Ingredient{{"minecraft:stick"}, 1, false}}};
        r->result = {"minecraft:wooden_sword", 1};
        r->width = 1; r->height = 3;
        registerRecipe(std::move(r));
    }

    // Wooden shovel
    {
        auto r = std::make_unique<ShapedRecipe>();
        r->id = "minecraft:wooden_shovel";
        r->category = RecipeCategory::Equipment;
        r->pattern = {"#", "|", "|"};
        r->key = {{'#', Ingredient{{"minecraft:oak_planks"}, 1, false}},
                  {'|', Ingredient{{"minecraft:stick"}, 1, false}}};
        r->result = {"minecraft:wooden_shovel", 1};
        r->width = 1; r->height = 3;
        registerRecipe(std::move(r));
    }

    // Wooden hoe
    {
        auto r = std::make_unique<ShapedRecipe>();
        r->id = "minecraft:wooden_hoe";
        r->category = RecipeCategory::Equipment;
        r->pattern = {"##", " |", " |"};
        r->key = {{'#', Ingredient{{"minecraft:oak_planks"}, 1, false}},
                  {'|', Ingredient{{"minecraft:stick"}, 1, false}}};
        r->result = {"minecraft:wooden_hoe", 1};
        r->width = 2; r->height = 3;
        registerRecipe(std::move(r));
    }

    // Torch (coal + stick)
    {
        auto r = std::make_unique<ShapedRecipe>();
        r->id = "minecraft:torch";
        r->category = RecipeCategory::Misc;
        r->pattern = {"C", "|"};
        r->key = {{'C', Ingredient{{"minecraft:coal"}, 1, false}},
                  {'|', Ingredient{{"minecraft:stick"}, 1, false}}};
        r->result = {"minecraft:torch", 4};
        r->width = 1; r->height = 2;
        registerRecipe(std::move(r));
    }

    // --- Shapeless Recipes ---

    // Bread (3 wheat)
    {
        auto r = std::make_unique<ShapelessRecipe>();
        r->id = "minecraft:bread";
        r->category = RecipeCategory::Food;
        r->ingredients = {Ingredient{{"minecraft:wheat"}, 1, false},
                          Ingredient{{"minecraft:wheat"}, 1, false},
                          Ingredient{{"minecraft:wheat"}, 1, false}};
        r->result = {"minecraft:bread", 1};
        registerRecipe(std::move(r));
    }

    // --- Smelting Recipes ---

    // Iron ore -> Iron ingot
    {
        auto r = std::make_unique<CookingRecipe>(RecipeType::Smelting);
        r->id = "minecraft:iron_ingot_from_smelting";
        r->category = RecipeCategory::Misc;
        r->ingredients = {Ingredient{{"minecraft:iron_ore"}, 1, false}};
        r->result = {"minecraft:iron_ingot", 1};
        r->cookingTime = 10.0f;
        r->experience = 0.7f;
        registerRecipe(std::move(r));
    }

    // Gold ore -> Gold ingot
    {
        auto r = std::make_unique<CookingRecipe>(RecipeType::Smelting);
        r->id = "minecraft:gold_ingot_from_smelting";
        r->category = RecipeCategory::Misc;
        r->ingredients = {Ingredient{{"minecraft:gold_ore"}, 1, false}};
        r->result = {"minecraft:gold_ingot", 1};
        r->cookingTime = 10.0f;
        r->experience = 1.0f;
        registerRecipe(std::move(r));
    }

    // Sand -> Glass
    {
        auto r = std::make_unique<CookingRecipe>(RecipeType::Smelting);
        r->id = "minecraft:glass_from_smelting";
        r->category = RecipeCategory::Building;
        r->ingredients = {Ingredient{{"minecraft:sand"}, 1, false}};
        r->result = {"minecraft:glass", 1};
        r->cookingTime = 10.0f;
        r->experience = 0.1f;
        registerRecipe(std::move(r));
    }

    // Cobblestone -> Stone
    {
        auto r = std::make_unique<CookingRecipe>(RecipeType::Smelting);
        r->id = "minecraft:stone_from_smelting";
        r->category = RecipeCategory::Building;
        r->ingredients = {Ingredient{{"minecraft:cobblestone"}, 1, false}};
        r->result = {"minecraft:stone", 1};
        r->cookingTime = 10.0f;
        r->experience = 0.1f;
        registerRecipe(std::move(r));
    }

    // Raw beef -> Steak
    {
        auto r = std::make_unique<CookingRecipe>(RecipeType::Smelting);
        r->id = "minecraft:cooked_beef_from_smelting";
        r->category = RecipeCategory::Food;
        r->ingredients = {Ingredient{{"minecraft:raw_beef"}, 1, false}};
        r->result = {"minecraft:cooked_beef", 1};
        r->cookingTime = 10.0f;
        r->experience = 0.35f;
        registerRecipe(std::move(r));
    }

    VF_INFO("Registered {} vanilla recipes", recipes.size());
}

void RecipeRegistry::loadFromDirectory(const std::string& path) {
    // Walk directory for .json recipe files
    VF_INFO("Loading recipes from directory: {}", path);
    // This would use std::filesystem in a full implementation
    // For now, it's a placeholder for the mod loading pipeline
}

void RecipeRegistry::loadFromJson(const nlohmann::json& json) {
    if (json.is_array()) {
        for (const auto& j : json) {
            auto recipe = Recipe::fromJson(j);
            if (recipe) {
                registerRecipe(std::move(recipe));
            }
        }
    }
}

nlohmann::json RecipeRegistry::saveToJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (auto& [id, recipe] : recipes) {
        arr.push_back(recipe->toJson());
    }
    return arr;
}

size_t RecipeRegistry::getRecipeCountByType(RecipeType type) const {
    auto it = recipesByType.find(type);
    return it != recipesByType.end() ? it->second.size() : 0;
}

} // namespace VoxelForge
