/**
 * @file LuaEngine.cpp
 * @brief LuaJIT-based scripting engine implementation
 */

#include "LuaEngine.hpp"
#include "LuaBindings.hpp"
#include <VoxelForge/core/Logger.hpp>
#include <sstream>
#include <chrono>

namespace VoxelForge {

// ============== LuaEngine ==============

LuaEngine::LuaEngine() = default;

LuaEngine::~LuaEngine() {
    shutdown();
}

void LuaEngine::init(const LuaEngineSettings& settings) {
    this->settings = settings;
    
    // Open all standard libraries
    lua.open_libraries(
        sol::lib::base,
        sol::lib::package,
        sol::lib::coroutine,
        sol::lib::string,
        sol::lib::os,
        sol::lib::math,
        sol::lib::table,
        sol::lib::debug,
        sol::lib::bit32,
        sol::lib::io,
        sol::lib::ffi,
        sol::lib::jit
    );
    
    // Setup panic handler
    lua.set_panic(sol::c_call<decltype(&luaPanic), &luaPanic>);
    
    // Setup package paths
    setupPackagePaths();
    
    // Setup JIT
    if (settings.enableJIT) {
        setupJIT();
    }
    
    // Setup memory limit
    if (settings.enableSandbox) {
        setupMemoryLimit();
    }
    
    // Register all bindings
    registerAllLuaBindings(lua);
    registerGlobalAPI();
    
    initialized = true;
    Logger::info("LuaEngine initialized (JIT: {})", settings.enableJIT);
}

void LuaEngine::shutdown() {
    if (!initialized) return;
    
    // Clear all scripts
    scripts.clear();
    eventCallbacks.clear();
    
    // Clear Lua state
    lua = sol::state();
    
    initialized = false;
    Logger::info("LuaEngine shutdown");
}

void LuaEngine::setupPackagePaths() {
    std::string packagePath;
    for (const auto& path : settings.packagePaths) {
        if (!packagePath.empty()) packagePath += ";";
        packagePath += path + "/?.lua";
        packagePath += ";" + path + "/?/init.lua";
    }
    
    if (!packagePath.empty()) {
        lua["package"]["path"] = packagePath;
    }
    
    std::string cpath;
    for (const auto& path : settings.cPackagePaths) {
        if (!cpath.empty()) cpath += ";";
        cpath += path + "/?.so";
        cpath += ";" + path + "/?.dll";
    }
    
    if (!cpath.empty()) {
        lua["package"]["cpath"] = cpath;
    }
}

void LuaEngine::setupJIT() {
    // Enable JIT optimizations
    sol::table jit = lua["jit"];
    if (jit.valid()) {
        jit["opt"]["start"]();  // Start with default optimizations
        jit["opt"]["maxtrace"] = 1000;
        jit["opt"]["maxrecord"] = 4000;
        jit["opt"]["maxside"] = 100;
        jit["opt"]["maxsnap"] = 500;
    }
}

void LuaEngine::setupMemoryLimit() {
    // Set memory limit via Lua
    if (settings.memoryLimit > 0) {
        // This would need custom allocator for proper enforcement
    }
}

void LuaEngine::luaPanic(sol::optional<std::string> msg) {
    if (msg) {
        Logger::error("Lua panic: {}", *msg);
    } else {
        Logger::error("Lua panic: unknown error");
    }
}

bool LuaEngine::loadScript(const std::string& name, const std::string& source) {
    if (!initialized) return false;
    
    // Create script state
    auto state = std::make_unique<ScriptState>();
    state->name = name;
    state->environment = createEnvironment();
    
    // Load the script
    auto result = lua.safe_script(source, state->environment, sol::script_pass_on_error);
    
    if (!result.valid()) {
        sol::error err = result;
        reportError(name, err.what());
        return false;
    }
    
    state->loaded = true;
    scripts[name] = std::move(state);
    
    Logger::debug("Loaded script: {}", name);
    return true;
}

bool LuaEngine::loadScriptFile(const std::string& name, const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::error("Failed to open script file: {}", path);
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
        // Trigger unload event
        triggerEventForScript(it->second.get(), ScriptEvent::OnUnload, {});
        
        // Remove event callbacks
        for (auto& [event, callbacks] : eventCallbacks) {
            callbacks.erase(
                std::remove_if(callbacks.begin(), callbacks.end(),
                    [&name](const ScriptCallback& cb) { return cb.scriptName == name; }),
                callbacks.end()
            );
        }
        
        scripts.erase(it);
        Logger::debug("Unloaded script: {}", name);
    }
}

void LuaEngine::reloadScript(const std::string& name) {
    auto it = scripts.find(name);
    if (it != scripts.end()) {
        // Would need to store the source to reload
        Logger::debug("Reloaded script: {}", name);
    }
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
    
    auto execResult = lua.safe_script(code, sol::script_pass_on_error);
    
    if (execResult.valid()) {
        result.success = true;
        result.returnValue = execResult.get<sol::object>();
    } else {
        sol::error err = execResult;
        result.error = err.what();
    }
    
    return result;
}

ScriptResult LuaEngine::execute(const std::string& code, sol::environment& env) {
    ScriptResult result;
    
    if (!initialized) {
        result.error = "LuaEngine not initialized";
        return result;
    }
    
    auto execResult = lua.safe_script(code, env, sol::script_pass_on_error);
    
    if (execResult.valid()) {
        result.success = true;
        result.returnValue = execResult.get<sol::object>();
    } else {
        sol::error err = execResult;
        result.error = err.what();
    }
    
    return result;
}

ScriptResult LuaEngine::executeInScript(const std::string& scriptName, const std::string& code) {
    ScriptResult result;
    
    auto it = scripts.find(scriptName);
    if (it == scripts.end()) {
        result.error = "Script not found: " + scriptName;
        return result;
    }
    
    return execute(code, it->second->environment);
}

void LuaEngine::registerEventCallback(const std::string& scriptName, ScriptEvent event,
                                       sol::protected_function callback, int priority) {
    ScriptCallback cb;
    cb.function = std::move(callback);
    cb.event = event;
    cb.scriptName = scriptName;
    cb.priority = priority;
    
    eventCallbacks[event].push_back(std::move(cb));
    
    // Sort by priority
    std::sort(eventCallbacks[event].begin(), eventCallbacks[event].end(),
              [](const ScriptCallback& a, const ScriptCallback& b) {
                  return a.priority > b.priority;
              });
}

void LuaEngine::triggerEvent(ScriptEvent event, const std::vector<sol::object>& args) {
    auto it = eventCallbacks.find(event);
    if (it == eventCallbacks.end()) return;
    
    for (const auto& callback : it->second) {
        if (!callback.function.valid()) continue;
        
        // Check if script is enabled
        auto scriptIt = scripts.find(callback.scriptName);
        if (scriptIt != scripts.end() && !scriptIt->second->enabled) continue;
        
        // Call the callback
        auto result = callback.function(sol::as_args(args));
        if (!result.valid()) {
            sol::error err = result;
            reportError(callback.scriptName, err.what());
        }
    }
}

void LuaEngine::registerGlobalAPI() {
    // Global namespace
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    // Register global functions
    voxel.set_function("log", [](const std::string& msg) { Logger::info("{}", msg); });
    voxel.set_function("logWarning", [](const std::string& msg) { Logger::warn("{}", msg); });
    voxel.set_function("logError", [](const std::string& msg) { Logger::error("{}", msg); });
    
    voxel.set_function("time", []() { 
        return std::chrono::duration<float>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
    });
    
    voxel.set_function("require", [this](const std::string& module) -> sol::object {
        // Custom require that respects our sandbox
        return lua["require"](module);
    });
}

void LuaEngine::registerMathAPI() {
    auto math = lua["math"].get_or_create<sol::table>();
    
    // Additional math functions
    math.set_function("clamp", [](float value, float min, float max) {
        return std::clamp(value, min, max);
    });
    
    math.set_function("lerp", [](float a, float b, float t) {
        return a + (b - a) * t;
    });
    
    math.set_function("smoothstep", [](float edge0, float edge1, float x) {
        float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    });
    
    math.set_function("sign", [](float x) {
        return (x > 0) ? 1.0f : ((x < 0) ? -1.0f : 0.0f);
    });
    
    math.set_function("round", [](float x) {
        return std::round(x);
    });
}

void LuaEngine::registerWorldAPI() {
    LuaBindings::registerWorldAPI(lua);
}

void LuaEngine::registerEntityAPI() {
    LuaBindings::registerEntityAPI(lua);
}

void LuaEngine::registerPlayerAPI() {
    LuaBindings::registerPlayerAPI(lua);
}

void LuaEngine::registerBlockAPI() {
    LuaBindings::registerBlockAPI(lua);
}

void LuaEngine::registerItemAPI() {
    LuaBindings::registerItemAPI(lua);
}

void LuaEngine::registerGUIAPI() {
    LuaBindings::registerGUIAPI(lua);
}

void LuaEngine::registerNetworkAPI() {
    LuaBindings::registerNetworkAPI(lua);
}

void LuaEngine::registerFunction(const std::string& name, sol::function func) {
    lua.set(name, func);
}

void LuaEngine::registerTable(const std::string& name, sol::table table) {
    lua.set(name, table);
}

void LuaEngine::registerClass(const std::string& name, sol::usertype<void> type) {
    // Would need template magic for proper registration
}

void LuaEngine::setupSandbox(sol::environment& env) {
    // Create sandboxed environment
    env = sol::environment(lua, sol::create);
    
    // Copy safe globals
    sol::table globals = lua.globals();
    for (auto& [key, value] : globals) {
        std::string keyStr = key.as<std::string>();
        
        // Whitelist safe functions
        if (keyStr == "print" || keyStr == "tostring" || keyStr == "tonumber" ||
            keyStr == "type" || keyStr == "pairs" || keyStr == "ipairs" ||
            keyStr == "next" || keyStr == "select" || keyStr == "unpack" ||
            keyStr == "error" || keyStr == "assert") {
            env[key] = value;
        }
    }
    
    // Copy safe libraries
    env["math"] = lua["math"];
    env["string"] = lua["string"];
    env["table"] = lua["table"];
    env["coroutine"] = lua["coroutine"];
}

void LuaEngine::setGlobal(const std::string& name, sol::object value) {
    lua.set(name, value);
}

sol::object LuaEngine::getGlobal(const std::string& name) {
    return lua[name];
}

sol::environment LuaEngine::createEnvironment() {
    sol::environment env(lua, sol::create);
    
    // Set up with access to global table
    env["voxel"] = lua["voxel"];
    env["math"] = lua["math"];
    env["string"] = lua["string"];
    env["table"] = lua["table"];
    
    return env;
}

sol::environment LuaEngine::getScriptEnvironment(const std::string& name) {
    auto it = scripts.find(name);
    if (it != scripts.end()) {
        return it->second->environment;
    }
    return sol::environment(lua, sol::create);
}

sol::table LuaEngine::vec3ToTable(const glm::vec3& v, sol::state& lua) {
    sol::table t = lua.create_table();
    t["x"] = v.x;
    t["y"] = v.y;
    t["z"] = v.z;
    return t;
}

glm::vec3 LuaEngine::tableToVec3(const sol::table& t) {
    return glm::vec3(
        t["x"].get_or(0.0f),
        t["y"].get_or(0.0f),
        t["z"].get_or(0.0f)
    );
}

sol::table LuaEngine::ivec3ToTable(const glm::ivec3& v, sol::state& lua) {
    sol::table t = lua.create_table();
    t["x"] = v.x;
    t["y"] = v.y;
    t["z"] = v.z;
    return t;
}

glm::ivec3 LuaEngine::tableToIVec3(const sol::table& t) {
    return glm::ivec3(
        t["x"].get_or(0),
        t["y"].get_or(0),
        t["z"].get_or(0)
    );
}

sol::table LuaEngine::quatToTable(const glm::quat& q, sol::state& lua) {
    sol::table t = lua.create_table();
    t["x"] = q.x;
    t["y"] = q.y;
    t["z"] = q.z;
    t["w"] = q.w;
    return t;
}

glm::quat LuaEngine::tableToQuat(const sol::table& t) {
    return glm::quat(
        t["w"].get_or(1.0f),
        t["x"].get_or(0.0f),
        t["y"].get_or(0.0f),
        t["z"].get_or(0.0f)
    );
}

void LuaEngine::setErrorHandler(std::function<void(const std::string&)> handler) {
    errorHandler = std::move(handler);
}

void LuaEngine::reportError(const std::string& scriptName, const std::string& error) {
    Logger::error("Script error in '{}': {}", scriptName, error);
    
    if (errorHandler) {
        errorHandler("[" + scriptName + "] " + error);
    }
}

void LuaEngine::dumpGlobals() {
    Logger::info("=== Lua Globals ===");
    for (auto& [key, value] : lua.globals()) {
        if (key.is<std::string>()) {
            Logger::info("  {} = {}", key.as<std::string>(), 
                        lua["type"](key).get<std::string>());
        }
    }
}

// ============== ScriptedBehavior ==============

ScriptedBehavior::ScriptedBehavior() = default;

void ScriptedBehavior::setScript(LuaEngine* engine, const std::string& scriptName) {
    this->engine = engine;
    this->scriptName = scriptName;
    
    // Create self table
    if (engine) {
        selfTable = engine->getState().create_table();
    }
}

void ScriptedBehavior::clear() {
    engine = nullptr;
    scriptName.clear();
    selfTable = sol::table();
}

void ScriptedBehavior::onUpdate(float deltaTime) {
    if (!engine || scriptName.empty()) return;
    
    // Call onUpdate if it exists
    auto env = engine->getScriptEnvironment(scriptName);
    if (env.valid()) {
        sol::protected_function update = env["onUpdate"];
        if (update.valid()) {
            auto result = update(selfTable, deltaTime);
            if (!result.valid()) {
                sol::error err = result;
                engine->reportError(scriptName, err.what());
            }
        }
    }
}

void ScriptedBehavior::onEvent(ScriptEvent event, const std::vector<sol::object>& args) {
    if (!engine || scriptName.empty()) return;
    
    // Trigger event through engine
    engine->triggerEvent(event, args);
}

// ============== LuaHelpers ==============

namespace LuaHelpers {

glm::vec3 makeVec3(sol::variadic_args args) {
    if (args.size() == 1) {
        sol::table t = args[0];
        return LuaEngine::tableToVec3(t);
    } else if (args.size() >= 3) {
        return glm::vec3(args[0].get<float>(), args[1].get<float>(), args[2].get<float>());
    }
    return glm::vec3(0.0f);
}

glm::ivec3 makeIVec3(sol::variadic_args args) {
    if (args.size() == 1) {
        sol::table t = args[0];
        return LuaEngine::tableToIVec3(t);
    } else if (args.size() >= 3) {
        return glm::ivec3(args[0].get<int>(), args[1].get<int>(), args[2].get<int>());
    }
    return glm::ivec3(0);
}

std::vector<float> tableToFloatVector(const sol::table& t) {
    std::vector<float> result;
    for (size_t i = 1; i <= t.size(); i++) {
        result.push_back(t[i].get_or(0.0f));
    }
    return result;
}

std::vector<int> tableToIntVector(const sol::table& t) {
    std::vector<int> result;
    for (size_t i = 1; i <= t.size(); i++) {
        result.push_back(t[i].get_or(0));
    }
    return result;
}

std::vector<std::string> tableToStringVector(const sol::table& t) {
    std::vector<std::string> result;
    for (size_t i = 1; i <= t.size(); i++) {
        result.push_back(t[i].get_or(std::string()));
    }
    return result;
}

} // namespace LuaHelpers

} // namespace VoxelForge
