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
    
    // Set user pointer for callbacks
    glfwSetWindowUserPointer(window, this);
    
    // Set callbacks
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharCallback(window, charCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    
    // Set up default key bindings
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

bool Input::isKeyDown(int key) const {
    return isKeyPressed(key);
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

bool Input::isMouseButtonDown(int button) const {
    return isMouseButtonPressed(button);
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
        if (action == Action::Press || action == Action::Repeat) {
            input->current.keys[key] = true;
        } else if (action == Action::Release) {
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
        if (action == Action::Press) {
            input->current.mouseButtons[button] = true;
        } else if (action == Action::Release) {
            input->current.mouseButtons[button] = false;
        }
    }
}

void Input::cursorPosCallback(GLFWwindow* win, double xpos, double ypos) {
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(win));
    if (!input) return;
    
    glm::vec2 newPos(static_cast<float>(xpos), static_cast<float>(ypos));
    input->current.mouseDelta = newPos - input->current.mousePosition;
    input->current.mousePosition = newPos;
}

void Input::scrollCallback(GLFWwindow* win, double xoffset, double yoffset) {
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(win));
    if (!input) return;
    
    input->current.scrollDelta = glm::vec2(static_cast<float>(xoffset), 
                                            static_cast<float>(yoffset));
}

} // namespace VoxelForge
