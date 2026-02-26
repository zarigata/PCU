/**
 * @file main.cpp
 * @brief Main entry point for VoxelForge
 */

#include <VoxelForge/Engine.hpp>
#include <VoxelForge/game/Game.hpp>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
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
        // Create and run game
        auto app = VoxelForge::createApplication();
        app->run();
        delete app;
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
