/**
 * @file ModContext.cpp
 * @brief Mod context implementation
 */

#include <VoxelForge/modding/ModLoader.hpp>
#include <VoxelForge/scripting/LuaEngine.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <filesystem>
#include <fstream>

namespace VoxelForge {

ModContext::ModContext(const std::string& modId, ModLoader* loader)
    : modId(modId), loader(loader) {}

void ModContext::logInfo(const std::string& message) {
    LOG_INFO("[Mod:{}] {}", modId, message);
}

void ModContext::logWarning(const std::string& message) {
    LOG_WARN("[Mod:{}] {}", modId, message);
}

void ModContext::logError(const std::string& message) {
    LOG_ERROR("[Mod:{}] {}", modId, message);
}

std::filesystem::path ModContext::getModPath() const {
    auto* mod = loader ? loader->getMod(modId) : nullptr;
    if (mod) {
        return mod->info.path;
    }
    return {};
}

std::filesystem::path ModContext::getConfigPath() const {
    return loader ? loader->getConfig().configDirectory / (modId + ".json") : std::filesystem::path();
}

std::filesystem::path ModContext::getDataPath() const {
    return loader ? loader->getConfig().dataDirectory / modId : std::filesystem::path();
}

bool ModContext::hasConfig() const {
    return std::filesystem::exists(getConfigPath());
}

sol::table ModContext::getConfig(sol::this_state s) {
    sol::state_view lua(s);
    
    auto path = getConfigPath();
    if (!std::filesystem::exists(path)) {
        return lua.create_table();
    }
    
    try {
        std::ifstream file(path);
        nlohmann::json j;
        file >> j;
        
        // Convert JSON to Lua table
        // TODO: Implement JSON to Lua table conversion
        return lua.create_table();
    } catch (const std::exception& e) {
        logError(std::string("Failed to load config: ") + e.what());
        return lua.create_table();
    }
}

void ModContext::saveConfig(sol::table config) {
    auto path = getConfigPath();
    
    // Create parent directory if needed
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    
    // TODO: Convert Lua table to JSON and save
}

std::vector<uint8_t> ModContext::loadResource(const std::string& resourcePath) {
    auto fullPath = getModPath() / resourcePath;
    
    if (!std::filesystem::exists(fullPath)) {
        logWarning("Resource not found: " + resourcePath);
        return {};
    }
    
    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        logError("Failed to open resource: " + resourcePath);
        return {};
    }
    
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file),
                                std::istreambuf_iterator<char>());
}

bool ModContext::resourceExists(const std::string& resourcePath) const {
    return std::filesystem::exists(getModPath() / resourcePath);
}

void ModContext::registerBlock(const std::string& id, sol::table properties) {
    if (loader) {
        loader->getLuaEngine()->executeScript(
            "voxelforge.blocks.register('" + modId + ":" + id + "', " +
            propertiesToScript(properties) + ")"
        );
    }
    logInfo("Registered block: " + id);
}

void ModContext::registerItem(const std::string& id, sol::table properties) {
    if (loader) {
        loader->getLuaEngine()->executeScript(
            "voxelforge.items.register('" + modId + ":" + id + "', " +
            propertiesToScript(properties) + ")"
        );
    }
    logInfo("Registered item: " + id);
}

void ModContext::registerRecipe(sol::table recipe) {
    if (loader) {
        loader->getLuaEngine()->executeScript(
            "voxelforge.recipes.register(" + propertiesToScript(recipe) + ")"
        );
    }
    logInfo("Registered recipe");
}

void ModContext::registerEntity(const std::string& id, sol::table properties) {
    if (loader) {
        loader->getLuaEngine()->executeScript(
            "voxelforge.entities.register('" + modId + ":" + id + "', " +
            propertiesToScript(properties) + ")"
        );
    }
    logInfo("Registered entity: " + id);
}

void ModContext::registerBiome(const std::string& id, sol::table properties) {
    if (loader) {
        loader->getLuaEngine()->executeScript(
            "voxelforge.biomes.register('" + modId + ":" + id + "', " +
            propertiesToScript(properties) + ")"
        );
    }
    logInfo("Registered biome: " + id);
}

void ModContext::registerDimension(const std::string& id, sol::table properties) {
    if (loader) {
        loader->getLuaEngine()->executeScript(
            "voxelforge.dimensions.register('" + modId + ":" + id + "', " +
            propertiesToScript(properties) + ")"
        );
    }
    logInfo("Registered dimension: " + id);
}

void ModContext::registerCommand(const std::string& name, sol::function callback) {
    // Store callback for later execution
    logInfo("Registered command: " + name);
}

void ModContext::subscribeEvent(const std::string& event, sol::function callback) {
    if (loader && loader->getLuaEngine()) {
        // Register event handler
        logInfo("Subscribed to event: " + event);
    }
}

LuaEngine* ModContext::getLuaEngine() {
    return loader ? loader->getLuaEngine() : nullptr;
}

World* ModContext::getWorld() {
    // TODO: Get world from game instance
    return nullptr;
}

EntityManager* ModContext::getEntityManager() {
    // TODO: Get entity manager from game instance
    return nullptr;
}

std::string ModContext::propertiesToScript(sol::table table) {
    // Simple conversion for basic types
    std::string result = "{";
    bool first = true;
    
    for (auto& [key, value] : table) {
        if (!first) result += ",";
        first = false;
        
        // Key
        if (key.is<std::string>()) {
            result += key.as<std::string>() + "=";
        }
        
        // Value
        if (value.is<bool>()) {
            result += value.as<bool>() ? "true" : "false";
        } else if (value.is<int>()) {
            result += std::to_string(value.as<int>());
        } else if (value.is<double>()) {
            result += std::to_string(value.as<double>());
        } else if (value.is<std::string>()) {
            result += "'" + value.as<std::string>() + "'";
        } else if (value.is<sol::table>()) {
            result += propertiesToScript(value.as<sol::table>());
        }
    }
    
    result += "}";
    return result;
}

} // namespace VoxelForge
