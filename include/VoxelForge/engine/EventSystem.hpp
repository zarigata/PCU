/**
 * @file EventSystem.hpp
 * @brief Event system for VoxelForge
 */

#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <mutex>

namespace VoxelForge {

// Base event class
class Event {
public:
    virtual ~Event() = default;
    
    bool isHandled() const { return handled; }
    void handle() { handled = true; }
    
protected:
    bool handled = false;
};

// Window Events
class WindowResizeEvent : public Event {
public:
    WindowResizeEvent(uint32_t width, uint32_t height)
        : width(width), height(height) {}
    
    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
    
private:
    uint32_t width, height;
};

class WindowCloseEvent : public Event {
public:
    WindowCloseEvent() = default;
};

class WindowFocusEvent : public Event {
public:
    WindowFocusEvent(bool focused) : focused(focused) {}
    bool isFocused() const { return focused; }
private:
    bool focused;
};

// Key Events
class KeyEvent : public Event {
public:
    KeyEvent(int key, int scancode, int action, int mods)
        : key(key), scancode(scancode), action(action), mods(mods) {}
    
    int getKey() const { return key; }
    int getScanCode() const { return scancode; }
    int getAction() const { return action; }
    int getMods() const { return mods; }
    
private:
    int key, scancode, action, mods;
};

class KeyPressedEvent : public KeyEvent {
public:
    KeyPressedEvent(int key, int scancode, int mods, bool repeat)
        : KeyEvent(key, scancode, 1, mods), repeat(repeat) {}
    
    bool isRepeat() const { return repeat; }
private:
    bool repeat;
};

class KeyReleasedEvent : public KeyEvent {
public:
    KeyReleasedEvent(int key, int scancode)
        : KeyEvent(key, scancode, 0, 0) {}
};

class KeyTypedEvent : public Event {
public:
    KeyTypedEvent(uint32_t character) : character(character) {}
    uint32_t getCharacter() const { return character; }
private:
    uint32_t character;
};

// Mouse Events
class MouseMovedEvent : public Event {
public:
    MouseMovedEvent(float x, float y, float dx = 0, float dy = 0)
        : x(x), y(y), dx(dx), dy(dy) {}
    
    float getX() const { return x; }
    float getY() const { return y; }
    float getDeltaX() const { return dx; }
    float getDeltaY() const { return dy; }
    
private:
    float x, y, dx, dy;
};

class MouseButtonEvent : public Event {
public:
    MouseButtonEvent(int button, int action, int mods)
        : button(button), action(action), mods(mods) {}
    
    int getButton() const { return button; }
    int getAction() const { return action; }
    int getMods() const { return mods; }
    
private:
    int button, action, mods;
};

class MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(float xOffset, float yOffset)
        : xOffset(xOffset), yOffset(yOffset) {}
    
    float getXOffset() const { return xOffset; }
    float getYOffset() const { return yOffset; }
    
private:
    float xOffset, yOffset;
};

// Event Bus
class EventBus {
public:
    template<typename T>
    using Callback = std::function<void(T&)>;
    
    template<typename T>
    void subscribe(Callback<T> callback) {
        std::type_index type(typeid(T));
        auto& handlers = handlers_[type];
        
        auto wrapper = [callback](Event& e) {
            callback(static_cast<T&>(e));
        };
        
        handlers.push_back(std::make_unique<Handler<T>>(wrapper, callback));
    }
    
    template<typename T>
    void publish(T& event) {
        std::type_index type(typeid(T));
        auto it = handlers_.find(type);
        
        if (it != handlers_.end()) {
            for (auto& handler : it->second) {
                handler->invoke(event);
                if (event.isHandled()) break;
            }
        }
    }
    
    void clear() {
        handlers_.clear();
    }
    
private:
    struct HandlerBase {
        virtual ~HandlerBase() = default;
        virtual void invoke(Event& e) = 0;
    };
    
    template<typename T>
    struct Handler : HandlerBase {
        std::function<void(Event&)> invokeFunc;
        Callback<T> originalCallback;
        
        Handler(std::function<void(Event&)> func, Callback<T> cb)
            : invokeFunc(func), originalCallback(cb) {}
        
        void invoke(Event& e) override {
            invokeFunc(e);
        }
    };
    
    std::unordered_map<std::type_index, std::vector<std::unique_ptr<HandlerBase>>> handlers_;
};

// Global event bus
inline EventBus& getEventBus() {
    static EventBus instance;
    return instance;
}

} // namespace VoxelForge

// Macro helpers
#define VF_SUBSCRIBE(event_type, callback) \
    VoxelForge::getEventBus().subscribe<event_type>(callback)

#define VF_PUBLISH(event) \
    VoxelForge::getEventBus().publish(event)
