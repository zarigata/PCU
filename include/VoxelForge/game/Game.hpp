/**
 * @file Game.hpp
 * @brief Main game class for VoxelForge
 */

#pragma once

#include <VoxelForge/core/Application.hpp>
#include <VoxelForge/core/ECS.hpp>
#include <VoxelForge/rendering/Renderer.hpp>
#include <VoxelForge/rendering/Camera.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <array>
#include <string>

namespace VoxelForge {

// Forward declarations
class World;
class Player;
class EntityManager;

struct GameSettings {
    int renderDistance = 8;
    int simulationDistance = 6;
    bool particlesEnabled = true;
    bool cloudsEnabled = true;
    float fov = 70.0f;
    float mouseSensitivity = 0.1f;
    bool invertMouseY = false;
    bool invertMouseX = false;
    bool vsync = true;
    bool fullscreen = false;
    int maxFPS = 120;
    
    // Day/Night cycle
    float dayNightCycleDuration = 12000.0f;
    float dayStartTime = 0.0f;
};

class Game : public Application {
public:
    Game(const ApplicationProps& props = ApplicationProps{"VoxelForge"});
    ~Game();
    
    // Game state
    bool isPaused() const { return paused; }
    void setPaused(bool p) { paused = p; }
    void togglePause() { paused = !paused; }
    
    // World access
    World* getWorld() { return world.get(); }
    const World* getWorld() const { return world.get(); }
    
    // Settings
    GameSettings& getSettings() { return settings; }
    const GameSettings& getSettings() const { return settings; }
    
    // Game mode
    enum class GameMode { Survival, Creative, Adventure, Spectator };
    GameMode getGameMode() const { return gameMode; }
    void setGameMode(GameMode mode) { gameMode = mode; }

    // Day/Night cycle management
    float getDayTime() const;
    void setDayTime(float time);
    
    bool isDay() const;  // Returns true during day phase
    bool isNight() const;  // Returns true during night phase
    float getDayProgress() const;  // 0.0 to 1.0 through current day
    void advanceDayNightCycle();  // Manually advance to next phase
    void resetDayNightCycle();  // Reset to dawn

    
protected:
    void onInit() override;
    void onShutdown() override;
    void onUpdate(float deltaTime) override;
    void onRender() override;
    
private:
    void processInput(float deltaTime);
    void updateEntities(float deltaTime);
    void updateWorld(float deltaTime);
    void handleBlockInteraction();
    glm::ivec3 raycastBlocks(const glm::vec3& origin, const glm::vec3& direction, float maxDist, glm::ivec3& hitNormal);
    void saveWorld();
    void loadWorld();
    
    std::unique_ptr<World> world;
    std::unique_ptr<ECSWorld> ecsWorld;
    
    Renderer renderer;
    Camera camera;
    glm::vec3 playerPos = glm::vec3(0.0f, 80.0f, 0.0f);
    float playerYaw = 0.0f;
    float playerPitch = 0.0f;
    bool rendererInitialized = false;
    
    bool paused = false;
    GameMode gameMode = GameMode::Creative;
    GameSettings settings;
    
    float gameTime = 0.0f;
    int64_t tickCount = 0;
    
    std::array<uint32_t, 9> hotbarBlocks = {{1,2,3,4,5,6,13,14,19}};
    int selectedSlot = 0;
    bool leftMousePressed = false;
    bool rightMousePressed = false;
    
    glm::vec3 playerVelocity = glm::vec3(0.0f);
    bool onGround = false;
    bool flyMode = true;
    
    static constexpr float PLAYER_WIDTH = 0.6f;
    static constexpr float PLAYER_HEIGHT = 1.8f;
    static constexpr float GRAVITY = 28.0f;
    static constexpr float JUMP_VELOCITY = 8.5f;
    static constexpr float TERMINAL_VELOCITY = -78.4f;
    
    bool checkCollision(const glm::vec3& pos) const;
    void resolveCollisions(glm::vec3& pos, const glm::vec3& delta);
    
    float fpsAccumulator = 0.0f;
    int frameCount = 0;
    float currentFPS = 0.0f;
    float fpsUpdateTimer = 0.0f;
    
    int pauseMenuSelection = 0;
};

// Game instance access
inline Game& getGame() {
    return static_cast<Game&>(Application::get());
}

} // namespace VoxelForge
