/**
 * @file NativePlugin.cpp
 * @brief Native plugin loading implementation (stubbed for compilation)
 */

#include <VoxelForge/modding/ModLoader.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

// Platform-specific implementations are in ModLoader.cpp
// This file provides additional helper functions

namespace NativePlugin {

// Plugin metadata query
std::string getPluginName(ModHandle handle) {
    VF_TRACE("NativePlugin::getPluginName not implemented");
    (void)handle;
    return {};
}

std::string getPluginVersion(ModHandle handle) {
    VF_TRACE("NativePlugin::getPluginVersion not implemented");
    (void)handle;
    return {};
}

std::string getPluginDescription(ModHandle handle) {
    VF_TRACE("NativePlugin::getPluginDescription not implemented");
    (void)handle;
    return {};
}

// Plugin API version check
bool checkAPIVersion(ModHandle handle, int expectedMajor, int expectedMinor) {
    VF_TRACE("NativePlugin::checkAPIVersion not implemented");
    (void)handle; (void)expectedMajor; (void)expectedMinor;
    return false;
}

// Plugin dependency query
std::vector<std::string> getDependencies(ModHandle handle) {
    VF_TRACE("NativePlugin::getDependencies not implemented");
    (void)handle;
    return {};
}

// Plugin initialization data
struct PluginInitData {
    void* engine;
    void* world;
    void* eventSystem;
    void* logger;
};

bool initializePlugin(ModHandle handle, const PluginInitData& data) {
    VF_TRACE("NativePlugin::initializePlugin not implemented");
    (void)handle; (void)data;
    return false;
}

void shutdownPlugin(ModHandle handle) {
    VF_TRACE("NativePlugin::shutdownPlugin not implemented");
    (void)handle;
}

} // namespace NativePlugin

// C API for native plugins to implement (stubbed)
extern "C" {

// Default implementations that plugins can override
MOD_EXPORT const char* getPluginName() { return "Unknown Plugin"; }
MOD_EXPORT const char* getPluginVersion() { return "1.0.0"; }
MOD_EXPORT const char* getPluginDescription() { return ""; }
MOD_EXPORT int getAPIMajorVersion() { return 1; }
MOD_EXPORT int getAPIMinorVersion() { return 0; }
MOD_EXPORT const char** getDependencies(int* count) {
    *count = 0;
    return nullptr;
}

// Plugin entry points (stubbed - IMod not defined in stub API)
// MOD_EXPORT void* createMod() { return nullptr; }
// MOD_EXPORT void destroyMod(void* mod) { (void)mod; }

} // extern "C"

} // namespace VoxelForge
