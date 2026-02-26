/**
 * @file Profiler.hpp
 * @brief Performance profiler for VoxelForge
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <mutex>
#include <fstream>

namespace VoxelForge {

// Profiling scope
class ProfilerScope {
public:
    explicit ProfilerScope(const std::string& name);
    ~ProfilerScope();
    
private:
    std::string name;
    std::chrono::high_resolution_clock::time_point start;
};

// Profile result
struct ProfileResult {
    std::string name;
    int64_t startNs;
    int64_t endNs;
    uint32_t threadId;
};

// Frame timing data
struct FrameTime {
    float totalMs = 0.0f;
    float updateMs = 0.0f;
    float renderMs = 0.0f;
    float waitMs = 0.0f;
    
    float worldTickMs = 0.0f;
    float chunkMeshMs = 0.0f;
    float gpuWaitMs = 0.0f;
};

// Profiler statistics
struct ProfilerStats {
    float minMs = 0.0f;
    float maxMs = 0.0f;
    float avgMs = 0.0f;
    float lastMs = 0.0f;
    uint32_t callCount = 0;
};

// Main profiler class
class Profiler {
public:
    static Profiler& get();
    
    void beginFrame();
    void endFrame();
    
    void beginSection(const std::string& name);
    void endSection(const std::string& name);
    
    // Get statistics
    const FrameTime& getLastFrame() const { return lastFrame; }
    const std::unordered_map<std::string, ProfilerStats>& getStats() const { return stats; }
    
    // Output
    void printStats();
    void saveReport(const std::string& path);
    void reset();
    
    // Enable/disable
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
private:
    Profiler() = default;
    
    bool enabled = true;
    FrameTime lastFrame;
    FrameTime currentFrame;
    
    std::unordered_map<std::string, ProfilerStats> stats;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> activeSections;
    
    std::vector<ProfileResult> frameResults;
    std::mutex mutex;
    
    int frameCount = 0;
    static constexpr int SAMPLE_SIZE = 60;
};

// Scoped profiler macro
#define VF_PROFILE_SCOPE(name) VoxelForge::ProfilerScope profilerScope##__LINE__(name)
#define VF_PROFILE_FUNCTION() VF_PROFILE_SCOPE(__FUNCTION__)

// Convenience functions
namespace Profile {
    inline void BeginFrame() { Profiler::get().beginFrame(); }
    inline void EndFrame() { Profiler::get().endFrame(); }
    inline void Begin(const std::string& name) { Profiler::get().beginSection(name); }
    inline void End(const std::string& name) { Profiler::get().endSection(name); }
}

// Timed section helper
template<typename Func>
auto ProfileTimed(const std::string& name, Func&& func) -> decltype(func()) {
    Profiler::get().beginSection(name);
    auto result = func();
    Profiler::get().endSection(name);
    return result;
}

} // namespace VoxelForge
