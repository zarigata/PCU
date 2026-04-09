/**
 * @file EventSystem.cpp
 * @brief Event system implementation
 */

#include <VoxelForge/Engine.hpp>
#include <VoxelForge/engine/EventSystem.hpp>

namespace VoxelForge {

EventBus& EventBus::get() {
    static EventBus instance;
    return instance;
}

void EventBus::processEvents() {
    processing_ = true;
    while (!eventQueue_.empty()) {
        auto event = std::move(eventQueue_.front());
        eventQueue_.pop();
        event();
    }
    processing_ = false;
}

void EventBus::clear() {
    while (!eventQueue_.empty()) {
        eventQueue_.pop();
    }
}

} // namespace VoxelForge