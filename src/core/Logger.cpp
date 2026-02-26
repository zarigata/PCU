/**
 * @file Logger.cpp
 * @brief Logging system implementation using spdlog
 */

#include <VoxelForge/core/Logger.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <filesystem>
#include <iostream>

namespace VoxelForge {

std::shared_ptr<spdlog::logger> Logger::coreLogger;
std::shared_ptr<spdlog::logger> Logger::clientLogger;
bool Logger::initialized = false;

void Logger::init() {
    if (initialized) return;
    
    try {
        // Create logs directory
        std::filesystem::create_directories("logs");
        
        // Create sink vector
        std::vector<spdlog::sink_ptr> sinks;
        
        // Console sink with colors
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_level(spdlog::level::trace);
        consoleSink->set_pattern("%^[%T] %n: %v%$");
        sinks.push_back(consoleSink);
        
        // Rotating file sink (10MB max, 3 files)
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/voxelforge.log", 1048576 * 10, 3);
        fileSink->set_level(spdlog::level::trace);
        fileSink->set_pattern("[%Y-%m-%d %T.%e] [%l] %n: %v");
        sinks.push_back(fileSink);
        
        // Core logger (for engine)
        coreLogger = std::make_shared<spdlog::logger>("VOXELFORGE", sinks.begin(), sinks.end());
        coreLogger->set_level(spdlog::level::trace);
        coreLogger->flush_on(spdlog::level::trace);
        spdlog::register_logger(coreLogger);
        
        // Client logger (for game/mods)
        clientLogger = std::make_shared<spdlog::logger>("GAME", sinks.begin(), sinks.end());
        clientLogger->set_level(spdlog::level::trace);
        clientLogger->flush_on(spdlog::level::trace);
        spdlog::register_logger(clientLogger);
        
        // Set default logger
        spdlog::set_default_logger(coreLogger);
        
        initialized = true;
        
        coreLogger->info("Logger initialized");
        coreLogger->info("VoxelForge Version: {}.{}.{}", 
            VOXELFORGE_VERSION_MAJOR, 
            VOXELFORGE_VERSION_MINOR, 
            VOXELFORGE_VERSION_PATCH);
    }
    catch (const spdlog::spdlog_ex& e) {
        std::cerr << "Logger initialization failed: " << e.what() << std::endl;
    }
}

void Logger::shutdown() {
    if (!initialized) return;
    
    coreLogger->info("Shutting down logger...");
    spdlog::shutdown();
    initialized = false;
}

std::shared_ptr<spdlog::logger>& Logger::get() {
    if (!initialized) {
        // Emergency initialization
        init();
    }
    return coreLogger;
}

std::shared_ptr<spdlog::logger>& Logger::getClientLogger() {
    if (!initialized) {
        init();
    }
    return clientLogger;
}

} // namespace VoxelForge
