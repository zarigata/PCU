#include <VoxelForge/game/Game.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <VoxelForge/core/Input.hpp>
#include <VoxelForge/core/Timer.hpp>
#include <VoxelForge/core/Diagnostics.hpp>
#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/world/BlockRegistry.hpp>
#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/engine/JobSystem.hpp>
#include <GLFW/glfw3.h>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <thread>
#include <chrono>

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
    loadWorld();
    
    world->updateChunks(glm::vec3(0.0f));
    int spawnHeight = world->getHeight(0, 0);
    float spawnY = static_cast<float>(spawnHeight + 5);
    
    auto blockBelow = world->getBlock(0, spawnHeight, 0);
    auto blockAtSpawn = world->getBlock(0, static_cast<int>(spawnY), 0);
    VF_CORE_INFO("Spawn: terrainSurface={}, spawnY={:.0f}, blockAtSurface={} solid={}, blockAtSpawn={} solid={}",
                 spawnHeight, spawnY,
                 blockBelow.getBlockId(), blockBelow.isSolid(),
                 blockAtSpawn.getBlockId(), blockAtSpawn.isSolid());
    
    playerPos = glm::vec3(0.0f, spawnY, 0.0f);
    
    camera.setPerspective(settings.fov,
                          static_cast<float>(getWindow().getWidth()) / static_cast<float>(getWindow().getHeight()),
                          0.1f, 1000.0f);
    camera.setPosition(playerPos + glm::vec3(0.0f, PLAYER_HEIGHT * 0.85f, 0.0f));
    try {
        renderer.init(getWindow().getGLFWWindow());
        renderer.initChunkRendering();
        rendererInitialized = true;
        VF_CORE_INFO("Renderer initialized");
    } catch (const std::exception& e) {
        VF_ERROR("Failed to initialize renderer: {}", e.what());
        VF_ERROR("Game will run without rendering");
    }
    
    glfwSetInputMode(getWindow().getGLFWWindow(), GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(getWindow().getGLFWWindow(), GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
    glfwFocusWindow(getWindow().getGLFWWindow());
    VF_CORE_INFO("Game initialized — click to capture cursor");
}

void Game::onShutdown() {
    VF_CORE_INFO("Shutting down game...");
    
    saveWorld();
    
    asyncWorker_.shutdown();
    ShutdownJobSystem();
    
    if (rendererInitialized) {
        renderer.cleanupChunkRendering();
        renderer.shutdown();
    }
    
    world.reset();
    ecsWorld.reset();
    
    VF_CORE_INFO("Game shut down");
}

void Game::onUpdate(float deltaTime) {
    if (rendererInitialized) {
        renderer.waitForPendingUpload();
    }

    processInput(deltaTime);
    
    if (paused) {
        return;
    }
    
    if (world) {
        world->setDayTime(world->getDayTime() + deltaTime * 100.0f);
        world->updateChunks(playerPos);
    }
    
    if (playerPos.y < -64.0f) {
        int surfaceY = world ? world->getHeight((int)playerPos.x, (int)playerPos.z) : 80;
        playerPos.y = static_cast<float>(surfaceY + 5);
        playerVelocity = glm::vec3(0.0f);
        onGround = false;
    }
    
    float aspect = getWindow().getAspectRatio();
    float farPlane = (float)(settings.renderDistance * 16) * 1.5f;
    camera.setPerspective(settings.fov, aspect, 0.1f, farPlane);
    
    if (rendererInitialized && world) {
        auto& diag = Diagnostics::get();

        diag.beginSection("chunkUpdate");

        if (pendingUploads_.size() < 8) {
            asyncWorker_.update(world.get(), playerPos, settings.renderDistance, {});
        }

        {
            auto completed = asyncWorker_.pollCompleted(64);
            pendingUploads_.insert(pendingUploads_.end(),
                std::make_move_iterator(completed.begin()),
                std::make_move_iterator(completed.end()));
        }

        int uploadBytes = 0;
        int uploadedCount = 0;
        if (!pendingUploads_.empty()) {
            diag.beginSection("upload");
            renderer.submitUploadBatch({pendingUploads_.begin(), pendingUploads_.begin() + 1});
            uploadBytes = (int)(pendingUploads_[0].vertices.size() * sizeof(float) +
                                pendingUploads_[0].indices.size() * sizeof(uint32_t));
            uploadedCount = 1;
            pendingUploads_.erase(pendingUploads_.begin());
            diag.endSection("upload");
        }

        static int cleanupCounter = 0;
        if (++cleanupCounter >= 8) {
            cleanupCounter = 0;
            renderer.evictDistantMeshes(playerPos);
            auto pending = asyncWorker_.getPendingSnapshot();
            world->unloadDistantChunks(playerPos, settings.renderDistance,
                [&pending](int cx, int cz) {
                    uint64_t key = (uint64_t)(uint32_t)cx | ((uint64_t)(uint32_t)cz << 40);
                    return pending.count(key) > 0;
                });
        }
        diag.endSection("chunkUpdate");

        if (diag.config().enabled) {
            diag.currentFrame().chunksVisible = (int)renderer.getChunkMeshKeys().size();
            diag.currentFrame().chunksLoaded = (int)world->getLoadedChunkCount();
            diag.currentFrame().chunksUploadedThisFrame = uploadedCount;
            diag.currentFrame().uploadBytesThisFrame = uploadBytes;
        }
    }
    
    gameTime += deltaTime;
    
    fpsUpdateTimer += deltaTime;
    frameCount++;
    if (fpsUpdateTimer >= 0.5f) {
        currentFPS = (float)frameCount / fpsUpdateTimer;
        frameCount = 0;
        fpsUpdateTimer = 0.0f;
        getWindow().setTitle("VoxelForge - " + std::to_string((int)currentFPS) + " FPS" +
            (flyMode ? " [FLY]" : " [WALK]") +
            " | XYZ: " + std::to_string((int)playerPos.x) + ", " + 
            std::to_string((int)playerPos.y) + ", " + std::to_string((int)playerPos.z));
    }
    
    static float tickAccumulator = 0.0f;
    tickAccumulator += deltaTime;
    
    while (tickAccumulator >= 0.05f) {
        updateEntities(0.05f);
        updateWorld(0.05f);
        tickCount++;
        tickAccumulator -= 0.05f;
    }
    
    if (ecsWorld) {
        ecsWorld->updateSystems(deltaTime);
    }
    
    if (rendererInitialized) {
        renderer.getSettings().renderDistance = settings.renderDistance;
        renderer.getSettings().enableVsync = settings.vsync;
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
            renderer.renderWorldChunks(world.get(), &camera, skyR, skyG, skyB);
        }
        renderer.drawClouds(&camera, gameTime);
        renderer.resetUIBatch();
        if (!cursorCaptured) {
            renderer.drawClickToPlay();
        } else if (!paused) {
            renderer.drawCrosshair();
            renderer.drawHotbar(selectedSlot, hotbarBlocks);
        } else {
            UIMenuState menu;
            menu.selected = pauseMenuSelection;
            menu.hovered = hoveredMenuItem;
            menu.fov = settings.fov;
            menu.sensitivity = settings.mouseSensitivity;
            menu.renderDistance = settings.renderDistance;
            menu.maxFPS = settings.maxFPS;
            menu.invertY = settings.invertMouseY;
            menu.invertX = settings.invertMouseX;
            menu.vsync = settings.vsync;
            menu.flyMode = flyMode;
            renderer.drawPauseMenu(menu);
        }
        auto& rstats = renderer.getStats();
        renderer.drawDebugOverlay(currentFPS, (currentFPS > 0.0f) ? 1.0f/currentFPS : 0.0f,
                                  (int)rstats.chunksRendered, (int)rstats.drawCalls, playerPitch, playerYaw);
        renderer.endFrame();
    } catch (const std::exception& e) {
        VF_ERROR("Render error: {}", e.what());
    }
}

void Game::processInput(float deltaTime) {
    GLFWwindow* win = getWindow().getGLFWWindow();
    
    if (!cursorCaptured) {
        if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            cursorCaptured = true;
            getInput().setCursorCaptured(true);
        }
        return;
    }
    
    bool escPressed = glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    static bool escWasPressed = false;
    if (escPressed && !escWasPressed) {
        togglePause();
        if (paused) {
            pauseMenuSelection = 0;
            hoveredMenuItem = -1;
            menuDragging = false;
            cursorCaptured = false;
            getInput().setCursorCaptured(false);
        } else {
            cursorCaptured = true;
            getInput().setCursorCaptured(true);
        }
    }
    escWasPressed = escPressed;
    
    if (paused) {
        constexpr int MENU_COUNT = 9;
        constexpr float MENU_ITEM_Y = 0.58f;
        constexpr float MENU_ITEM_H = 0.058f;
        constexpr float MENU_ITEM_GAP = 0.008f;
        constexpr float MENU_ITEM_W = 0.55f;
        
        double mx, my;
        glfwGetCursorPos(win, &mx, &my);
        int ww, wh;
        glfwGetWindowSize(win, &ww, &wh);
        float ndcX = ((float)mx / (float)ww) * 2.0f - 1.0f;
        float ndcY = 1.0f - ((float)my / (float)wh) * 2.0f;
        
        hoveredMenuItem = -1;
        for (int i = 0; i < MENU_COUNT; i++) {
            float y = MENU_ITEM_Y - i * (MENU_ITEM_H + MENU_ITEM_GAP);
            if (ndcX >= -MENU_ITEM_W && ndcX <= MENU_ITEM_W &&
                ndcY >= (y - MENU_ITEM_H) && ndcY <= y) {
                hoveredMenuItem = i;
                break;
            }
        }
        
        bool leftHeld = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        static bool leftWasHeld = false;
        bool leftJust = leftHeld && !leftWasHeld;
        leftWasHeld = leftHeld;
        
        if (leftJust && hoveredMenuItem >= 0) {
            pauseMenuSelection = hoveredMenuItem;
            switch (hoveredMenuItem) {
                case 0: togglePause();
                    cursorCaptured = true;
                    getInput().setCursorCaptured(true);
                    break;
                case 5: settings.invertMouseY = !settings.invertMouseY; break;
                case 6: settings.invertMouseX = !settings.invertMouseX; break;
                case 7: settings.vsync = !settings.vsync; break;
                case 8: flyMode = !flyMode; playerVelocity = glm::vec3(0.0f); onGround = false; break;
                default: menuDragging = true; menuDragItem = hoveredMenuItem; break;
            }
        }
        
        if (!leftHeld) {
            menuDragging = false;
            menuDragItem = -1;
        }
        
        if (menuDragging && menuDragItem >= 1 && menuDragItem <= 4) {
            float sliderVal = (ndcX + MENU_ITEM_W) / (2.0f * MENU_ITEM_W);
            sliderVal = std::clamp(sliderVal, 0.0f, 1.0f);
            switch (menuDragItem) {
                case 1: settings.fov = 30.0f + sliderVal * 90.0f; break;
                case 2: settings.renderDistance = 2 + (int)(sliderVal * 14.0f); break;
                case 3: settings.maxFPS = 30 + (int)(sliderVal * 270.0f); break;
                case 4: settings.mouseSensitivity = 0.01f + sliderVal * 0.99f; break;
            }
        }
        
        static bool upWas = false, downWas = false, leftWas = false, rightWas = false, enterWas = false;
        bool upNow = glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS;
        bool downNow = glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS;
        bool leftNow = glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS;
        bool rightNow = glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS;
        bool enterNow = glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS;
        
        if (upNow && !upWas) pauseMenuSelection = (pauseMenuSelection - 1 + MENU_COUNT) % MENU_COUNT;
        if (downNow && !downWas) pauseMenuSelection = (pauseMenuSelection + 1) % MENU_COUNT;
        
        auto toggleSetting = [&](int sel) {
            switch (sel) {
                case 5: settings.invertMouseY = !settings.invertMouseY; break;
                case 6: settings.invertMouseX = !settings.invertMouseX; break;
                case 7: settings.vsync = !settings.vsync; break;
                case 8: flyMode = !flyMode; playerVelocity = glm::vec3(0.0f); onGround = false; break;
            }
        };
        auto adjustSlider = [&](int sel, float dir) {
            switch (sel) {
                case 1: settings.fov = std::clamp(settings.fov + dir * 5.0f, 30.0f, 120.0f); break;
                case 2: settings.renderDistance = std::clamp(settings.renderDistance + (int)dir, 2, 16); break;
                case 3: settings.maxFPS = std::clamp(settings.maxFPS + (int)(dir * 10), 30, 300); break;
                case 4: settings.mouseSensitivity = std::clamp(settings.mouseSensitivity + dir * 0.02f, 0.01f, 1.0f); break;
            }
        };
        
        if (leftNow && !leftWas) { adjustSlider(pauseMenuSelection, -1.0f); toggleSetting(pauseMenuSelection); }
        if (rightNow && !rightWas) { adjustSlider(pauseMenuSelection, 1.0f); toggleSetting(pauseMenuSelection); }
        if (enterNow && !enterWas) {
            if (pauseMenuSelection == 0) {
                togglePause();
                cursorCaptured = true;
                getInput().setCursorCaptured(true);
            }
            else toggleSetting(pauseMenuSelection);
        }
        
        upWas = upNow; downWas = downNow; leftWas = leftNow; rightWas = rightNow; enterWas = enterNow;
        return;
    }
    
    static bool fWasPressed = false;
    bool fNow = glfwGetKey(win, GLFW_KEY_F) == GLFW_PRESS;
    if (fNow && !fWasPressed) {
        flyMode = !flyMode;
        playerVelocity = glm::vec3(0.0f);
        onGround = false;
    }
    fWasPressed = fNow;
    
    static bool f12WasPressed = false;
    bool f12Now = glfwGetKey(win, GLFW_KEY_F12) == GLFW_PRESS;
    if (f12Now && !f12WasPressed) {
        auto now = std::time(nullptr);
        auto tm = std::localtime(&now);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", tm);
        std::string dir = "screenshots";
        std::filesystem::create_directories(dir);
        std::string path = dir + "/" + std::string(buf) + ".png";
        renderer.takeScreenshot(path);
    }
    f12WasPressed = f12Now;
    
    bool keyW = glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS;
    bool keyS = glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS;
    bool keyA = glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS;
    bool keyD = glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS;
    bool keySpace = glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS;
    bool keyShift = glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    bool keyCtrl = glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
    
    if (flyMode) {
        constexpr float FLY_BASE_SPEED = 10.0f;
        constexpr float FLY_MAX_MULT = 5.0f;
        constexpr float FLY_ACCEL = 4.0f;
        constexpr float FLY_DECEL = 8.0f;

        if (keyShift && (keyW || keyS || keyA || keyD || keySpace || keyCtrl)) {
            flySpeedMult += FLY_ACCEL * deltaTime;
            if (flySpeedMult > FLY_MAX_MULT) flySpeedMult = FLY_MAX_MULT;
        } else {
            flySpeedMult -= FLY_DECEL * deltaTime;
            if (flySpeedMult < 1.0f) flySpeedMult = 1.0f;
        }

        float speed = FLY_BASE_SPEED * flySpeedMult * deltaTime;
        
        glm::vec3 forward = camera.getForward();
        forward.y = 0.0f;
        if (glm::length(forward) > 0.001f) forward = glm::normalize(forward);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        
        glm::vec3 movedir(0.0f);
        if (keyW) { movedir += forward; }
        if (keyS) { movedir -= forward; }
        if (keyA) { movedir -= right; }
        if (keyD) { movedir += right; }
        if (keySpace) { movedir.y += 1.0f; }
        if (keyCtrl) { movedir.y -= 1.0f; }
        
        if (glm::length(movedir) > 0.0f) {
            playerPos += glm::normalize(movedir) * speed;
        }
    } else {
        float moveSpeed = 4.317f * deltaTime;
        if (keyShift) moveSpeed = 5.612f * deltaTime;
        
        glm::vec3 forward = camera.getForward();
        forward.y = 0.0f;
        if (glm::length(forward) > 0.001f) forward = glm::normalize(forward);
        else forward = glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        
        glm::vec3 inputDir(0.0f);
        if (keyW) inputDir += forward;
        if (keyS) inputDir -= forward;
        if (keyA) inputDir -= right;
        if (keyD) inputDir += right;
        
        if (glm::length(inputDir) > 0.0f) {
            inputDir = glm::normalize(inputDir);
        }
        
        glm::vec3 delta = inputDir * moveSpeed;
        
        if (onGround && keySpace) {
            playerVelocity.y = JUMP_VELOCITY;
            onGround = false;
        }
        
        playerVelocity.y -= GRAVITY * deltaTime;
        if (playerVelocity.y < TERMINAL_VELOCITY) playerVelocity.y = TERMINAL_VELOCITY;
        
        delta.y += playerVelocity.y * deltaTime;
        
        resolveCollisions(playerPos, delta);
        
        if (onGround) {
            playerVelocity.y = 0.0f;
        }
    }
    
    auto& input = getInput();
    auto mouseDelta = input.getMouseDelta();
    
    constexpr float MAX_MOUSE_DELTA = 50.0f;
    mouseDelta.x = std::clamp(mouseDelta.x, -MAX_MOUSE_DELTA, MAX_MOUSE_DELTA);
    mouseDelta.y = std::clamp(mouseDelta.y, -MAX_MOUSE_DELTA, MAX_MOUSE_DELTA);
    
    float appliedDX = 0.0f, appliedDY = 0.0f;
    if (glm::length(mouseDelta) > 0.0f) {
        float xMul = settings.invertMouseX ? -1.0f : 1.0f;
        float yMul = settings.invertMouseY ? 1.0f : -1.0f;
        appliedDX = mouseDelta.x * settings.mouseSensitivity * xMul;
        appliedDY = mouseDelta.y * settings.mouseSensitivity * yMul;
        playerYaw += appliedDX;
        playerPitch += appliedDY;
        playerPitch = std::clamp(playerPitch, -89.0f, 89.0f);
    }
    
    auto& diag = Diagnostics::get();
    diag.recordMouseDelta(mouseDelta.x, mouseDelta.y, appliedDX, appliedDY, cursorCaptured);
    
    camera.setPosition(playerPos + glm::vec3(0.0f, PLAYER_HEIGHT * 0.85f, 0.0f));
    camera.setRotation(playerPitch, playerYaw);
    
    for (int i = 0; i < 9; i++) {
        static bool numWas[9] = {};
        bool numNow = glfwGetKey(win, GLFW_KEY_1 + i) == GLFW_PRESS;
        if (numNow && !numWas[i]) selectedSlot = i;
        numWas[i] = numNow;
    }
    
    auto scrollDelta = input.getScrollDelta();
    if (scrollDelta.y > 0.0f) {
        selectedSlot = (selectedSlot - 1 + 9) % 9;
    } else if (scrollDelta.y < 0.0f) {
        selectedSlot = (selectedSlot + 1) % 9;
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
    auto& diag = Diagnostics::get();
    
    bool leftDown = input.isMouseButtonPressed(0);
    bool rightDown = input.isMouseButtonPressed(1);
    
    glm::ivec3 hitNormal;
    glm::ivec3 hitBlock = raycastBlocks(camera.getPosition(), camera.getForward(), 6.0f, hitNormal);
    
    bool hit = (hitBlock.x != INT_MAX);
    float rayDist = hit ? glm::length(glm::vec3(hitBlock) - camera.getPosition()) : 0.0f;
    diag.recordInteraction(hit, rayDist, hitBlock.x, hitBlock.y, hitBlock.z, selectedSlot);
    
    if (!hit) {
        leftMousePressed = leftDown;
        rightMousePressed = rightDown;
        return;
    }
    
    if (leftDown && !leftMousePressed) {
        world->setBlock(hitBlock.x, hitBlock.y, hitBlock.z, BlockState());
        int cx = (int)floor((float)hitBlock.x / 16.0f);
        int cz = (int)floor((float)hitBlock.z / 16.0f);
        renderer.invalidateChunkMesh(cx, cz);
        asyncWorker_.forgetMesh(cx, cz);
        if (hitNormal.x != 0 && (hitBlock.x & 15) == (hitNormal.x > 0 ? 0 : 15)) {
            renderer.invalidateChunkMesh(cx - hitNormal.x, cz);
            asyncWorker_.forgetMesh(cx - hitNormal.x, cz);
        }
        if (hitNormal.z != 0 && (hitBlock.z & 15) == (hitNormal.z > 0 ? 0 : 15)) {
            renderer.invalidateChunkMesh(cx, cz - hitNormal.z);
            asyncWorker_.forgetMesh(cx, cz - hitNormal.z);
        }
        diag.recordRemoveAttempt(true);
    }
    
    if (rightDown && !rightMousePressed) {
        glm::ivec3 placePos = hitBlock + hitNormal;
        if (placePos.y >= -64 && placePos.y < 320) {
            uint32_t blockId = hotbarBlocks[selectedSlot];
            if (blockId != 0) {
                float hw = PLAYER_WIDTH * 0.5f;
                bool overlapsPlayer =
                    placePos.x + 1 > playerPos.x - hw && placePos.x < playerPos.x + hw &&
                    placePos.y + 1 > playerPos.y && placePos.y < playerPos.y + PLAYER_HEIGHT &&
                    placePos.z + 1 > playerPos.z - hw && placePos.z < playerPos.z + hw;
                if (!overlapsPlayer) {
                    auto blockState = BlockRegistry::get().getDefaultState(blockId);
                    world->setBlock(placePos.x, placePos.y, placePos.z, blockState);
                    int cx = (int)floor((float)placePos.x / 16.0f);
                    int cz = (int)floor((float)placePos.z / 16.0f);
                    renderer.invalidateChunkMesh(cx, cz);
                    asyncWorker_.forgetMesh(cx, cz);
                    if (hitNormal.x != 0 && (placePos.x & 15) == (hitNormal.x > 0 ? 0 : 15)) {
                        renderer.invalidateChunkMesh(cx - hitNormal.x, cz);
                        asyncWorker_.forgetMesh(cx - hitNormal.x, cz);
                    }
                    if (hitNormal.z != 0 && (placePos.z & 15) == (hitNormal.z > 0 ? 0 : 15)) {
                        renderer.invalidateChunkMesh(cx, cz - hitNormal.z);
                        asyncWorker_.forgetMesh(cx, cz - hitNormal.z);
                    }
                    diag.recordPlaceAttempt(true);
                } else {
                    diag.recordPlaceAttempt(false);
                }
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

bool Game::checkCollision(const glm::vec3& pos) const {
    if (!world) return false;
    
    float hw = PLAYER_WIDTH * 0.5f;
    int minX = (int)floor(pos.x - hw);
    int maxX = (int)floor(pos.x + hw);
    int minY = (int)floor(pos.y);
    int maxY = (int)floor(pos.y + PLAYER_HEIGHT);
    int minZ = (int)floor(pos.z - hw);
    int maxZ = (int)floor(pos.z + hw);
    
    for (int y = minY; y <= maxY; y++) {
        for (int z = minZ; z <= maxZ; z++) {
            for (int x = minX; x <= maxX; x++) {
                auto block = world->getBlock(x, y, z);
                if (!block.isAir() && block.isSolid()) {
                    return true;
                }
            }
        }
    }
    return false;
}

void Game::resolveCollisions(glm::vec3& pos, const glm::vec3& delta) {
    glm::vec3 newPos = pos;
    
    newPos.x += delta.x;
    if (checkCollision(newPos)) {
        newPos.x = pos.x;
        playerVelocity.x = 0.0f;
    }
    
    newPos.y += delta.y;
    if (checkCollision(newPos)) {
        if (delta.y < 0.0f) {
            onGround = true;
            float testY = floor(newPos.y);
            newPos.y = testY + 1.0f;
            while (checkCollision(newPos) && newPos.y < pos.y + 2.0f) {
                newPos.y += 1.0f;
            }
            if (checkCollision(newPos)) {
                newPos.y = pos.y;
            }
        } else {
            newPos.y = pos.y;
            playerVelocity.y = 0.0f;
        }
    } else {
        onGround = false;
    }
    
    newPos.z += delta.z;
    if (checkCollision(newPos)) {
        newPos.z = pos.z;
        playerVelocity.z = 0.0f;
    }
    
    pos = newPos;
}

void Game::saveWorld() {
    if (!world) return;
    
    std::string worldDir = "saves/world_0";
    std::filesystem::create_directories(worldDir);
    
    world->forEachChunk([&](Chunk& chunk) {
        ChunkPos pos = chunk.getPosition();
        std::string filename = worldDir + "/chunk_" + std::to_string(pos.x) + "_" + std::to_string(pos.z) + ".dat";
        
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return;
        
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_WIDTH; z++) {
                for (int x = 0; x < CHUNK_WIDTH; x++) {
                    uint64_t encoded = chunk.getBlock(x, y, z).encode();
                    file.write(reinterpret_cast<const char*>(&encoded), sizeof(encoded));
                }
            }
        }
    });
    
    VF_CORE_INFO("World saved to {}", worldDir);
}

void Game::loadWorld() {
    if (!world) return;
    
    std::string worldDir = "saves/world_0";
    if (!std::filesystem::exists(worldDir)) return;
    
    int loaded = 0;
    for (const auto& entry : std::filesystem::directory_iterator(worldDir)) {
        if (entry.path().extension() != ".dat") continue;
        
        std::string filename = entry.path().stem().string();
        int cx, cz;
        if (sscanf(filename.c_str(), "chunk_%d_%d", &cx, &cz) != 2) continue;
        
        std::ifstream file(entry.path().string(), std::ios::binary);
        if (!file.is_open()) continue;
        
        ChunkPos pos{cx, cz};
        auto* chunk = world->getOrCreateChunk(pos);
        if (!chunk) continue;
        
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_WIDTH; z++) {
                for (int x = 0; x < CHUNK_WIDTH; x++) {
                    uint64_t encoded;
                    file.read(reinterpret_cast<char*>(&encoded), sizeof(encoded));
                    if (file.eof()) goto next_chunk;
                    chunk->setBlock(x, y, z, BlockState::decode(encoded));
                }
            }
        }
        loaded++;
        next_chunk:;
    }
    
    if (loaded > 0) {
        VF_CORE_INFO("Loaded {} chunks from {}", loaded, worldDir);
    }
}

Application* createApplication() {
    return new Game();
}

} // namespace VoxelForge
