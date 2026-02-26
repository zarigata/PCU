/**
 * @file GUIRenderer.hpp
 * @brief Immediate mode and retained mode GUI rendering
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <VoxelForge/rendering/VulkanBuffer.hpp>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace VoxelForge {

class VulkanDevice;
class VulkanCommandBuffer;
class Input;

// GUI vertex format
struct GUIVertex {
    glm::vec2 position;
    glm::vec2 texCoord;
    glm::vec4 color;
    
    static vk::VertexInputBindingDescription getBindingDescription() {
        return {0, sizeof(GUIVertex), vk::VertexInputRate::eVertex};
    }
    
    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() {
        return {
            {0, 0, vk::Format::eR32G32Sfloat, offsetof(GUIVertex, position)},
            {1, 0, vk::Format::eR32G32Sfloat, offsetof(GUIVertex, texCoord)},
            {2, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(GUIVertex, color)}
        };
    }
};

// GUI draw command
struct GUIDrawCmd {
    uint32_t vertexOffset;
    uint32_t indexOffset;
    uint32_t indexCount;
    uint32_t textureId;
    glm::vec4 clipRect;
};

// Font glyph
struct Glyph {
    glm::vec2 uvMin;
    glm::vec2 uvMax;
    glm::ivec2 size;
    glm::ivec2 bearing;
    int advance;
};

// Font data
struct Font {
    std::string name;
    int size;
    float lineHeight;
    std::vector<Glyph> glyphs;
    vk::Image texture;
    vk::ImageView textureView;
    std::vector<uint8_t> textureData;
    int textureWidth;
    int textureHeight;
};

// GUI style
struct GUIStyle {
    // Colors
    glm::vec4 backgroundColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.9f);
    glm::vec4 borderColor = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
    glm::vec4 textColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec4 accentColor = glm::vec4(0.2f, 0.6f, 1.0f, 1.0f);
    glm::vec4 hoverColor = glm::vec4(0.2f, 0.2f, 0.25f, 1.0f);
    glm::vec4 activeColor = glm::vec4(0.15f, 0.15f, 0.2f, 1.0f);
    glm::vec4 disabledColor = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
    
    // Sizes
    float borderWidth = 1.0f;
    float borderRadius = 4.0f;
    float padding = 8.0f;
    float spacing = 4.0f;
    float itemHeight = 24.0f;
    
    // Animation
    float animationSpeed = 8.0f;
    
    // Minecraft-style presets
    static GUIStyle Minecraft();
    static GUIStyle Dark();
    static GUIStyle Light();
};

// GUI context for immediate mode rendering
struct GUIContext {
    glm::vec2 screenSize;
    glm::vec2 mousePos;
    bool mouseDown[3] = {false};
    bool mousePressed[3] = {false};
    bool mouseReleased[3] = {false};
    float mouseWheel = 0.0f;
    uint32_t activeId = 0;
    uint32_t hotId = 0;
    uint32_t keyboardFocusId = 0;
    bool keyboardFocusRequested = false;
    char keyChar = 0;
    int keyCode = 0;
    bool keyDown = false;
    bool keyRepeat = false;
    
    // Rendering data
    std::vector<GUIVertex> vertices;
    std::vector<uint16_t> indices;
    std::vector<GUIDrawCmd> drawCommands;
    glm::vec4 clipRect;
    std::vector<glm::vec4> clipStack;
    
    // Current state
    Font* currentFont = nullptr;
    uint32_t currentTexture = 0;
    
    // Style
    GUIStyle style;
};

// Main GUI renderer
class GUIRenderer {
public:
    GUIRenderer();
    ~GUIRenderer();
    
    // No copy
    GUIRenderer(const GUIRenderer&) = delete;
    GUIRenderer& operator=(const GUIRenderer&) = delete;
    
    void init(VulkanDevice* device);
    void cleanup();
    
    // Frame management
    void beginFrame(uint32_t width, uint32_t height);
    void endFrame();
    void render(vk::CommandBuffer cmd);
    
    // Input
    void setMousePosition(float x, float y);
    void setMouseDown(int button, bool down);
    void setMouseWheel(float delta);
    void setKeyChar(char c);
    void setKeyEvent(int keyCode, bool down, bool repeat);
    
    // Style
    void setStyle(const GUIStyle& style);
    const GUIStyle& getStyle() const { return ctx.style; }
    
    // Font management
    Font* loadFont(const std::string& name, int size, const std::string& path);
    Font* getDefaultFont() { return defaultFont; }
    void setFont(Font* font);
    
    // Text measurement
    glm::vec2 measureText(const std::string& text, Font* font = nullptr);
    float getTextWidth(const std::string& text, Font* font = nullptr);
    float getTextHeight(const std::string& text, Font* font = nullptr);
    
    // Immediate mode widgets
    bool button(const std::string& label, float x, float y, float w, float h);
    bool button(const std::string& label, const glm::vec4& rect);
    
    void label(const std::string& text, float x, float y);
    void label(const std::string& text, const glm::vec4& rect, int align = 0);
    
    bool textInput(std::string& text, float x, float y, float w, float h, 
                   uint32_t maxLength = 256, uint32_t id = 0);
    bool textInput(std::string& text, const glm::vec4& rect, 
                   uint32_t maxLength = 256, uint32_t id = 0);
    
    bool checkbox(bool& checked, const std::string& label, float x, float y);
    bool checkbox(bool& checked, const std::string& label, const glm::vec4& rect);
    
    bool slider(float& value, float min, float max, float x, float y, float w, float h);
    bool slider(float& value, float min, float max, const glm::vec4& rect);
    
    bool sliderInt(int& value, int min, int max, float x, float y, float w, float h);
    bool sliderInt(int& value, int min, int max, const glm::vec4& rect);
    
    int comboBox(int current, const std::vector<std::string>& items, 
                 float x, float y, float w, float h);
    int comboBox(int current, const std::vector<std::string>& items,
                 const glm::vec4& rect);
    
    void progressBar(float progress, float x, float y, float w, float h);
    void progressBar(float progress, const glm::vec4& rect);
    
    void image(uint32_t textureId, float x, float y, float w, float h,
               const glm::vec4& tint = glm::vec4(1.0f));
    void image(uint32_t textureId, const glm::vec4& rect,
               const glm::vec4& tint = glm::vec4(1.0f));
    
    bool imageButton(uint32_t textureId, float x, float y, float w, float h,
                     const glm::vec4& tint = glm::vec4(1.0f));
    bool imageButton(uint32_t textureId, const glm::vec4& rect,
                     const glm::vec4& tint = glm::vec4(1.0f));
    
    // Layout helpers
    void beginGroup(float x, float y, float w);
    void endGroup();
    
    void beginScrollArea(float x, float y, float w, float h, float* scrollY);
    void endScrollArea();
    
    int beginMenu(const std::string& label, float x, float y);
    void endMenu();
    
    bool menuItem(const std::string& label);
    bool menuItem(const std::string& label, const std::string& shortcut);
    
    // Panels
    void beginPanel(const std::string& title, float x, float y, float w, float h);
    void endPanel();
    
    bool beginWindow(const std::string& title, float& x, float& y, float w, float h, 
                     uint32_t id, bool* open = nullptr);
    void endWindow();
    
    // Drawing primitives
    void drawRect(float x, float y, float w, float h, const glm::vec4& color);
    void drawRect(const glm::vec4& rect, const glm::vec4& color);
    void drawRectBordered(float x, float y, float w, float h, 
                          const glm::vec4& fillColor, const glm::vec4& borderColor);
    void drawText(const std::string& text, float x, float y, const glm::vec4& color);
    void drawText(const std::string& text, float x, float y, float w, float h, 
                  const glm::vec4& color, int align = 0);
    void drawLine(float x1, float y1, float x2, float y2, const glm::vec4& color, float thickness = 1.0f);
    void drawCircle(float cx, float cy, float radius, const glm::vec4& color, int segments = 32);
    
    // Scissor/clip
    void pushClipRect(float x, float y, float w, float h);
    void popClipRect();
    
    // Getters
    GUIContext& getContext() { return ctx; }
    const GUIContext& getContext() const { return ctx; }
    
private:
    void createPipeline();
    void createBuffers();
    void updateBuffers();
    bool isMouseOver(float x, float y, float w, float h) const;
    uint32_t generateId();
    void addVertex(const GUIVertex& v);
    void addRect(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color,
                 const glm::vec2& uvMin = glm::vec2(0.0f), 
                 const glm::vec2& uvMax = glm::vec2(1.0f));
    
    VulkanDevice* device = nullptr;
    GUIContext ctx;
    
    // Vulkan resources
    vk::Pipeline pipeline;
    vk::PipelineLayout pipelineLayout;
    vk::DescriptorSetLayout descriptorLayout;
    std::vector<vk::DescriptorSet> descriptorSets;
    
    // Buffers
    Buffer vertexBuffer;
    Buffer indexBuffer;
    Buffer uniformBuffer;
    
    // Font texture
    vk::Image fontTexture;
    vk::DeviceMemory fontMemory;
    vk::ImageView fontView;
    vk::Sampler sampler;
    
    // Default font
    Font* defaultFont = nullptr;
    std::vector<std::unique_ptr<Font>> fonts;
    
    // ID generation
    uint32_t nextId = 1;
    
    // Layout state
    float groupX = 0.0f;
    float groupY = 0.0f;
    float groupW = 0.0f;
    float groupCursorY = 0.0f;
    bool inGroup = false;
    
    // Scroll state
    float scrollAreaHeight = 0.0f;
    float scrollContentHeight = 0.0f;
    bool inScrollArea = false;
};

// Minecraft-specific UI screens
namespace MinecraftUI {

// Inventory screen
void drawInventory(GUIRenderer& gui, int screenWidth, int screenHeight);
void drawInventorySlots(GUIRenderer& gui, int startX, int startY, int cols, int rows);

// Crafting screen
void drawCraftingTable(GUIRenderer& gui, int screenWidth, int screenHeight);
void drawFurnace(GUIRenderer& gui, int screenWidth, int screenHeight);

// Chat screen
void drawChat(GUIRenderer& gui, int screenWidth, int screenHeight, 
              std::string& inputText, const std::vector<std::string>& messages);

// Hotbar
void drawHotbar(GUIRenderer& gui, int screenWidth, int screenHeight, 
                 int selectedSlot, const std::vector<std::string>& items);

// Health/Hunger bars
void drawHealthBar(GUIRenderer& gui, int screenWidth, int screenHeight, 
                   float health, float maxHealth);
void drawHungerBar(GUIRenderer& gui, int screenWidth, int screenHeight,
                   float hunger, float maxHunger);
void drawExperienceBar(GUIRenderer& gui, int screenWidth, int screenHeight,
                       float progress, int level);

// Crosshair
void drawCrosshair(GUIRenderer& gui, int screenWidth, int screenHeight);

// Main menu
void drawMainMenu(GUIRenderer& gui, int screenWidth, int screenHeight);
void drawPauseMenu(GUIRenderer& gui, int screenWidth, int screenHeight);

// Debug overlay
void drawDebugOverlay(GUIRenderer& gui, int screenWidth, int screenHeight,
                      const std::vector<std::string>& lines);

} // namespace MinecraftUI

} // namespace VoxelForge
