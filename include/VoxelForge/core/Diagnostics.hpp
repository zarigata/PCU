#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <glm/glm.hpp>

namespace VoxelForge {

struct DiagnosticsConfig {
    bool enabled = false;
    bool profileCpu = false;
    bool profileGpu = false;
    bool profileInput = false;
    bool profileStreaming = false;
    bool logFrameSpikes = false;
    bool logInputJitter = false;
    bool showHud = false;
    int64_t fixedSeed = -1;
    float quitAfterSeconds = 0.0f;
    std::string traceFile;
};

struct FrameMetrics {
    float frameTime = 0;
    float updateTime = 0;
    float renderTime = 0;
    float inputTime = 0;
    float chunkUpdateTime = 0;
    float uploadTime = 0;
    float uiTime = 0;

    float mouseDeltaX = 0;
    float mouseDeltaY = 0;
    float appliedDeltaX = 0;
    float appliedDeltaY = 0;
    bool cursorCaptured = false;

    int chunksVisible = 0;
    int chunksLoaded = 0;
    int chunksUploadedThisFrame = 0;
    int chunkJobsPending = 0;
    int chunkJobsCompleted = 0;
    int chunkGenTimeUs = 0;
    int uploadBytesThisFrame = 0;

    int drawCalls = 0;
    int triangles = 0;

    bool raycastHit = false;
    float raycastDist = 0;
    int raycastBlockX = 0;
    int raycastBlockY = 0;
    int raycastBlockZ = 0;
    int selectedSlot = 0;
    bool placeAttempted = false;
    bool placeSucceeded = false;
    bool removeAttempted = false;
    bool removeSucceeded = false;

    int frameIndex = 0;
};

class Diagnostics {
public:
    static Diagnostics& get();

    void init(const DiagnosticsConfig& config);
    void shutdown();

    void beginFrame();
    void endFrame();

    void beginSection(const std::string& name);
    void endSection(const std::string& name);

    void recordMouseDelta(float rawX, float rawY, float appliedX, float appliedY, bool captured);
    void recordChunkUpdate(int pending, int completed, int genTimeUs, int uploadBytes);
    void recordInteraction(bool hit, float dist, int bx, int by, int bz, int slot);
    void recordPlaceAttempt(bool succeeded);
    void recordRemoveAttempt(bool succeeded);

    FrameMetrics& currentFrame() { return current_; }
    const DiagnosticsConfig& config() const { return config_; }

    bool shouldQuit() const;
    float fps() const { return fps_; }
    float frameTime1pctLow() const { return frameTime1pctLow_; }
    float worstFrameTime() const { return worstFrameTime_; }
    int totalFrames() const { return totalFrames_; }
    int spikeCount() const { return spikeCount_; }

    std::string getHudText() const;
    std::string getSummaryReport() const;

    void writeTraceFrame();

private:
    Diagnostics() = default;
    float sectionTime(const std::string& name) const;

    DiagnosticsConfig config_;
    FrameMetrics current_;

    std::chrono::high_resolution_clock::time_point frameStart_;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> sectionStarts_;
    std::unordered_map<std::string, float> sectionTimes_;

    float fps_ = 0;
    float frameTime1pctLow_ = 0;
    float worstFrameTime_ = 0;
    int totalFrames_ = 0;
    int spikeCount_ = 0;
    float elapsedTime_ = 0;

    std::vector<float> frameTimeHistory_;
    std::ofstream traceFile_;

    static constexpr int FRAME_HISTORY = 300;
    static constexpr float SPIKE_THRESHOLD = 1.0f / 30.0f;
};

} // namespace VoxelForge
