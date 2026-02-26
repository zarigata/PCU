/**
 * @file Window.cpp
 * @brief Window implementation using GLFW
 */

#include <VoxelForge/core/Window.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <GLFW/glfw3.h>

namespace VoxelForge {

static bool glfwInitialized = false;
static int windowCount = 0;

void Window::initGLFW() {
    if (glfwInitialized) return;
    
    int success = glfwInit();
    VF_ASSERT(success, "Could not initialize GLFW!");
    
    glfwInitialized = true;
    VF_CORE_INFO("GLFW initialized: {}", glfwGetVersionString());
}

void Window::terminateGLFW() {
    if (!glfwInitialized) return;
    if (windowCount > 0) return;
    
    glfwTerminate();
    glfwInitialized = false;
    VF_CORE_INFO("GLFW terminated");
}

Window::Window(const WindowProps& props) {
    init(props);
    windowCount++;
}

Window::~Window() {
    shutdown();
    windowCount--;
    
    if (windowCount == 0) {
        terminateGLFW();
    }
}

void Window::init(const WindowProps& props) {
    initGLFW();
    
    data.title = props.title;
    data.width = props.width;
    data.height = props.height;
    data.vsync = props.vsync;
    data.fullscreen = props.fullscreen;
    
    VF_CORE_INFO("Creating window: {} ({}x{})", props.title, props.width, props.height);
    
    // Configure GLFW for Vulkan (no OpenGL context)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, props.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, props.decorated ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    
    // Create window
    GLFWmonitor* monitor = props.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    window = glfwCreateWindow(
        static_cast<int>(props.width),
        static_cast<int>(props.height),
        props.title.c_str(),
        monitor,
        nullptr
    );
    
    VF_ASSERT(window, "Failed to create GLFW window!");
    
    // Store window data pointer
    glfwSetWindowUserPointer(window, &data);
    
    setVSync(props.vsync);
    setupCallbacks();
    
    VF_CORE_INFO("Window created successfully");
}

void Window::shutdown() {
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
        VF_CORE_INFO("Window destroyed");
    }
}

void Window::setupCallbacks() {
    // Window resize
    glfwSetWindowSizeCallback(window, [](GLFWwindow* win, int width, int height) {
        auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(win));
        data->width = static_cast<uint32_t>(width);
        data->height = static_cast<uint32_t>(height);
        
        // TODO: Dispatch WindowResizeEvent
    });
    
    // Window close
    glfwSetWindowCloseCallback(window, [](GLFWwindow* win) {
        auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(win));
        // TODO: Dispatch WindowCloseEvent
    });
    
    // Key callback
    glfwSetKeyCallback(window, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
        auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(win));
        // TODO: Dispatch KeyEvent
    });
    
    // Mouse button callback
    glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int mods) {
        auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(win));
        // TODO: Dispatch MouseButtonEvent
    });
    
    // Mouse scroll callback
    glfwSetScrollCallback(window, [](GLFWwindow* win, double xoffset, double yoffset) {
        auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(win));
        // TODO: Dispatch MouseScrollEvent
    });
    
    // Cursor position callback
    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double xpos, double ypos) {
        auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(win));
        // TODO: Dispatch MouseMovedEvent
    });
    
    // Character input callback
    glfwSetCharCallback(window, [](GLFWwindow* win, unsigned int codepoint) {
        auto* data = static_cast<WindowData*>(glfwGetWindowUserPointer(win));
        // TODO: Dispatch KeyTypedEvent
    });
}

void Window::onUpdate() {
    glfwPollEvents();
}

void Window::setVSync(bool enabled) {
    data.vsync = enabled;
    glfwSwapInterval(enabled ? 1 : 0);
}

void Window::setFullscreen(bool fullscreen) {
    if (data.fullscreen == fullscreen) return;
    
    if (fullscreen) {
        // Save windowed position and size
        glfwGetWindowPos(window, &windowedX, &windowedY);
        glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
        
        // Switch to fullscreen
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        
        data.width = static_cast<uint32_t>(mode->width);
        data.height = static_cast<uint32_t>(mode->height);
    } else {
        // Restore windowed mode
        glfwSetWindowMonitor(window, nullptr, windowedX, windowedY, 
                            windowedWidth, windowedHeight, 0);
        data.width = static_cast<uint32_t>(windowedWidth);
        data.height = static_cast<uint32_t>(windowedHeight);
    }
    
    data.fullscreen = fullscreen;
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(window);
}

void Window::close() {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void Window::setTitle(const std::string& title) {
    data.title = title;
    glfwSetWindowTitle(window, title.c_str());
}

void Window::setSize(uint32_t width, uint32_t height) {
    glfwSetWindowSize(window, static_cast<int>(width), static_cast<int>(height));
    data.width = width;
    data.height = height;
}

void Window::setCursorMode(int mode) {
    glfwSetInputMode(window, GLFW_CURSOR, mode);
}

void Window::setCursorPos(double x, double y) {
    glfwSetCursorPos(window, x, y);
}

void Window::getCursorPos(double& x, double& y) {
    glfwGetCursorPos(window, &x, &y);
}

void Window::hideCursor() {
    setCursorMode(GLFW_CURSOR_HIDDEN);
}

void Window::showCursor() {
    setCursorMode(GLFW_CURSOR_NORMAL);
}

void Window::disableCursor() {
    setCursorMode(GLFW_CURSOR_DISABLED);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::waitEvents() {
    glfwWaitEvents();
}

} // namespace VoxelForge
