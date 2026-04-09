/**
 * @file Engine.cpp
 * @brief Main engine module
 */

#include <VoxelForge/Engine.hpp>
#include <spdlog/spdlog.h>

namespace VoxelForge {

void Engine::initialize() {
    spdlog::info("VoxelForge Engine initializing...");
    initialized_ = true;
}

void Engine::shutdown() {
    spdlog::info("VoxelForge Engine shutting down...");
    initialized_ = false;
}

Engine& Engine::get() {
    static Engine instance;
    return instance;
}

} // namespace VoxelForge