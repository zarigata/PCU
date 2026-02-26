/**
 * @file ModLoader.hpp
 * @brief Mod loading and management system
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <filesystem>

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

// Mod info structure
struct ModInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::vector<std::string> authors;
    std::string website;
    std::string license;
    std::vector<std::string> dependencies;
    std::vector<std::string> softDependencies;
    std::vector<std::string> incompatibilities;
    std::string mainClass;      // For native mods
    std::string mainScript;     // For Lua mods
    std::string entryPoint;     // Function to call
    std::filesystem::path path;
    bool enabled = true;
    bool isNative = false;
    int loadPriority = 0;
};

// Mod state
enum class ModState {
    Unloaded,
    Loading,
    Loaded,
    Enabled,
    Disabled,
    Error
};

// Mod type
enum class ModType {
    Native,     // C++ plugin
    Lua,        // Lua script mod
    Hybrid,     // Both native and Lua
    DataPack    // Data only (blocks, items, recipes)
};

// Mod loading result
struct ModLoadResult {
    bool success = false;
    std::string error;
    std::string modId;
};

// Mod interface for native plugins
class IMod {
public:
    virtual ~IMod() = default;
    
    // Lifecycle
    virtual bool onLoad() = 0;
    virtual void onUnload() = 0;
    virtual void onEnable() = 0;
    virtual void onDisable() = 0;
    
    // Info
    virtual const char* getId() const = 0;
    virtual const char* getName() const = 0;
    virtual const char* getVersion() const = 0;
    
    // Tick
    virtual void onTick(float deltaTime) {}
    
    // Events
    virtual void onServerStart() {}
    virtual void onServerStop() {}
    virtual void onPlayerJoin(uint32_t playerId) {}
    virtual void onPlayerLeave(uint32_t playerId) {}
    virtual void onBlockPlace(int x, int y, int z, uint32_t blockId, uint32_t playerId) {}
    virtual void onBlockBreak(int x, int y, int z, uint32_t blockId, uint32_t playerId) {}
    virtual void onEntitySpawn(uint32_t entityId) {}
    virtual void onEntityDeath(uint32_t entityId) {}
};

// Function pointer types for mod creation
using CreateModFunc = IMod*(*)();
using DestroyModFunc = void(*)(IMod*);

// Loaded mod data
struct LoadedMod {
    ModInfo info;
    ModType type = ModType::Lua;
    ModState state = ModState::Unloaded;
    
    // Native mod
    ModHandle handle = nullptr;
    IMod* modInstance = nullptr;
    CreateModFunc createFunc = nullptr;
    DestroyModFunc destroyFunc = nullptr;
    
    // Lua mod
    std::string scriptPath;
    
    // Context
    class ModContext* context = nullptr;
    
    // Error info
    std::string lastError;
};

// Mod configuration
struct ModConfig {
    std::string modsDirectory = "mods";
    std::string dataDirectory = "data";
    std::string configDirectory = "config";
    bool enableHotReload = false;
    bool enableNativeMods = true;
    bool enableLuaMods = true;
    bool enableDataPacks = true;
    int maxLoadTimeMs = 5000;
    bool verifySignatures = false;
};

// Mod event types
enum class ModEvent {
    ModLoaded,
    ModUnloaded,
    ModEnabled,
    ModDisabled,
    ModError
};

// Mod event data
struct ModEventData {
    ModEvent event;
    std::string modId;
    std::string message;
};

// Mod loader class
class ModLoader {
public:
    ModLoader();
    ~ModLoader();
    
    // No copy
    ModLoader(const ModLoader&) = delete;
    ModLoader& operator=(const ModLoader&) = delete;
    
    void init(LuaEngine* luaEngine, const ModConfig& config = {});
    void shutdown();
    
    // Mod discovery
    std::vector<ModInfo> discoverMods();
    std::vector<ModInfo> discoverModsInDirectory(const std::filesystem::path& directory);
    
    // Mod loading
    ModLoadResult loadMod(const std::filesystem::path& path);
    ModLoadResult loadMod(const ModInfo& info);
    void unloadMod(const std::string& modId);
    void reloadMod(const std::string& modId);
    
    // Batch operations
    void loadAllMods();
    void unloadAllMods();
    void reloadAllMods();
    
    // Enable/disable
    void enableMod(const std::string& modId);
    void disableMod(const std::string& modId);
    bool isModEnabled(const std::string& modId) const;
    bool isModLoaded(const std::string& modId) const;
    
    // Dependency management
    bool checkDependencies(const ModInfo& info) const;
    std::vector<std::string> getDependencyOrder();
    std::vector<std::string> getLoadOrder();
    
    // Mod access
    LoadedMod* getMod(const std::string& modId);
    const LoadedMod* getMod(const std::string& modId) const;
    std::vector<LoadedMod*> getAllMods();
    std::vector<const LoadedMod*> getAllMods() const;
    std::vector<std::string> getLoadedModIds() const;
    
    // Mod info parsing
    ModInfo parseModInfo(const std::filesystem::path& modPath);
    ModInfo parseModJson(const std::filesystem::path& jsonPath);
    ModInfo parseModToml(const std::filesystem::path& tomlPath);
    
    // Events
    using ModEventCallback = std::function<void(const ModEventData&)>;
    void setEventCallback(ModEventCallback callback);
    
    // Update (for hot reload)
    void update();
    
    // Getters
    LuaEngine* getLuaEngine() const { return luaEngine; }
    const ModConfig& getConfig() const { return config; }
    size_t getLoadedCount() const { return mods.size(); }
    
private:
    // Native mod loading
    ModLoadResult loadNativeMod(const ModInfo& info);
    void unloadNativeMod(LoadedMod& mod);
    ModHandle loadLibrary(const std::filesystem::path& path);
    void unloadLibrary(ModHandle handle);
    void* getSymbol(ModHandle handle, const std::string& name);
    
    // Lua mod loading
    ModLoadResult loadLuaMod(const ModInfo& info);
    void unloadLuaMod(LoadedMod& mod);
    
    // Data pack loading
    ModLoadResult loadDataPack(const ModInfo& info);
    
    // Utility
    void sortModsByDependency();
    void triggerEvent(ModEvent event, const std::string& modId, const std::string& message = "");
    
    LuaEngine* luaEngine = nullptr;
    ModConfig config;
    
    std::unordered_map<std::string, std::unique_ptr<LoadedMod>> mods;
    std::vector<std::string> loadOrder;
    
    ModEventCallback eventCallback;
    bool initialized = false;
};

// Mod context for API access
class ModContext {
public:
    ModContext(const std::string& modId, ModLoader* loader);
    ~ModContext() = default;
    
    // Logging
    void logInfo(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);
    
    // Mod info
    const std::string& getModId() const { return modId; }
    std::filesystem::path getModPath() const;
    std::filesystem::path getConfigPath() const;
    std::filesystem::path getDataPath() const;
    
    // Configuration
    bool hasConfig() const;
    sol::table getConfig(sol::this_state s);
    void saveConfig(sol::table config);
    
    // Resource loading
    std::vector<uint8_t> loadResource(const std::string& path);
    bool resourceExists(const std::string& path) const;
    
    // Registration
    void registerBlock(const std::string& id, sol::table properties);
    void registerItem(const std::string& id, sol::table properties);
    void registerRecipe(sol::table recipe);
    void registerEntity(const std::string& id, sol::table properties);
    void registerBiome(const std::string& id, sol::table properties);
    void registerDimension(const std::string& id, sol::table properties);
    void registerCommand(const std::string& name, sol::function callback);
    
    // Event subscription
    void subscribeEvent(const std::string& event, sol::function callback);
    
    // API access
    LuaEngine* getLuaEngine();
    class World* getWorld();
    class EntityManager* getEntityManager();
    
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
        sol::table properties;
    };
    
    struct RegisteredItem {
        std::string modId;
        std::string itemId;
        uint32_t numericId;
        sol::table properties;
    };
    
    struct RegisteredEntity {
        std::string modId;
        std::string entityId;
        sol::table properties;
    };
    
    void registerBlock(const std::string& modId, const std::string& blockId, sol::table properties);
    void registerItem(const std::string& modId, const std::string& itemId, sol::table properties);
    void registerEntity(const std::string& modId, const std::string& entityId, sol::table properties);
    void registerRecipe(const std::string& modId, sol::table recipe);
    
    const RegisteredBlock* getBlock(const std::string& fullId) const;
    const RegisteredItem* getItem(const std::string& fullId) const;
    const RegisteredEntity* getEntity(const std::string& fullId) const;
    
    std::vector<std::string> getBlocksByMod(const std::string& modId) const;
    std::vector<std::string> getItemsByMod(const std::string& modId) const;
    std::vector<std::string> getEntitiesByMod(const std::string& modId) const;
    
    void clearModContent(const std::string& modId);
    void clear();
    
    size_t getBlockCount() const { return blocks.size(); }
    size_t getItemCount() const { return items.size(); }
    size_t getEntityCount() const { return entities.size(); }
    
private:
    std::unordered_map<std::string, RegisteredBlock> blocks;
    std::unordered_map<std::string, RegisteredItem> items;
    std::unordered_map<std::string, RegisteredEntity> entities;
    std::vector<sol::table> recipes;
    
    uint32_t nextBlockId = 1000;  // Start after vanilla blocks
    uint32_t nextItemId = 1000;
};

} // namespace VoxelForge
