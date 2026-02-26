/**
 * @file LuaBindings.cpp
 * @brief Lua bindings implementation
 */

#include "LuaBindings.hpp"
#include <VoxelForge/core/Logger.hpp>
#include <cmath>

namespace VoxelForge {

void registerAllLuaBindings(sol::state& lua) {
    LuaBindings::registerVec2(lua);
    LuaBindings::registerVec3(lua);
    LuaBindings::registerVec4(lua);
    LuaBindings::registerIVec2(lua);
    LuaBindings::registerIVec3(lua);
    LuaBindings::registerIVec4(lua);
    LuaBindings::registerQuat(lua);
    LuaBindings::registerMat4(lua);
    
    LuaBindings::registerLogger(lua);
    LuaBindings::registerTimer(lua);
    LuaBindings::registerRandom(lua);
    LuaBindings::registerNoise(lua);
    
    LuaBindings::registerBlock(lua);
    LuaBindings::registerBlockState(lua);
    LuaBindings::registerChunk(lua);
    LuaBindings::registerWorld(lua);
    
    LuaBindings::registerEntity(lua);
    LuaBindings::registerPlayer(lua);
    
    LuaBindings::registerItem(lua);
    LuaBindings::registerItemStack(lua);
    LuaBindings::registerInventory(lua);
    
    LuaBindings::registerPhysics(lua);
    LuaBindings::registerAudio(lua);
    LuaBindings::registerNetwork(lua);
    LuaBindings::registerGUI(lua);
    
    Logger::debug("All Lua bindings registered");
}

namespace LuaBindings {

void registerVec2(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    voxel.new_usertype<glm::vec2>("Vec2",
        sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
        "x", &glm::vec2::x,
        "y", &glm::vec2::y,
        
        // Operators
        sol::meta_function::addition, sol::overload(
            [](const glm::vec2& a, const glm::vec2& b) { return a + b; },
            [](const glm::vec2& a, float b) { return a + b; }
        ),
        sol::meta_function::subtraction, sol::overload(
            [](const glm::vec2& a, const glm::vec2& b) { return a - b; },
            [](const glm::vec2& a, float b) { return a - b; }
        ),
        sol::meta_function::multiplication, sol::overload(
            [](const glm::vec2& a, const glm::vec2& b) { return a * b; },
            [](const glm::vec2& a, float b) { return a * b; }
        ),
        sol::meta_function::division, sol::overload(
            [](const glm::vec2& a, const glm::vec2& b) { return a / b; },
            [](const glm::vec2& a, float b) { return a / b; }
        ),
        sol::meta_function::unary_minus, [](const glm::vec2& v) { return -v; },
        sol::meta_function::to_string, [](const glm::vec2& v) {
            return "Vec2(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
        },
        
        // Methods
        "length", [](const glm::vec2& v) { return glm::length(v); },
        "normalize", [](const glm::vec2& v) { return glm::normalize(v); },
        "dot", [](const glm::vec2& a, const glm::vec2& b) { return glm::dot(a, b); },
        "distance", [](const glm::vec2& a, const glm::vec2& b) { return glm::distance(a, b); },
        "lerp", [](const glm::vec2& a, const glm::vec2& b, float t) { return glm::mix(a, b, t); }
    );
}

void registerVec3(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    voxel.new_usertype<glm::vec3>("Vec3",
        sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z,
        
        // Operators
        sol::meta_function::addition, sol::overload(
            [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
            [](const glm::vec3& a, float b) { return a + b; }
        ),
        sol::meta_function::subtraction, sol::overload(
            [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
            [](const glm::vec3& a, float b) { return a - b; }
        ),
        sol::meta_function::multiplication, sol::overload(
            [](const glm::vec3& a, const glm::vec3& b) { return a * b; },
            [](const glm::vec3& a, float b) { return a * b; }
        ),
        sol::meta_function::division, sol::overload(
            [](const glm::vec3& a, const glm::vec3& b) { return a / b; },
            [](const glm::vec3& a, float b) { return a / b; }
        ),
        sol::meta_function::unary_minus, [](const glm::vec3& v) { return -v; },
        sol::meta_function::to_string, [](const glm::vec3& v) {
            return "Vec3(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
        },
        
        // Methods
        "length", [](const glm::vec3& v) { return glm::length(v); },
        "normalize", [](const glm::vec3& v) { return glm::normalize(v); },
        "dot", [](const glm::vec3& a, const glm::vec3& b) { return glm::dot(a, b); },
        "cross", [](const glm::vec3& a, const glm::vec3& b) { return glm::cross(a, b); },
        "distance", [](const glm::vec3& a, const glm::vec3& b) { return glm::distance(a, b); },
        "lerp", [](const glm::vec3& a, const glm::vec3& b, float t) { return glm::mix(a, b, t); },
        "reflect", [](const glm::vec3& v, const glm::vec3& n) { return glm::reflect(v, n); }
    );
    
    // Static factory
    voxel["Vec3"]["up"] = []() { return glm::vec3(0.0f, 1.0f, 0.0f); };
    voxel["Vec3"]["down"] = []() { return glm::vec3(0.0f, -1.0f, 0.0f); };
    voxel["Vec3"]["forward"] = []() { return glm::vec3(0.0f, 0.0f, -1.0f); };
    voxel["Vec3"]["back"] = []() { return glm::vec3(0.0f, 0.0f, 1.0f); };
    voxel["Vec3"]["right"] = []() { return glm::vec3(1.0f, 0.0f, 0.0f); };
    voxel["Vec3"]["left"] = []() { return glm::vec3(-1.0f, 0.0f, 0.0f); };
    voxel["Vec3"]["zero"] = []() { return glm::vec3(0.0f); };
    voxel["Vec3"]["one"] = []() { return glm::vec3(1.0f); };
}

void registerVec4(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    voxel.new_usertype<glm::vec4>("Vec4",
        sol::constructors<glm::vec4(), glm::vec4(float), glm::vec4(float, float, float, float)>(),
        "x", &glm::vec4::x,
        "y", &glm::vec4::y,
        "z", &glm::vec4::z,
        "w", &glm::vec4::w,
        "r", &glm::vec4::r,
        "g", &glm::vec4::g,
        "b", &glm::vec4::b,
        "a", &glm::vec4::a,
        
        "length", [](const glm::vec4& v) { return glm::length(v); },
        "normalize", [](const glm::vec4& v) { return glm::normalize(v); },
        "dot", [](const glm::vec4& a, const glm::vec4& b) { return glm::dot(a, b); }
    );
}

void registerIVec2(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    voxel.new_usertype<glm::ivec2>("IVec2",
        sol::constructors<glm::ivec2(), glm::ivec2(int), glm::ivec2(int, int)>(),
        "x", &glm::ivec2::x,
        "y", &glm::ivec2::y
    );
}

void registerIVec3(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    voxel.new_usertype<glm::ivec3>("IVec3",
        sol::constructors<glm::ivec3(), glm::ivec3(int), glm::ivec3(int, int, int)>(),
        "x", &glm::ivec3::x,
        "y", &glm::ivec3::y,
        "z", &glm::ivec3::z,
        
        sol::meta_function::addition, [](const glm::ivec3& a, const glm::ivec3& b) { return a + b; },
        sol::meta_function::subtraction, [](const glm::ivec3& a, const glm::ivec3& b) { return a - b; },
        sol::meta_function::to_string, [](const glm::ivec3& v) {
            return "IVec3(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
        }
    );
}

void registerIVec4(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    voxel.new_usertype<glm::ivec4>("IVec4",
        sol::constructors<glm::ivec4(), glm::ivec4(int), glm::ivec4(int, int, int, int)>(),
        "x", &glm::ivec4::x,
        "y", &glm::ivec4::y,
        "z", &glm::ivec4::z,
        "w", &glm::ivec4::w
    );
}

void registerQuat(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    voxel.new_usertype<glm::quat>("Quat",
        sol::constructors<glm::quat(), glm::quat(float, float, float, float)>(),
        "x", &glm::quat::x,
        "y", &glm::quat::y,
        "z", &glm::quat::z,
        "w", &glm::quat::w,
        
        sol::meta_function::multiplication, sol::overload(
            [](const glm::quat& a, const glm::quat& b) { return a * b; },
            [](const glm::quat& q, const glm::vec3& v) { return q * v; }
        ),
        
        "normalize", [](const glm::quat& q) { return glm::normalize(q); },
        "inverse", [](const glm::quat& q) { return glm::inverse(q); },
        "eulerAngles", [](const glm::quat& q) { return glm::eulerAngles(q); },
        "rotateVector", [](const glm::quat& q, const glm::vec3& v) { return q * v; }
    );
    
    // Static factory methods
    voxel["Quat"]["identity"] = []() { return glm::quat(1.0f, 0.0f, 0.0f, 0.0f); };
    voxel["Quat"]["fromEuler"] = [](float x, float y, float z) {
        return glm::quat(glm::vec3(x, y, z));
    };
    voxel["Quat"]["fromAxisAngle"] = [](const glm::vec3& axis, float angle) {
        return glm::angleAxis(angle, glm::normalize(axis));
    };
    voxel["Quat"]["lookRotation"] = [](const glm::vec3& forward, const glm::vec3& up) {
        return glm::quatLookAt(glm::normalize(forward), glm::normalize(up));
    };
}

void registerMat4(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    voxel.new_usertype<glm::mat4>("Mat4",
        sol::constructors<glm::mat4()>(),
        
        sol::meta_function::multiplication, sol::overload(
            [](const glm::mat4& a, const glm::mat4& b) { return a * b; },
            [](const glm::mat4& m, const glm::vec4& v) { return m * v; }
        ),
        
        "inverse", [](const glm::mat4& m) { return glm::inverse(m); },
        "transpose", [](const glm::mat4& m) { return glm::transpose(m); }
    );
    
    // Static factory methods
    voxel["Mat4"]["identity"] = []() { return glm::mat4(1.0f); };
    voxel["Mat4"]["translate"] = [](const glm::vec3& v) { return glm::translate(glm::mat4(1.0f), v); };
    voxel["Mat4"]["rotate"] = [](float angle, const glm::vec3& axis) {
        return glm::rotate(glm::mat4(1.0f), angle, axis);
    };
    voxel["Mat4"]["scale"] = [](const glm::vec3& v) { return glm::scale(glm::mat4(1.0f), v); };
    voxel["Mat4"]["perspective"] = [](float fov, float aspect, float near, float far) {
        return glm::perspective(fov, aspect, near, far);
    };
    voxel["Mat4"]["ortho"] = [](float left, float right, float bottom, float top, float near, float far) {
        return glm::ortho(left, right, bottom, top, near, far);
    };
    voxel["Mat4"]["lookAt"] = [](const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up) {
        return glm::lookAt(eye, center, up);
    };
}

void registerLogger(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    auto log = lua.create_table();
    log["info"] = [](const std::string& msg) { Logger::info("{}", msg); };
    log["warning"] = [](const std::string& msg) { Logger::warn("{}", msg); };
    log["error"] = [](const std::string& msg) { Logger::error("{}", msg); };
    log["debug"] = [](const std::string& msg) { Logger::debug("{}", msg); };
    
    voxel["log"] = log;
}

void registerTimer(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    auto timer = lua.create_table();
    timer["time"] = []() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration<float>(duration).count();
    };
    timer["since"] = [](float start) {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        float nowf = std::chrono::duration<float>(duration).count();
        return nowf - start;
    };
    
    voxel["timer"] = timer;
}

void registerRandom(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    auto random = lua.create_table();
    random["int"] = [](int min, int max) {
        return min + (rand() % (max - min + 1));
    };
    random["float"] = [](float min, float max) {
        return min + (static_cast<float>(rand()) / RAND_MAX) * (max - min);
    };
    random["chance"] = [](float probability) {
        return (static_cast<float>(rand()) / RAND_MAX) < probability;
    };
    random["pick"] = [](sol::table t) {
        int size = t.size();
        if (size == 0) return sol::object();
        return t[1 + (rand() % size)];
    };
    random["shuffle"] = [](sol::table t) {
        int size = t.size();
        for (int i = size; i > 1; i--) {
            int j = 1 + (rand() % i);
            sol::object temp = t[i];
            t[i] = t[j];
            t[j] = temp;
        }
    };
    
    voxel["random"] = random;
}

void registerNoise(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    auto noise = lua.create_table();
    noise["perlin2"] = [](float x, float y) {
        // Simplified 2D noise
        return sin(x * 1.5f + y * 0.8f) * 0.5f + 0.5f;
    };
    noise["perlin3"] = [](float x, float y, float z) {
        return sin(x * 1.5f + y * 0.8f + z * 0.6f) * 0.5f + 0.5f;
    };
    noise["simplex2"] = [](float x, float y) {
        return sin(x * 1.2f + y * 1.5f) * 0.5f + 0.5f;
    };
    noise["simplex3"] = [](float x, float y, float z) {
        return sin(x * 1.2f + y * 1.5f + z * 0.9f) * 0.5f + 0.5f;
    };
    
    voxel["noise"] = noise;
}

// Placeholder implementations for other types
void registerBlock(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    // Block type registration
}

void registerBlockState(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    // BlockState type registration
}

void registerChunk(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    // Chunk type registration
}

void registerWorld(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    // World API namespace
    auto world = lua.create_table();
    
    world["getBlock"] = [](int x, int y, int z) -> std::string {
        // Placeholder
        return "minecraft:air";
    };
    
    world["setBlock"] = [](int x, int y, int z, const std::string& blockId) {
        // Placeholder
    };
    
    world["getHighestBlock"] = [](int x, int z) -> int {
        return 64;  // Placeholder
    };
    
    world["spawnParticle"] = [](const std::string& type, float x, float y, float z, sol::optional<sol::table> options) {
        // Placeholder
    };
    
    world["playSound"] = [](const std::string& sound, float x, float y, float z, sol::optional<float> volume, sol::optional<float> pitch) {
        // Placeholder
    };
    
    voxel["world"] = world;
}

void registerEntity(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    auto entity = lua.create_table();
    
    entity["spawn"] = [](const std::string& type, float x, float y, float z) -> uint32_t {
        return 0;  // Placeholder
    };
    
    entity["despawn"] = [](uint32_t id) {
        // Placeholder
    };
    
    entity["exists"] = [](uint32_t id) -> bool {
        return false;  // Placeholder
    };
    
    entity["getPosition"] = [](uint32_t id) -> glm::vec3 {
        return glm::vec3(0.0f);  // Placeholder
    };
    
    entity["setPosition"] = [](uint32_t id, float x, float y, float z) {
        // Placeholder
    };
    
    entity["damage"] = [](uint32_t id, float amount, sol::optional<std::string> source) {
        // Placeholder
    };
    
    entity["kill"] = [](uint32_t id) {
        // Placeholder
    };
    
    voxel["entity"] = entity;
}

void registerLivingEntity(sol::state& lua) {
    // Inherits from Entity
}

void registerPlayer(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    auto player = lua.create_table();
    
    player["getAll"] = []() -> sol::table {
        return sol::table();  // Placeholder
    };
    
    player["getByName"] = [](const std::string& name) -> uint32_t {
        return 0;  // Placeholder
    };
    
    player["sendMessage"] = [](uint32_t id, const std::string& message) {
        // Placeholder
    };
    
    player["teleport"] = [](uint32_t id, float x, float y, float z, sol::optional<float> yaw, sol::optional<float> pitch) {
        // Placeholder
    };
    
    player["kick"] = [](uint32_t id, const std::string& reason) {
        // Placeholder
    };
    
    voxel["player"] = player;
}

void registerMob(sol::state& lua) {
    // Mob type registration
}

void registerProjectile(sol::state& lua) {
    // Projectile type registration
}

void registerItemEntity(sol::state& lua) {
    // ItemEntity type registration
}

void registerItem(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    auto item = lua.create_table();
    
    item["exists"] = [](const std::string& id) -> bool {
        return true;  // Placeholder
    };
    
    item["getName"] = [](const std::string& id) -> std::string {
        return id;  // Placeholder
    };
    
    item["getMaxStackSize"] = [](const std::string& id) -> int {
        return 64;  // Placeholder
    };
    
    voxel["item"] = item;
}

void registerItemStack(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    // ItemStack type registration
}

void registerInventory(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    // Inventory type registration
}

void registerContainer(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    // Container type registration
}

void registerRecipe(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    // Recipe type registration
}

void registerCrafting(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    auto crafting = lua.create_table();
    
    crafting["addRecipe"] = [](sol::table recipe) {
        // Placeholder
    };
    
    crafting["getRecipes"] = [](const std::string& itemId) -> sol::table {
        return sol::table();  // Placeholder
    };
    
    voxel["crafting"] = crafting;
}

void registerAchievement(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    // Achievement type registration
}

void registerStatistic(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    // Statistic type registration
}

void registerPhysics(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    auto physics = lua.create_table();
    
    physics["raycast"] = [](float ox, float oy, float oz, float dx, float dy, float dz, float maxDist) -> sol::table {
        return sol::table();  // Placeholder
    };
    
    physics["overlapSphere"] = [](float x, float y, float z, float radius) -> sol::table {
        return sol::table();  // Placeholder
    };
    
    voxel["physics"] = physics;
}

void registerRaycastHit(sol::state& lua) {
    // RaycastHit type registration
}

void registerCollisionInfo(sol::state& lua) {
    // CollisionInfo type registration
}

void registerAudio(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    auto audio = lua.create_table();
    
    audio["play"] = [](const std::string& sound, float x, float y, float z, sol::optional<float> volume, sol::optional<float> pitch) {
        // Placeholder
    };
    
    audio["playMusic"] = [](const std::string& music) {
        // Placeholder
    };
    
    audio["stopAll"] = []() {
        // Placeholder
    };
    
    voxel["audio"] = audio;
}

void registerSound(sol::state& lua) {
    // Sound type registration
}

void registerNetwork(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    auto network = lua.create_table();
    
    network["sendTo"] = [](uint32_t playerId, const std::string& channel, sol::table data) {
        // Placeholder
    };
    
    network["sendToAll"] = [](const std::string& channel, sol::table data) {
        // Placeholder
    };
    
    network["registerChannel"] = [](const std::string& channel, sol::function callback) {
        // Placeholder
    };
    
    voxel["network"] = network;
}

void registerPacket(sol::state& lua) {
    // Packet type registration
}

void registerGUI(sol::state& lua) {
    auto voxel = lua["voxel"].get_or_create<sol::table>();
    
    auto gui = lua.create_table();
    
    gui["showScreen"] = [](uint32_t playerId, const std::string& screenId) {
        // Placeholder
    };
    
    gui["closeScreen"] = [](uint32_t playerId) {
        // Placeholder
    };
    
    voxel["gui"] = gui;
}

void registerWidget(sol::state& lua) {
    // Widget type registration
}

void registerButton(sol::state& lua) {
    // Button widget registration
}

void registerLabel(sol::state& lua) {
    // Label widget registration
}

void registerTextField(sol::state& lua) {
    // TextField widget registration
}

} // namespace LuaBindings

// LuaAPI implementations (placeholders that delegate to actual systems)

namespace LuaAPI {

// Global implementations
void Global::log(const std::string& message) {
    Logger::info("{}", message);
}

void Global::logWarning(const std::string& message) {
    Logger::warn("{}", message);
}

void Global::logError(const std::string& message) {
    Logger::error("{}", message);
}

float Global::getTime() {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float>(now.time_since_epoch()).count();
}

float Global::getDeltaTime() {
    return 1.0f / 60.0f;  // Placeholder
}

float Global::getFPS() {
    return 60.0f;  // Placeholder
}

void Global::schedule(float delay, sol::function callback) {
    // Placeholder - would use timer system
}

void Global::scheduleRepeating(float interval, sol::function callback) {
    // Placeholder - would use timer system
}

std::string Global::getVersion() {
    return "1.0.0";
}

bool Global::isServer() {
    return true;  // Placeholder
}

bool Global::isClient() {
    return false;  // Placeholder
}

bool Global::isDedicatedServer() {
    return true;  // Placeholder
}

// Additional placeholder implementations for other API functions...

} // namespace LuaAPI

} // namespace VoxelForge
