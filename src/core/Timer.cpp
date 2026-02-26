/**
 * @file Timer.cpp
 * @brief Timer implementation
 */

#include <VoxelForge/core/Timer.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>

namespace VoxelForge {

// ============================================
// Timer
// ============================================

Timer::Timer() {
    reset();
}

void Timer::reset() {
    start = std::chrono::high_resolution_clock::now();
}

float Timer::elapsed() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start);
    return duration.count() / 1000000.0f;
}

float Timer::elapsedMillis() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start);
    return duration.count() / 1000.0f;
}

i64 Timer::elapsedMicros() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start);
    return duration.count();
}

i64 Timer::elapsedNanos() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start);
    return duration.count();
}

float Timer::getCurrentTime() {
    auto now = std::chrono::high_resolution_clock::now();
    auto epoch = now.time_since_epoch();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(epoch);
    return duration.count() / 1000000.0f;
}

i64 Timer::getCurrentNanos() {
    auto now = std::chrono::high_resolution_clock::now();
    auto epoch = now.time_since_epoch();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch);
    return duration.count();
}

// ============================================
// ScopedTimer
// ============================================

ScopedTimer::ScopedTimer(const std::string& name) 
    : name(name), callback(nullptr) {
}

ScopedTimer::ScopedTimer(std::function<void(float)> callback)
    : callback(callback) {
}

ScopedTimer::~ScopedTimer() {
    float elapsed = timer.elapsedMillis();
    if (callback) {
        callback(elapsed);
    } else {
        VF_CORE_TRACE("{}: {:.2f}ms", name, elapsed);
    }
}

// ============================================
// FPSCounter
// ============================================

FPSCounter::FPSCounter() {
    frameTimer.reset();
}

void FPSCounter::frame() {
    frameTime = frameTimer.elapsedMillis();
    frameTimer.reset();
    
    frameTimes[sampleIndex] = frameTime;
    sampleIndex = (sampleIndex + 1) % SAMPLE_SIZE;
    
    accumTime += frameTime;
    frameCount++;
    
    if (accumTime >= 1000.0f) {
        fps = frameCount * 1000.0f / accumTime;
        frameCount = 0;
        accumTime = 0.0f;
    }
}

float FPSCounter::getAverageFrameTime() const {
    float sum = 0.0f;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        sum += frameTimes[i];
    }
    return sum / SAMPLE_SIZE;
}

} // namespace VoxelForge
