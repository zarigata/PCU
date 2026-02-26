/**
 * @file Logger.hpp
 * @brief Logging system header using spdlog
 */

#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

namespace VoxelForge {

class Logger {
public:
    static void init();
    static void shutdown();
    
    static std::shared_ptr<spdlog::logger>& get();
    static std::shared_ptr<spdlog::logger>& getClientLogger();
    
    // Convenience macros
    #define VF_CORE_TRACE(...)    ::VoxelForge::Logger::get()->trace(__VA_ARGS__)
    #define VF_CORE_INFO(...)     ::VoxelForge::Logger::get()->info(__VA_ARGS__)
    #define VF_CORE_WARN(...)     ::VoxelForge::Logger::get()->warn(__VA_ARGS__)
    #define VF_CORE_ERROR(...)    ::VoxelForge::Logger::get()->error(__VA_ARGS__)
    #define VF_CORE_CRITICAL(...) ::VoxelForge::Logger::get()->critical(__VA_ARGS__)
    
    #define VF_TRACE(...)         ::VoxelForge::Logger::getClientLogger()->trace(__VA_ARGS__)
    #define VF_INFO(...)          ::VoxelForge::Logger::getClientLogger()->info(__VA_ARGS__)
    #define VF_WARN(...)          ::VoxelForge::Logger::getClientLogger()->warn(__VA_ARGS__)
    #define VF_ERROR(...)         ::VoxelForge::Logger::getClientLogger()->error(__VA_ARGS__)
    #define VF_CRITICAL(...)      ::VoxelForge::Logger::getClientLogger()->critical(__VA_ARGS__)

private:
    static std::shared_ptr<spdlog::logger> coreLogger;
    static std::shared_ptr<spdlog::logger> clientLogger;
    static bool initialized;
};

} // namespace VoxelForge
