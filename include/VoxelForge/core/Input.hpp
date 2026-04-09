/**
 * @file Input.hpp
 * @brief Input system for VoxelForge
 */

#pragma once

#include <GLFW/glfw3.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include <glm/glm.hpp>

namespace VoxelForge {

// Key codes (GLFW keys)
namespace Key {
    enum : int {
        Unknown = -1,
        Space = 32,
        Apostrophe = 39,
        Comma = 44,
        Minus = 45,
        Period = 46,
        Slash = 47,
        Num0 = 48,
        Num1 = 49,
        Num2 = 50,
        Num3 = 51,
        Num4 = 52,
        Num5 = 53,
        Num6 = 54,
        Num7 = 55,
        Num8 = 56,
        Num9 = 57,
        Semicolon = 59,
        Equal = 61,
        A = 65,
        B = 66,
        C = 67,
        D = 68,
        E = 69,
        F = 70,
        G = 71,
        H = 72,
        I = 73,
        J = 74,
        K = 75,
        L = 76,
        M = 77,
        N = 78,
        O = 79,
        P = 80,
        Q = 81,
        R = 82,
        S = 83,
        T = 84,
        U = 85,
        V = 86,
        W = 87,
        X = 88,
        Y = 89,
        Z = 90,
        LeftBracket = 91,
        Backslash = 92,
        RightBracket = 93,
        GraveAccent = 96,
        Escape = 256,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Insert = 260,
        Delete = 261,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        LeftShift = 340,
        LeftControl = 341,
        LeftAlt = 342,
        LeftSuper = 343,
        RightShift = 344,
        RightControl = 345,
        RightAlt = 346,
        RightSuper = 347
    };
}

// Mouse buttons
namespace Mouse {
    enum : int {
        Button0 = 0,
        Button1 = 1,
        Button2 = 2,
        Button3 = 3,
        Button4 = 4,
        Button5 = 5,
        Button6 = 6,
        Button7 = 7
    };
    
    constexpr int Left = Button0;
    constexpr int Right = Button1;
    constexpr int Middle = Button2;
}

// Cursor modes
namespace CursorMode {
    enum : int {
        Normal = 0x00034001,
        Hidden = 0x00034002,
        Disabled = 0x00034003
    };
}

// Input state for current frame
struct InputState {
    std::unordered_map<int, bool> keys;
    std::unordered_map<int, bool> mouseButtons;
    glm::vec2 mousePosition = glm::vec2(0.0f);
    glm::vec2 mouseDelta = glm::vec2(0.0f);
    glm::vec2 scrollDelta = glm::vec2(0.0f);
    std::vector<uint32_t> typedCharacters;
};

class Input {
public:
    Input() = default;
    ~Input() = default;
    
    // Initialize with window
    void init(GLFWwindow* window);
    void shutdown();
    
    // Update - call at start of frame
    void update();
    
    // Key state
    bool isKeyPressed(int key) const;
    bool isKeyJustPressed(int key) const;
    bool isKeyJustReleased(int key) const;
    
    // Mouse state
    bool isMouseButtonPressed(int button) const;
    bool isMouseButtonJustPressed(int button) const;
    bool isMouseButtonJustReleased(int button) const;
    
    // Mouse position
    float getMouseX() const { return current.mousePosition.x; }
    float getMouseY() const { return current.mousePosition.y; }
    glm::vec2 getMousePosition() const { return current.mousePosition; }
    glm::vec2 getMouseDelta() const { return current.mouseDelta; }
    
    // Scroll
    glm::vec2 getScrollDelta() const { return current.scrollDelta; }
    
    // Typed characters
    const std::vector<uint32_t>& getTypedCharacters() const { return current.typedCharacters; }
    
    // Cursor control
    void setCursorMode(int mode);
    void setMousePosition(float x, float y);
    
    // Key bindings
    void bindKey(const std::string& action, int key);
    int getKeyBinding(const std::string& action) const;
    bool isActionPressed(const std::string& action) const;
    
private:
    GLFWwindow* window = nullptr;
    InputState current;
    InputState previous;
    std::unordered_map<std::string, int> keyBindings;
    
    // GLFW callbacks
    static void keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
    static void charCallback(GLFWwindow* win, unsigned int codepoint);
    static void mouseButtonCallback(GLFWwindow* win, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* win, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* win, double xoffset, double yoffset);
};

} // namespace VoxelForge