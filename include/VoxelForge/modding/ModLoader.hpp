#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <filesystem>
#include <cstdint>

#include <sol/sol.hpp>

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

class LuaEngine;
class World;
class EntityManager;

struct ModConfig {
    std::filesystem::path modsDirectory = "mods";
    std::filesystem::path dataDirectory = "data";
    std::filesystem::path configDirectory = "config";
    bool enableNativeMods = false;
    bool enableLuaMods = true;
    bool enableDataPacks = true;
    bool enableHotReload = false;
};

struct ModInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::vector<std::string> authors;
    std::string website;
    std::string license;
    std::string mainClass;
    std::string mainScript;
    std::string entryPoint;
    std::string iconPath;
    std::vector<std::string> dependencies;
    std::vector<std::string> softDependencies;
    std::vector<std::string> incompatibilities;
    int loadPriority = 0;
    bool enabled = true;
    bool isNative = false;
    std::filesystem::path path;
};

struct ModLoadResult {
    bool success = false;
    std::string error;
    std::string modId;
    ModInfo info;
};

enum class ModState { Loading, Loaded, Enabled, Disabled, Error };
enum class ModType { Lua, Native, DataPack };
enum class ModEvent { ModLoaded, ModUnloaded, ModEnabled, ModDisabled, ModError };

struct ModEventData {
    ModEvent event;
    std::string modId;
    std::string message;
};

using ModEventCallback = std::function<void(const ModEventData&)>;

class IMod {
public:
    virtual ~IMod() = default;
    virtual bool onLoad() = 0;
    virtual void onUnload() = 0;
    virtual void onEnable() = 0;
    virtual void onDisable() = 0;
};

using CreateModFunc = IMod*(*)();
using DestroyModFunc = void(*)(IMod*);

class ModLoader;
class ModContext;

struct LoadedMod {
    ModInfo info;
    ModState state = ModState::Disabled;
    ModType type = ModType::Lua;
    ModHandle handle = nullptr;
    IMod* modInstance = nullptr;
    ModContext* context = nullptr;
    std::string scriptPath;
    std::string lastError;
    CreateModFunc createFunc = nullptr;
    DestroyModFunc destroyFunc = nullptr;
};

class ModContext {
public:
    ModContext(const std::string& modId, ModLoader* loader);
    ~ModContext() = default;

    void logInfo(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);

    std::filesystem::path getModPath() const;
    std::filesystem::path getConfigPath() const;
    std::filesystem::path getDataPath() const;
    bool hasConfig() const;

    sol::table getConfig(sol::this_state s);
    void saveConfig(sol::table config);

    std::vector<uint8_t> loadResource(const std::string& path);
    bool resourceExists(const std::string& path) const;

    void registerBlock(const std::string& id, sol::table properties);
    void registerItem(const std::string& id, sol::table properties);
    void registerRecipe(sol::table recipe);
    void registerEntity(const std::string& id, sol::table properties);
    void registerBiome(const std::string& id, sol::table properties);
    void registerDimension(const std::string& id, sol::table properties);
    void registerCommand(const std::string& name, sol::function callback);

    void subscribeEvent(const std::string& event, sol::function callback);

    LuaEngine* getLuaEngine();
    World* getWorld();
    EntityManager* getEntityManager();

    std::string propertiesToScript(sol::table table);

private:
    std::string modId;
    ModLoader* loader;
};

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
    void clearModContent(const std::string& modId);
    void clear();

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
    std::vector<sol::table> recipes;
    uint32_t nextBlockId = 1000;
    uint32_t nextItemId = 1000;
};

class ModLoader {
public:
    static ModLoader& get();

    void init(LuaEngine* luaEngine, const ModConfig& config);
    void shutdown();

    std::vector<ModInfo> discoverMods();
    std::vector<ModInfo> discoverModsInDirectory(const std::filesystem::path& directory);

    ModLoadResult loadMod(const std::filesystem::path& path);
    ModLoadResult loadMod(const ModInfo& info);
    void unloadMod(const std::string& modId);
    void reloadMod(const std::string& modId);

    void loadAllMods();
    void unloadAllMods();
    void reloadAllMods();

    void enableMod(const std::string& modId);
    void disableMod(const std::string& modId);

    bool isModEnabled(const std::string& modId) const;
    bool isModLoaded(const std::string& modId) const;

    bool checkDependencies(const ModInfo& info) const;

    std::vector<std::string> getDependencyOrder();
    std::vector<std::string> getLoadOrder();

    LoadedMod* getMod(const std::string& modId);
    const LoadedMod* getMod(const std::string& modId) const;

    std::vector<LoadedMod*> getAllMods();
    std::vector<const LoadedMod*> getAllMods() const;

    std::vector<std::string> getLoadedModIds() const;

    ModContext* getModContext(const std::string& modId);

    void setEventCallback(ModEventCallback callback);
    void update();

    const ModConfig& getConfig() const { return config; }
    LuaEngine* getLuaEngine() const { return luaEngine; }

private:
    ModLoader() = default;
    ~ModLoader();

    ModInfo parseModInfo(const std::filesystem::path& modPath);
    ModInfo parseModJson(const std::filesystem::path& jsonPath);
    ModInfo parseModToml(const std::filesystem::path& tomlPath);

    ModLoadResult loadNativeMod(const ModInfo& info);
    void unloadNativeMod(LoadedMod& mod);

    ModLoadResult loadLuaMod(const ModInfo& info);
    void unloadLuaMod(LoadedMod& mod);

    ModLoadResult loadDataPack(const ModInfo& info);

    ModHandle loadLibrary(const std::filesystem::path& path);
    void unloadLibrary(ModHandle handle);
    void* getSymbol(ModHandle handle, const std::string& name);

    void sortModsByDependency();
    void triggerEvent(ModEvent event, const std::string& modId, const std::string& message = "");

    LuaEngine* luaEngine = nullptr;
    ModConfig config;
    bool initialized = false;

    std::unordered_map<std::string, std::unique_ptr<LoadedMod>> mods;
    std::vector<std::string> loadOrder;
    ModEventCallback eventCallback;
};

class ModLibrary {
public:
    static ModHandle load(const std::filesystem::path& path);
    static void unload(ModHandle handle);
    static void* getSymbol(ModHandle handle, const std::string& symbolName);
    static std::string getError();
};

} // namespace VoxelForge
