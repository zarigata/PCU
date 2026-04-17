#include <VoxelForge/scripting/LuaBindings.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <chrono>

namespace VoxelForge {

void registerAllLuaBindings(LuaState&) {
    VF_DEBUG("All Lua bindings registered (stubbed)");
}

namespace LuaBindings {

void registerVec2(LuaState&) {}
void registerVec3(LuaState&) {}
void registerVec4(LuaState&) {}
void registerQuat(LuaState&) {}
void registerMat4(LuaState&) {}
void registerLogger(LuaState&) {}
void registerTimer(LuaState&) {}
void registerRandom(LuaState&) {}
void registerNoise(LuaState&) {}
void registerBlock(LuaState&) {}
void registerBlockState(LuaState&) {}
void registerChunk(LuaState&) {}
void registerWorld(LuaState&) {}
void registerBiome(LuaState&) {}
void registerDimension(LuaState&) {}
void registerEntity(LuaState&) {}
void registerPlayer(LuaState&) {}
void registerItem(LuaState&) {}
void registerItemStack(LuaState&) {}
void registerInventory(LuaState&) {}
void registerPhysics(LuaState&) {}
void registerAudio(LuaState&) {}
void registerNetwork(LuaState&) {}
void registerGUI(LuaState&) {}

}

namespace LuaAPI {

void Global::log(const std::string& message) { VF_INFO("{}", message); }
void Global::logWarning(const std::string& message) { VF_WARN("{}", message); }
void Global::logError(const std::string& message) { VF_ERROR("{}", message); }

float Global::getTime() {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float>(now.time_since_epoch()).count();
}

float Global::getDeltaTime() { return 1.0f / 60.0f; }
float Global::getFPS() { return 60.0f; }
std::string Global::getVersion() { return "1.0.0"; }
bool Global::isServer() { return true; }
bool Global::isClient() { return false; }

void WorldAPI::setBlock(int, int, int, const std::string&) {}
int WorldAPI::getHighestBlock(int, int) { return 64; }
bool WorldAPI::isBlockLoaded(int, int) { return false; }
int64_t WorldAPI::getSeed() { return 0; }
int64_t WorldAPI::getTime() { return 0; }
void WorldAPI::setTime(int64_t) {}
bool WorldAPI::isDay() { return true; }
bool WorldAPI::isNight() { return false; }

uint32_t EntityAPI::spawn(const std::string&, float, float, float) { return 0; }
void EntityAPI::despawn(uint32_t) {}
bool EntityAPI::exists(uint32_t) { return false; }
glm::vec3 EntityAPI::getPosition(uint32_t) { return glm::vec3(0.0f); }
void EntityAPI::setPosition(uint32_t, float, float, float) {}
std::string EntityAPI::getType(uint32_t) { return ""; }
float EntityAPI::getHealth(uint32_t) { return 0.0f; }
void EntityAPI::setHealth(uint32_t, float) {}
bool EntityAPI::isAlive(uint32_t) { return false; }

void PlayerAPI::sendMessage(uint32_t, const std::string&) {}
std::string PlayerAPI::getGamemode(uint32_t) { return "survival"; }
void PlayerAPI::setGamemode(uint32_t, const std::string&) {}
bool PlayerAPI::isFlying(uint32_t) { return false; }
void PlayerAPI::setFlying(uint32_t, bool) {}

bool ItemAPI::exists(const std::string&) { return false; }
std::string ItemAPI::getName(const std::string& id) { return id; }
int ItemAPI::getMaxStackSize(const std::string&) { return 64; }

bool BlockAPI::exists(const std::string&) { return false; }
std::string BlockAPI::getName(const std::string& id) { return id; }
float BlockAPI::getHardness(const std::string&) { return 1.0f; }
bool BlockAPI::isSolid(const std::string&) { return true; }
bool BlockAPI::isTransparent(const std::string&) { return false; }

void CommandAPI::registerCommand(const std::string&, std::function<void()>) {}
bool CommandAPI::execute(uint32_t, const std::string&) { return false; }

void EventAPI::subscribe(const std::string&, std::function<void()>) {}
void EventAPI::trigger(const std::string&) {}

}

} // namespace VoxelForge
