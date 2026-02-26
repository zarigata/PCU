/**
 * @file Window.hpp
 * @brief Window management using GLFW
 */

#pragma once

#include <string>
#include <functional>
#include <memory>

struct GLFWwindow;

namespace VoxelForge {

struct WindowProps {
    std::string title = "VoxelForge";
    uint32_t width = 1280;
    uint32_t height = 720;
    bool fullscreen = false;
    bool vsync = true;
    bool resizable = true;
    bool decorated = true;
    
    WindowProps() = default;
    WindowProps(const std::string& title, uint32_t w, uint32_t h)
        : title(title), width(w), height(h) {}
};

class Window {
public:
    using EventCallback = std::function<void(struct Event&)>;
    
    Window(const WindowProps& props = WindowProps());
    ~Window();
    
    // No copy
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    
    void onUpdate();
    
    uint32_t getWidth() const { return data.width; }
    uint32_t getHeight() const { return data.height; }
    float getAspectRatio() const { 
        return static_cast<float>(data.width) / static_cast<float>(data.height); 
    }
    
    void setEventCallback(const EventCallback& callback) {
        data.eventCallback = callback;
    }
    
    void setVSync(bool enabled);
    bool isVSync() const { return data.vsync; }
    
    void setFullscreen(bool fullscreen);
    bool isFullscreen() const { return data.fullscreen; }
    
    void* getNativeWindow() const { return window; }
    GLFWwindow* getGLFWWindow() const { return window; }
    
    bool shouldClose() const;
    void close();
    
    void setTitle(const std::string& title);
    void setSize(uint32_t width, uint32_t height);
    
    // Cursor
    void setCursorMode(int mode);
    void setCursorPos(double x, double y);
    void getCursorPos(double& x, double& y);
    void hideCursor();
    void showCursor();
    void disableCursor();
    
    static void pollEvents();
    static void waitEvents();
    static void initGLFW();
    static void terminateGLFW();
    
private:
    void init(const WindowProps& props);
    void shutdown();
    void setupCallbacks();
    
    GLFWwindow* window = nullptr;
    
    struct WindowData {
        std::string title;
        uint32_t width, height;
        bool vsync;
        bool fullscreen;
        EventCallback eventCallback;
    };
    
    WindowData data;
    
    // For fullscreen toggle
    int windowedX = 0, windowedY = 0;
    int windowedWidth = 0, windowedHeight = 0;
};

} // namespace VoxelForge
