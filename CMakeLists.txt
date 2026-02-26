cmake_minimum_required(VERSION 3.26)

# Project Definition
project(VoxelForge
    VERSION 1.0.0
    DESCRIPTION "C++ Minecraft Clone with Integrated Modding"
    LANGUAGES CXX C
)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Options
option(VOXELFORGE_BUILD_TESTS "Build unit tests" ON)
option(VOXELFORGE_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(VOXELFORGE_BUILD_TOOLS "Build development tools" ON)
option(VOXELFORGE_ENABLE_SANITIZERS "Enable address/undefined sanitizers" OFF)
option(VOXELFORGE_ENABLE_PROFILING "Enable profiling support" OFF)
option(VOXELFORGE_USE_LUAJIT "Use LuaJIT instead of PUC Lua" ON)

# Output Directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================
# Dependencies
# ============================================

include(FetchContent)

# Vulkan
find_package(Vulkan REQUIRED)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8
)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# stb (header-only)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

# tinygltf
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG        v2.8.21
)
FetchContent_MakeAvailable(tinygltf)

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# Zstandard
FetchContent_Declare(
    zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG        v1.5.5
    SOURCE_SUBDIR  build/cmake
)
FetchContent_MakeAvailable(zstd)

# ENet
FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG        v1.3.17
)
FetchContent_MakeAvailable(enet)

# Lua
if(VOXELFORGE_USE_LUAJIT)
    FetchContent_Declare(
        luajit
        GIT_REPOSITORY https://github.com/LuaJIT/LuaJIT.git
        GIT_TAG        v2.1
    )
    # LuaJIT has custom build, handle separately
else()
    FetchContent_Declare(
        lua
        GIT_REPOSITORY https://github.com/lua/lua.git
        GIT_TAG        v5.4.6
    )
    # Lua has custom build, handle separately
endif()

# FreeType
find_package(Freetype REQUIRED)

# ============================================
# Compiler Flags
# ============================================

if(MSVC)
    add_compile_options(
        /W4
        /WX-
        /MP
        /EHsc
        /utf-8
    )
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -fPIC
    )
    
    if(VOXELFORGE_ENABLE_SANITIZERS)
        add_compile_options(-fsanitize=address,undefined)
        add_link_options(-fsanitize=address,undefined)
    endif()
    
    if(VOXELFORGE_ENABLE_PROFILING)
        add_compile_options(-pg)
        add_link_options(-pg)
    endif()
endif()

# ============================================
# VoxelForge Engine Library
# ============================================

add_library(VoxelForge_Engine STATIC
    # Core
    src/core/Engine.cpp
    src/core/Application.cpp
    src/core/Window.cpp
    src/core/Input.cpp
    src/core/Logger.cpp
    src/core/Memory.cpp
    src/core/Timer.cpp
    
    # Engine Systems
    src/engine/EventSystem.cpp
    src/engine/ResourceManager.cpp
    src/engine/JobSystem.cpp
    src/engine/Config.cpp
    
    # World
    src/world/World.cpp
    src/world/Chunk.cpp
    src/world/ChunkSection.cpp
    src/world/BlockRegistry.cpp
    src/world/BlockState.cpp
    src/world/WorldGenerator.cpp
    src/world/BiomeRegistry.cpp
    src/world/DimensionRegistry.cpp
    src/world/ChunkMesher.cpp
    src/world/ChunkManager.cpp
    src/world/LightEngine.cpp
    src/world/AnvilLoader.cpp
    
    # Entity
    src/entity/EntityManager.cpp
    src/entity/EntityBase.cpp
    src/entity/LivingEntity.cpp
    src/entity/Player.cpp
    src/entity/MobBase.cpp
    src/entity/ProjectileEntity.cpp
    src/entity/ItemEntity.cpp
    
    # Rendering
    src/rendering/VulkanContext.cpp
    src/rendering/VulkanDevice.cpp
    src/rendering/VulkanSwapchain.cpp
    src/rendering/VulkanPipeline.cpp
    src/rendering/VulkanCommandBuffer.cpp
    src/rendering/VulkanBuffer.cpp
    src/rendering/VulkanImage.cpp
    src/rendering/VulkanDescriptor.cpp
    src/rendering/VulkanRenderPass.cpp
    src/rendering/VulkanFramebuffer.cpp
    src/rendering/VulkanSync.cpp
    src/rendering/Renderer.cpp
    src/rendering/ChunkRenderer.cpp
    src/rendering/EntityRenderer.cpp
    src/rendering/ParticleRenderer.cpp
    src/rendering/GUIRenderer.cpp
    src/rendering/TextureAtlas.cpp
    src/rendering/ShaderManager.cpp
    src/rendering/PostProcessor.cpp
    src/rendering/Camera.cpp
    src/rendering/SkyRenderer.cpp
    src/rendering/ShadowRenderer.cpp
    
    # Physics
    src/physics/PhysicsSystem.cpp
    src/physics/CharacterController.cpp
    src/physics/CollisionManager.cpp
    
    # Audio
    src/audio/AudioSystem.cpp
    src/audio/SoundManager.cpp
    
    # Networking
    src/networking/NetworkManager.cpp
    src/networking/Server.cpp
    src/networking/Client.cpp
    src/networking/Packet.cpp
    src/networking/Connection.cpp
    
    # Scripting
    src/scripting/LuaEngine.cpp
    src/scripting/LuaBindings.cpp
    
    # Modding
    src/modding/ModLoader.cpp
    src/modding/ModContext.cpp
    src/modding/ModRegistry.cpp
    src/modding/NativePlugin.cpp
    
    # Game
    src/game/Game.cpp
    src/game/GameState.cpp
    src/game/Inventory.cpp
    src/game/CraftingSystem.cpp
    src/game/RecipeRegistry.cpp
    src/game/CommandManager.cpp
    src/game/AchievementSystem.cpp
    src/game/Statistics.cpp
    
    # Utils
    src/utils/Noise.cpp
    src/utils/NBT.cpp
    src/utils/Compression.cpp
    src/utils/Random.cpp
    src/utils/Profiler.cpp
)

target_include_directories(VoxelForge_Engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/VoxelForge
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(VoxelForge_Engine
    PUBLIC
        Vulkan::Vulkan
        glfw
        glm::glm
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        zstd::zstd_static
        enet::enet
        Freetype::Freetype
)

target_compile_definitions(VoxelForge_Engine
    PUBLIC
        VOXELFORGE_VERSION="${PROJECT_VERSION}"
        VOXELFORGE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        VOXELFORGE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        VOXELFORGE_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# ============================================
# Main Executable
# ============================================

add_executable(VoxelForge
    src/main.cpp
)

target_link_libraries(VoxelForge
    PRIVATE VoxelForge_Engine
)

# ============================================
# Dedicated Server
# ============================================

add_executable(VoxelForge_Server
    src/server_main.cpp
)

target_link_libraries(VoxelForge_Server
    PRIVATE VoxelForge_Engine
)

target_compile_definitions(VoxelForge_Server
    PRIVATE VOXELFORGE_DEDICATED_SERVER
)

# ============================================
# Tests
# ============================================

if(VOXELFORGE_BUILD_TESTS)
    enable_testing()
    
    # GoogleTest
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    
    add_subdirectory(tests)
endif()

# ============================================
# Benchmarks
# ============================================

if(VOXELFORGE_BUILD_BENCHMARKS)
    # Google Benchmark
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.8.3
    )
    FetchContent_MakeAvailable(benchmark)
    
    add_subdirectory(tests/benchmark)
endif()

# ============================================
# Development Tools
# ============================================

if(VOXELFORGE_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

# ============================================
# Install
# ============================================

install(TARGETS VoxelForge VoxelForge_Server
    RUNTIME DESTINATION bin
)

install(TARGETS VoxelForge_Engine
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/VoxelForge
    DESTINATION include
)

install(DIRECTORY assets
    DESTINATION share/VoxelForge
)

# ============================================
# Package
# ============================================

set(CPACK_PACKAGE_NAME "VoxelForge")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_CONTACT "team@voxelforge.org")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
else()
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
endif()

include(CPack)
