/**
 * @file Application.hpp
 * @brief Main application class
 */

#pragma once

#include <memory>
#include <string>
#include <VoxelForge/core/Window.hpp>
#include <VoxelForge/core/Input.hpp>
#include <VoxelForge/core/Timer.hpp>

namespace VoxelForge {

struct ApplicationProps {
    std::string name = "VoxelForge";
    std::string workingDirectory = "";
    uint32_t windowWidth = 1280;
    uint32_t windowHeight = 720;
    bool fullscreen = false;
    bool vsync = true;
    bool headless = false;
};

class Application {
public:
    Application(const ApplicationProps& props = ApplicationProps());
    virtual ~Application();
    
    // No copy
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    
    // Main loop
    void run();
    void close();
    
    // Accessors
    Window& getWindow() { return *window; }
    Input& getInput() { return *input; }
    
    static Application& get() { return *instance; }
    
    bool isRunning() const { return running; }
    float getDeltaTime() const { return deltaTime; }
    float getFPS() const { return fpsCounter.getFPS(); }
    
protected:
    virtual void onInit() {}
    virtual void onShutdown() {}
    virtual void onUpdate(float deltaTime) {}
    virtual void onRender() {}
    virtual void onImGuiRender() {}
    
private:
    void init();
    void shutdown();
    
    std::unique_ptr<Window> window;
    std::unique_ptr<Input> input;
    
    bool running = false;
    bool minimized = false;
    float deltaTime = 0.0f;
    float lastFrameTime = 0.0f;
    
    FPSCounter fpsCounter;
    
    static Application* instance;
};

// To be defined by client
Application* createApplication();

} // namespace VoxelForge
