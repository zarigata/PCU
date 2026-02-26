/**
 * @file NativePlugin.cpp
 * @brief Native plugin loading implementation
 */

#include <VoxelForge/modding/ModLoader.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

// Platform-specific implementations are in ModLoader.cpp
// This file provides additional helper functions

namespace NativePlugin {

// Plugin metadata query
std::string getPluginName(ModHandle handle) {
    if (!handle) return "";
    
    auto getNameFunc = reinterpret_cast<const char*(*)()>(
#ifdef _WIN32
        GetProcAddress(handle, "getPluginName")
#else
        dlsym(handle, "getPluginName")
#endif
    );
    
    return getNameFunc ? getNameFunc() : "";
}

std::string getPluginVersion(ModHandle handle) {
    if (!handle) return "";
    
    auto getVersionFunc = reinterpret_cast<const char*(*)()>(
#ifdef _WIN32
        GetProcAddress(handle, "getPluginVersion")
#else
        dlsym(handle, "getPluginVersion")
#endif
    );
    
    return getVersionFunc ? getVersionFunc() : "";
}

std::string getPluginDescription(ModHandle handle) {
    if (!handle) return "";
    
    auto getDescFunc = reinterpret_cast<const char*(*)()>(
#ifdef _WIN32
        GetProcAddress(handle, "getPluginDescription")
#else
        dlsym(handle, "getPluginDescription")
#endif
    );
    
    return getDescFunc ? getDescFunc() : "";
}

// Plugin API version check
bool checkAPIVersion(ModHandle handle, int expectedMajor, int expectedMinor) {
    if (!handle) return false;
    
    auto getAPIMajorFunc = reinterpret_cast<int(*)()>(
#ifdef _WIN32
        GetProcAddress(handle, "getAPIMajorVersion")
#else
        dlsym(handle, "getAPIMajorVersion")
#endif
    );
    
    auto getAPIMinorFunc = reinterpret_cast<int(*)()>(
#ifdef _WIN32
        GetProcAddress(handle, "getAPIMinorVersion")
#else
        dlsym(handle, "getAPIMinorVersion")
#endif
    );
    
    if (!getAPIMajorFunc || !getAPIMinorFunc) return false;
    
    int major = getAPIMajorFunc();
    int minor = getAPIMinorFunc();
    
    return major == expectedMajor && minor >= expectedMinor;
}

// Plugin dependency query
std::vector<std::string> getDependencies(ModHandle handle) {
    std::vector<std::string> deps;
    if (!handle) return deps;
    
    auto getDepsFunc = reinterpret_cast<const char**(*)(int*)>(
#ifdef _WIN32
        GetProcAddress(handle, "getDependencies")
#else
        dlsym(handle, "getDependencies")
#endif
    );
    
    if (getDepsFunc) {
        int count = 0;
        const char** depArray = getDepsFunc(&count);
        for (int i = 0; i < count; ++i) {
            if (depArray[i]) {
                deps.push_back(depArray[i]);
            }
        }
    }
    
    return deps;
}

// Plugin initialization data
struct PluginInitData {
    void* engine;
    void* world;
    void* eventSystem;
    void* logger;
};

bool initializePlugin(ModHandle handle, const PluginInitData& data) {
    if (!handle) return false;
    
    auto initFunc = reinterpret_cast<bool(*)(const PluginInitData*)>(
#ifdef _WIN32
        GetProcAddress(handle, "initializePlugin")
#else
        dlsym(handle, "initializePlugin")
#endif
    );
    
    return initFunc ? initFunc(&data) : false;
}

void shutdownPlugin(ModHandle handle) {
    if (!handle) return;
    
    auto shutdownFunc = reinterpret_cast<void(*)()>(
#ifdef _WIN32
        GetProcAddress(handle, "shutdownPlugin")
#else
        dlsym(handle, "shutdownPlugin")
#endif
    );
    
    if (shutdownFunc) {
        shutdownFunc();
    }
}

} // namespace NativePlugin

// C API for native plugins to implement
extern "C" {

// Default implementations that plugins can override
MOD_EXPORT const char* getPluginName() { return "Unknown Plugin"; }
MOD_EXPORT const char* getPluginVersion() { return "1.0.0"; }
MOD_EXPORT const char* getPluginDescription() { return ""; }
MOD_EXPORT int getAPIMajorVersion() { return 1; }
MOD_EXPORT int getAPIMinorVersion() { return 0; }
MOD_EXPORT const char** getDependencies(int* count) { 
    static const char* noDeps[] = {};
    *count = 0;
    return noDeps;
}

// Plugin entry points
MOD_EXPORT IMod* createMod() { return nullptr; }
MOD_EXPORT void destroyMod(IMod* mod) { delete mod; }

} // extern "C"

} // namespace VoxelForge
