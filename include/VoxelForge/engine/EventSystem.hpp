/**
 * @file EventSystem.hpp
 * @brief Event system for VoxelForge
 */

#pragma once

#include <functional>
#include <vector>
#include <queue>
#include <memory>
#include <unordered_map>
#include <typeindex>

namespace VoxelForge {

class EventBus {
public:
    static EventBus& get();
    
    void processEvents();
    void clear();
    
    template<typename EventT, typename Handler>
    void subscribe(Handler&& handler) {
        auto& handlers = getHandlers<EventT>();
        handlers.push_back(std::forward<Handler>(handler));
    }
    
    template<typename EventT>
    void publish(EventT&& event) {
        auto& handlers = getHandlers<EventT>();
        for (auto& handler : handlers) {
            handler(std::forward<EventT>(event));
        }
    }

private:
    EventBus() = default;
    ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    
    std::vector<std::function<void()>> handlers_;
    std::queue<std::function<void()>> eventQueue_;
    bool processing_ = false;
    
    template<typename EventT>
    std::vector<std::function<void(const EventT&)>>& getHandlers() {
        static std::vector<std::function<void(const EventT&)>> handlers;
        return handlers;
    }
};

} // namespace VoxelForge