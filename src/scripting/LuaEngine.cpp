/**
 * @file LuaEngine.cpp
 * @brief Lua scripting engine implementation (stubbed)
 * 
 * NOTE: sol2 integration temporarily stubbed pending Lua library fixes.
 * All methods are stub implementations matching LuaEngine.hpp.
 */

#include <VoxelForge/scripting/LuaEngine.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <fstream>
#include <sstream>

namespace VoxelForge {

// ============== LuaEngine ==============

LuaEngine::LuaEngine() = default;

LuaEngine::~LuaEngine() {
    shutdown();
}

void LuaEngine::init(const LuaEngineSettings& settings) {
    this->settings = settings;
    setupPackagePaths();
    initialized = true;
    VF_INFO("LuaEngine initialized (JIT: {})", settings.enableJIT);
}

void LuaEngine::shutdown() {
    if (!initialized) return;
    scripts.clear();
    eventCallbacks.clear();
    initialized = false;
    VF_INFO("LuaEngine shutdown");
}

void LuaEngine::setupPackagePaths() {
    // Stub: would set up Lua package paths
}

bool LuaEngine::loadScript(const std::string& name, const std::string& source) {
    if (!initialized) return false;
    
    auto state = std::make_unique<ScriptState>();
    state->name = name;
    state->loaded = true;
    scripts[name] = std::move(state);
    
    VF_DEBUG("Loaded script: {}", name);
    return true;
}

bool LuaEngine::loadScriptFile(const std::string& name, const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        VF_ERROR("Failed to open script file: {}", path);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadScript(name, buffer.str());
}

bool LuaEngine::loadScriptFile(const ScriptInfo& info) {
    return loadScriptFile(info.name, info.path);
}

void LuaEngine::unloadScript(const std::string& name) {
    auto it = scripts.find(name);
    if (it != scripts.end()) {
        scripts.erase(it);
        VF_DEBUG("Unloaded script: {}", name);
    }
}

void LuaEngine::reloadScript(const std::string& name) {
    VF_DEBUG("Reloaded script: {}", name);
}

void LuaEngine::reloadAllScripts() {
    for (auto& [name, state] : scripts) {
        reloadScript(name);
    }
}

bool LuaEngine::isScriptLoaded(const std::string& name) const {
    return scripts.find(name) != scripts.end();
}

bool LuaEngine::isScriptEnabled(const std::string& name) const {
    auto it = scripts.find(name);
    return it != scripts.end() && it->second->enabled;
}

void LuaEngine::setScriptEnabled(const std::string& name, bool enabled) {
    auto it = scripts.find(name);
    if (it != scripts.end()) {
        it->second->enabled = enabled;
    }
}

ScriptResult LuaEngine::execute(const std::string& code) {
    ScriptResult result;
    if (!initialized) {
        result.error = "LuaEngine not initialized";
        return result;
    }
    // Stub: would execute Lua code
    result.success = true;
    return result;
}

ScriptResult LuaEngine::executeInScript(const std::string& scriptName, const std::string& code) {
    ScriptResult result;
    auto it = scripts.find(scriptName);
    if (it == scripts.end()) {
        result.error = "Script not found: " + scriptName;
        return result;
    }
    // Stub: would execute in script environment
    result.success = true;
    return result;
}

void LuaEngine::registerEventCallback(const std::string& scriptName, ScriptEvent event,
                                       std::function<void()> callback, int priority) {
    ScriptCallback cb;
    cb.function = std::move(callback);
    cb.event = event;
    cb.scriptName = scriptName;
    cb.priority = priority;
    eventCallbacks[event].push_back(std::move(cb));
}

void LuaEngine::triggerEvent(ScriptEvent event) {
    auto it = eventCallbacks.find(event);
    if (it == eventCallbacks.end()) return;
    
    for (const auto& callback : it->second) {
        auto scriptIt = scripts.find(callback.scriptName);
        if (scriptIt != scripts.end() && !scriptIt->second->enabled) continue;
        if (callback.function) {
            callback.function();
        }
    }
}

void LuaEngine::registerGlobalAPI() { /* Stub */ }
void LuaEngine::registerMathAPI() { /* Stub */ }
void LuaEngine::registerWorldAPI() { /* Stub */ }
void LuaEngine::registerEntityAPI() { /* Stub */ }
void LuaEngine::registerPlayerAPI() { /* Stub */ }
void LuaEngine::registerBlockAPI() { /* Stub */ }
void LuaEngine::registerItemAPI() { /* Stub */ }
void LuaEngine::registerGUIAPI() { /* Stub */ }
void LuaEngine::registerNetworkAPI() { /* Stub */ }

void LuaEngine::registerFunction(const std::string& name, std::function<void()> func) {
    // Stub
}

void LuaEngine::setErrorHandler(std::function<void(const std::string&)> handler) {
    errorHandler = std::move(handler);
}

void LuaEngine::reportError(const std::string& scriptName, const std::string& error) {
    VF_ERROR("Script error in '{}': {}", scriptName, error);
    if (errorHandler) {
        errorHandler("[" + scriptName + "] " + error);
    }
}

// ============== ScriptedBehavior ==============

ScriptedBehavior::ScriptedBehavior() = default;

void ScriptedBehavior::setScript(LuaEngine* engine, const std::string& scriptName) {
    this->engine = engine;
    this->scriptName = scriptName;
}

void ScriptedBehavior::clear() {
    engine = nullptr;
    scriptName.clear();
}

void ScriptedBehavior::onUpdate(float deltaTime) {
    if (!engine || scriptName.empty()) return;
    // Stub: would call script onUpdate
}

void ScriptedBehavior::onEvent(ScriptEvent event) {
    if (!engine || scriptName.empty()) return;
    engine->triggerEvent(event);
}

// ============== LuaHelpers ==============

namespace LuaHelpers {

glm::vec3 makeVec3(float x, float y, float z) {
    return glm::vec3(x, y, z);
}

glm::ivec3 makeIVec3(int x, int y, int z) {
    return glm::ivec3(x, y, z);
}

std::vector<float> toFloatVector(const std::string& s) {
    return {};
}

std::vector<int> toIntVector(const std::string& s) {
    return {};
}

std::vector<std::string> toStringVector(const std::string& s) {
    return {};
}

} // namespace LuaHelpers

} // namespace VoxelForge
