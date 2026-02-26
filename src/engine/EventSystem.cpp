/**
 * @file EventSystem.cpp
 * @brief Event system implementation
 */

#include <VoxelForge/engine/EventSystem.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

EventBus& EventBus::get() {
    static EventBus instance;
    return instance;
}

void EventBus::processEvents() {
    // Process queued events
    processing_ = true;
    for (auto& handler : handlers_) {
        if (handler.callback) {
            // Process events
        }
    }
    processing_ = false;
}

void EventBus::clear() {
    handlers_.clear();
    eventQueue_ = std::queue<std::function<void()>>();
}

} // namespace VoxelForge
