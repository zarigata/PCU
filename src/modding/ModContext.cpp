/**
 * @file ModContext.cpp
 * @brief Mod context implementation (stubbed for compilation)
 */

#include <VoxelForge/modding/ModLoader.hpp>
#include <VoxelForge/scripting/LuaEngine.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

// ============================================================================
// ModContext Implementation
// ============================================================================

ModContext::ModContext(const std::string& modId, ModLoader* loader)
    : modId(modId), loader(loader) {}

ModContext::~ModContext() = default;

std::string ModContext::getResourcePath(const std::string& relativePath) const {
    VF_TRACE("ModContext::getResourcePath not implemented");
    (void)relativePath;
    return {};
}

std::vector<uint8_t> ModContext::readResource(const std::string& relativePath) const {
    VF_TRACE("ModContext::readResource not implemented");
    (void)relativePath;
    return {};
}

bool ModContext::resourceExists(const std::string& path) const {
    VF_TRACE("ModContext::resourceExists not implemented");
    (void)path;
    return false;
}

void ModContext::registerBlock(const std::string& id, LuaTable properties) {
    VF_TRACE("ModContext::registerBlock not implemented");
    (void)id; (void)properties;
}

void ModContext::registerItem(const std::string& id, LuaTable properties) {
    VF_TRACE("ModContext::registerItem not implemented");
    (void)id; (void)properties;
}

void ModContext::registerRecipe(LuaTable recipe) {
    VF_TRACE("ModContext::registerRecipe not implemented");
    (void)recipe;
}

void ModContext::registerEntity(const std::string& id, LuaTable properties) {
    VF_TRACE("ModContext::registerEntity not implemented");
    (void)id; (void)properties;
}

void ModContext::registerBiome(const std::string& id, LuaTable properties) {
    VF_TRACE("ModContext::registerBiome not implemented");
    (void)id; (void)properties;
}

void ModContext::registerDimension(const std::string& id, LuaTable properties) {
    VF_TRACE("ModContext::registerDimension not implemented");
    (void)id; (void)properties;
}

void ModContext::registerCommand(const std::string& name, LuaFunction callback) {
    VF_TRACE("ModContext::registerCommand not implemented");
    (void)name; (void)callback;
}

void ModContext::subscribeEvent(const std::string& event, LuaFunction callback) {
    VF_TRACE("ModContext::subscribeEvent not implemented");
    (void)event; (void)callback;
}

LuaEngine* ModContext::getLuaEngine() {
    VF_TRACE("ModContext::getLuaEngine not implemented");
    return nullptr;
}

World* ModContext::getWorld() {
    VF_TRACE("ModContext::getWorld not implemented");
    return nullptr;
}

EntityManager* ModContext::getEntityManager() {
    VF_TRACE("ModContext::getEntityManager not implemented");
    return nullptr;
}

} // namespace VoxelForge
