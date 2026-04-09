/**
 * @file ModLoader.hpp
 * @brief Mod loading and management system
 * 
 * NOTE: sol2 integration temporarily stubbed pending Lua library fixes.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <filesystem>
#include <any>

// Platform-specific dynamic library handling
#ifdef _WIN32
    #define MOD_EXPORT __declspec(dllexport)
    #include <windows.h>
    using ModHandle = HMODULE;
#else
    #define MOD_EXPORT __attribute__((visibility("default")))
    #include <dlfcn.h>
    using ModHandle = void*;
#endif

namespace VoxelForge {

// Forward declarations
class LuaEngine;
class World;
class EntityManager;

// Stub types for Lua integration (will be replaced with sol2 types when fixed)
using LuaTable = std::any;
using LuaFunction = std::function<void()>;

// Mod metadata
struct ModInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::vector<std::string> dependencies;
    std::string mainScript;
    std::string iconPath;
};

// Mod loading result
struct ModLoadResult {
    bool success = false;
    std::string error;
    ModInfo info;
};

// Mod context for API access
class ModContext {
public:
    explicit ModContext(const std::string& modId, class ModLoader* loader);
    ~ModContext() = default;
    
    // Resource access
    std::string getResourcePath(const std::string& relativePath) const;
    std::vector<uint8_t> readResource(const std::string& relativePath) const;
    bool resourceExists(const std::string& path) const;
    
    // Registration (stubbed)
    void registerBlock(const std::string& id, LuaTable properties);
    void registerItem(const std::string& id, LuaTable properties);
    void registerRecipe(LuaTable recipe);
    void registerEntity(const std::string& id, LuaTable properties);
    void registerBiome(const std::string& id, LuaTable properties);
    void registerDimension(const std::string& id, LuaTable properties);
    void registerCommand(const std::string& name, LuaFunction callback);
    
    // Event subscription (stubbed)
    void subscribeEvent(const std::string& event, LuaFunction callback);
    
    // API access
    LuaEngine* getLuaEngine();
    World* getWorld();
    EntityManager* getEntityManager();
    
private:
    std::string modId;
    ModLoader* loader;
};

// Mod registry for tracking registered content
class ModRegistry {
public:
    struct RegisteredBlock {
        std::string modId;
        std::string blockId;
        uint32_t numericId;
    };
    
    struct RegisteredItem {
        std::string modId;
        std::string itemId;
        uint32_t numericId;
    };
    
    struct RegisteredEntity {
        std::string modId;
        std::string entityId;
    };
    
    void registerBlock(const std::string& modId, const std::string& blockId);
    void registerItem(const std::string& modId, const std::string& itemId);
    void registerEntity(const std::string& modId, const std::string& entityId);
    
    const RegisteredBlock* getBlock(const std::string& fullId) const;
    const RegisteredItem* getItem(const std::string& fullId) const;
    const RegisteredEntity* getEntity(const std::string& fullId) const;
    
    std::vector<std::string> getBlocksByMod(const std::string& modId) const;
    std::vector<std::string> getItemsByMod(const std::string& modId) const;
    std::vector<std::string> getEntitiesByMod(const std::string& modId) const;
    
    static ModRegistry& get();
    
private:
    std::unordered_map<std::string, RegisteredBlock> blocks;
    std::unordered_map<std::string, RegisteredItem> items;
    std::unordered_map<std::string, RegisteredEntity> entities;
};

// Mod loader
class ModLoader {
public:
    static ModLoader& get();
    
    // Mod discovery
    std::vector<ModInfo> discoverMods(const std::filesystem::path& modsDir);
    
    // Mod loading
    ModLoadResult loadMod(const std::filesystem::path& modPath);
    bool unloadMod(const std::string& modId);
    bool reloadMod(const std::string& modId);
    
    // Mod access
    ModContext* getModContext(const std::string& modId);
    bool isModLoaded(const std::string& modId) const;
    std::vector<std::string> getLoadedMods() const;
    
    // Lifecycle
    bool initialize();
    void shutdown();
    
    // Event callbacks
    void onWorldCreated(World* world);
    void onWorldDestroyed(World* world);
    
private:
    ModLoader() = default;
    ~ModLoader() = default;
    
    std::unordered_map<std::string, std::unique_ptr<ModContext>> contexts;
    std::unordered_map<std::string, ModHandle> handles;
    std::unordered_map<std::string, ModInfo> modInfos;
};

// Dynamic library loading utilities
class ModLibrary {
public:
    static ModHandle load(const std::filesystem::path& path);
    static void unload(ModHandle handle);
    static void* getSymbol(ModHandle handle, const std::string& symbolName);
    static std::string getError();
};

} // namespace VoxelForge