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
    
    BlockRegistry::get().registerVanillaBlocks();
    VF_CORE_INFO("Block registry: {} blocks registered", BlockRegistry::get().getBlockCount());
    
    ecsWorld = std::make_unique<ECSWorld>();
    
    world = std::make_unique<World>(0);
    VF_CORE_INFO("World created with seed 0");
    
    camera.setPerspective(settings.fov,
                          static_cast<float>(getWindow().getWidth()) / static_cast<float>(getWindow().getHeight()),
                          0.1f, 1000.0f);
    camera.setPosition(playerPos);
    
    try {
        renderer.init(getWindow().getGLFWWindow());
        renderer.initChunkRendering();
        rendererInitialized = true;
        VF_CORE_INFO("Renderer initialized");
    } catch (const std::exception& e) {
        VF_ERROR("Failed to initialize renderer: {}", e.what());
        VF_ERROR("Game will run without rendering");
    }
    
    getWindow().disableCursor();
    
    VF_CORE_INFO("Game initialized");
}

void Game::onShutdown() {
    VF_CORE_INFO("Shutting down game...");
    
    if (rendererInitialized) {
        renderer.cleanupChunkRendering();
        renderer.shutdown();
    }
    
    world.reset();
    ecsWorld.reset();
    
    VF_CORE_INFO("Game shut down");
}

void Game::onUpdate(float deltaTime) {
    if (paused) {
        return;
    }
    
    if (world) {
        world->setDayTime(world->getDayTime() + deltaTime * 100.0f);
        world->updateChunks(playerPos);
    }
    
    if (rendererInitialized && world) {
        renderer.generateAndUploadChunks(world.get());
    }
    
    gameTime += deltaTime;
    
    static float tickAccumulator = 0.0f;
    tickAccumulator += deltaTime;
    
    while (tickAccumulator >= 0.05f) {
        updateEntities(0.05f);
        updateWorld(0.05f);
        tickCount++;
        tickAccumulator -= 0.05f;
    }
    
    processInput(deltaTime);
    
    if (ecsWorld) {
        ecsWorld->updateSystems(deltaTime);
    }
}

void Game::onRender() {
    if (!rendererInitialized) return;
    
    float dayTime = world ? world->getDayTime() : 0.0f;
    float t = std::fmod(dayTime, 24000.0f) / 24000.0f;
    float skyR = 0.53f, skyG = 0.81f, skyB = 0.98f;
    if (t > 0.5f) {
        float night = (t - 0.5f) * 2.0f;
        if (night > 1.0f) night = 2.0f - night;
        skyR = 0.53f * (1.0f - night * 0.9f);
        skyG = 0.81f * (1.0f - night * 0.9f);
        skyB = 0.98f * (1.0f - night * 0.7f);
    }
    
    try {
        renderer.setClearColor(skyR, skyG, skyB);
        renderer.beginFrame();
        if (world) {
            renderer.renderWorldChunks(world.get(), &camera);
        }
        renderer.endFrame();
    } catch (const std::exception& e) {
        VF_ERROR("Render error: {}", e.what());
    }
}

void Game::processInput(float deltaTime) {
    auto& input = getInput();
    
    if (input.isKeyJustPressed(Key::Escape)) {
        togglePause();
        if (paused) {
            getWindow().showCursor();
        } else {
            getWindow().disableCursor();
        }
    }
    
    if (paused) return;
    
    float speed = 10.0f * deltaTime;
    if (input.isActionPressed("sprint")) speed *= 2.0f;
    
    glm::vec3 forward = camera.getForward();
    forward.y = 0.0f;
    forward = glm::normalize(forward);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    
    if (input.isActionPressed("forward"))  playerPos += forward * speed;
    if (input.isActionPressed("backward")) playerPos -= forward * speed;
    if (input.isActionPressed("left"))     playerPos -= right * speed;
    if (input.isActionPressed("right"))    playerPos += right * speed;
    if (input.isActionPressed("jump"))     playerPos.y += speed;
    if (input.isActionPressed("sneak"))    playerPos.y -= speed;
    
    auto mouseDelta = input.getMouseDelta();
    if (glm::length(mouseDelta) > 0.0f) {
        playerYaw -= mouseDelta.x * settings.mouseSensitivity;
        playerPitch -= mouseDelta.y * settings.mouseSensitivity;
        playerPitch = std::clamp(playerPitch, -89.0f, 89.0f);
    }
    
    camera.setPosition(playerPos);
    camera.setRotation(playerPitch, playerYaw);
}

void Game::updateEntities(float deltaTime) {
}

void Game::updateWorld(float deltaTime) {
}

float Game::getDayProgress() const {
    if (!world) return 0.0f;
    float dayTime = world->getDayTime();
    return std::fmod(dayTime, 24000.0f) / 24000.0f;
}

void Game::advanceDayNightCycle() {
    if (!world) return;
    float dayTime = world->getDayTime() + settings.dayNightCycleDuration;
    while (dayTime >= 24000.0f) dayTime -= 24000.0f;
    world->setDayTime(dayTime);
}

void Game::resetDayNightCycle() {
    if (!world) return;
    world->setDayTime(0.0f);
}

bool Game::isDay() const {
    if (!world) return true;
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

Application* createApplication() {
    return new Game();
}

} // namespace VoxelForge
