/**
 * @file JobSystem.hpp
 * @brief Job system for parallel task execution
 */

#pragma once

#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>
#include <cstdint>

namespace VoxelForge {

// Job priority levels
enum class Priority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

// Forward declarations
struct Job;
class JobSystem;

// Job handle for tracking
struct JobHandle {
    uint64_t id = 0;
    
    bool isValid() const { return id != 0; }
    bool operator==(const JobHandle& other) const { return id == other.id; }
    bool operator!=(const JobHandle& other) const { return id != other.id; }
};

// Job state
enum class JobState {
    Queued,
    Running,
    Completed
};

// Job definition
struct Job {
    uint64_t id = 0;
    std::function<void()> task;
    Priority priority = Priority::Normal;
    JobState state = JobState::Queued;
    std::vector<JobHandle> dependencies;
};

// Job system for parallel execution
class JobSystem {
public:
    explicit JobSystem(size_t numThreads = 0);  // 0 = auto
    ~JobSystem();
    
    // No copy
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;
    
    // Submit a job
    JobHandle submit(Job&& job);
    JobHandle submit(std::function<void()>&& task, Priority priority = Priority::Normal);
    
    // Submit with dependencies
    JobHandle submitAfter(JobHandle dependency, std::function<void()>&& task, Priority priority = Priority::Normal);
    JobHandle submitAfter(const std::vector<JobHandle>& dependencies, std::function<void()>&& task, Priority priority = Priority::Normal);
    
    // Wait for completion
    void waitFor(JobHandle handle);
    void waitFor(const std::vector<JobHandle>& handles);
    void waitAll();
    
    // Check status
    bool isComplete(JobHandle handle) const;
    bool isComplete(const std::vector<JobHandle>& handles) const;
    
    // Parallel for
    template<typename T>
    void parallelFor(std::vector<T>& data, std::function<void(T&)> func, size_t batchSize = 100);
    
    template<typename T>
    void parallelFor(const std::vector<T>& data, std::function<void(const T&)> func, size_t batchSize = 100);
    
    // Parallel for with index
    void parallelFor(size_t start, size_t end, std::function<void(size_t)> func, size_t batchSize = 100);
    
    // Get worker count
    size_t getWorkerCount() const { return workers.size(); }
    
private:
    void workerThread(size_t index);
    
    struct Worker {
        std::thread thread;
    };
    
    std::vector<Worker> workers;
    mutable std::shared_mutex queueMutex;
    std::condition_variable_any queueCV;
    std::atomic<bool> running{true};
    
    std::unordered_map<uint64_t, Job> jobs;
    std::atomic<uint64_t> nextJobId{1};
};

// Parallel for implementations
template<typename T>
void JobSystem::parallelFor(std::vector<T>& data, std::function<void(T&)> func, size_t batchSize) {
    if (data.empty()) return;
    
    std::atomic<size_t> index{0};
    std::vector<JobHandle> handles;
    
    size_t numBatches = (data.size() + batchSize - 1) / batchSize;
    
    for (size_t b = 0; b < numBatches; b++) {
        handles.push_back(submit([&data, &index, &func, batchSize]() {
            size_t start = index.fetch_add(batchSize);
            size_t end = std::min(start + batchSize, data.size());
            for (size_t i = start; i < end; i++) {
                func(data[i]);
            }
        }, Priority::Normal));
    }
    
    waitFor(handles);
}

template<typename T>
void JobSystem::parallelFor(const std::vector<T>& data, std::function<void(const T&)> func, size_t batchSize) {
    if (data.empty()) return;
    
    std::atomic<size_t> index{0};
    std::vector<JobHandle> handles;
    
    size_t numBatches = (data.size() + batchSize - 1) / batchSize;
    
    for (size_t b = 0; b < numBatches; b++) {
        handles.push_back(submit([&data, &index, &func, batchSize]() {
            size_t start = index.fetch_add(batchSize);
            size_t end = std::min(start + batchSize, data.size());
            for (size_t i = start; i < end; i++) {
                func(data[i]);
            }
        }, Priority::Normal));
    }
    
    waitFor(handles);
}

// Global job system access
JobSystem& GetJobSystem();
void InitJobSystem(size_t numThreads = 0);
void ShutdownJobSystem();

} // namespace VoxelForge
