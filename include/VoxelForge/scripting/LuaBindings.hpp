/**
 * @file LuaBindings.hpp
 * @brief Lua bindings for engine types
 * 
 * NOTE: sol2 integration temporarily stubbed pending Lua library fixes.
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <functional>

namespace VoxelForge {

// Forward declarations
class World;
class Chunk;
class Entity;
class Player;
class Block;
class BlockState;
class ItemStack;
class Inventory;
class Container;

// Stub for sol::state
struct LuaState;

// Register all engine bindings (stubbed)
void registerAllLuaBindings(LuaState& lua);

// Individual module registrations (stubbed)
namespace LuaBindings {

void registerVec2(LuaState& lua);
void registerVec3(LuaState& lua);
void registerVec4(LuaState& lua);
void registerQuat(LuaState& lua);
void registerMat4(LuaState& lua);

void registerLogger(LuaState& lua);
void registerTimer(LuaState& lua);
void registerRandom(LuaState& lua);
void registerNoise(LuaState& lua);

void registerBlock(LuaState& lua);
void registerBlockState(LuaState& lua);
void registerChunk(LuaState& lua);
void registerWorld(LuaState& lua);
void registerBiome(LuaState& lua);
void registerDimension(LuaState& lua);

void registerEntity(LuaState& lua);
void registerPlayer(LuaState& lua);
void registerItem(LuaState& lua);
void registerItemStack(LuaState& lua);
void registerInventory(LuaState& lua);

void registerPhysics(LuaState& lua);
void registerAudio(LuaState& lua);
void registerNetwork(LuaState& lua);
void registerGUI(LuaState& lua);

} // namespace LuaBindings

// Lua API namespaces (stubbed)
namespace LuaAPI {

namespace Global {
    void log(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);
    float getTime();
    float getDeltaTime();
    float getFPS();
    std::string getVersion();
    bool isServer();
    bool isClient();
}

namespace WorldAPI {
    void setBlock(int x, int y, int z, const std::string& blockId);
    int getHighestBlock(int x, int z);
    bool isBlockLoaded(int x, int z);
    int64_t getSeed();
    int64_t getTime();
    void setTime(int64_t time);
    bool isDay();
    bool isNight();
}

namespace EntityAPI {
    uint32_t spawn(const std::string& type, float x, float y, float z);
    void despawn(uint32_t entityId);
    bool exists(uint32_t entityId);
    glm::vec3 getPosition(uint32_t entityId);
    void setPosition(uint32_t entityId, float x, float y, float z);
    std::string getType(uint32_t entityId);
    float getHealth(uint32_t entityId);
    void setHealth(uint32_t entityId, float health);
    bool isAlive(uint32_t entityId);
}

namespace PlayerAPI {
    void sendMessage(uint32_t playerId, const std::string& message);
    std::string getGamemode(uint32_t playerId);
    void setGamemode(uint32_t playerId, const std::string& gamemode);
    bool isFlying(uint32_t playerId);
    void setFlying(uint32_t playerId, bool flying);
}

namespace ItemAPI {
    bool exists(const std::string& itemId);
    std::string getName(const std::string& itemId);
    int getMaxStackSize(const std::string& itemId);
}

namespace BlockAPI {
    bool exists(const std::string& blockId);
    std::string getName(const std::string& blockId);
    float getHardness(const std::string& blockId);
    bool isSolid(const std::string& blockId);
    bool isTransparent(const std::string& blockId);
}

namespace CommandAPI {
    void registerCommand(const std::string& name, std::function<void()> callback);
    bool execute(uint32_t executorId, const std::string& command);
}

namespace EventAPI {
    void subscribe(const std::string& event, std::function<void()> callback);
    void trigger(const std::string& event);
}

} // namespace LuaAPI

} // namespace VoxelForge