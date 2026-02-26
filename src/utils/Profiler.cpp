/**
 * @file Profiler.cpp
 * @brief Profiler implementation
 */

#include <VoxelForge/utils/Profiler.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <sstream>
#include <thread>
#include <iomanip>

namespace VoxelForge {

// ============================================
// ProfilerScope
// ============================================

ProfilerScope::ProfilerScope(const std::string& name)
    : name(name),    start(std::chrono::high_resolution_clock::now()) {
}

ProfilerScope::~ProfilerScope() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    
    Profiler::get().endSection(name);
}

// ============================================
// Profiler
// ============================================

Profiler& Profiler::get() {
    static Profiler instance;
    return instance;
}

void Profiler::beginFrame() {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    currentFrame = FrameTime{};
    frameResults.clear();
    frameCount++;
}

void Profiler::endFrame() {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    lastFrame = currentFrame;
}

void Profiler::beginSection(const std::string& name) {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    activeSections[name] = std::chrono::high_resolution_clock::now();
}

void Profiler::endSection(const std::string& name) {
    if (!enabled) return;
    
    auto end = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = activeSections.find(name);
    if (it == activeSections.end()) return;
    
    auto start = it->second;
    auto durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    float durationMs = durationNs / 1000000.0f;
    
    activeSections.erase(it);
    
    // Update stats
    auto& statsEntry = stats[name];
    statsEntry.lastMs = durationMs;
    statsEntry.callCount++;
    
    if (statsEntry.callCount == 1) {
        statsEntry.minMs = durationMs;
        statsEntry.maxMs = durationMs;
        statsEntry.avgMs = durationMs;
    } else {
        statsEntry.minMs = std::min(statsEntry.minMs, durationMs);
        statsEntry.maxMs = std::max(statsEntry.maxMs, durationMs);
        // Rolling average
        statsEntry.avgMs = statsEntry.avgMs * 0.95f + durationMs * 0.05f;
    }
    
    // Update frame time based on section name
    if (name == "Update") currentFrame.updateMs += durationMs;
    else if (name == "Render") currentFrame.renderMs += durationMs;
    else if (name == "WorldTick") currentFrame.worldTickMs += durationMs;
    else if (name == "ChunkMesh") currentFrame.chunkMeshMs += durationMs;
    else if (name == "GPUWait") currentFrame.gpuWaitMs += durationMs;
    
    // Store result
    frameResults.push_back({
        name,
        std::chrono::duration_cast<std::chrono::nanoseconds>(start.time_since_epoch()).count(),
        std::chrono::duration_cast<std::chrono::nanoseconds>(end.time_since_epoch()).count(),
        std::hash<std::thread::id>{}()
    });
}

void Profiler::printStats() {
    VF_CORE_INFO("=== Profiler Stats ===");
    
    std::vector<std::pair<std::string, ProfilerStats>> sortedStats(
        stats.begin(), stats.end()
    );
    
    std::sort(sortedStats.begin(), sortedStats.end(),
        [](const auto& a, const auto& b) {
            return a.second.avgMs > b.second.avgMs;
        });
    
    for (const auto& [name, stat] : sortedStats) {
        VF_CORE_INFO("  {:<30} - avg: {:6.2f}ms, min: {:6.2f}ms, max: {:6.2f}ms, calls: {}",
            name,
            stat.avgMs,
            stat.minMs,
            stat.maxMs,
            stat.callCount
        );
    }
    
    VF_CORE_INFO("====================");
}

void Profiler::saveReport(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        VF_CORE_ERROR("Could not open profiler report file: {}", path);
        return;
    }
    
    file << "VoxelForge Profiler Report\n";
    file << "========================\n\n";
    
    file << "Frame Statistics:\n";
    file << "  Total: " << lastFrame.totalMs << "ms\n";
    file << "  Update: " << lastFrame.updateMs << "ms\n";
    file << "  Render: " << lastFrame.renderMs << "ms\n";
    file << "  World Tick: " << lastFrame.worldTickMs << "ms\n";
    file << "  Chunk Mesh: " << lastFrame.chunkMeshMs << "ms\n";
    file << "  GPU Wait: " << lastFrame.gpuWaitMs << "ms\n\n";
    
    file << "Section Statistics:\n";
    for (const auto& [name, stat] : stats) {
        file << "  " << name << ":\n";
        file << "    Average: " << stat.avgMs << "ms\n";
        file << "    Min: " << stat.minMs << "ms\n";
        file << "    Max: " << stat.maxMs << "ms\n";
        file << "    Calls: " << stat.callCount << "\n";
    }
    
    file << "\n--- End of Report ---\n";
    
    VF_CORE_INFO("Profiler report saved to: {}", path);
}

void Profiler::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    stats.clear();
    frameResults.clear();
    frameCount = 0;
}

} // namespace VoxelForge
