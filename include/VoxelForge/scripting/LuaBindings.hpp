/**
 * @file LuaBindings.hpp
 * @brief Lua bindings for engine types
 */

#pragma once

#include <sol/sol.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

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

// Register all engine bindings
void registerAllLuaBindings(sol::state& lua);

// Individual module registrations
namespace LuaBindings {

// Math types
void registerVec2(sol::state& lua);
void registerVec3(sol::state& lua);
void registerVec4(sol::state& lua);
void registerIVec2(sol::state& lua);
void registerIVec3(sol::state& lua);
void registerIVec4(sol::state& lua);
void registerQuat(sol::state& lua);
void registerMat4(sol::state& lua);

// Core types
void registerLogger(sol::state& lua);
void registerTimer(sol::state& lua);
void registerRandom(sol::state& lua);
void registerNoise(sol::state& lua);

// World types
void registerBlock(sol::state& lua);
void registerBlockState(sol::state& lua);
void registerChunk(sol::state& lua);
void registerWorld(sol::state& lua);
void registerBiome(sol::state& lua);
void registerDimension(sol::state& lua);

// Entity types
void registerEntity(sol::state& lua);
void registerLivingEntity(sol::state& lua);
void registerPlayer(sol::state& lua);
void registerMob(sol::state& lua);
void registerProjectile(sol::state& lua);
void registerItemEntity(sol::state& lua);

// Item types
void registerItem(sol::state& lua);
void registerItemStack(sol::state& lua);
void registerInventory(sol::state& lua);
void registerContainer(sol::state& lua);

// Game types
void registerRecipe(sol::state& lua);
void registerCrafting(sol::state& lua);
void registerAchievement(sol::state& lua);
void registerStatistic(sol::state& lua);

// Physics types
void registerPhysics(sol::state& lua);
void registerRaycastHit(sol::state& lua);
void registerCollisionInfo(sol::state& lua);

// Audio types
void registerAudio(sol::state& lua);
void registerSound(sol::state& lua);

// Network types
void registerNetwork(sol::state& lua);
void registerPacket(sol::state& lua);

// GUI types
void registerGUI(sol::state& lua);
void registerWidget(sol::state& lua);
void registerButton(sol::state& lua);
void registerLabel(sol::state& lua);
void registerTextField(sol::state& lua);

} // namespace LuaBindings

// Lua API namespaces
namespace LuaAPI {

// Global API functions
namespace Global {
    void log(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);
    
    float getTime();
    float getDeltaTime();
    float getFPS();
    
    void schedule(float delay, sol::function callback);
    void scheduleRepeating(float interval, sol::function callback);
    
    std::string getVersion();
    bool isServer();
    bool isClient();
    bool isDedicatedServer();
}

// World API
namespace WorldAPI {
    sol::table getBlock(sol::this_state s, int x, int y, int z);
    void setBlock(int x, int y, int z, const std::string& blockId);
    void setBlockState(int x, int y, int z, sol::table state);
    
    sol::table getBiome(sol::this_state s, int x, int z);
    void setBiome(int x, int z, const std::string& biomeId);
    
    int getHighestBlock(int x, int z);
    bool isBlockLoaded(int x, int z);
    
    void spawnParticle(const std::string& type, float x, float y, float z, 
                       sol::optional<sol::table> options);
    
    void playSound(const std::string& sound, float x, float y, float z,
                   sol::optional<float> volume, sol::optional<float> pitch);
    
    void explode(float x, float y, float z, float power, sol::optional<bool> fire);
    
    sol::table raycast(sol::this_state s, float ox, float oy, float oz,
                       float dx, float dy, float dz, float maxDist);
    
    std::string getDimension();
    void setDimension(const std::string& dimension);
    
    int64_t getSeed();
    int64_t getTime();
    void setTime(int64_t time);
    
    bool isDay();
    bool isNight();
    bool isThundering();
    bool isRaining();
    void setWeather(const std::string& weather, int duration);
}

// Entity API
namespace EntityAPI {
    sol::table spawn(sol::this_state s, const std::string& type, float x, float y, float z);
    void despawn(uint32_t entityId);
    bool exists(uint32_t entityId);
    
    sol::table get(sol::this_state s, uint32_t entityId);
    
    sol::table getPosition(sol::this_state s, uint32_t entityId);
    void setPosition(uint32_t entityId, float x, float y, float z);
    
    sol::table getVelocity(sol::this_state s, uint32_t entityId);
    void setVelocity(uint32_t entityId, float x, float y, float z);
    
    sol::table getRotation(sol::this_state s, uint32_t entityId);
    void setRotation(uint32_t entityId, float yaw, float pitch);
    
    std::string getType(uint32_t entityId);
    std::string getName(uint32_t entityId);
    void setName(uint32_t entityId, const std::string& name);
    
    float getHealth(uint32_t entityId);
    void setHealth(uint32_t entityId, float health);
    void damage(uint32_t entityId, float amount, sol::optional<std::string> source);
    void heal(uint32_t entityId, float amount);
    void kill(uint32_t entityId);
    
    bool isAlive(uint32_t entityId);
    bool isOnGround(uint32_t entityId);
    bool isInWater(uint32_t entityId);
    bool isInLava(uint32_t entityId);
    
    void addEffect(uint32_t entityId, const std::string& effect, int duration, int amplifier);
    void removeEffect(uint32_t entityId, const std::string& effect);
    bool hasEffect(uint32_t entityId, const std::string& effect);
    
    sol::table getEntitiesInRange(sol::this_state s, float x, float y, float z, float range);
    sol::table getEntitiesInBox(sol::this_state s, float x1, float y1, float z1,
                                 float x2, float y2, float z2);
}

// Player API
namespace PlayerAPI {
    sol::table getAll(sol::this_state s);
    sol::table getByName(sol::this_state s, const std::string& name);
    sol::table getByUUID(sol::this_state s, const std::string& uuid);
    
    void sendMessage(uint32_t playerId, const std::string& message);
    void sendTitle(uint32_t playerId, const std::string& title, sol::optional<std::string> subtitle);
    void sendActionBar(uint32_t playerId, const std::string& message);
    
    float getFoodLevel(uint32_t playerId);
    void setFoodLevel(uint32_t playerId, float level);
    
    float getSaturation(uint32_t playerId);
    void setSaturation(uint32_t playerId, float saturation);
    
    int getExperience(uint32_t playerId);
    void setExperience(uint32_t playerId, int xp);
    int getLevel(uint32_t playerId);
    void setLevel(uint32_t playerId, int level);
    
    std::string getGamemode(uint32_t playerId);
    void setGamemode(uint32_t playerId, const std::string& gamemode);
    
    bool isFlying(uint32_t playerId);
    void setFlying(uint32_t playerId, bool flying);
    
    bool isSprinting(uint32_t playerId);
    bool isSneaking(uint32_t playerId);
    
    sol::table getInventory(sol::this_state s, uint32_t playerId);
    sol::table getEnderChest(sol::this_state s, uint32_t playerId);
    
    void giveItem(uint32_t playerId, const std::string& itemId, int count, sol::optional<sol::table> nbt);
    void clearInventory(uint32_t playerId);
    
    void teleport(uint32_t playerId, float x, float y, float z, 
                  sol::optional<float> yaw, sol::optional<float> pitch);
    void teleportTo(uint32_t playerId, uint32_t targetId);
    
    void kick(uint32_t playerId, const std::string& reason);
    void ban(uint32_t playerId, const std::string& reason, sol::optional<int64_t> duration);
    
    std::string getIP(uint32_t playerId);
    std::string getUUID(uint32_t playerId);
}

// Item API
namespace ItemAPI {
    sol::table get(sol::this_state s, const std::string& itemId);
    bool exists(const std::string& itemId);
    
    sol::table createStack(sol::this_state s, const std::string& itemId, int count, 
                            sol::optional<sol::table> nbt);
    
    std::string getName(const std::string& itemId);
    int getMaxStackSize(const std::string& itemId);
    int getMaxDamage(const std::string& itemId);
    bool isDamageable(const std::string& itemId);
    bool isBlock(const std::string& itemId);
}

// Block API
namespace BlockAPI {
    sol::table get(sol::this_state s, const std::string& blockId);
    bool exists(const std::string& blockId);
    
    std::string getName(const std::string& blockId);
    std::string getDisplayName(const std::string& blockId);
    
    float getHardness(const std::string& blockId);
    float getBlastResistance(const std::string& blockId);
    
    bool isSolid(const std::string& blockId);
    bool isTransparent(const std::string& blockId);
    bool isAir(const std::string& blockId);
    bool isLiquid(const std::string& blockId);
    
    sol::table getDrops(sol::this_state s, const std::string& blockId, 
                         sol::optional<sol::table> tool);
    
    sol::table getDefaultState(sol::this_state s, const std::string& blockId);
    sol::table getPossibleStates(sol::this_state s, const std::string& blockId);
}

// Crafting API
namespace CraftingAPI {
    void addRecipe(sol::table recipe);
    void removeRecipe(const std::string& recipeId);
    
    sol::table getRecipes(sol::this_state s, const std::string& itemId);
    sol::table getRecipesFor(sol::this_state s, sol::table ingredients);
    
    bool hasRecipe(const std::string& recipeId);
    sol::table craft(sol::this_state s, sol::table ingredients);
}

// Command API
namespace CommandAPI {
    void registerCommand(const std::string& name, sol::function callback,
                         sol::optional<std::string> permission,
                         sol::optional<std::string> description);
    void unregisterCommand(const std::string& name);
    
    bool execute(uint32_t executorId, const std::string& command);
    
    sol::table getRegistered(sol::this_state s);
}

// GUI API
namespace GUIAPI {
    sol::table createScreen(sol::this_state s, const std::string& id);
    void showScreen(uint32_t playerId, const std::string& screenId);
    void closeScreen(uint32_t playerId);
    
    sol::table createWidget(sol::this_state s, const std::string& type);
    void addWidget(const std::string& screenId, sol::table widget);
    void removeWidget(const std::string& screenId, const std::string& widgetId);
}

// Network API
namespace NetworkAPI {
    void sendTo(uint32_t playerId, const std::string& channel, sol::table data);
    void sendToAll(const std::string& channel, sol::table data);
    void sendToRange(float x, float y, float z, float range, 
                     const std::string& channel, sol::table data);
    
    void registerChannel(const std::string& channel, sol::function callback);
    void unregisterChannel(const std::string& channel);
}

// Event API
namespace EventAPI {
    void subscribe(const std::string& event, sol::function callback, sol::optional<int> priority);
    void unsubscribe(const std::string& event, sol::function callback);
    void trigger(const std::string& event, sol::variadic_args args);
    
    bool isCancelled(const std::string& event);
    void setCancelled(const std::string& event, bool cancelled);
}

// Storage API
namespace StorageAPI {
    void set(const std::string& key, sol::object value);
    sol::object get(sol::this_state s, const std::string& key);
    bool has(const std::string& key);
    void remove(const std::string& key);
    
    void setPlayerData(uint32_t playerId, const std::string& key, sol::object value);
    sol::object getPlayerData(sol::this_state s, uint32_t playerId, const std::string& key);
    
    void setWorldData(const std::string& key, sol::object value);
    sol::object getWorldData(sol::this_state s, const std::string& key);
}

} // namespace LuaAPI

} // namespace VoxelForge
