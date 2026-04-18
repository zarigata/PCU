/**
 * @file Input.cpp
 * @brief Input system implementation
 */

#include <VoxelForge/core/Input.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <GLFW/glfw3.h>
#include <cstring>

namespace VoxelForge {

void Input::init(GLFWwindow* win) {
    window = win;
    
    glfwSetWindowUserPointer(window, this);
    
    if (glfwRawMouseMotionSupported()) {
        rawMotionSupported_ = true;
        VF_CORE_INFO("Raw mouse motion supported");
    } else {
        rawMotionSupported_ = false;
        VF_CORE_WARN("Raw mouse motion NOT supported — falling back to cursor delta");
    }
    
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharCallback(window, charCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    
    bindKey("forward", Key::W);
    bindKey("backward", Key::S);
    bindKey("left", Key::A);
    bindKey("right", Key::D);
    bindKey("jump", Key::Space);
    bindKey("sneak", Key::LeftShift);
    bindKey("sprint", Key::LeftControl);
    bindKey("inventory", Key::E);
    bindKey("chat", Key::T);
    bindKey("pause", Key::Escape);
    
    VF_CORE_INFO("Input system initialized");
}

void Input::shutdown() {
    window = nullptr;
    current.keys.clear();
    current.mouseButtons.clear();
    previous.keys.clear();
    previous.mouseButtons.clear();
    keyBindings.clear();
    VF_CORE_INFO("Input system shut down");
}

void Input::update() {
    // Store previous state
    previous = current;
    
    // Clear per-frame data
    current.mouseDelta = glm::vec2(0.0f);
    current.scrollDelta = glm::vec2(0.0f);
    current.typedCharacters.clear();
    
    // Poll events
    glfwPollEvents();
}

bool Input::isKeyPressed(int key) const {
    return current.keys.count(key) && current.keys.at(key);
}

bool Input::isKeyJustPressed(int key) const {
    return isKeyPressed(key) && !(previous.keys.count(key) && previous.keys.at(key));
}

bool Input::isKeyJustReleased(int key) const {
    return !isKeyPressed(key) && (previous.keys.count(key) && previous.keys.at(key));
}

bool Input::isMouseButtonPressed(int button) const {
    return current.mouseButtons.count(button) && current.mouseButtons.at(button);
}

bool Input::isMouseButtonJustPressed(int button) const {
    return isMouseButtonPressed(button) && 
           !(previous.mouseButtons.count(button) && previous.mouseButtons.at(button));
}

bool Input::isMouseButtonJustReleased(int button) const {
    return !isMouseButtonPressed(button) && 
           (previous.mouseButtons.count(button) && previous.mouseButtons.at(button));
}

void Input::setCursorCaptured(bool captured) {
    cursorCaptured_ = captured;
    if (captured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        if (rawMotionSupported_) {
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        current.mousePosition = glm::vec2(static_cast<float>(mx), static_cast<float>(my));
        current.mouseDelta = glm::vec2(0.0f);
    } else {
        if (rawMotionSupported_) {
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        }
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void Input::setCursorMode(int mode) {
    glfwSetInputMode(window, GLFW_CURSOR, mode);
}

void Input::setMousePosition(float x, float y) {
    glfwSetCursorPos(window, x, y);
    current.mousePosition = glm::vec2(x, y);
}

void Input::bindKey(const std::string& action, int key) {
    keyBindings[action] = key;
}

int Input::getKeyBinding(const std::string& action) const {
    auto it = keyBindings.find(action);
    if (it != keyBindings.end()) {
        return it->second;
    }
    return Key::Unknown;
}

bool Input::isActionPressed(const std::string& action) const {
    int key = getKeyBinding(action);
    return key != Key::Unknown && isKeyPressed(key);
}

void Input::keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods) {
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(win));
    if (!input) return;
    
    if (key >= 0 && key < 512) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            input->current.keys[key] = true;
        } else if (action == GLFW_RELEASE) {
            input->current.keys[key] = false;
        }
    }
}

void Input::charCallback(GLFWwindow* win, unsigned int codepoint) {
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(win));
    if (!input) return;
    
    input->current.typedCharacters.push_back(codepoint);
}

void Input::mouseButtonCallback(GLFWwindow* win, int button, int action, int mods) {
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(win));
    if (!input) return;
    
    if (button >= 0 && button < 8) {
        if (action == GLFW_PRESS) {
            input->current.mouseButtons[button] = true;
        } else if (action == GLFW_RELEASE) {
            input->current.mouseButtons[button] = false;
        }
    }
}

void Input::cursorPosCallback(GLFWwindow* win, double xpos, double ypos) {
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(win));
    if (!input) return;
    
    glm::vec2 newPos(static_cast<float>(xpos), static_cast<float>(ypos));
    
    if (input->cursorCaptured_) {
        input->current.mouseDelta += newPos - input->current.mousePosition;
    }
    input->current.mousePosition = newPos;
}

void Input::scrollCallback(GLFWwindow* win, double xoffset, double yoffset) {
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(win));
    if (!input) return;
    
    input->current.scrollDelta = glm::vec2(static_cast<float>(xoffset), 
                                            static_cast<float>(yoffset));
}

} // namespace VoxelForge
