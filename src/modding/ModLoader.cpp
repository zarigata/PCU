/**
 * @file ModLoader.cpp
 * @brief Mod loading and management implementation
 */

#include "ModLoader.hpp"
#include <VoxelForge/scripting/LuaEngine.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>

// Include JSON parsing
#include <nlohmann/json.hpp>

namespace VoxelForge {

// ============== ModLoader ==============

ModLoader::ModLoader() = default;

ModLoader::~ModLoader() {
    shutdown();
}

void ModLoader::init(LuaEngine* luaEngine, const ModConfig& config) {
    this->luaEngine = luaEngine;
    this->config = config;
    
    // Create directories if they don't exist
    std::filesystem::create_directories(config.modsDirectory);
    std::filesystem::create_directories(config.dataDirectory);
    std::filesystem::create_directories(config.configDirectory);
    
    initialized = true;
    Logger::info("ModLoader initialized (mods directory: {})", config.modsDirectory);
}

void ModLoader::shutdown() {
    if (!initialized) return;
    
    unloadAllMods();
    mods.clear();
    loadOrder.clear();
    
    initialized = false;
    Logger::info("ModLoader shutdown");
}

std::vector<ModInfo> ModLoader::discoverMods() {
    return discoverModsInDirectory(config.modsDirectory);
}

std::vector<ModInfo> ModLoader::discoverModsInDirectory(const std::filesystem::path& directory) {
    std::vector<ModInfo> discoveredMods;
    
    if (!std::filesystem::exists(directory)) {
        return discoveredMods;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_directory()) {
            // Check for mod.json or mod.toml
            auto modPath = entry.path();
            ModInfo info = parseModInfo(modPath);
            
            if (!info.id.empty()) {
                discoveredMods.push_back(info);
            }
        } else if (entry.is_regular_file()) {
            auto path = entry.path();
            std::string ext = path.extension().string();
            
#ifdef _WIN32
            if (ext == ".dll") {
#else
            if (ext == ".so" || ext == ".dylib") {
#endif
                // Native mod as single file
                ModInfo info = parseModInfo(path);
                if (!info.id.empty()) {
                    info.isNative = true;
                    discoveredMods.push_back(info);
                }
            }
        }
    }
    
    return discoveredMods;
}

ModLoadResult ModLoader::loadMod(const std::filesystem::path& path) {
    ModInfo info = parseModInfo(path);
    if (info.id.empty()) {
        ModLoadResult result;
        result.error = "Failed to parse mod info from: " + path.string();
        return result;
    }
    
    return loadMod(info);
}

ModLoadResult ModLoader::loadMod(const ModInfo& info) {
    ModLoadResult result;
    result.modId = info.id;
    
    if (!initialized) {
        result.error = "ModLoader not initialized";
        return result;
    }
    
    // Check if already loaded
    if (isModLoaded(info.id)) {
        result.error = "Mod already loaded: " + info.id;
        return result;
    }
    
    // Check dependencies
    if (!checkDependencies(info)) {
        result.error = "Missing dependencies for: " + info.id;
        return result;
    }
    
    // Create loaded mod
    auto mod = std::make_unique<LoadedMod>();
    mod->info = info;
    mod->state = ModState::Loading;
    
    // Determine mod type
    if (info.isNative || !info.mainClass.empty()) {
        mod->type = ModType::Native;
        result = loadNativeMod(info);
    } else if (!info.mainScript.empty() || !info.entryPoint.empty()) {
        mod->type = ModType::Lua;
        result = loadLuaMod(info);
    } else {
        mod->type = ModType::DataPack;
        result = loadDataPack(info);
    }
    
    if (result.success) {
        mod->state = info.enabled ? ModState::Enabled : ModState::Disabled;
        mods[info.id] = std::move(mod);
        loadOrder.push_back(info.id);
        sortModsByDependency();
        
        triggerEvent(ModEvent::ModLoaded, info.id);
        Logger::info("Loaded mod: {} v{}", info.id, info.version);
    } else {
        mod->state = ModState::Error;
        mod->lastError = result.error;
        triggerEvent(ModEvent::ModError, info.id, result.error);
    }
    
    return result;
}

ModLoadResult ModLoader::loadNativeMod(const ModInfo& info) {
    ModLoadResult result;
    result.modId = info.id;
    
    if (!config.enableNativeMods) {
        result.error = "Native mods are disabled";
        return result;
    }
    
    // Find the library file
    std::filesystem::path libPath = info.path;
    
#ifdef _WIN32
    libPath /= info.id + ".dll";
#else
#ifdef __APPLE__
    libPath /= "lib" + info.id + ".dylib";
#else
    libPath /= "lib" + info.id + ".so";
#endif
#endif
    
    if (!std::filesystem::exists(libPath)) {
        result.error = "Native library not found: " + libPath.string();
        return result;
    }
    
    // Load the library
    ModHandle handle = loadLibrary(libPath);
    if (!handle) {
        result.error = "Failed to load library: " + libPath.string();
        return result;
    }
    
    // Get create function
    auto createFunc = reinterpret_cast<CreateModFunc>(
        getSymbol(handle, "createMod"));
    
    if (!createFunc) {
        unloadLibrary(handle);
        result.error = "Mod entry point 'createMod' not found";
        return result;
    }
    
    // Get destroy function
    auto destroyFunc = reinterpret_cast<DestroyModFunc>(
        getSymbol(handle, "destroyMod"));
    
    // Create mod instance
    IMod* modInstance = createFunc();
    if (!modInstance) {
        unloadLibrary(handle);
        result.error = "Failed to create mod instance";
        return result;
    }
    
    // Store in loaded mod
    auto& mod = mods[info.id];
    if (!mod) {
        mod = std::make_unique<LoadedMod>();
    }
    mod->handle = handle;
    mod->modInstance = modInstance;
    mod->createFunc = createFunc;
    mod->destroyFunc = destroyFunc;
    
    // Call onLoad
    try {
        if (!modInstance->onLoad()) {
            if (destroyFunc) {
                destroyFunc(modInstance);
            }
            unloadLibrary(handle);
            mod->handle = nullptr;
            mod->modInstance = nullptr;
            result.error = "Mod onLoad() returned false";
            return result;
        }
    } catch (const std::exception& e) {
        result.error = std::string("Exception in onLoad: ") + e.what();
        if (destroyFunc) {
            destroyFunc(modInstance);
        }
        unloadLibrary(handle);
        mod->handle = nullptr;
        mod->modInstance = nullptr;
        return result;
    }
    
    result.success = true;
    return result;
}

void ModLoader::unloadNativeMod(LoadedMod& mod) {
    if (mod.modInstance) {
        try {
            mod.modInstance->onUnload();
        } catch (const std::exception& e) {
            Logger::error("Exception in mod onUnload: {}", e.what());
        }
        
        if (mod.destroyFunc) {
            mod.destroyFunc(mod.modInstance);
        }
        mod.modInstance = nullptr;
    }
    
    if (mod.handle) {
        unloadLibrary(mod.handle);
        mod.handle = nullptr;
    }
}

ModHandle ModLoader::loadLibrary(const std::filesystem::path& path) {
#ifdef _WIN32
    return LoadLibraryA(path.string().c_str());
#else
    return dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
}

void ModLoader::unloadLibrary(ModHandle handle) {
    if (!handle) return;
    
#ifdef _WIN32
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif
}

void* ModLoader::getSymbol(ModHandle handle, const std::string& name) {
    if (!handle) return nullptr;
    
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(handle, name.c_str()));
#else
    return dlsym(handle, name.c_str());
#endif
}

ModLoadResult ModLoader::loadLuaMod(const ModInfo& info) {
    ModLoadResult result;
    result.modId = info.id;
    
    if (!config.enableLuaMods) {
        result.error = "Lua mods are disabled";
        return result;
    }
    
    if (!luaEngine) {
        result.error = "LuaEngine not available";
        return result;
    }
    
    // Find main script
    std::filesystem::path scriptPath = info.path;
    if (!info.mainScript.empty()) {
        scriptPath /= info.mainScript;
    } else {
        scriptPath /= "main.lua";
    }
    
    if (!std::filesystem::exists(scriptPath)) {
        scriptPath = info.path / "init.lua";
    }
    
    if (!std::filesystem::exists(scriptPath)) {
        result.error = "Main script not found for mod: " + info.id;
        return result;
    }
    
    // Read script content
    std::ifstream file(scriptPath);
    if (!file.is_open()) {
        result.error = "Failed to open script: " + scriptPath.string();
        return result;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    // Create mod environment
    sol::environment env = luaEngine->createEnvironment();
    
    // Add mod context
    auto& mod = mods[info.id];
    if (!mod) {
        mod = std::make_unique<LoadedMod>();
    }
    mod->scriptPath = scriptPath.string();
    mod->context = new ModContext(info.id, this);
    
    // Set up mod API in environment
    env["MOD_ID"] = info.id;
    env["MOD_NAME"] = info.name;
    env["MOD_VERSION"] = info.version;
    env["mod"] = luaEngine->getState().create_table();
    
    // Register mod API functions
    // env["mod"]["registerBlock"] = ...
    // env["mod"]["registerItem"] = ...
    // etc.
    
    // Execute script
    auto execResult = luaEngine->execute(source, env);
    if (!execResult.success) {
        delete mod->context;
        mod->context = nullptr;
        result.error = execResult.error;
        return result;
    }
    
    result.success = true;
    return result;
}

void ModLoader::unloadLuaMod(LoadedMod& mod) {
    // Clean up context
    if (mod.context) {
        delete mod.context;
        mod.context = nullptr;
    }
    
    // Lua environment will be cleaned up automatically
}

ModLoadResult ModLoader::loadDataPack(const ModInfo& info) {
    ModLoadResult result;
    result.modId = info.id;
    
    if (!config.enableDataPacks) {
        result.error = "Data packs are disabled";
        return result;
    }
    
    // Load data pack files (blocks, items, recipes, etc.)
    // This would parse JSON/TOML files in the data directory
    
    result.success = true;
    return result;
}

void ModLoader::unloadMod(const std::string& modId) {
    auto it = mods.find(modId);
    if (it == mods.end()) return;
    
    auto& mod = it->second;
    
    // Disable first
    if (mod->state == ModState::Enabled) {
        disableMod(modId);
    }
    
    // Unload based on type
    switch (mod->type) {
        case ModType::Native:
            unloadNativeMod(*mod);
            break;
        case ModType::Lua:
            unloadLuaMod(*mod);
            break;
        default:
            break;
    }
    
    // Remove from load order
    loadOrder.erase(
        std::remove(loadOrder.begin(), loadOrder.end(), modId),
        loadOrder.end()
    );
    
    mods.erase(it);
    
    triggerEvent(ModEvent::ModUnloaded, modId);
    Logger::info("Unloaded mod: {}", modId);
}

void ModLoader::reloadMod(const std::string& modId) {
    auto it = mods.find(modId);
    if (it == mods.end()) return;
    
    ModInfo info = it->second->info;
    unloadMod(modId);
    loadMod(info);
}

void ModLoader::loadAllMods() {
    auto discovered = discoverMods();
    
    // Sort by load priority
    std::sort(discovered.begin(), discovered.end(),
              [](const ModInfo& a, const ModInfo& b) {
                  return a.loadPriority > b.loadPriority;
              });
    
    // Load in dependency order
    for (const auto& info : discovered) {
        if (!isModLoaded(info.id)) {
            loadMod(info);
        }
    }
}

void ModLoader::unloadAllMods() {
    // Unload in reverse order
    for (auto it = loadOrder.rbegin(); it != loadOrder.rend(); ++it) {
        unloadMod(*it);
    }
    
    loadOrder.clear();
    mods.clear();
}

void ModLoader::reloadAllMods() {
    std::vector<ModInfo> modInfos;
    for (const auto& [id, mod] : mods) {
        modInfos.push_back(mod->info);
    }
    
    unloadAllMods();
    
    for (const auto& info : modInfos) {
        loadMod(info);
    }
}

void ModLoader::enableMod(const std::string& modId) {
    auto* mod = getMod(modId);
    if (!mod || mod->state == ModState::Enabled) return;
    
    if (mod->type == ModType::Native && mod->modInstance) {
        mod->modInstance->onEnable();
    }
    
    mod->state = ModState::Enabled;
    mod->info.enabled = true;
    
    triggerEvent(ModEvent::ModEnabled, modId);
    Logger::info("Enabled mod: {}", modId);
}

void ModLoader::disableMod(const std::string& modId) {
    auto* mod = getMod(modId);
    if (!mod || mod->state != ModState::Enabled) return;
    
    if (mod->type == ModType::Native && mod->modInstance) {
        mod->modInstance->onDisable();
    }
    
    mod->state = ModState::Disabled;
    mod->info.enabled = false;
    
    triggerEvent(ModEvent::ModDisabled, modId);
    Logger::info("Disabled mod: {}", modId);
}

bool ModLoader::isModEnabled(const std::string& modId) const {
    auto it = mods.find(modId);
    return it != mods.end() && it->second->state == ModState::Enabled;
}

bool ModLoader::isModLoaded(const std::string& modId) const {
    return mods.find(modId) != mods.end();
}

bool ModLoader::checkDependencies(const ModInfo& info) const {
    for (const auto& dep : info.dependencies) {
        if (!isModLoaded(dep)) {
            Logger::warn("Missing dependency '{}' for mod '{}'", dep, info.id);
            return false;
        }
    }
    
    // Check incompatibilities
    for (const auto& incompat : info.incompatibilities) {
        if (isModLoaded(incompat)) {
            Logger::warn("Mod '{}' is incompatible with '{}'", info.id, incompat);
            return false;
        }
    }
    
    return true;
}

std::vector<std::string> ModLoader::getDependencyOrder() {
    // Topological sort of mods based on dependencies
    std::vector<std::string> order;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> visiting;
    
    std::function<bool(const std::string&)> visit = [&](const std::string& modId) -> bool {
        if (visited.count(modId)) return true;
        if (visiting.count(modId)) {
            Logger::error("Circular dependency detected: {}", modId);
            return false;
        }
        
        visiting.insert(modId);
        
        auto it = mods.find(modId);
        if (it != mods.end()) {
            for (const auto& dep : it->second->info.dependencies) {
                if (!visit(dep)) return false;
            }
        }
        
        visiting.erase(modId);
        visited.insert(modId);
        order.push_back(modId);
        return true;
    };
    
    for (const auto& [id, mod] : mods) {
        visit(id);
    }
    
    return order;
}

std::vector<std::string> ModLoader::getLoadOrder() {
    return loadOrder;
}

LoadedMod* ModLoader::getMod(const std::string& modId) {
    auto it = mods.find(modId);
    return it != mods.end() ? it->second.get() : nullptr;
}

const LoadedMod* ModLoader::getMod(const std::string& modId) const {
    auto it = mods.find(modId);
    return it != mods.end() ? it->second.get() : nullptr;
}

std::vector<LoadedMod*> ModLoader::getAllMods() {
    std::vector<LoadedMod*> result;
    for (auto& [id, mod] : mods) {
        result.push_back(mod.get());
    }
    return result;
}

std::vector<const LoadedMod*> ModLoader::getAllMods() const {
    std::vector<const LoadedMod*> result;
    for (const auto& [id, mod] : mods) {
        result.push_back(mod.get());
    }
    return result;
}

std::vector<std::string> ModLoader::getLoadedModIds() const {
    std::vector<std::string> ids;
    for (const auto& [id, mod] : mods) {
        ids.push_back(id);
    }
    return ids;
}

ModInfo ModLoader::parseModInfo(const std::filesystem::path& modPath) {
    // Try JSON first
    auto jsonPath = modPath / "mod.json";
    if (std::filesystem::exists(jsonPath)) {
        return parseModJson(jsonPath);
    }
    
    // Try TOML
    auto tomlPath = modPath / "mod.toml";
    if (std::filesystem::exists(tomlPath)) {
        return parseModToml(tomlPath);
    }
    
    // Try mcmod.info (Forge format)
    auto forgePath = modPath / "mcmod.info";
    if (std::filesystem::exists(forgePath)) {
        return parseModJson(forgePath);
    }
    
    return {};
}

ModInfo ModLoader::parseModJson(const std::filesystem::path& jsonPath) {
    ModInfo info;
    
    try {
        std::ifstream file(jsonPath);
        nlohmann::json j;
        file >> j;
        
        info.id = j.value("id", "");
        info.name = j.value("name", info.id);
        info.version = j.value("version", "1.0.0");
        info.description = j.value("description", "");
        info.author = j.value("author", "");
        info.website = j.value("website", "");
        info.license = j.value("license", "");
        info.mainClass = j.value("mainClass", "");
        info.mainScript = j.value("mainScript", "");
        info.entryPoint = j.value("entryPoint", "");
        info.loadPriority = j.value("priority", 0);
        info.enabled = j.value("enabled", true);
        info.path = jsonPath.parent_path();
        
        if (j.contains("authors")) {
            for (const auto& author : j["authors"]) {
                info.authors.push_back(author.get<std::string>());
            }
        }
        
        if (j.contains("dependencies")) {
            for (const auto& dep : j["dependencies"]) {
                info.dependencies.push_back(dep.get<std::string>());
            }
        }
        
        if (j.contains("softDependencies")) {
            for (const auto& dep : j["softDependencies"]) {
                info.softDependencies.push_back(dep.get<std::string>());
            }
        }
        
        if (j.contains("incompatibilities")) {
            for (const auto& incompat : j["incompatibilities"]) {
                info.incompatibilities.push_back(incompat.get<std::string>());
            }
        }
        
    } catch (const std::exception& e) {
        Logger::error("Failed to parse mod.json: {} - {}", jsonPath.string(), e.what());
    }
    
    return info;
}

ModInfo ModLoader::parseModToml(const std::filesystem::path& tomlPath) {
    ModInfo info;
    
    // Simple TOML parsing (would use a proper library in production)
    std::ifstream file(tomlPath);
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t\""));
        value.erase(value.find_last_not_of(" \t\",") + 1);
        
        if (key == "id") info.id = value;
        else if (key == "name") info.name = value;
        else if (key == "version") info.version = value;
        else if (key == "description") info.description = value;
        else if (key == "author") info.author = value;
        else if (key == "main_script") info.mainScript = value;
    }
    
    info.path = tomlPath.parent_path();
    
    return info;
}

void ModLoader::setEventCallback(ModEventCallback callback) {
    eventCallback = std::move(callback);
}

void ModLoader::update() {
    if (!config.enableHotReload) return;
    
    // Check for modified mod files
    // This would check file timestamps and reload if changed
}

void ModLoader::sortModsByDependency() {
    loadOrder = getDependencyOrder();
}

void ModLoader::triggerEvent(ModEvent event, const std::string& modId, const std::string& message) {
    if (eventCallback) {
        ModEventData data;
        data.event = event;
        data.modId = modId;
        data.message = message;
        eventCallback(data);
    }
}

// ============== ModContext ==============

ModContext::ModContext(const std::string& modId, ModLoader* loader)
    : modId(modId), loader(loader) {}

void ModContext::logInfo(const std::string& message) {
    Logger::info("[{}] {}", modId, message);
}

void ModContext::logWarning(const std::string& message) {
    Logger::warn("[{}] {}", modId, message);
}

void ModContext::logError(const std::string& message) {
    Logger::error("[{}] {}", modId, message);
}

std::filesystem::path ModContext::getModPath() const {
    auto* mod = loader->getMod(modId);
    return mod ? mod->info.path : std::filesystem::path();
}

std::filesystem::path ModContext::getConfigPath() const {
    return loader->getConfig().configDirectory / (modId + ".json");
}

std::filesystem::path ModContext::getDataPath() const {
    return loader->getConfig().dataDirectory / modId;
}

bool ModContext::hasConfig() const {
    return std::filesystem::exists(getConfigPath());
}

sol::table ModContext::getConfig(sol::this_state s) {
    sol::state_view lua(s);
    sol::table config = lua.create_table();
    
    auto configPath = getConfigPath();
    if (std::filesystem::exists(configPath)) {
        std::ifstream file(configPath);
        std::stringstream buffer;
        buffer << file.rdbuf();
        
        auto result = lua.script(buffer.str());
        if (result.valid()) {
            config = result;
        }
    }
    
    return config;
}

void ModContext::saveConfig(sol::table config) {
    // Serialize table to JSON and save
}

std::vector<uint8_t> ModContext::loadResource(const std::string& path) {
    auto fullPath = getModPath() / path;
    
    if (!std::filesystem::exists(fullPath)) {
        return {};
    }
    
    std::ifstream file(fullPath, std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    return data;
}

bool ModContext::resourceExists(const std::string& path) const {
    return std::filesystem::exists(getModPath() / path);
}

void ModContext::registerBlock(const std::string& id, sol::table properties) {
    // Register block with full ID (modid:blockname)
    std::string fullId = modId + ":" + id;
    logInfo("Registering block: {}", fullId);
}

void ModContext::registerItem(const std::string& id, sol::table properties) {
    std::string fullId = modId + ":" + id;
    logInfo("Registering item: {}", fullId);
}

void ModContext::registerRecipe(sol::table recipe) {
    logInfo("Registering recipe");
}

void ModContext::registerEntity(const std::string& id, sol::table properties) {
    std::string fullId = modId + ":" + id;
    logInfo("Registering entity: {}", fullId);
}

void ModContext::registerBiome(const std::string& id, sol::table properties) {
    std::string fullId = modId + ":" + id;
    logInfo("Registering biome: {}", fullId);
}

void ModContext::registerDimension(const std::string& id, sol::table properties) {
    std::string fullId = modId + ":" + id;
    logInfo("Registering dimension: {}", fullId);
}

void ModContext::registerCommand(const std::string& name, sol::function callback) {
    logInfo("Registering command: {}", name);
}

void ModContext::subscribeEvent(const std::string& event, sol::function callback) {
    // Subscribe to game events
}

LuaEngine* ModContext::getLuaEngine() {
    return loader->getLuaEngine();
}

World* ModContext::getWorld() {
    return nullptr;  // Would get from game
}

EntityManager* ModContext::getEntityManager() {
    return nullptr;  // Would get from game
}

// ============== ModRegistry ==============

void ModRegistry::registerBlock(const std::string& modId, const std::string& blockId, 
                                 sol::table properties) {
    std::string fullId = modId + ":" + blockId;
    
    RegisteredBlock block;
    block.modId = modId;
    block.blockId = fullId;
    block.numericId = nextBlockId++;
    block.properties = properties;
    
    blocks[fullId] = block;
}

void ModRegistry::registerItem(const std::string& modId, const std::string& itemId,
                                sol::table properties) {
    std::string fullId = modId + ":" + itemId;
    
    RegisteredItem item;
    item.modId = modId;
    item.itemId = fullId;
    item.numericId = nextItemId++;
    item.properties = properties;
    
    items[fullId] = item;
}

void ModRegistry::registerEntity(const std::string& modId, const std::string& entityId,
                                  sol::table properties) {
    std::string fullId = modId + ":" + entityId;
    
    RegisteredEntity entity;
    entity.modId = modId;
    entity.entityId = fullId;
    entity.properties = properties;
    
    entities[fullId] = entity;
}

void ModRegistry::registerRecipe(const std::string& modId, sol::table recipe) {
    recipes.push_back(recipe);
}

const ModRegistry::RegisteredBlock* ModRegistry::getBlock(const std::string& fullId) const {
    auto it = blocks.find(fullId);
    return it != blocks.end() ? &it->second : nullptr;
}

const ModRegistry::RegisteredItem* ModRegistry::getItem(const std::string& fullId) const {
    auto it = items.find(fullId);
    return it != items.end() ? &it->second : nullptr;
}

const ModRegistry::RegisteredEntity* ModRegistry::getEntity(const std::string& fullId) const {
    auto it = entities.find(fullId);
    return it != entities.end() ? &it->second : nullptr;
}

std::vector<std::string> ModRegistry::getBlocksByMod(const std::string& modId) const {
    std::vector<std::string> result;
    for (const auto& [id, block] : blocks) {
        if (block.modId == modId) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<std::string> ModRegistry::getItemsByMod(const std::string& modId) const {
    std::vector<std::string> result;
    for (const auto& [id, item] : items) {
        if (item.modId == modId) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<std::string> ModRegistry::getEntitiesByMod(const std::string& modId) const {
    std::vector<std::string> result;
    for (const auto& [id, entity] : entities) {
        if (entity.modId == modId) {
            result.push_back(id);
        }
    }
    return result;
}

void ModRegistry::clearModContent(const std::string& modId) {
    // Remove all content registered by this mod
    for (auto it = blocks.begin(); it != blocks.end(); ) {
        if (it->second.modId == modId) {
            it = blocks.erase(it);
        } else {
            ++it;
        }
    }
    
    for (auto it = items.begin(); it != items.end(); ) {
        if (it->second.modId == modId) {
            it = items.erase(it);
        } else {
            ++it;
        }
    }
    
    for (auto it = entities.begin(); it != entities.end(); ) {
        if (it->second.modId == modId) {
            it = entities.erase(it);
        } else {
            ++it;
        }
    }
}

void ModRegistry::clear() {
    blocks.clear();
    items.clear();
    entities.clear();
    recipes.clear();
    nextBlockId = 1000;
    nextItemId = 1000;
}

} // namespace VoxelForge
