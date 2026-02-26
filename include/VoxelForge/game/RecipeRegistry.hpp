/**
 * @file RecipeRegistry.hpp
 * @brief Recipe registry for all crafting and smelting recipes
 */

#pragma once

#include <VoxelForge/game/Inventory.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace VoxelForge {

// Forward declarations
class ItemStack;

// Recipe type
enum class RecipeType {
    Crafting,
    Smelting,
    Blasting,
    Smoking,
    Campfire,
    Stonecutting,
    Smithing
};

// Recipe category
enum class RecipeCategory {
    Building,
    Redstone,
    Equipment,
    Misc,
    Food
};

// Item identifier (mod:item format)
using ItemId = std::string;

// Ingredient (can be a tag or specific item)
struct Ingredient {
    std::vector<ItemId> items;  // List of possible items
    int count = 1;
    bool isTag = false;         // If true, items are tag names
    
    bool matches(const ItemId& item) const;
    bool isEmpty() const { return items.empty(); }
};

// Recipe result
struct RecipeResult {
    ItemId item;
    int count = 1;
    float chance = 1.0f;  // For random outputs
};

// Base recipe class
class Recipe {
public:
    std::string id;              // Recipe identifier
    std::string group;           // Recipe group for UI
    RecipeType type;
    RecipeCategory category = RecipeCategory::Misc;
    std::vector<Ingredient> ingredients;
    RecipeResult result;
    bool isShapeless = false;
    
    // Shaped recipe specific
    int width = 0;
    int height = 0;
    std::vector<std::string> pattern;  // Pattern for shaped recipes
    std::unordered_map<char, Ingredient> key;  // Key for pattern
    
    // Cooking recipe specific
    float cookingTime = 10.0f;   // Seconds
    float experience = 0.0f;
    
    // Check if recipe matches
    virtual bool matches(const std::vector<ItemStack>& inputs) const;
    virtual bool matchesShaped(const std::vector<ItemStack>& inputs, int gridWidth, int gridHeight) const;
    virtual bool matchesShapeless(const std::vector<ItemStack>& inputs) const;
    
    // Get result
    virtual ItemStack getResult() const;
    virtual std::vector<ItemStack> getRemainingItems(const std::vector<ItemStack>& inputs) const;
    
    // Serialization
    nlohmann::json toJson() const;
    static std::unique_ptr<Recipe> fromJson(const nlohmann::json& json);
};

// Shaped recipe
class ShapedRecipe : public Recipe {
public:
    ShapedRecipe() { isShapeless = false; }
    
    bool matches(const std::vector<ItemStack>& inputs) const override;
    bool matchesShaped(const std::vector<ItemStack>& inputs, int gridWidth, int gridHeight) const override;
};

// Shapeless recipe
class ShapelessRecipe : public Recipe {
public:
    ShapelessRecipe() { isShapeless = true; }
    
    bool matches(const std::vector<ItemStack>& inputs) const override;
};

// Cooking recipe
class CookingRecipe : public Recipe {
public:
    CookingRecipe(RecipeType cookType = RecipeType::Smelting) { 
        type = cookType; 
    }
    
    bool matches(const std::vector<ItemStack>& inputs) const override;
};

// Stonecutting recipe
class StonecuttingRecipe : public Recipe {
public:
    StonecuttingRecipe() { type = RecipeType::Stonecutting; }
    
    bool matches(const std::vector<ItemStack>& inputs) const override;
};

// Recipe registry
class RecipeRegistry {
public:
    static RecipeRegistry& getInstance();
    
    // Registration
    void registerRecipe(std::unique_ptr<Recipe> recipe);
    void unregisterRecipe(const std::string& id);
    void clear();
    void clearModRecipes(const std::string& modId);
    
    // Query
    const Recipe* getRecipe(const std::string& id) const;
    std::vector<const Recipe*> getRecipesByType(RecipeType type) const;
    std::vector<const Recipe*> getRecipesByCategory(RecipeCategory category) const;
    std::vector<const Recipe*> getRecipesForItem(const ItemId& item) const;
    std::vector<const Recipe*> getRecipesUsingItem(const ItemId& item) const;
    
    // Matching
    std::vector<const Recipe*> findMatchingRecipes(const std::vector<ItemStack>& inputs, 
                                                    RecipeType type = RecipeType::Crafting) const;
    const Recipe* findFirstMatchingRecipe(const std::vector<ItemStack>& inputs,
                                           RecipeType type = RecipeType::Crafting) const;
    
    // Check if player can craft
    bool canCraft(const Recipe& recipe, const std::vector<ItemStack>& inventory) const;
    
    // Vanilla recipes
    void registerVanillaRecipes();
    
    // Serialization
    void loadFromDirectory(const std::string& path);
    void loadFromJson(const nlohmann::json& json);
    nlohmann::json saveToJson() const;
    
    // Stats
    size_t getRecipeCount() const { return recipes.size(); }
    size_t getRecipeCountByType(RecipeType type) const;
    
private:
    RecipeRegistry();
    ~RecipeRegistry() = default;
    
    std::unordered_map<std::string, std::unique_ptr<Recipe>> recipes;
    std::unordered_map<RecipeType, std::vector<std::string>> recipesByType;
    std::unordered_map<ItemId, std::vector<std::string>> recipesByOutput;
    std::unordered_map<ItemId, std::vector<std::string>> recipesByInput;
};

} // namespace VoxelForge
