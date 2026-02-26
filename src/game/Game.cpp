/**
 * @file Game.cpp
 * @brief Game implementation
 */

#include <VoxelForge/game/Game.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <VoxelForge/core/Input.hpp>
#include <VoxelForge/core/Timer.hpp>
#include <VoxelForge/world/Block.hpp>

namespace VoxelForge {

Game::Game(const ApplicationProps& props) : Application(props) {
}

Game::~Game() {
}

void Game::onInit() {
    VF_CORE_INFO("Initializing game...");
    
    // Initialize block registry with vanilla blocks
    BlockRegistry::get().registerVanillaBlocks();
    VF_CORE_INFO("Block registry: {} blocks registered", BlockRegistry::get().getBlockCount());
    
    // Initialize ECS world
    ecsWorld = std::make_unique<ECSWorld>();
    
    // TODO: Initialize world
    // world = std::make_unique<World>(0);  // seed = 0
    
    VF_CORE_INFO("Game initialized");
}

void Game::onShutdown() {
    VF_CORE_INFO("Shutting down game...");
    
    world.reset();
    ecsWorld.reset();
    
    VF_CORE_INFO("Game shut down");
}

void Game::onUpdate(float deltaTime) {
    if (paused) {
        return;
    }
    
    gameTime += deltaTime;
    
    // Game tick (20 TPS)
    static float tickAccumulator = 0.0f;
    tickAccumulator += deltaTime;
    
    while (tickAccumulator >= 0.05f) {
        // Game tick
        updateEntities(0.05f);
        updateWorld(0.05f);
        tickCount++;
        tickAccumulator -= 0.05f;
    }
    
    // Process input
    processInput(deltaTime);
    
    // Update ECS systems
    ecsWorld->updateSystems(deltaTime);
}

void Game::onRender() {
    // TODO: Vulkan rendering
    // For now, just clear to sky blue
}

void Game::processInput(float deltaTime) {
    auto& input = getInput();
    
    // Toggle pause
    if (input.isKeyJustPressed(Key::Escape)) {
        togglePause();
        if (paused) {
            getWindow().showCursor();
        } else {
            getWindow().disableCursor();
        }
    }
    
    if (paused) return;
    
    // TODO: Player movement
    // float speed = 5.0f * deltaTime;
    // 
    // if (input.isActionPressed("sprint")) speed *= 1.5f;
    // 
    // if (input.isActionPressed("forward"))  player.move(0, 0, -speed);
    // if (input.isActionPressed("backward")) player.move(0, 0, speed);
    // if (input.isActionPressed("left"))     player.move(-speed, 0, 0);
    // if (input.isActionPressed("right"))    player.move(speed, 0, 0);
    // if (input.isActionPressed("jump"))     player.jump();
    // if (input.isActionPressed("sneak"))    player.sneak();
    
    // Mouse look
    auto mouseDelta = input.getMouseDelta();
    if (glm::length(mouseDelta) > 0.0f) {
        // TODO: Apply to camera
        // camera.rotate(mouseDelta.x * settings.mouseSensitivity,
        //               mouseDelta.y * settings.mouseSensitivity);
    }
}

void Game::updateEntities(float deltaTime) {
    // Update all entities
    // This is handled by ECS systems
}

void Game::updateWorld(float deltaTime) {
    if (world) {
        // world->tick();
    }
}

// Game factory function
Application* createApplication() {
    ApplicationProps props;
    props.name = "VoxelForge";
    props.windowWidth = 1280;
    props.windowHeight = 720;
    props.vsync = true;
    return new Game(props);
}

} // namespace VoxelForge
