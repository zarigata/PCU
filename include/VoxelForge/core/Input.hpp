/**
 * @file Input.hpp
 * @brief Input handling system
 */

#pragma once

#include <string>

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <functional>
#include <glm/glm.hpp>

// Forward declarations
struct GLFWwindow;

namespace VoxelForge {

// Key codes (GLFW compatible)
namespace Key {
    enum : int {
        Unknown = -1,
        Space = 32,
        Apostrophe = 39,
        Comma = 44,
        Minus = 45,
        Period = 46,
        Slash = 47,
        D0 = 48, D1, D2, D3, D4, D5, D6, D7, D8, D9,
        Semicolon = 59,
        Equal = 61,
        A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
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
        PageUp = 266,
        PageDown = 267,
        Home = 268,
        End = 269,
        CapsLock = 280,
        ScrollLock = 281,
        NumLock = 282,
        PrintScreen = 283,
        Pause = 284,
        F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25,
        KP0 = 320, KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9,
        KPDecimal = 330,
        KPDivide = 331,
        KPMultiply = 332,
        KPSubtract = 333,
        KPAdd = 334,
        KPEnter = 335,
        KPEqual = 336,
        LeftShift = 340,
        LeftControl = 341,
        LeftAlt = 342,
        LeftSuper = 343,
        RightShift = 344,
        RightControl = 345,
        RightAlt = 346,
        RightSuper = 347,
        Menu = 348
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
        Button7 = 7,
        Left = Button0,
        Right = Button1,
        Middle = Button2
    };
}

// Action types
namespace Action {
    enum : int {
        Release = 0,
        Press = 1,
        Repeat = 2
    };
}

// Modifier keys
namespace Mod {
    enum : int {
        None = 0,
        Shift = 1,
        Control = 2,
        Alt = 4,
        Super = 8,
        CapsLock = 16,
        NumLock = 32
    };
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
    bool isKeyDown(int key) const;
    
    // Mouse state
    bool isMouseButtonPressed(int button) const;
    bool isMouseButtonJustPressed(int button) const;
    bool isMouseButtonJustReleased(int button) const;
    bool isMouseButtonDown(int button) const;
    
    // Mouse position
    glm::vec2 getMousePosition() const { return current.mousePosition; }
    glm::vec2 getMouseDelta() const { return current.mouseDelta; }
    float getMouseX() const { return current.mousePosition.x; }
    float getMouseY() const { return current.mousePosition.y; }
    
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
