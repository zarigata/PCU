/**
 * @file LuaEngine.hpp
 * @brief Lua scripting engine
 * 
 * NOTE: sol2 integration temporarily stubbed pending Lua library fixes.
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <any>

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
    std::any returnValue;
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
    std::function<void()> function;  // Stubbed
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
    bool loaded = false;
    bool enabled = true;
};

// Main Lua scripting engine (stubbed for compilation)
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
    ScriptResult executeInScript(const std::string& scriptName, const std::string& code);
    
    // Event handling
    void registerEventCallback(const std::string& scriptName, ScriptEvent event,
                               std::function<void()> callback, int priority = 0);
    void triggerEvent(ScriptEvent event);
    
    // Global API registration (stubbed)
    void registerGlobalAPI();
    void registerMathAPI();
    void registerWorldAPI();
    void registerEntityAPI();
    void registerPlayerAPI();
    void registerBlockAPI();
    void registerItemAPI();
    void registerGUIAPI();
    void registerNetworkAPI();
    
    // Custom API registration (stubbed)
    void registerFunction(const std::string& name, std::function<void()> func);
    
    // Utility
    bool isInitialized() const { return initialized; }
    
    // Error handling
    void setErrorHandler(std::function<void(const std::string&)> handler);
    void reportError(const std::string& scriptName, const std::string& error);
    
    // Debug
    void setDebugEnabled(bool enabled) { debugEnabled = enabled; }
    bool isDebugEnabled() const { return debugEnabled; }
    
private:
    void setupPackagePaths();
    
    LuaEngineSettings settings;
    std::unordered_map<std::string, std::unique_ptr<ScriptState>> scripts;
    std::unordered_map<ScriptEvent, std::vector<ScriptCallback>> eventCallbacks;
    std::function<void(const std::string&)> errorHandler;
    bool initialized = false;
    bool debugEnabled = false;
};

// Scripted behavior component for entities (stubbed)
class ScriptedBehavior {
public:
    ScriptedBehavior();
    ~ScriptedBehavior() = default;
    
    void setScript(LuaEngine* engine, const std::string& scriptName);
    void clear();
    
    void onUpdate(float deltaTime);
    void onEvent(ScriptEvent event);
    
    bool hasScript() const { return !scriptName.empty(); }
    const std::string& getScriptName() const { return scriptName; }
    
private:
    LuaEngine* engine = nullptr;
    std::string scriptName;
};

// Helper functions (stubbed)
namespace LuaHelpers {

glm::vec3 makeVec3(float x, float y, float z);
glm::ivec3 makeIVec3(int x, int y, int z);

std::vector<float> toFloatVector(const std::string& s);
std::vector<int> toIntVector(const std::string& s);
std::vector<std::string> toStringVector(const std::string& s);

} // namespace LuaHelpers

} // namespace VoxelForge