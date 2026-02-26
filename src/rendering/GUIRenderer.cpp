/**
 * @file GUIRenderer.cpp
 * @brief Immediate mode GUI rendering implementation
 */

#include "GUIRenderer.hpp"
#include <VoxelForge/rendering/VulkanDevice.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>
#include <cmath>

// STB for font loading
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace VoxelForge {

// GUI style presets
GUIStyle GUIStyle::Minecraft() {
    GUIStyle style;
    style.backgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.85f);
    style.borderColor = glm::vec4(0.25f, 0.25f, 0.25f, 1.0f);
    style.accentColor = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
    style.hoverColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    style.activeColor = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
    style.borderWidth = 2.0f;
    style.borderRadius = 0.0f;  // Minecraft has no rounded corners
    style.padding = 4.0f;
    style.itemHeight = 20.0f;
    return style;
}

GUIStyle GUIStyle::Dark() {
    GUIStyle style;
    style.backgroundColor = glm::vec4(0.12f, 0.12f, 0.14f, 0.95f);
    style.borderColor = glm::vec4(0.2f, 0.2f, 0.24f, 1.0f);
    style.accentColor = glm::vec4(0.26f, 0.59f, 0.98f, 1.0f);
    return style;
}

GUIStyle GUIStyle::Light() {
    GUIStyle style;
    style.backgroundColor = glm::vec4(0.95f, 0.95f, 0.95f, 0.95f);
    style.borderColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
    style.textColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    style.accentColor = glm::vec4(0.0f, 0.48f, 1.0f, 1.0f);
    return style;
}

// ============== GUIRenderer ==============

GUIRenderer::GUIRenderer() = default;

GUIRenderer::~GUIRenderer() {
    cleanup();
}

void GUIRenderer::init(VulkanDevice* device) {
    this->device = device;
    
    ctx.style = GUIStyle::Minecraft();
    
    createPipeline();
    createBuffers();
    
    // Create default font (built-in)
    defaultFont = loadFont("default", 16, "");
    
    Logger::info("GUIRenderer initialized");
}

void GUIRenderer::cleanup() {
    if (!device) return;
    
    auto vkDevice = device->getDevice();
    vkDevice.waitIdle();
    
    // Clean up fonts
    fonts.clear();
    defaultFont = nullptr;
    
    // Clean up font texture
    if (fontView) { vkDevice.destroyImageView(fontView); fontView = nullptr; }
    if (fontTexture) { vkDevice.destroyImage(fontTexture); fontTexture = nullptr; }
    if (fontMemory) { vkDevice.freeMemory(fontMemory); fontMemory = nullptr; }
    if (sampler) { vkDevice.destroySampler(sampler); sampler = nullptr; }
    
    // Clean up buffers
    if (vertexBuffer.buffer) { VulkanBuffer::destroyBuffer(vkDevice, vertexBuffer); }
    if (indexBuffer.buffer) { VulkanBuffer::destroyBuffer(vkDevice, indexBuffer); }
    if (uniformBuffer.buffer) { VulkanBuffer::destroyBuffer(vkDevice, uniformBuffer); }
    
    // Clean up pipeline
    if (pipeline) { vkDevice.destroyPipeline(pipeline); pipeline = nullptr; }
    if (pipelineLayout) { vkDevice.destroyPipelineLayout(pipelineLayout); pipelineLayout = nullptr; }
    if (descriptorLayout) { vkDevice.destroyDescriptorSetLayout(descriptorLayout); descriptorLayout = nullptr; }
    
    device = nullptr;
}

void GUIRenderer::createPipeline() {
    // Create descriptor layout
    vk::DescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
    
    vk::DescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 1;
    samplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    
    std::array bindings = {uboBinding, samplerBinding};
    
    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    descriptorLayout = device->getDevice().createDescriptorSetLayout(layoutInfo);
    
    // Create pipeline layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorLayout;
    
    // Push constants for transforms
    vk::PushConstantRange pushRange{};
    pushRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
    pushRange.offset = 0;
    pushRange.size = sizeof(glm::mat4);
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
    
    pipelineLayout = device->getDevice().createPipelineLayout(pipelineLayoutInfo);
    
    // Create pipeline using builder
    VulkanPipelineBuilder builder(device);
    builder.setVertexInput(
        {GUIVertex::getBindingDescription()},
        GUIVertex::getAttributeDescriptions()
    );
    
    builder.addShaderStage(vk::ShaderStageFlagBits::eVertex, "gui.vert");
    builder.addShaderStage(vk::ShaderStageFlagBits::eFragment, "gui.frag");
    
    builder.setInputAssembly(vk::PrimitiveTopology::eTriangleList);
    builder.setViewport(ctx.screenSize.x, ctx.screenSize.y);
    builder.setRasterizer(vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone);
    builder.setMultisampling(vk::SampleCountFlagBits::e1);
    builder.setDepthStencil(false, false, vk::CompareOp::eAlways);
    builder.addColorBlendAttachment(vk::BlendOp::eAdd, 
                                    vk::BlendFactor::eSrcAlpha,
                                    vk::BlendFactor::eOneMinusSrcAlpha);
    
    builder.setLayout(pipelineLayout);
    
    // TODO: Build pipeline with render pass
}

void GUIRenderer::createBuffers() {
    // Create vertex buffer (dynamic)
    vk::DeviceSize vertexBufferSize = 65536 * sizeof(GUIVertex);
    vertexBuffer = VulkanBuffer::createBuffer(
        device->getDevice(),
        device->getPhysicalDevice(),
        vertexBufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | 
        vk::MemoryPropertyFlagBits::eHostCoherent
    );
    vertexBuffer.map(device->getDevice());
    
    // Create index buffer (dynamic)
    vk::DeviceSize indexBufferSize = 131072 * sizeof(uint16_t);
    indexBuffer = VulkanBuffer::createBuffer(
        device->getDevice(),
        device->getPhysicalDevice(),
        indexBufferSize,
        vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent
    );
    indexBuffer.map(device->getDevice());
    
    // Create uniform buffer
    uniformBuffer = VulkanBuffer::createUniformBuffer(
        device->getDevice(),
        device->getPhysicalDevice(),
        sizeof(glm::mat4)
    );
    uniformBuffer.map(device->getDevice());
}

void GUIRenderer::beginFrame(uint32_t width, uint32_t height) {
    ctx.screenSize = glm::vec2(width, height);
    ctx.vertices.clear();
    ctx.indices.clear();
    ctx.drawCommands.clear();
    ctx.clipRect = glm::vec4(0.0f, 0.0f, width, height);
    ctx.clipStack.clear();
    
    // Reset input state
    for (int i = 0; i < 3; i++) {
        ctx.mousePressed[i] = false;
        ctx.mouseReleased[i] = false;
    }
    ctx.keyChar = 0;
    ctx.keyCode = 0;
    ctx.keyDown = false;
    ctx.keyRepeat = false;
    ctx.mouseWheel = 0.0f;
}

void GUIRenderer::endFrame() {
    // Update hot item
    if (!ctx.mouseDown[0]) {
        ctx.hotId = 0;
    }
}

void GUIRenderer::render(vk::CommandBuffer cmd) {
    if (ctx.vertices.empty() || ctx.indices.empty()) return;
    
    // Update vertex buffer
    size_t vertexSize = ctx.vertices.size() * sizeof(GUIVertex);
    if (vertexSize > vertexBuffer.size) {
        Logger::warn("GUI vertex buffer overflow, truncating");
        vertexSize = vertexBuffer.size;
    }
    memcpy(vertexBuffer.mapped, ctx.vertices.data(), vertexSize);
    
    // Update index buffer
    size_t indexSize = ctx.indices.size() * sizeof(uint16_t);
    if (indexSize > indexBuffer.size) {
        Logger::warn("GUI index buffer overflow, truncating");
        indexSize = indexBuffer.size;
    }
    memcpy(indexBuffer.mapped, ctx.indices.data(), indexSize);
    
    // Update uniform buffer (ortho projection)
    float left = 0.0f;
    float right = ctx.screenSize.x;
    float top = 0.0f;
    float bottom = ctx.screenSize.y;
    
    glm::mat4 projection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
    memcpy(uniformBuffer.mapped, &projection, sizeof(projection));
    
    // Bind pipeline
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    
    // Bind vertex buffer
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, &vertexBuffer.buffer, offsets);
    
    // Bind index buffer
    cmd.bindIndexBuffer(indexBuffer.buffer, 0, vk::IndexType::eUint16);
    
    // Bind descriptor sets
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout,
                          0, 1, &descriptorSets[0], 0, nullptr);
    
    // Draw all commands
    uint32_t currentTexture = 0;
    for (const auto& cmd : ctx.drawCommands) {
        // Bind texture if changed
        if (cmd.textureId != currentTexture) {
            // Bind new texture
            currentTexture = cmd.textureId;
        }
        
        // Set scissor
        vk::Rect2D scissor;
        scissor.offset.x = static_cast<int32_t>(cmd.clipRect.x);
        scissor.offset.y = static_cast<int32_t>(cmd.clipRect.y);
        scissor.extent.width = static_cast<uint32_t>(cmd.clipRect.z);
        scissor.extent.height = static_cast<uint32_t>(cmd.clipRect.w);
        cmd.setScissor(0, scissor);
        
        // Draw indexed
        cmd.drawIndexed(cmd.indexCount, 1, cmd.indexOffset, cmd.vertexOffset, 0);
    }
}

void GUIRenderer::setMousePosition(float x, float y) {
    ctx.mousePos = glm::vec2(x, y);
}

void GUIRenderer::setMouseDown(int button, bool down) {
    if (button >= 0 && button < 3) {
        if (down && !ctx.mouseDown[button]) {
            ctx.mousePressed[button] = true;
        } else if (!down && ctx.mouseDown[button]) {
            ctx.mouseReleased[button] = true;
        }
        ctx.mouseDown[button] = down;
    }
}

void GUIRenderer::setMouseWheel(float delta) {
    ctx.mouseWheel = delta;
}

void GUIRenderer::setKeyChar(char c) {
    ctx.keyChar = c;
}

void GUIRenderer::setKeyEvent(int keyCode, bool down, bool repeat) {
    ctx.keyCode = keyCode;
    ctx.keyDown = down;
    ctx.keyRepeat = repeat;
}

void GUIRenderer::setStyle(const GUIStyle& style) {
    ctx.style = style;
}

Font* GUIRenderer::loadFont(const std::string& name, int size, const std::string& path) {
    auto font = std::make_unique<Font>();
    font->name = name;
    font->size = size;
    
    // Load font data
    std::vector<uint8_t> fontData;
    
    if (!path.empty()) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            fontData = std::vector<uint8_t>(
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            );
        }
    }
    
    // If no font file, generate a simple bitmap font
    if (fontData.empty()) {
        // Create simple 8x8 bitmap font
        font->textureWidth = 256;
        font->textureHeight = 256;
        font->textureData.resize(font->textureWidth * font->textureHeight, 0);
        
        // Generate basic ASCII characters
        for (int c = 32; c < 127; c++) {
            Glyph& glyph = font->glyphs[c];
            int col = (c - 32) % 16;
            int row = (c - 32) / 16;
            
            glyph.uvMin = glm::vec2(col / 16.0f, row / 8.0f);
            glyph.uvMax = glm::vec2((col + 1) / 16.0f, (row + 1) / 8.0f);
            glyph.size = glm::ivec2(8, 16);
            glyph.bearing = glm::ivec2(0, 0);
            glyph.advance = 8;
            
            // Simple character rendering
            // (Would be replaced with actual font rasterization)
        }
        
        font->lineHeight = 16.0f;
    } else {
        // Use stb_truetype to rasterize font
        stbtt_fontinfo fontInfo;
        if (!stbtt_InitFont(&fontInfo, fontData.data(), 0)) {
            Logger::error("Failed to initialize font: {}", path);
            return nullptr;
        }
        
        // Calculate font metrics
        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
        
        float scale = stbtt_ScaleForPixelHeight(&fontInfo, size);
        font->lineHeight = (ascent - descent) * scale;
        
        // Rasterize characters to texture atlas
        int textureSize = 512;
        font->textureWidth = textureSize;
        font->textureHeight = textureSize;
        font->textureData.resize(textureSize * textureSize, 0);
        
        int x = 0;
        int y = 0;
        int rowHeight = 0;
        
        font->glyphs.resize(256);
        
        for (int c = 32; c < 127; c++) {
            int width, height, xoff, yoff;
            uint8_t* bitmap = stbtt_GetCodepointBitmap(
                &fontInfo, 0, scale, c, &width, &height, &xoff, &yoff);
            
            if (x + width >= textureSize) {
                x = 0;
                y += rowHeight;
                rowHeight = 0;
            }
            
            if (height > rowHeight) {
                rowHeight = height;
            }
            
            // Copy bitmap to texture
            for (int py = 0; py < height; py++) {
                for (int px = 0; px < width; px++) {
                    int dstIdx = (y + py) * textureSize + (x + px);
                    font->textureData[dstIdx] = bitmap[py * width + px];
                }
            }
            
            Glyph& glyph = font->glyphs[c];
            glyph.uvMin = glm::vec2(x / (float)textureSize, y / (float)textureSize);
            glyph.uvMax = glm::vec2((x + width) / (float)textureSize, 
                                    (y + height) / (float)textureSize);
            glyph.size = glm::ivec2(width, height);
            glyph.bearing = glm::ivec2(xoff, yoff);
            
            int advance;
            stbtt_GetCodepointHMetrics(&fontInfo, c, &advance, nullptr);
            glyph.advance = advance * scale;
            
            x += width + 1;
            
            stbtt_FreeBitmap(bitmap);
        }
    }
    
    // Create Vulkan texture
    // (Would create actual Vulkan image here)
    
    Font* result = font.get();
    fonts.push_back(std::move(font));
    return result;
}

void GUIRenderer::setFont(Font* font) {
    ctx.currentFont = font ? font : defaultFont;
}

glm::vec2 GUIRenderer::measureText(const std::string& text, Font* font) {
    if (!font) font = ctx.currentFont;
    if (!font) return glm::vec2(0.0f);
    
    float width = 0.0f;
    float height = font->lineHeight;
    
    for (char c : text) {
        if (c >= 0 && c < 256) {
            width += font->glyphs[c].advance;
        }
    }
    
    return glm::vec2(width, height);
}

float GUIRenderer::getTextWidth(const std::string& text, Font* font) {
    return measureText(text, font).x;
}

float GUIRenderer::getTextHeight(const std::string& text, Font* font) {
    return measureText(text, font).y;
}

bool GUIRenderer::isMouseOver(float x, float y, float w, float h) const {
    return ctx.mousePos.x >= x && ctx.mousePos.x < x + w &&
           ctx.mousePos.y >= y && ctx.mousePos.y < y + h;
}

uint32_t GUIRenderer::generateId() {
    return nextId++;
}

void GUIRenderer::addVertex(const GUIVertex& v) {
    ctx.vertices.push_back(v);
}

void GUIRenderer::addRect(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color,
                           const glm::vec2& uvMin, const glm::vec2& uvMax) {
    uint32_t idx = static_cast<uint32_t>(ctx.vertices.size());
    
    // Add vertices
    addVertex({{pos.x, pos.y}, {uvMin.x, uvMin.y}, color});
    addVertex({{pos.x + size.x, pos.y}, {uvMax.x, uvMin.y}, color});
    addVertex({{pos.x + size.x, pos.y + size.y}, {uvMax.x, uvMax.y}, color});
    addVertex({{pos.x, pos.y + size.y}, {uvMin.x, uvMax.y}, color});
    
    // Add indices
    ctx.indices.push_back(idx);
    ctx.indices.push_back(idx + 1);
    ctx.indices.push_back(idx + 2);
    ctx.indices.push_back(idx);
    ctx.indices.push_back(idx + 2);
    ctx.indices.push_back(idx + 3);
}

// Widget implementations

bool GUIRenderer::button(const std::string& label, float x, float y, float w, float h) {
    return button(label, glm::vec4(x, y, w, h));
}

bool GUIRenderer::button(const std::string& label, const glm::vec4& rect) {
    uint32_t id = generateId();
    bool clicked = false;
    bool hover = isMouseOver(rect.x, rect.y, rect.z, rect.w);
    
    // Update hot/active state
    if (hover) {
        ctx.hotId = id;
        if (ctx.activeId == 0 && ctx.mouseDown[0]) {
            ctx.activeId = id;
        }
    }
    
    // Check for click
    if (ctx.activeId == id && hover && !ctx.mouseDown[0]) {
        clicked = true;
        ctx.activeId = 0;
    }
    
    // Draw button
    glm::vec4 color = ctx.style.backgroundColor;
    if (ctx.activeId == id) {
        color = ctx.style.activeColor;
    } else if (hover) {
        color = ctx.style.hoverColor;
    }
    
    drawRectBordered(rect.x, rect.y, rect.z, rect.w, color, ctx.style.borderColor);
    
    // Draw label
    glm::vec2 textSize = measureText(label);
    float textX = rect.x + (rect.z - textSize.x) * 0.5f;
    float textY = rect.y + (rect.w - textSize.y) * 0.5f;
    drawText(label, textX, textY, ctx.style.textColor);
    
    return clicked;
}

void GUIRenderer::label(const std::string& text, float x, float y) {
    drawText(text, x, y, ctx.style.textColor);
}

void GUIRenderer::label(const std::string& text, const glm::vec4& rect, int align) {
    drawText(text, rect.x, rect.y, rect.z, rect.w, ctx.style.textColor, align);
}

bool GUIRenderer::textInput(std::string& text, float x, float y, float w, float h,
                             uint32_t maxLength, uint32_t id) {
    return textInput(text, glm::vec4(x, y, w, h), maxLength, id);
}

bool GUIRenderer::textInput(std::string& text, const glm::vec4& rect,
                             uint32_t maxLength, uint32_t id) {
    if (id == 0) id = generateId();
    bool changed = false;
    bool hover = isMouseOver(rect.x, rect.y, rect.z, rect.w);
    
    // Focus on click
    if (hover && ctx.mousePressed[0]) {
        ctx.keyboardFocusId = id;
    }
    
    bool focused = (ctx.keyboardFocusId == id);
    
    // Handle keyboard input
    if (focused) {
        if (ctx.keyChar != 0 && text.length() < maxLength) {
            text += ctx.keyChar;
            changed = true;
        }
        
        if (ctx.keyCode == 8 && !text.empty() && ctx.keyDown) {  // Backspace
            text.pop_back();
            changed = true;
        }
    }
    
    // Draw text field
    glm::vec4 bgColor = focused ? ctx.style.backgroundColor : ctx.style.disabledColor;
    if (hover) bgColor = ctx.style.hoverColor;
    
    drawRectBordered(rect.x, rect.y, rect.z, rect.w, bgColor, ctx.style.borderColor);
    
    // Draw text
    drawText(text.empty() ? "" : text, rect.x + 4, rect.y + 4, 
             rect.z - 8, rect.w - 8, ctx.style.textColor);
    
    // Draw cursor if focused
    if (focused) {
        float cursorX = rect.x + 4 + getTextWidth(text);
        drawLine(cursorX, rect.y + 4, cursorX, rect.y + rect.w - 4, 
                ctx.style.textColor, 2.0f);
    }
    
    return changed;
}

bool GUIRenderer::checkbox(bool& checked, const std::string& label, float x, float y) {
    return checkbox(checked, label, glm::vec4(x, y, 20.0f, 20.0f));
}

bool GUIRenderer::checkbox(bool& checked, const std::string& label, const glm::vec4& rect) {
    uint32_t id = generateId();
    bool changed = false;
    bool hover = isMouseOver(rect.x, rect.y, rect.z, rect.w);
    
    if (hover) {
        ctx.hotId = id;
        if (ctx.activeId == 0 && ctx.mouseDown[0]) {
            ctx.activeId = id;
        }
    }
    
    if (ctx.activeId == id && hover && !ctx.mouseDown[0]) {
        checked = !checked;
        changed = true;
        ctx.activeId = 0;
    }
    
    // Draw checkbox
    drawRectBordered(rect.x, rect.y, rect.z, rect.w, 
                    checked ? ctx.style.accentColor : ctx.style.backgroundColor,
                    ctx.style.borderColor);
    
    // Draw check mark
    if (checked) {
        drawRect(rect.x + 4, rect.y + 4, rect.z - 8, rect.w - 8, ctx.style.textColor);
    }
    
    // Draw label
    drawText(label, rect.x + rect.z + 4, rect.y, ctx.style.textColor);
    
    return changed;
}

bool GUIRenderer::slider(float& value, float min, float max, float x, float y, float w, float h) {
    return slider(value, min, max, glm::vec4(x, y, w, h));
}

bool GUIRenderer::slider(float& value, float min, float max, const glm::vec4& rect) {
    uint32_t id = generateId();
    bool changed = false;
    bool hover = isMouseOver(rect.x, rect.y, rect.z, rect.w);
    
    if (hover) {
        ctx.hotId = id;
    }
    
    if (ctx.activeId == id || (ctx.activeId == 0 && hover && ctx.mouseDown[0])) {
        if (ctx.mouseDown[0]) {
            ctx.activeId = id;
            
            float relX = ctx.mousePos.x - rect.x;
            float t = std::clamp(relX / rect.z, 0.0f, 1.0f);
            float newValue = min + t * (max - min);
            
            if (newValue != value) {
                value = newValue;
                changed = true;
            }
        } else {
            ctx.activeId = 0;
        }
    }
    
    // Draw track
    float trackHeight = 4.0f;
    float trackY = rect.y + (rect.w - trackHeight) * 0.5f;
    drawRect(rect.x, trackY, rect.z, trackHeight, ctx.style.disabledColor);
    
    // Draw filled portion
    float t = (value - min) / (max - min);
    drawRect(rect.x, trackY, rect.z * t, trackHeight, ctx.style.accentColor);
    
    // Draw handle
    float handleWidth = 8.0f;
    float handleX = rect.x + t * rect.z - handleWidth * 0.5f;
    drawRect(handleX, rect.y, handleWidth, rect.w, ctx.style.borderColor);
    
    return changed;
}

bool GUIRenderer::sliderInt(int& value, int min, int max, float x, float y, float w, float h) {
    float fValue = static_cast<float>(value);
    bool changed = slider(fValue, static_cast<float>(min), static_cast<float>(max), x, y, w, h);
    if (changed) {
        value = static_cast<int>(std::round(fValue));
    }
    return changed;
}

bool GUIRenderer::sliderInt(int& value, int min, int max, const glm::vec4& rect) {
    return sliderInt(value, min, max, rect.x, rect.y, rect.z, rect.w);
}

int GUIRenderer::comboBox(int current, const std::vector<std::string>& items,
                          float x, float y, float w, float h) {
    return comboBox(current, items, glm::vec4(x, y, w, h));
}

int GUIRenderer::comboBox(int current, const std::vector<std::string>& items,
                          const glm::vec4& rect) {
    // Simplified combo box - would need more complex implementation
    static bool open = false;
    uint32_t id = generateId();
    
    if (button(current >= 0 && current < items.size() ? items[current] : "Select",
               rect)) {
        open = !open;
    }
    
    if (open) {
        float itemY = rect.y + rect.w;
        for (int i = 0; i < items.size(); i++) {
            if (button(items[i], rect.x, itemY, rect.z, 20.0f)) {
                current = i;
                open = false;
            }
            itemY += 20.0f;
        }
    }
    
    return current;
}

void GUIRenderer::progressBar(float progress, float x, float y, float w, float h) {
    progressBar(progress, glm::vec4(x, y, w, h));
}

void GUIRenderer::progressBar(float progress, const glm::vec4& rect) {
    drawRect(rect.x, rect.y, rect.z, rect.w, ctx.style.disabledColor);
    drawRect(rect.x, rect.y, rect.z * std::clamp(progress, 0.0f, 1.0f), rect.w, 
            ctx.style.accentColor);
}

void GUIRenderer::image(uint32_t textureId, float x, float y, float w, float h,
                         const glm::vec4& tint) {
    image(textureId, glm::vec4(x, y, w, h), tint);
}

void GUIRenderer::image(uint32_t textureId, const glm::vec4& rect, const glm::vec4& tint) {
    addRect(glm::vec2(rect.x, rect.y), glm::vec2(rect.z, rect.w), tint);
}

bool GUIRenderer::imageButton(uint32_t textureId, float x, float y, float w, float h,
                               const glm::vec4& tint) {
    return imageButton(textureId, glm::vec4(x, y, w, h), tint);
}

bool GUIRenderer::imageButton(uint32_t textureId, const glm::vec4& rect, const glm::vec4& tint) {
    // Simplified - would draw texture and check for click
    return button("", rect);
}

void GUIRenderer::beginGroup(float x, float y, float w) {
    groupX = x;
    groupY = y;
    groupW = w;
    groupCursorY = 0.0f;
    inGroup = true;
}

void GUIRenderer::endGroup() {
    inGroup = false;
}

void GUIRenderer::beginScrollArea(float x, float y, float w, float h, float* scrollY) {
    scrollAreaHeight = h;
    inScrollArea = true;
    pushClipRect(x, y, w, h);
}

void GUIRenderer::endScrollArea() {
    popClipRect();
    inScrollArea = false;
}

int GUIRenderer::beginMenu(const std::string& label, float x, float y) {
    return 0;  // Placeholder
}

void GUIRenderer::endMenu() {
}

bool GUIRenderer::menuItem(const std::string& label) {
    return button(label, 0.0f, 0.0f, 100.0f, 20.0f);
}

bool GUIRenderer::menuItem(const std::string& label, const std::string& shortcut) {
    return button(label + " " + shortcut, 0.0f, 0.0f, 150.0f, 20.0f);
}

void GUIRenderer::beginPanel(const std::string& title, float x, float y, float w, float h) {
    drawRectBordered(x, y, w, h, ctx.style.backgroundColor, ctx.style.borderColor);
    drawRect(x, y, w, 24.0f, ctx.style.borderColor);
    drawText(title, x + 4, y + 4, ctx.style.textColor);
}

void GUIRenderer::endPanel() {
}

bool GUIRenderer::beginWindow(const std::string& title, float& x, float& y, float w, float h,
                               uint32_t id, bool* open) {
    // Draw window background
    drawRectBordered(x, y, w, h, ctx.style.backgroundColor, ctx.style.borderColor);
    
    // Draw title bar
    drawRect(x, y, w, 24.0f, ctx.style.borderColor);
    drawText(title, x + 4, y + 4, ctx.style.textColor);
    
    // Check for drag
    bool dragHover = isMouseOver(x, y, w, 24.0f);
    if (dragHover && ctx.mouseDown[0]) {
        // Would handle dragging
    }
    
    // Check for close button
    if (open && button("X", x + w - 24.0f, y, 24.0f, 24.0f)) {
        *open = false;
    }
    
    pushClipRect(x, y + 24.0f, w, h - 24.0f);
    return true;
}

void GUIRenderer::endWindow() {
    popClipRect();
}

// Drawing primitives

void GUIRenderer::drawRect(float x, float y, float w, float h, const glm::vec4& color) {
    addRect(glm::vec2(x, y), glm::vec2(w, h), color);
}

void GUIRenderer::drawRect(const glm::vec4& rect, const glm::vec4& color) {
    drawRect(rect.x, rect.y, rect.z, rect.w, color);
}

void GUIRenderer::drawRectBordered(float x, float y, float w, float h,
                                    const glm::vec4& fillColor, const glm::vec4& borderColor) {
    drawRect(x, y, w, h, fillColor);
    
    // Draw border (simplified - would use proper border drawing)
    drawRect(x, y, w, ctx.style.borderWidth, borderColor);  // Top
    drawRect(x, y + h - ctx.style.borderWidth, w, ctx.style.borderWidth, borderColor);  // Bottom
    drawRect(x, y, ctx.style.borderWidth, h, borderColor);  // Left
    drawRect(x + w - ctx.style.borderWidth, y, ctx.style.borderWidth, h, borderColor);  // Right
}

void GUIRenderer::drawText(const std::string& text, float x, float y, const glm::vec4& color) {
    if (!ctx.currentFont) return;
    
    float cursorX = x;
    for (char c : text) {
        if (c >= 0 && c < 256) {
            const Glyph& glyph = ctx.currentFont->glyphs[c];
            
            addRect(
                glm::vec2(cursorX + glyph.bearing.x, y + glyph.bearing.y),
                glm::vec2(glyph.size.x, glyph.size.y),
                color,
                glyph.uvMin,
                glyph.uvMax
            );
            
            cursorX += glyph.advance;
        }
    }
}

void GUIRenderer::drawText(const std::string& text, float x, float y, float w, float h,
                           const glm::vec4& color, int align) {
    glm::vec2 size = measureText(text);
    
    float textX = x;
    float textY = y;
    
    if (align == 0) {  // Center
        textX = x + (w - size.x) * 0.5f;
        textY = y + (h - size.y) * 0.5f;
    } else if (align == 1) {  // Left
        textY = y + (h - size.y) * 0.5f;
    } else if (align == 2) {  // Right
        textX = x + w - size.x;
        textY = y + (h - size.y) * 0.5f;
    }
    
    drawText(text, textX, textY, color);
}

void GUIRenderer::drawLine(float x1, float y1, float x2, float y2, 
                           const glm::vec4& color, float thickness) {
    // Simplified line drawing using rotated rectangles
    addRect(glm::vec2(x1, y1), glm::vec2(x2 - x1, thickness), color);
}

void GUIRenderer::drawCircle(float cx, float cy, float radius, const glm::vec4& color, int segments) {
    // Would need proper circle drawing
    drawRect(cx - radius, cy - radius, radius * 2, radius * 2, color);
}

void GUIRenderer::pushClipRect(float x, float y, float w, float h) {
    ctx.clipStack.push_back(ctx.clipRect);
    ctx.clipRect = glm::vec4(x, y, w, h);
}

void GUIRenderer::popClipRect() {
    if (!ctx.clipStack.empty()) {
        ctx.clipRect = ctx.clipStack.back();
        ctx.clipStack.pop_back();
    }
}

// Minecraft UI implementations

namespace MinecraftUI {

void drawInventory(GUIRenderer& gui, int screenWidth, int screenHeight) {
    // Draw inventory background (Minecraft-style)
    int invWidth = 176;
    int invHeight = 166;
    int invX = (screenWidth - invWidth) / 2;
    int invY = (screenHeight - invHeight) / 2;
    
    gui.beginPanel("Inventory", invX, invY, invWidth, invHeight);
    
    // Draw inventory slots
    drawInventorySlots(gui, invX + 8, invY + 84, 9, 3);  // Main inventory
    drawInventorySlots(gui, invX + 8, invY + 142, 9, 1);  // Hotbar
    
    gui.endPanel();
}

void drawInventorySlots(GUIRenderer& gui, int startX, int startY, int cols, int rows) {
    int slotSize = 16;
    int padding = 2;
    
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int x = startX + col * (slotSize + padding);
            int y = startY + row * (slotSize + padding);
            
            // Draw slot background
            gui.drawRect(x, y, slotSize, slotSize, glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
            gui.drawRect(x + 1, y + 1, slotSize - 2, slotSize - 2, glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
        }
    }
}

void drawCraftingTable(GUIRenderer& gui, int screenWidth, int screenHeight) {
    int invWidth = 176;
    int invHeight = 184;
    int invX = (screenWidth - invWidth) / 2;
    int invY = (screenHeight - invHeight) / 2;
    
    gui.beginPanel("Crafting", invX, invY, invWidth, invHeight);
    
    // Crafting grid
    drawInventorySlots(gui, invX + 30, invY + 32, 2, 2);
    
    // Result slot
    gui.drawRect(invX + 124, invY + 44, 16, 16, glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
    
    // Main inventory
    drawInventorySlots(gui, invX + 8, invY + 102, 9, 3);
    drawInventorySlots(gui, invX + 8, invY + 160, 9, 1);
    
    gui.endPanel();
}

void drawFurnace(GUIRenderer& gui, int screenWidth, int screenHeight) {
    int invWidth = 176;
    int invHeight = 166;
    int invX = (screenWidth - invWidth) / 2;
    int invY = (screenHeight - invHeight) / 2;
    
    gui.beginPanel("Furnace", invX, invY, invWidth, invHeight);
    
    // Input slot
    gui.drawRect(invX + 56, invY + 17, 16, 16, glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
    
    // Fuel slot
    gui.drawRect(invX + 56, invY + 53, 16, 16, glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
    
    // Output slot
    gui.drawRect(invX + 116, invY + 35, 16, 16, glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
    
    // Progress arrow
    gui.drawRect(invX + 79, invY + 35, 24, 16, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
    
    // Main inventory
    drawInventorySlots(gui, invX + 8, invY + 84, 9, 3);
    drawInventorySlots(gui, invX + 8, invY + 142, 9, 1);
    
    gui.endPanel();
}

void drawChat(GUIRenderer& gui, int screenWidth, int screenHeight,
              std::string& inputText, const std::vector<std::string>& messages) {
    int chatWidth = 320;
    int chatHeight = 180;
    int chatX = 10;
    int chatY = screenHeight - chatHeight - 30;
    
    // Draw chat background
    gui.drawRect(chatX, chatY, chatWidth, chatHeight, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
    
    // Draw messages
    float messageY = chatY + chatHeight - 24;
    for (int i = messages.size() - 1; i >= 0 && messageY > chatY; i--) {
        gui.label(messages[i], chatX + 4, messageY);
        messageY -= 12;
    }
    
    // Draw input field
    gui.drawRect(chatX, chatY + chatHeight, chatWidth, 20, glm::vec4(0.0f, 0.0f, 0.0f, 0.7f));
    gui.textInput(inputText, chatX + 4, chatY + chatHeight + 2, chatWidth - 8, 16);
}

void drawHotbar(GUIRenderer& gui, int screenWidth, int screenHeight,
                int selectedSlot, const std::vector<std::string>& items) {
    int hotbarWidth = 182;
    int hotbarHeight = 22;
    int hotbarX = (screenWidth - hotbarWidth) / 2;
    int hotbarY = screenHeight - hotbarHeight - 4;
    
    // Draw hotbar background
    gui.drawRect(hotbarX, hotbarY, hotbarWidth, hotbarHeight, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
    
    // Draw slots
    int slotSize = 20;
    for (int i = 0; i < 9; i++) {
        int slotX = hotbarX + i * slotSize + 1;
        
        // Highlight selected slot
        if (i == selectedSlot) {
            gui.drawRect(slotX - 1, hotbarY - 1, slotSize + 2, slotSize + 2,
                        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        
        gui.drawRect(slotX, hotbarY + 1, slotSize - 2, slotSize - 2,
                    glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
    }
}

void drawHealthBar(GUIRenderer& gui, int screenWidth, int screenHeight,
                   float health, float maxHealth) {
    int barWidth = 200;
    int barHeight = 10;
    int barX = 10;
    int barY = screenHeight - 40;
    
    gui.drawRect(barX, barY, barWidth, barHeight, glm::vec4(0.3f, 0.0f, 0.0f, 1.0f));
    gui.drawRect(barX, barY, barWidth * (health / maxHealth), barHeight, 
                glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
}

void drawHungerBar(GUIRenderer& gui, int screenWidth, int screenHeight,
                   float hunger, float maxHunger) {
    int barWidth = 200;
    int barHeight = 10;
    int barX = screenWidth - barWidth - 10;
    int barY = screenHeight - 40;
    
    gui.drawRect(barX, barY, barWidth, barHeight, glm::vec4(0.3f, 0.3f, 0.0f, 1.0f));
    gui.drawRect(barX, barY, barWidth * (hunger / maxHunger), barHeight,
                glm::vec4(0.8f, 0.6f, 0.0f, 1.0f));
}

void drawExperienceBar(GUIRenderer& gui, int screenWidth, int screenHeight,
                       float progress, int level) {
    int barWidth = 182;
    int barHeight = 5;
    int barX = (screenWidth - barWidth) / 2;
    int barY = screenHeight - 32;
    
    gui.drawRect(barX, barY, barWidth, barHeight, glm::vec4(0.0f, 0.0f, 0.3f, 1.0f));
    gui.drawRect(barX, barY, barWidth * progress, barHeight,
                glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    
    // Draw level number
    if (level > 0) {
        gui.label(std::to_string(level), barX + barWidth / 2 - 5, barY - 12);
    }
}

void drawCrosshair(GUIRenderer& gui, int screenWidth, int screenHeight) {
    int cx = screenWidth / 2;
    int cy = screenHeight / 2;
    int size = 10;
    int thickness = 2;
    
    // Draw crosshair lines
    gui.drawRect(cx - size, cy - thickness / 2, size * 2, thickness, 
                glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    gui.drawRect(cx - thickness / 2, cy - size, thickness, size * 2,
                glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void drawMainMenu(GUIRenderer& gui, int screenWidth, int screenHeight) {
    // Draw background
    gui.drawRect(0, 0, screenWidth, screenHeight, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    
    // Draw title
    gui.label("VoxelForge", screenWidth / 2 - 50, screenHeight / 4);
    
    // Draw menu buttons
    int buttonWidth = 200;
    int buttonHeight = 20;
    int buttonX = (screenWidth - buttonWidth) / 2;
    int buttonY = screenHeight / 2 - 50;
    
    gui.button("Singleplayer", buttonX, buttonY, buttonWidth, buttonHeight);
    gui.button("Multiplayer", buttonX, buttonY + 25, buttonWidth, buttonHeight);
    gui.button("Options", buttonX, buttonY + 50, buttonWidth, buttonHeight);
    gui.button("Quit", buttonX, buttonY + 75, buttonWidth, buttonHeight);
}

void drawPauseMenu(GUIRenderer& gui, int screenWidth, int screenHeight) {
    // Draw semi-transparent overlay
    gui.drawRect(0, 0, screenWidth, screenHeight, glm::vec4(0.0f, 0.0f, 0.0f, 0.7f));
    
    // Draw pause menu
    int buttonWidth = 200;
    int buttonHeight = 20;
    int buttonX = (screenWidth - buttonWidth) / 2;
    int buttonY = screenHeight / 2 - 50;
    
    gui.button("Resume", buttonX, buttonY, buttonWidth, buttonHeight);
    gui.button("Options", buttonX, buttonY + 25, buttonWidth, buttonHeight);
    gui.button("Save and Quit", buttonX, buttonY + 50, buttonWidth, buttonHeight);
}

void drawDebugOverlay(GUIRenderer& gui, int screenWidth, int screenHeight,
                      const std::vector<std::string>& lines) {
    int y = 10;
    for (const auto& line : lines) {
        gui.label(line, 10, y);
        y += 12;
    }
}

} // namespace MinecraftUI

} // namespace VoxelForge
