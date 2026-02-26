/**
 * @file JobSystem.cpp
 * * @brief Job system for parallel processing
 */

#include <VoxelForge/engine/JobSystem.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>
#include <thread>

namespace VoxelForge {

// ============================================
// Job System
// ============================================

JobSystem::JobSystem(size_t numThreads) {
    if (numThreads == 0) {
        numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);
    }
    
    VF_CORE_INFO("Job system initializing with {} threads", numThreads);
    
    workers.resize(numThreads);
    
    for (size_t i = 0; i < numThreads; i++) {
        workers[i].thread = std::thread([this, i]() {
            workerThread(i);
        });
    }
}

JobSystem::~JobSystem() {
    {
        std::unique_lock lock(queueMutex);
        running = false;
    }
    queueCV.notify_all();
    
    for (auto& worker : workers) {
        if (worker.thread.joinable()) {
            worker.thread.join();
        }
    }
    
    VF_CORE_INFO("Job system shut down");
}

JobHandle JobSystem::submit(Job&& job) {
    std::unique_lock lock(queueMutex);
    
    JobHandle handle(nextJobId++);
    jobs[handle.id] = std::move(job);
    jobs[handle.id].state = JobState::Queued;
    
    queueCV.notify_one();
    
    return handle;
}

JobHandle JobSystem::submit(std::function<void()>&& task, Priority priority) {
    Job job;
    job.task = std::move(task);
    job.priority = priority;
    job.id = nextJobId++;
    
    std::unique_lock lock(queueMutex);
    jobs[job.id] = std::move(job);
    jobs[job.id].state = JobState::Queued;
    
    JobHandle handle(job.id);
    
    queueCV.notify_one();
    
    return handle;
}

void JobSystem::waitFor(JobHandle handle) {
    while (true) {
        std::shared_lock lock(queueMutex);
        auto it = jobs.find(handle.id);
        if (it == jobs.end() || it->second.state == JobState::Completed) {
            return;
        }
        lock.unlock();
        std::this_thread::yield();
    }
}

bool JobSystem::isComplete(JobHandle handle) const {
    std::shared_lock lock(queueMutex);
    auto it = jobs.find(handle.id);
    return it == jobs.end() || it->second.state == JobState::Completed;
}

void JobSystem::waitAll() {
    while (true) {
        std::shared_lock lock(queueMutex);
        bool allComplete = true;
        for (const auto& [id, job] : jobs) {
            if (job.state != JobState::Completed) {
                allComplete = false;
                break;
            }
        }
        if (allComplete) return;
        lock.unlock();
        std::this_thread::yield();
    }
}

void JobSystem::workerThread(size_t index) {
    while (running) {
        Job job;
        
        {
            std::unique_lock lock(queueMutex);
            queueCV.wait(lock, [this]() {
                return !running || !jobs.empty();
            });
            
            if (!running) return;
            
            // Find highest priority job
            auto bestIt = jobs.end();
            int bestPriority = -1;
            
            for (auto it = jobs.begin(); it != jobs.end(); ++it) {
                if (it->second.state == JobState::Queued && 
                    static_cast<int>(it->second.priority) > bestPriority) {
                    bestPriority = static_cast<int>(it->second.priority);
                    bestIt = it;
                }
            }
            
            if (bestIt == jobs.end()) continue;
            
            job = std::move(bestIt->second);
            job.state = JobState::Running;
            bestIt->second = std::move(job);
            job = bestIt->second;
        }
        
        // Execute job
        try {
            if (job.task) {
                job.task();
            }
        }
        catch (const std::exception& e) {
            VF_CORE_ERROR("Job {} threw exception: {}", job.id, e.what());
        }
        
        // Mark complete
        {
            std::unique_lock lock(queueMutex);
            auto it = jobs.find(job.id);
            if (it != jobs.end()) {
                it->second.state = JobState::Completed;
            }
        }
    }
}

// Global job system
static std::unique_ptr<JobSystem> globalJobSystem;

JobSystem& GetJobSystem() {
    if (!globalJobSystem) {
        globalJobSystem = std::make_unique<JobSystem>();
    }
    return *globalJobSystem;
}

void InitJobSystem(size_t numThreads) {
    globalJobSystem = std::make_unique<JobSystem>(numThreads);
}

void ShutdownJobSystem() {
    globalJobSystem.reset();
}

} // namespace VoxelForge
