/**
 * @file main.cpp
 * @brief Main entry point for VoxelForge
 */

#include <VoxelForge/Engine.hpp>
#include <VoxelForge/game/Game.hpp>
#include <VoxelForge/core/Diagnostics.hpp>
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

static VoxelForge::DiagnosticsConfig parseArgs(int argc, char* argv[]) {
    VoxelForge::DiagnosticsConfig cfg;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--diagnose") == 0) {
            cfg.enabled = true;
            cfg.showHud = true;
            cfg.logFrameSpikes = true;
            cfg.logInputJitter = true;
            cfg.profileCpu = true;
            cfg.profileInput = true;
            cfg.profileStreaming = true;
        }
        else if (std::strcmp(argv[i], "--profile-cpu") == 0) { cfg.profileCpu = true; cfg.enabled = true; }
        else if (std::strcmp(argv[i], "--profile-input") == 0) { cfg.profileInput = true; cfg.enabled = true; }
        else if (std::strcmp(argv[i], "--profile-streaming") == 0) { cfg.profileStreaming = true; cfg.enabled = true; }
        else if (std::strcmp(argv[i], "--log-frame-spikes") == 0) { cfg.logFrameSpikes = true; cfg.enabled = true; }
        else if (std::strcmp(argv[i], "--log-input-jitter") == 0) { cfg.logInputJitter = true; cfg.enabled = true; }
        else if (std::strncmp(argv[i], "--trace-file=", 13) == 0) { cfg.traceFile = argv[i] + 13; cfg.enabled = true; }
        else if (std::strncmp(argv[i], "--quit-after-seconds=", 21) == 0) { cfg.quitAfterSeconds = std::atof(argv[i] + 21); }
        else if (std::strncmp(argv[i], "--fixed-seed=", 13) == 0) { cfg.fixedSeed = std::atoll(argv[i] + 13); }
        else if (std::strcmp(argv[i], "--diag-hud") == 0) { cfg.showHud = true; }
    }
    return cfg;
}

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
        auto diagConfig = parseArgs(argc, argv);
        if (diagConfig.enabled) {
            if (diagConfig.traceFile.empty()) diagConfig.traceFile = "diagnostics_trace.csv";
        }
        VoxelForge::Diagnostics::get().init(diagConfig);

        auto app = VoxelForge::createApplication();
        app->run();
        delete app;

        if (diagConfig.enabled) {
            std::cout << VoxelForge::Diagnostics::get().getSummaryReport();
            VoxelForge::Diagnostics::get().shutdown();
        }
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
