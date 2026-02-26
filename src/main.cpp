/**
 * @file main.cpp
 * @brief Main entry point for VoxelForge
 */

#include <VoxelForge/Engine.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <VoxelForge/core/Memory.hpp>
#include <VoxelForge/core/Timer.hpp>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace VoxelForge;

/**
 * @brief Main entry point
 */
int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Enable ANSI colors on Windows
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
    
    // Print banner
    std::cout << R"(
██╗   ██╗ ██████╗ ██╗██████╗ ███████╗██████╗ 
██║   ██║██╔═══██╗██║██╔══██╗██╔════╝██╔══██╗
██║   ██║██║   ██║██║██║  ██║█████╗  ██████╔╝
╚██╗ ██╔╝██║   ██║██║██║  ██║██╔══╝  ██╔══██╗
 ╚████╔╝ ╚██████╔╝██║██████╔╝███████╗██║  ██║
  ╚═══╝   ╚═════╝ ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝
        C++ Minecraft Clone with Modding
)" << std::endl;
    
    std::cout << "Version " << VOXELFORGE_VERSION_STRING << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        // Initialize systems
        VF_CORE_INFO("Starting VoxelForge...");
        
        // Initialize memory
        Memory::init();
        
        // Initialize timer
        Timer startupTimer;
        
        // TODO: Initialize window, Vulkan, etc.
        
        float initTime = startupTimer.elapsedMillis();
        VF_CORE_INFO("Initialization complete in {:.2f}ms", initTime);
        
        // Main game loop would go here
        VF_CORE_INFO("Game loop not yet implemented - exiting");
        
        // Shutdown
        VF_CORE_INFO("Shutting down...");
        Memory::shutdown();
        Logger::shutdown();
        
        return 0;
    }
    catch (const std::exception& e) {
        VF_CORE_CRITICAL("Fatal error: {}", e.what());
        return 1;
    }
    catch (...) {
        VF_CORE_CRITICAL("Unknown fatal error");
        return 1;
    }
}
