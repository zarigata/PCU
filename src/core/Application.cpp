/**
 * @file Application.cpp
 * @brief Application implementation
 */

#include <VoxelForge/core/Application.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <VoxelForge/core/Memory.hpp>
#include <VoxelForge/engine/EventSystem.hpp>
#include <iostream>

namespace VoxelForge {

Application* Application::instance = nullptr;

Application::Application(const ApplicationProps& props) {
    VF_ASSERT(!instance, "Application already exists!");
    instance = this;
    
    // Initialize logger
    Logger::init();
    
    VF_CORE_INFO("Creating application: {}", props.name);
    
    // Initialize memory system
    Memory::init();
    
    // Create window if not headless
    if (!props.headless) {
        WindowProps windowProps;
        windowProps.title = props.name;
        windowProps.width = props.windowWidth;
        windowProps.height = props.windowHeight;
        windowProps.fullscreen = props.fullscreen;
        windowProps.vsync = props.vsync;
        
        window = std::make_unique<Window>(windowProps);
        
        input = std::unique_ptr<Input>(new Input());
        input->init(window->getGLFWWindow());
    }
    
    // Call derived init
    onInit();
    
    VF_CORE_INFO("Application initialized");
}

Application::~Application() {
    VF_CORE_INFO("Destroying application");
    
    onShutdown();
    
    if (input) {
        input->shutdown();
    }
    
    window.reset();
    
    Memory::shutdown();
    Logger::shutdown();
}

void Application::run() {
    running = true;
    VF_CORE_INFO("Starting main loop");
    
    lastFrameTime = Timer::getCurrentTime();
    
    while (running) {
        // Calculate delta time
        float currentTime = Timer::getCurrentTime();
        deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        
        // Cap delta time to prevent spiral of death
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        // Update FPS counter
        fpsCounter.frame();
        
        // Update input
        if (input) {
            input->update();
        }
        
        // Check for window close
        if (window && window->shouldClose()) {
            running = false;
            break;
        }
        
        // Update
        onUpdate(deltaTime);
        
        // Render
        if (window) {
            onRender();
            // Window swap buffers would go here for OpenGL
            // Vulkan rendering doesn't need this
        }
        
        // Reset temp memory arena each frame
        Memory::resetTempArena();
    }
    
    VF_CORE_INFO("Main loop ended");
}

void Application::close() {
    running = false;
    VF_CORE_INFO("Application close requested");
}

} // namespace VoxelForge
