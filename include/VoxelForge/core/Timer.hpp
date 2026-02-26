/**
 * @file Timer.hpp
 * @brief High-resolution timer for profiling and game timing
 */

#pragma once

#include <chrono>
#include <string>
#include <functional>

namespace VoxelForge {

class Timer {
public:
    Timer();
    
    void reset();
    
    float elapsed() const;      // Seconds as float
    float elapsedMillis() const; // Milliseconds as float
    i64 elapsedMicros() const;  // Microseconds as integer
    i64 elapsedNanos() const;   // Nanoseconds as integer
    
    static float getCurrentTime(); // Current time in seconds
    static i64 getCurrentNanos();  // Current time in nanoseconds
    
private:
    std::chrono::high_resolution_clock::time_point start;
};

// Scoped timer for profiling
class ScopedTimer {
public:
    explicit ScopedTimer(const std::string& name);
    explicit ScopedTimer(std::function<void(float)> callback);
    ~ScopedTimer();
    
private:
    std::string name;
    std::function<void(float)> callback;
    Timer timer;
};

// FPS Counter
class FPSCounter {
public:
    FPSCounter();
    
    void frame();
    float getFPS() const { return fps; }
    float getFrameTime() const { return frameTime; }
    float getAverageFrameTime() const;
    
private:
    Timer frameTimer;
    float fps = 0.0f;
    float frameTime = 0.0f;
    int frameCount = 0;
    float accumTime = 0.0f;
    static constexpr int SAMPLE_SIZE = 60;
    float frameTimes[SAMPLE_SIZE] = {0};
    int sampleIndex = 0;
};

} // namespace VoxelForge
