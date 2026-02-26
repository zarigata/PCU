/**
 * @file Engine.cpp
 * @brief Main engine module
 */

#include <VoxelForge/Engine.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

void Engine::initialize() {
    LOG_INFO("VoxelForge Engine initializing...");
    initialized_ = true;
}

void Engine::shutdown() {
    LOG_INFO("VoxelForge Engine shutting down...");
    initialized_ = false;
}

Engine& Engine::get() {
    static Engine instance;
    return instance;
}

} // namespace VoxelForge
