/**
 * @file LuaEngine.hpp
 * @brief LuaJIT-based scripting engine
 */

#pragma once

#include <sol/sol.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace VoxelForge {

// Forward declarations
class World;
class Entity;
class Player;
class Block;
class ItemStack;

// Script loading info
struct ScriptInfo {
    std::string name;
    std::string path;
    std::string source;
    bool autoReload = true;
};

// Script execution result
struct ScriptResult {
    bool success = false;
    std::string error;
    sol::object returnValue;
};

// Script event types
enum class ScriptEvent {
    OnLoad,
    OnUnload,
    OnTick,
    OnBlockPlace,
    OnBlockBreak,
    OnBlockInteract,
    OnEntitySpawn,
    OnEntityDeath,
    OnEntityHurt,
    OnPlayerJoin,
    OnPlayerLeave,
    OnPlayerChat,
    OnPlayerDeath,
    OnItemUse,
    OnItemCraft,
    OnCommand
};

// Script callback info
struct ScriptCallback {
    sol::protected_function function;
    ScriptEvent event;
    std::string scriptName;
    int priority = 0;
};

// Lua engine settings
struct LuaEngineSettings {
    bool enableJIT = true;
    bool enableSandbox = true;
    size_t memoryLimit = 64 * 1024 * 1024;  // 64 MB
    float executionTimeout = 5.0f;  // seconds
    std::vector<std::string> packagePaths;
    std::vector<std::string> cPackagePaths;
};

// Lua script state
struct ScriptState {
    std::string name;
    sol::environment environment;
    bool loaded = false;
    bool enabled = true;
    std::unordered_map<ScriptEvent, std::vector<sol::protected_function>> callbacks;
};

// Main Lua scripting engine
class LuaEngine {
public:
    LuaEngine();
    ~LuaEngine();
    
    // No copy
    LuaEngine(const LuaEngine&) = delete;
    LuaEngine& operator=(const LuaEngine&) = delete;
    
    void init(const LuaEngineSettings& settings = {});
    void shutdown();
    
    // Script management
    bool loadScript(const std::string& name, const std::string& source);
    bool loadScriptFile(const std::string& name, const std::string& path);
    bool loadScriptFile(const ScriptInfo& info);
    void unloadScript(const std::string& name);
    void reloadScript(const std::string& name);
    void reloadAllScripts();
    
    bool isScriptLoaded(const std::string& name) const;
    bool isScriptEnabled(const std::string& name) const;
    void setScriptEnabled(const std::string& name, bool enabled);
    
    // Script execution
    ScriptResult execute(const std::string& code);
    ScriptResult execute(const std::string& code, sol::environment& env);
    ScriptResult executeInScript(const std::string& scriptName, const std::string& code);
    
    // Event handling
    void registerEventCallback(const std::string& scriptName, ScriptEvent event,
                               sol::protected_function callback, int priority = 0);
    void triggerEvent(ScriptEvent event, const std::vector<sol::object>& args = {});
    
    // Global API registration
    void registerGlobalAPI();
    void registerMathAPI();
    void registerWorldAPI();
    void registerEntityAPI();
    void registerPlayerAPI();
    void registerBlockAPI();
    void registerItemAPI();
    void registerGUIAPI();
    void registerNetworkAPI();
    
    // Custom API registration
    void registerFunction(const std::string& name, sol::function func);
    void registerTable(const std::string& name, sol::table table);
    void registerClass(const std::string& name, sol::usertype<void> type);
    
    // Sandbox
    void setupSandbox(sol::environment& env);
    void setGlobal(const std::string& name, sol::object value);
    sol::object getGlobal(const std::string& name);
    
    // Utility
    sol::state& getState() { return lua; }
    const sol::state& getState() const { return lua; }
    
    sol::environment createEnvironment();
    sol::environment getScriptEnvironment(const std::string& name);
    
    // Type conversion helpers
    static sol::table vec3ToTable(const glm::vec3& v, sol::state& lua);
    static glm::vec3 tableToVec3(const sol::table& t);
    static sol::table ivec3ToTable(const glm::ivec3& v, sol::state& lua);
    static glm::ivec3 tableToIVec3(const sol::table& t);
    static sol::table quatToTable(const glm::quat& q, sol::state& lua);
    static glm::quat tableToQuat(const sol::table& t);
    
    // Error handling
    void setErrorHandler(std::function<void(const std::string&)> handler);
    void reportError(const std::string& scriptName, const std::string& error);
    
    // Debug
    void setDebugEnabled(bool enabled) { debugEnabled = enabled; }
    bool isDebugEnabled() const { return debugEnabled; }
    void dumpGlobals();
    
private:
    void setupPackagePaths();
    void setupJIT();
    void setupMemoryLimit();
    static void luaPanic(sol::optional<std::string> msg);
    
    sol::state lua;
    LuaEngineSettings settings;
    
    std::unordered_map<std::string, std::unique_ptr<ScriptState>> scripts;
    std::unordered_map<ScriptEvent, std::vector<ScriptCallback>> eventCallbacks;
    
    std::function<void(const std::string&)> errorHandler;
    bool initialized = false;
    bool debugEnabled = false;
};

// Scripted behavior component for entities
class ScriptedBehavior {
public:
    ScriptedBehavior();
    ~ScriptedBehavior() = default;
    
    void setScript(LuaEngine* engine, const std::string& scriptName);
    void clear();
    
    void onUpdate(float deltaTime);
    void onEvent(ScriptEvent event, const std::vector<sol::object>& args);
    
    bool hasScript() const { return !scriptName.empty(); }
    const std::string& getScriptName() const { return scriptName; }
    
private:
    LuaEngine* engine = nullptr;
    std::string scriptName;
    sol::table selfTable;
};

// Inline helper functions for Lua integration
namespace LuaHelpers {

// Safe table access
template<typename T>
T getTableValue(const sol::table& t, const std::string& key, T defaultValue = T{}) {
    auto val = t[key];
    if (val.valid()) {
        return val.get<T>();
    }
    return defaultValue;
}

// Safe function call
template<typename... Args>
sol::protected_function_result callFunction(sol::protected_function& func, Args&&... args) {
    auto result = func(std::forward<Args>(args)...);
    if (!result.valid()) {
        sol::error err = result;
        // Log error
    }
    return result;
}

// Create a vector from Lua args
glm::vec3 makeVec3(sol::variadic_args args);
glm::ivec3 makeIVec3(sol::variadic_args args);

// Convert array to vector
std::vector<float> tableToFloatVector(const sol::table& t);
std::vector<int> tableToIntVector(const sol::table& t);
std::vector<std::string> tableToStringVector(const sol::table& t);

} // namespace LuaHelpers

} // namespace VoxelForge
