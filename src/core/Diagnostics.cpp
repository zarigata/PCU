#include <VoxelForge/core/Diagnostics.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace VoxelForge {

Diagnostics& Diagnostics::get() {
    static Diagnostics instance;
    return instance;
}

void Diagnostics::init(const DiagnosticsConfig& config) {
    config_ = config;
    frameTimeHistory_.reserve(FRAME_HISTORY);
    if (!config_.traceFile.empty()) {
        traceFile_.open(config_.traceFile, std::ios::out | std::ios::trunc);
        if (traceFile_.is_open()) {
            traceFile_ << "frame,frameTime,updateTime,renderTime,inputTime,chunkTime,uploadTime,uiTime,"
                       << "rawDX,rawDY,appliedDX,appliedDY,cursorCaptured,"
                       << "chunksVisible,chunksLoaded,uploadedThisFrame,jobsPending,"
                       << "drawCalls,triangles,"
                       << "rayHit,rayDist,rayBX,rayBY,rayBZ,slot,"
                       << "placeAttempt,placeOk,removeAttempt,removeOk"
                       << "\n";
        }
    }
    SPDLOG_INFO("Diagnostics mode: enabled={} hud={} trace={} quitAfter={:.1f}s",
                config_.enabled, config_.showHud, config_.traceFile, config_.quitAfterSeconds);
}

void Diagnostics::shutdown() {
    if (traceFile_.is_open()) {
        traceFile_.close();
    }
    SPDLOG_INFO("Diagnostics summary: frames={} spikes={} worst={:.2f}ms 1%low={:.2f}ms",
                totalFrames_, spikeCount_, worstFrameTime_ * 1000.0f, frameTime1pctLow_ * 1000.0f);
}

void Diagnostics::beginFrame() {
    frameStart_ = std::chrono::high_resolution_clock::now();
    current_ = FrameMetrics{};
    current_.frameIndex = totalFrames_;
}

void Diagnostics::endFrame() {
    auto now = std::chrono::high_resolution_clock::now();
    float frameTime = std::chrono::duration<float>(now - frameStart_).count();
    current_.frameTime = frameTime;

    totalFrames_++;
    elapsedTime_ += frameTime;

    frameTimeHistory_.push_back(frameTime);
    if ((int)frameTimeHistory_.size() > FRAME_HISTORY) {
        frameTimeHistory_.erase(frameTimeHistory_.begin());
    }

    if (!frameTimeHistory_.empty()) {
        auto sorted = frameTimeHistory_;
        std::sort(sorted.begin(), sorted.end());
        frameTime1pctLow_ = sorted[std::max(0, (int)sorted.size() - (int)sorted.size() / 100 - 1)];
        worstFrameTime_ = sorted.back();
    }

    if (frameTime > 0.016f) fps_ = 1.0f / frameTime;
    else fps_ = 60.0f;
    if (!frameTimeHistory_.empty()) {
        float avg = 0;
        for (auto t : frameTimeHistory_) avg += t;
        avg /= frameTimeHistory_.size();
        if (avg > 0.001f) fps_ = 1.0f / avg;
    }

    if (frameTime > SPIKE_THRESHOLD) {
        spikeCount_++;
        if (config_.logFrameSpikes) {
            SPDLOG_WARN("FRAME SPIKE #{}, ft={:.2f}ms update={:.2f}ms render={:.2f}ms chunk={:.2f}ms upload={:.2f}ms ui={:.2f}ms jobs={}",
                        spikeCount_, frameTime*1000, current_.updateTime*1000, current_.renderTime*1000,
                        current_.chunkUpdateTime*1000, current_.uploadTime*1000, current_.uiTime*1000,
                        current_.chunkJobsPending);
        }
    }

    if (config_.logInputJitter) {
        float rawMag = std::sqrt(current_.mouseDeltaX * current_.mouseDeltaX + current_.mouseDeltaY * current_.mouseDeltaY);
        if (rawMag > 200.0f) {
            SPDLOG_WARN("INPUT JITTER: raw=({:.1f},{:.1f}) applied=({:.1f},{:.1f}) captured={} frame={}",
                        current_.mouseDeltaX, current_.mouseDeltaY,
                        current_.appliedDeltaX, current_.appliedDeltaY,
                        current_.cursorCaptured, current_.frameIndex);
        }
    }

    writeTraceFrame();
}

void Diagnostics::beginSection(const std::string& name) {
    sectionStarts_[name] = std::chrono::high_resolution_clock::now();
}

void Diagnostics::endSection(const std::string& name) {
    auto it = sectionStarts_.find(name);
    if (it == sectionStarts_.end()) return;
    auto now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now - it->second).count();
    sectionTimes_[name] = dt;

    if (name == "update") current_.updateTime = dt;
    else if (name == "render") current_.renderTime = dt;
    else if (name == "input") current_.inputTime = dt;
    else if (name == "chunkUpdate") current_.chunkUpdateTime = dt;
    else if (name == "upload") current_.uploadTime = dt;
    else if (name == "ui") current_.uiTime = dt;
}

float Diagnostics::sectionTime(const std::string& name) const {
    auto it = sectionTimes_.find(name);
    return it != sectionTimes_.end() ? it->second : 0.0f;
}

void Diagnostics::recordMouseDelta(float rawX, float rawY, float appliedX, float appliedY, bool captured) {
    current_.mouseDeltaX = rawX;
    current_.mouseDeltaY = rawY;
    current_.appliedDeltaX = appliedX;
    current_.appliedDeltaY = appliedY;
    current_.cursorCaptured = captured;
}

void Diagnostics::recordChunkUpdate(int pending, int completed, int genTimeUs, int uploadBytes) {
    current_.chunkJobsPending = pending;
    current_.chunkJobsCompleted = completed;
    current_.chunkGenTimeUs = genTimeUs;
    current_.uploadBytesThisFrame = uploadBytes;
}

void Diagnostics::recordInteraction(bool hit, float dist, int bx, int by, int bz, int slot) {
    current_.raycastHit = hit;
    current_.raycastDist = dist;
    current_.raycastBlockX = bx;
    current_.raycastBlockY = by;
    current_.raycastBlockZ = bz;
    current_.selectedSlot = slot;
}

void Diagnostics::recordPlaceAttempt(bool succeeded) {
    current_.placeAttempted = true;
    current_.placeSucceeded = succeeded;
}

void Diagnostics::recordRemoveAttempt(bool succeeded) {
    current_.removeAttempted = true;
    current_.removeSucceeded = succeeded;
}

bool Diagnostics::shouldQuit() const {
    if (config_.quitAfterSeconds > 0 && elapsedTime_ >= config_.quitAfterSeconds) return true;
    return false;
}

void Diagnostics::writeTraceFrame() {
    if (!traceFile_.is_open()) return;
    auto& f = current_;
    traceFile_ << f.frameIndex << ","
               << f.frameTime << "," << f.updateTime << "," << f.renderTime << ","
               << f.inputTime << "," << f.chunkUpdateTime << "," << f.uploadTime << "," << f.uiTime << ","
               << f.mouseDeltaX << "," << f.mouseDeltaY << ","
               << f.appliedDeltaX << "," << f.appliedDeltaY << ","
               << f.cursorCaptured << ","
               << f.chunksVisible << "," << f.chunksLoaded << ","
               << f.chunksUploadedThisFrame << "," << f.chunkJobsPending << ","
               << f.drawCalls << "," << f.triangles << ","
               << f.raycastHit << "," << f.raycastDist << ","
               << f.raycastBlockX << "," << f.raycastBlockY << "," << f.raycastBlockZ << ","
               << f.selectedSlot << ","
               << f.placeAttempted << "," << f.placeSucceeded << ","
               << f.removeAttempted << "," << f.removeSucceeded
               << "\n";
}

std::string Diagnostics::getHudText() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);

    ss << "== FRAME ==\n";
    ss << "FPS: " << fps_ << " ft: " << current_.frameTime * 1000 << "ms\n";
    ss << "1%low: " << frameTime1pctLow_ * 1000 << "ms worst: " << worstFrameTime_ * 1000 << "ms\n";
    ss << "update: " << current_.updateTime * 1000 << " render: " << current_.renderTime * 1000 << "ms\n";
    ss << "input: " << current_.inputTime * 1000 << " chunk: " << current_.chunkUpdateTime * 1000 << "ms\n";
    ss << "upload: " << current_.uploadTime * 1000 << " ui: " << current_.uiTime * 1000 << "ms\n";

    ss << "\n== INPUT ==\n";
    ss << "raw: (" << current_.mouseDeltaX << "," << current_.mouseDeltaY << ")\n";
    ss << "applied: (" << current_.appliedDeltaX << "," << current_.appliedDeltaY << ")\n";
    ss << "captured: " << (current_.cursorCaptured ? "YES" : "NO") << "\n";

    ss << "\n== WORLD ==\n";
    ss << "visible: " << current_.chunksVisible << " loaded: " << current_.chunksLoaded << "\n";
    ss << "pending: " << current_.chunkJobsPending << " uploaded: " << current_.chunksUploadedThisFrame << "\n";
    ss << "uploadBytes: " << current_.uploadBytesThisFrame << "\n";

    ss << "\n== INTERACTION ==\n";
    if (current_.raycastHit) {
        ss << "target: (" << current_.raycastBlockX << "," << current_.raycastBlockY << "," << current_.raycastBlockZ << ")\n";
        ss << "dist: " << current_.raycastDist << " slot: " << current_.selectedSlot << "\n";
    } else {
        ss << "target: NONE\n";
    }

    return ss.str();
}

std::string Diagnostics::getSummaryReport() const {
    std::ostringstream ss;
    ss << "===== DIAGNOSTICS SUMMARY =====\n";
    ss << "Total frames: " << totalFrames_ << "\n";
    ss << "Total time: " << elapsedTime_ << "s\n";
    ss << "Average FPS: " << (elapsedTime_ > 0 ? totalFrames_ / elapsedTime_ : 0) << "\n";
    ss << "Worst frame time: " << worstFrameTime_ * 1000 << "ms\n";
    ss << "1% low frame time: " << frameTime1pctLow_ * 1000 << "ms\n";
    ss << "Frame spikes (>33ms): " << spikeCount_ << "\n";
    ss << "===============================\n";
    return ss.str();
}

} // namespace VoxelForge
