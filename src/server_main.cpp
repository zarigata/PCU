/**
 * @file server_main.cpp
 * @brief Dedicated server entry point
 */

#include <VoxelForge/Engine.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <VoxelForge/core/Memory.hpp>
#include <VoxelForge/core/Timer.hpp>
#include <iostream>
#include <csignal>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace VoxelForge;

static std::atomic<bool> shouldRun(true);

void signalHandler(int signal) {
    VF_CORE_INFO("Received signal {} - shutting down...", signal);
    shouldRun = false;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    
    std::cout << R"(
██╗   ██╗ ██████╗ ██╗██████╗ ███████╗██████╗ 
██║   ██║██╔═══██╗██║██╔══██╗██╔════╝██╔══██╗
██║   ██║██║   ██║██║██║  ██║█████╗  ██████╔╝
╚██╗ ██╔╝██║   ██║██║██║  ██║██╔══╝  ██╔══██╗
 ╚████╔╝ ╚██████╔╝██║██████╔╝███████╗██║  ██║
  ╚═══╝   ╚═════╝ ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝
              Dedicated Server
)" << std::endl;
    
    std::cout << "Version " << VOXELFORGE_VERSION_STRING << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    try {
        VF_CORE_INFO("Starting VoxelForge Dedicated Server...");
        
        Memory::init();
        
        Timer startupTimer;
        
        // TODO: Initialize server systems
        
        float initTime = startupTimer.elapsedMillis();
        VF_CORE_INFO("Server initialized in {:.2f}ms", initTime);
        
        // Main server loop
        VF_CORE_INFO("Starting server loop...");
        
        FPSCounter fpsCounter;
        while (shouldRun) {
            float deltaTime = fpsCounter.getFrameTime() / 1000.0f;
            
            // TODO: Server tick
            
            fpsCounter.frame();
            
            // Sleep to maintain tick rate (20 TPS)
            float targetFrameTime = 50.0f; // 50ms per tick
            if (fpsCounter.getFrameTime() < targetFrameTime) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(
                        static_cast<int>(targetFrameTime - fpsCounter.getFrameTime())
                    )
                );
            }
        }
        
        VF_CORE_INFO("Shutting down server...");
        Memory::shutdown();
        Logger::shutdown();
        
        return 0;
    }
    catch (const std::exception& e) {
        VF_CORE_CRITICAL("Fatal error: {}", e.what());
        return 1;
    }
}
