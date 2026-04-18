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
    bool vsync = true;
    bool fullscreen = false;
    int maxFPS = 120;
    
    // Day/Night cycle
    float dayNightCycleDuration = 12000.0f;  // 20 minutes (in ticks)
    float dayStartTime = 0.0f;           // When day started (in ticks since world time)
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
};

// Game instance access
inline Game& getGame() {
    return static_cast<Game&>(Application::get());
}

} // namespace VoxelForge
