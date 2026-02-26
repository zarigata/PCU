/**
 * @file Engine.hpp
 * @brief Main engine header - includes all engine components
 * 
 * VoxelForge - C++ Minecraft Clone with Integrated Modding
 * Copyright (c) 2024 VoxelForge Team
 */

#pragma once

// Version Information
#define VOXELFORGE_VERSION_MAJOR 1
#define VOXELFORGE_VERSION_MINOR 0
#define VOXELFORGE_VERSION_PATCH 0
#define VOXELFORGE_VERSION_STRING "1.0.0"

// Platform Detection
#if defined(_WIN32) || defined(_WIN64)
    #define VOXELFORGE_PLATFORM_WINDOWS 1
    #define VOXELFORGE_PLATFORM_LINUX 0
    #define VOXELFORGE_PLATFORM_MACOS 0
#elif defined(__linux__)
    #define VOXELFORGE_PLATFORM_WINDOWS 0
    #define VOXELFORGE_PLATFORM_LINUX 1
    #define VOXELFORGE_PLATFORM_MACOS 0
#elif defined(__APPLE__)
    #define VOXELFORGE_PLATFORM_WINDOWS 0
    #define VOXELFORGE_PLATFORM_LINUX 0
    #define VOXELFORGE_PLATFORM_MACOS 1
#else
    #error "Unsupported platform"
#endif

// Compiler Detection
#if defined(__clang__)
    #define VOXELFORGE_COMPILER_CLANG 1
    #define VOXELFORGE_COMPILER_GCC 0
    #define VOXELFORGE_COMPILER_MSVC 0
#elif defined(__GNUC__)
    #define VOXELFORGE_COMPILER_CLANG 0
    #define VOXELFORGE_COMPILER_GCC 1
    #define VOXELFORGE_COMPILER_MSVC 0
#elif defined(_MSC_VER)
    #define VOXELFORGE_COMPILER_CLANG 0
    #define VOXELFORGE_COMPILER_GCC 0
    #define VOXELFORGE_COMPILER_MSVC 1
#else
    #error "Unsupported compiler"
#endif

// Export Macros
#if VOXELFORGE_PLATFORM_WINDOWS
    #ifdef VOXELFORGE_BUILD_DLL
        #define VOXELFORGE_API __declspec(dllexport)
    #else
        #define VOXELFORGE_API __declspec(dllimport)
    #endif
#else
    #define VOXELFORGE_API __attribute__((visibility("default")))
#endif

// Assert Macros
#ifdef VOXELFORGE_DEBUG
    #define VF_ASSERT(cond, msg) \
        do { \
            if (!(cond)) { \
                VoxelForge::Core::Logger::get()->error("Assertion failed: {} at {}:{}", msg, __FILE__, __LINE__); \
                std::abort(); \
            } \
        } while(0)
#else
    #define VF_ASSERT(cond, msg) ((void)0)
#endif

// Core includes
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <cmath>
#include <algorithm>

// GLM for math
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Logging
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

// JSON
#include <nlohmann/json.hpp>

namespace VoxelForge {

// Common type aliases
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

// String types
using String = std::string;
using StringView = std::string_view;

// UUID type (128-bit)
struct UUID {
    u64 low;
    u64 high;
    
    static UUID generate();
    static UUID fromString(const String& str);
    String toString() const;
    bool operator==(const UUID& other) const { return low == other.low && high == other.high; }
    bool operator!=(const UUID& other) const { return !(*this == other); }
};

// Common math types
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Vec2i = glm::ivec2;
using Vec3i = glm::ivec3;
using Vec4i = glm::ivec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;
using Quat = glm::quat;

// Block position type (integer coordinates)
using BlockPos = Vec3i;
using ChunkPos = Vec3i;

// Color type
struct Color {
    f32 r, g, b, a;
    
    Color() : r(0), g(0), b(0), a(1) {}
    Color(f32 r, f32 g, f32 b, f32 a = 1.0f) : r(r), g(g), b(b), a(a) {}
    
    static Color fromHex(u32 hex);
    static Color fromRGB(u8 r, u8 g, u8 b, u8 a = 255);
    
    u32 toHex() const;
};

// Rectangle type
struct Rect {
    f32 x, y, width, height;
    
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(f32 x, f32 y, f32 w, f32 h) : x(x), y(y), width(w), height(h) {}
    
    bool contains(f32 px, f32 py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }
};

// AABB (Axis-Aligned Bounding Box)
struct AABB {
    Vec3 min;
    Vec3 max;
    
    AABB() = default;
    AABB(const Vec3& min, const Vec3& max) : min(min), max(max) {}
    AABB(f32 minX, f32 minY, f32 minZ, f32 maxX, f32 maxY, f32 maxZ)
        : min(minX, minY, minZ), max(maxX, maxY, maxZ) {}
    
    Vec3 getCenter() const { return (min + max) * 0.5f; }
    Vec3 getSize() const { return max - min; }
    Vec3 getExtents() const { return getSize() * 0.5f; }
    
    bool contains(const Vec3& point) const;
    bool intersects(const AABB& other) const;
    AABB expand(f32 amount) const;
    AABB translate(const Vec3& offset) const;
    
    static AABB blockBox(const BlockPos& pos);
};

// Smart pointer aliases
template<typename T>
using Scope = std::unique_ptr<T>;
template<typename T>
using Ref = std::shared_ptr<T>;
template<typename T>
using WeakRef = std::weak_ptr<T>;

template<typename T, typename... Args>
Scope<T> createScope(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
Ref<T> createRef(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// Defer macro for cleanup
template<typename F>
class Defer {
    F func;
public:
    explicit Defer(F&& f) : func(std::forward<F>(f)) {}
    ~Defer() { func(); }
};

#define VF_DEFER_IMPL(x, y) x##y
#define VF_DEFER(x) auto VF_DEFER_IMPL(defer_, __LINE__) = VoxelForge::Defer([&]() { x; })

} // namespace VoxelForge

// Core systems
#include "core/Logger.hpp"
#include "core/Timer.hpp"
#include "core/Memory.hpp"

// Engine systems
#include "engine/EventSystem.hpp"
#include "engine/ResourceManager.hpp"
#include "engine/JobSystem.hpp"
#include "engine/Config.hpp"

// World systems
#include "world/Block.hpp"
#include "world/BlockRegistry.hpp"
#include "world/Chunk.hpp"
#include "world/World.hpp"

// Entity systems
#include "entity/Entity.hpp"
#include "entity/EntityManager.hpp"

// Rendering
#include "rendering/Renderer.hpp"
#include "rendering/Camera.hpp"

// Physics
#include "physics/PhysicsSystem.hpp"

// Audio
#include "audio/AudioSystem.hpp"

// Networking
#include "networking/NetworkManager.hpp"

// Scripting
#include "scripting/LuaEngine.hpp"

// Modding
#include "modding/ModLoader.hpp"

// Game
#include "game/Game.hpp"
