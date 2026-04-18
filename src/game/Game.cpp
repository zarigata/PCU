#include <VoxelForge/game/Game.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <VoxelForge/core/Input.hpp>
#include <VoxelForge/core/Timer.hpp>
#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/world/BlockRegistry.hpp>
#include <cmath>
#include <algorithm>

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
        renderer.generateAndUploadChunks(world.get(), playerPos);
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
        if (!paused) {
            renderer.drawCrosshair();
            renderer.drawHotbar(selectedSlot, hotbarBlocks);
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
    
    for (int i = 0; i < 9; i++) {
        if (input.isKeyJustPressed(49 + i)) {
            selectedSlot = i;
        }
    }
    
    handleBlockInteraction();
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

glm::ivec3 Game::raycastBlocks(const glm::vec3& origin, const glm::vec3& direction, float maxDist, glm::ivec3& hitNormal) {
    glm::vec3 pos = origin;
    glm::vec3 dir = glm::normalize(direction);
    
    int x = (int)floor(pos.x);
    int y = (int)floor(pos.y);
    int z = (int)floor(pos.z);
    
    int stepX = dir.x > 0 ? 1 : (dir.x < 0 ? -1 : 0);
    int stepY = dir.y > 0 ? 1 : (dir.y < 0 ? -1 : 0);
    int stepZ = dir.z > 0 ? 1 : (dir.z < 0 ? -1 : 0);
    
    float tMaxX = (dir.x != 0) ? ((dir.x > 0 ? (x + 1 - pos.x) : (pos.x - x)) / fabs(dir.x)) : 1e30f;
    float tMaxY = (dir.y != 0) ? ((dir.y > 0 ? (y + 1 - pos.y) : (pos.y - y)) / fabs(dir.y)) : 1e30f;
    float tMaxZ = (dir.z != 0) ? ((dir.z > 0 ? (z + 1 - pos.z) : (pos.z - z)) / fabs(dir.z)) : 1e30f;
    
    float tDeltaX = (dir.x != 0) ? fabs(1.0f / dir.x) : 1e30f;
    float tDeltaY = (dir.y != 0) ? fabs(1.0f / dir.y) : 1e30f;
    float tDeltaZ = (dir.z != 0) ? fabs(1.0f / dir.z) : 1e30f;
    
    hitNormal = glm::ivec3(0, 0, 0);
    float dist = 0.0f;
    
    for (int i = 0; i < 200 && dist < maxDist; i++) {
        if (world) {
            auto block = world->getBlock(x, y, z);
            if (!block.isAir()) {
                return glm::ivec3(x, y, z);
            }
        }
        
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                dist = tMaxX;
                x += stepX;
                tMaxX += tDeltaX;
                hitNormal = glm::ivec3(-stepX, 0, 0);
            } else {
                dist = tMaxZ;
                z += stepZ;
                tMaxZ += tDeltaZ;
                hitNormal = glm::ivec3(0, 0, -stepZ);
            }
        } else {
            if (tMaxY < tMaxZ) {
                dist = tMaxY;
                y += stepY;
                tMaxY += tDeltaY;
                hitNormal = glm::ivec3(0, -stepY, 0);
            } else {
                dist = tMaxZ;
                z += stepZ;
                tMaxZ += tDeltaZ;
                hitNormal = glm::ivec3(0, 0, -stepZ);
            }
        }
    }
    
    return glm::ivec3(INT_MAX, INT_MAX, INT_MAX);
}

void Game::handleBlockInteraction() {
    if (!world || paused) return;
    
    auto& input = getInput();
    
    bool leftDown = input.isMouseButtonPressed(0);
    bool rightDown = input.isMouseButtonPressed(1);
    
    glm::ivec3 hitNormal;
    glm::ivec3 hitBlock = raycastBlocks(camera.getPosition(), camera.getForward(), 6.0f, hitNormal);
    
    if (hitBlock.x == INT_MAX) return;
    
    if (leftDown && !leftMousePressed) {
        world->setBlock(hitBlock.x, hitBlock.y, hitBlock.z, BlockState());
        int cx = (int)floor((float)hitBlock.x / 16.0f);
        int cz = (int)floor((float)hitBlock.z / 16.0f);
        renderer.invalidateChunkMesh(cx, cz);
        if (hitNormal.x != 0 && (hitBlock.x & 15) == (hitNormal.x > 0 ? 0 : 15)) {
            renderer.invalidateChunkMesh(cx - hitNormal.x, cz);
        }
        if (hitNormal.z != 0 && (hitBlock.z & 15) == (hitNormal.z > 0 ? 0 : 15)) {
            renderer.invalidateChunkMesh(cx, cz - hitNormal.z);
        }
    }
    
    if (rightDown && !rightMousePressed) {
        glm::ivec3 placePos = hitBlock + hitNormal;
        if (placePos.y >= -64 && placePos.y < 320) {
            uint32_t blockId = hotbarBlocks[selectedSlot];
            if (blockId != 0) {
                auto blockState = BlockRegistry::get().getDefaultState(blockId);
                world->setBlock(placePos.x, placePos.y, placePos.z, blockState);
                int cx = (int)floor((float)placePos.x / 16.0f);
                int cz = (int)floor((float)placePos.z / 16.0f);
                renderer.invalidateChunkMesh(cx, cz);
            }
        }
    }
    
    leftMousePressed = leftDown;
    rightMousePressed = rightDown;
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
