/**
 * @file Game.hpp
 * @brief Main game class for VoxelForge
 */

#pragma once

#include <VoxelForge/core/Application.hpp>
#include <VoxelForge/core/ECS.hpp>
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
    
    bool paused = false;
    GameMode gameMode = GameMode::Survival;
    GameSettings settings;
    
    float gameTime = 0.0f;
    i64 tickCount = 0;
};

// Game instance access
inline Game& getGame() {
    return *static_cast<Game*>(Application::get());
}

} // namespace VoxelForge
