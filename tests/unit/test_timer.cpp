/**
 * @file test_timer.cpp
 * @brief Timer unit tests
 */

#include <gtest/gtest.h>
#include <VoxelForge/core/Timer.hpp>
#include <thread>
#include <chrono>

using namespace VoxelForge;

class TimerTest : public ::testing::Test {
protected:
    Timer timer;
};

TEST_F(TimerTest, TimerStartsAtZero) {
    // Timer should have just been created, so elapsed should be tiny
    EXPECT_LT(timer.elapsed(), 0.01f);
}

TEST_F(TimerTest, TimerMeasuresElapsedTime) {
    timer.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    float elapsed = timer.elapsedMillis();
    EXPECT_GE(elapsed, 45.0f);  // Allow some tolerance
    EXPECT_LT(elapsed, 100.0f);
}

TEST_F(TimerTest, TimerResetWorks) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    timer.reset();
    
    float elapsed = timer.elapsedMillis();
    EXPECT_LT(elapsed, 10.0f);
}

TEST_F(TimerTest, TimerMicrosWorks) {
    timer.reset();
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
    
    i64 micros = timer.elapsedMicros();
    EXPECT_GE(micros, 900);
    EXPECT_LT(micros, 5000);
}

TEST_F(TimerTest, TimerNanosWorks) {
    timer.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    
    i64 nanos = timer.elapsedNanos();
    EXPECT_GE(nanos, 900000);
}

TEST_F(TimerTest, GetCurrentTimeIncreases) {
    float t1 = Timer::getCurrentTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    float t2 = Timer::getCurrentTime();
    
    EXPECT_GT(t2, t1);
}

TEST_F(TimerTest, ScopedTimerWorks) {
    bool callbackCalled = false;
    float capturedTime = 0;
    
    {
        ScopedTimer scoped([&](float time) {
            callbackCalled = true;
            capturedTime = time;
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    EXPECT_TRUE(callbackCalled);
    EXPECT_GE(capturedTime, 8.0f);
}

TEST_F(TimerTest, FPSCounterWorks) {
    FPSCounter counter;
    
    // Simulate some frames
    for (int i = 0; i < 100; i++) {
        counter.frame();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    float fps = counter.getFPS();
    EXPECT_GT(fps, 30.0f);
    EXPECT_LT(fps, 120.0f);
}

TEST_F(TimerTest, FPSCounterFrameTime) {
    FPSCounter counter;
    
    counter.frame();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    counter.frame();
    
    float frameTime = counter.getFrameTime();
    EXPECT_GE(frameTime, 14.0f);
    EXPECT_LT(frameTime, 30.0f);
}
