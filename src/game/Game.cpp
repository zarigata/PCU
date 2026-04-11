/**
 * @file Game.cpp
 * @brief Game implementation
 */

#include <VoxelForge/game/Game.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <VoxelForge/core/Input.hpp>
#include <VoxelForge/core/Timer.hpp>
#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/world/World.hpp>

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
    
    // Update day/night cycle time
    settings.dayTime += deltaTime;
    
    // Check for day/night cycle advancement (every 20 minutes = 12000 ticks)
    float cycleProgress = getDayProgress();
    if (cycleProgress >= 1.0f) {
        // Day ended, transition to night
        resetDayNightCycle();
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

float Game::getDayProgress() const {
    if (!world) return 0.0f;
    
    // Calculate progress through current day (0.0 = dawn, 1.0 = full day)
    // Based on SkyRenderer's time of day: Dawn=0-6000, Day=6000-12000, Dusk=12000-18000, Night=18000-24000
    // Normalize to 0.0-1.0 range
    float dayTime = world->getDayTime();
    float normalizedTime = std::fmod(dayTime, 24000.0f) / 24000.0f;
    
    // Map phases to progress:
    // 0.0 - 0.25: Dawn
    // 0.25 - 0.75: Day
    // 0.75 - 1.0: Dusk
    // 1.0+: Night
    
    if (normalizedTime < 0.25f) {
        return normalizedTime / 0.25f;  // 0.0 to 1.0 during dawn
    } else if (normalizedTime < 0.75f) {
        return 0.25f + (normalizedTime - 0.25f) / 0.5f;  // 0.25 to 1.0 during day
    } else if (normalizedTime < 1.0f) {
        return 0.75f + (normalizedTime - 0.75f) / 0.25f;  // 0.75 to 1.0 during dusk
    }
    // Night phase - can go beyond 1.0 but clamped later
    return std::min(1.0f, normalizedTime - 1.0f);  // Night starts at 1.0 and goes to 2.0
}

void Game::advanceDayNightCycle() {
    if (!world) return;
    
    // Advance to next day/night phase by adding one cycle duration
    float dayTime = world->getDayTime() + settings.dayNightCycleDuration;
    
    // Normalize to keep within 0-24000 range
    while (dayTime >= 24000.0f) {
        dayTime -= 24000.0f;
    }
    
    world->setDayTime(dayTime);
    
    // Cycle: Day -> Night -> Day...
    VF_INFO("Day/Night cycle advanced: new time = {}", dayTime);
}

void Game::resetDayNightCycle() {
    if (!world) return;
    
    // Reset to dawn (start of day)
    world->setDayTime(0.0f);
    VF_INFO("Day/Night cycle reset to dawn");
}

bool Game::isDay() const {
    if (!world) return true; // Default to day if no world
    
    // Use world's isDay() method
    return world->isDay();
}

bool Game::isNight() const {
    return !isDay();
}

float Game::getDayTime() const {
    if (!world) return 0.0f;
    return world->getDayTime();
}

void Game::setDayTime(float time) {
    if (!world) return;
    world->setDayTime(time);
}

    props.windowWidth = 1280;
    props.windowHeight = 720;
    props.vsync = true;
    return new Game(props);
}

} // namespace VoxelForge
