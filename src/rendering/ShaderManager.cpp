/**
 * @file ShaderManager.cpp
 * @brief Shader loading and management implementation
 */

#include "ShaderManager.hpp"
#include <VoxelForge/rendering/VulkanDevice.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <fstream>
#include <sstream>
#include <chrono>

// For SPIRV compilation
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

namespace VoxelForge {

ShaderManager::ShaderManager() = default;

ShaderManager::~ShaderManager() {
    cleanup();
}

void ShaderManager::init(VulkanDevice* device, const ShaderManagerSettings& settings) {
    this->device = device;
    this->settings = settings;
    
    // Initialize glslang
    glslang::InitializeProcess();
    
    Logger::info("ShaderManager initialized");
}

void ShaderManager::cleanup() {
    if (!device) return;
    
    auto vkDevice = device->getDevice();
    
    // Destroy all shader modules
    for (auto& [name, module] : shaderModules) {
        destroyShaderModule(module);
    }
    shaderModules.clear();
    
    // Destroy all programs
    for (auto& [name, program] : programs) {
        for (auto& layout : program.descriptorLayouts) {
            if (layout) {
                vkDevice.destroyDescriptorSetLayout(layout);
            }
        }
        if (program.pipelineLayout) {
            vkDevice.destroyPipelineLayout(program.pipelineLayout);
        }
    }
    programs.clear();
    
    // Finalize glslang
    glslang::FinalizeProcess();
    
    device = nullptr;
}

ShaderModule ShaderManager::loadShader(const std::string& path, vk::ShaderStageFlagBits stage) {
    // Check cache
    auto it = shaderModules.find(path);
    if (it != shaderModules.end()) {
        return it->second;
    }
    
    // Read file
    std::string source = readFile(path);
    if (source.empty()) {
        Logger::error("Failed to read shader file: {}", path);
        return {};
    }
    
    // Preprocess
    source = preprocessShader(source, path);
    
    // Compile
    auto result = compileGLSL(source, stage, path);
    if (!result.success) {
        Logger::error("Shader compilation failed: {} - {}", path, result.errorMessage);
        return {};
    }
    
    // Create module
    ShaderModule module;
    module.filePath = path;
    module.stage = stage;
    module.sourceCode = source;
    createShaderModule(module, result.spirv);
    
    // Cache
    shaderModules[path] = module;
    
    // Watch for changes
    if (settings.enableHotReload) {
        watchFile(path);
    }
    
    Logger::debug("Loaded shader: {}", path);
    return module;
}

ShaderModule ShaderManager::loadShaderFromSource(const std::string& source,
                                                  vk::ShaderStageFlagBits stage,
                                                  const std::string& name) {
    std::string shaderName = name.empty() ? "inline_" + std::to_string(std::hash<std::string>{}(source)) : name;
    
    auto result = compileGLSL(source, stage, shaderName);
    if (!result.success) {
        Logger::error("Shader compilation failed: {} - {}", shaderName, result.errorMessage);
        return {};
    }
    
    ShaderModule module;
    module.stage = stage;
    module.sourceCode = source;
    createShaderModule(module, result.spirv);
    
    return module;
}

ShaderModule ShaderManager::loadShaderFromSPIRV(const std::vector<uint32_t>& spirv,
                                                 vk::ShaderStageFlagBits stage,
                                                 const std::string& name) {
    ShaderModule module;
    module.stage = stage;
    createShaderModule(module, spirv);
    
    return module;
}

ShaderProgram* ShaderManager::createProgram(const std::string& name) {
    programs[name] = ShaderProgram{};
    programs[name].name = name;
    return &programs[name];
}

ShaderProgram* ShaderManager::getProgram(const std::string& name) {
    auto it = programs.find(name);
    if (it != programs.end()) {
        return &it->second;
    }
    return nullptr;
}

void ShaderManager::destroyProgram(const std::string& name) {
    auto it = programs.find(name);
    if (it != programs.end()) {
        auto vkDevice = device->getDevice();
        for (auto& layout : it->second.descriptorLayouts) {
            if (layout) {
                vkDevice.destroyDescriptorSetLayout(layout);
            }
        }
        if (it->second.pipelineLayout) {
            vkDevice.destroyPipelineLayout(it->second.pipelineLayout);
        }
        programs.erase(it);
    }
}

void ShaderManager::addVertexShader(ShaderProgram& program, const std::string& path) {
    auto module = loadShader(path, vk::ShaderStageFlagBits::eVertex);
    if (module) {
        program.modules.push_back(module);
    }
}

void ShaderManager::addFragmentShader(ShaderProgram& program, const std::string& path) {
    auto module = loadShader(path, vk::ShaderStageFlagBits::eFragment);
    if (module) {
        program.modules.push_back(module);
    }
}

void ShaderManager::addGeometryShader(ShaderProgram& program, const std::string& path) {
    auto module = loadShader(path, vk::ShaderStageFlagBits::eGeometry);
    if (module) {
        program.modules.push_back(module);
    }
}

void ShaderManager::addComputeShader(ShaderProgram& program, const std::string& path) {
    auto module = loadShader(path, vk::ShaderStageFlagBits::eCompute);
    if (module) {
        program.modules.push_back(module);
    }
}

void ShaderManager::addTessControlShader(ShaderProgram& program, const std::string& path) {
    auto module = loadShader(path, vk::ShaderStageFlagBits::eTessControl);
    if (module) {
        program.modules.push_back(module);
    }
}

void ShaderManager::addTessEvalShader(ShaderProgram& program, const std::string& path) {
    auto module = loadShader(path, vk::ShaderStageFlagBits::eTessellationEvaluation);
    if (module) {
        program.modules.push_back(module);
    }
}

ShaderCompileResult ShaderManager::compileGLSL(const std::string& source,
                                                vk::ShaderStageFlagBits stage,
                                                const std::string& name) {
    ShaderCompileResult result;
    
    // Convert stage to EShLanguage
    EShLanguage language;
    switch (stage) {
        case vk::ShaderStageFlagBits::eVertex: language = EShLangVertex; break;
        case vk::ShaderStageFlagBits::eFragment: language = EShLangFragment; break;
        case vk::ShaderStageFlagBits::eGeometry: language = EShLangGeometry; break;
        case vk::ShaderStageFlagBits::eCompute: language = EShLangCompute; break;
        case vk::ShaderStageFlagBits::eTessControl: language = EShLangTessControl; break;
        case vk::ShaderStageFlagBits::eTessellationEvaluation: language = EShLangTessEvaluation; break;
        default:
            result.errorMessage = "Unknown shader stage";
            return result;
    }
    
    // Create shader
    glslang::TShader shader(language);
    const char* sourcePtr = source.c_str();
    shader.setStrings(&sourcePtr, 1);
    
    // Set environment
    shader.setEnvInput(glslang::EShSourceGlsl, language, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);
    
    // Parse
    TBuiltInResource resources = *GetDefaultResources();
    EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);
    
    if (!shader.parse(&resources, 100, false, messages)) {
        result.errorMessage = shader.getInfoLog();
        return result;
    }
    
    // Link
    glslang::TProgram program;
    program.addShader(&shader);
    
    if (!program.link(messages)) {
        result.errorMessage = program.getInfoLog();
        return result;
    }
    
    // Generate SPIR-V
    glslang::GlslangToSpv(*program.getIntermediate(language), result.spirv);
    
    result.success = true;
    return result;
}

void ShaderManager::checkForChanges() {
    if (!settings.enableHotReload) return;
    
    for (auto& [path, watch] : watchedFiles) {
        auto currentTime = std::filesystem::last_write_time(path);
        if (currentTime > watch.lastWriteTime) {
            watch.lastWriteTime = currentTime;
            reloadShader(path);
        }
    }
}

void ShaderManager::reloadShader(const std::string& path) {
    auto it = shaderModules.find(path);
    if (it == shaderModules.end()) return;
    
    // Re-read and recompile
    std::string source = readFile(path);
    source = preprocessShader(source, path);
    
    auto result = compileGLSL(source, it->second.stage, path);
    if (!result.success) {
        Logger::error("Shader reload failed: {} - {}", path, result.errorMessage);
        return;
    }
    
    // Destroy old module
    destroyShaderModule(it->second);
    
    // Create new module
    it->second.sourceCode = source;
    createShaderModule(it->second, result.spirv);
    
    Logger::info("Shader reloaded: {}", path);
}

void ShaderManager::reloadAll() {
    for (auto& [path, module] : shaderModules) {
        reloadShader(path);
    }
}

std::vector<vk::PipelineShaderStageCreateInfo> ShaderManager::getShaderStages(const ShaderProgram& program) {
    std::vector<vk::PipelineShaderStageCreateInfo> stages;
    
    for (const auto& module : program.modules) {
        vk::PipelineShaderStageCreateInfo stageInfo{};
        stageInfo.stage = module.stage;
        stageInfo.module = module.module;
        stageInfo.pName = module.entryPoint.c_str();
        
        // Add specialization constants
        if (!program.specializationConstants.empty()) {
            // TODO: Handle specialization constants
        }
        
        stages.push_back(stageInfo);
    }
    
    return stages;
}

void ShaderManager::setSpecializationConstant(ShaderProgram& program, uint32_t constantId, uint32_t value) {
    program.specializationConstants[constantId] = value;
}

void ShaderManager::addIncludeDirectory(const std::filesystem::path& path) {
    settings.includeDirectories.push_back(path);
}

std::vector<ShaderManager::DescriptorBindingInfo> ShaderManager::reflectDescriptorBindings(const ShaderProgram& program) {
    // TODO: Implement SPIR-V reflection
    return {};
}

void ShaderManager::loadBuiltinShaders() {
    // Chunk shader
    auto* chunkProgram = createProgram(CHUNK_SHADER);
    chunkProgram->modules.push_back(loadShaderFromSource(BuiltinShaders::ChunkVertexShader, 
                                                          vk::ShaderStageFlagBits::eVertex, "chunk.vert"));
    chunkProgram->modules.push_back(loadShaderFromSource(BuiltinShaders::ChunkFragmentShader,
                                                          vk::ShaderStageFlagBits::eFragment, "chunk.frag"));
    
    // Entity shader
    auto* entityProgram = createProgram(ENTITY_SHADER);
    entityProgram->modules.push_back(loadShaderFromSource(BuiltinShaders::EntityVertexShader,
                                                           vk::ShaderStageFlagBits::eVertex, "entity.vert"));
    entityProgram->modules.push_back(loadShaderFromSource(BuiltinShaders::EntityFragmentShader,
                                                           vk::ShaderStageFlagBits::eFragment, "entity.frag"));
    
    // UI shader
    auto* uiProgram = createProgram(UI_SHADER);
    uiProgram->modules.push_back(loadShaderFromSource(BuiltinShaders::UIVertexShader,
                                                       vk::ShaderStageFlagBits::eVertex, "ui.vert"));
    uiProgram->modules.push_back(loadShaderFromSource(BuiltinShaders::UIFragmentShader,
                                                       vk::ShaderStageFlagBits::eFragment, "ui.frag"));
    
    // Sky shader
    auto* skyProgram = createProgram(SKY_SHADER);
    skyProgram->modules.push_back(loadShaderFromSource(BuiltinShaders::SkyVertexShader,
                                                        vk::ShaderStageFlagBits::eVertex, "sky.vert"));
    skyProgram->modules.push_back(loadShaderFromSource(BuiltinShaders::SkyFragmentShader,
                                                        vk::ShaderStageFlagBits::eFragment, "sky.frag"));
    
    // Shadow shader
    auto* shadowProgram = createProgram(SHADOW_SHADER);
    shadowProgram->modules.push_back(loadShaderFromSource(BuiltinShaders::ShadowVertexShader,
                                                           vk::ShaderStageFlagBits::eVertex, "shadow.vert"));
    shadowProgram->modules.push_back(loadShaderFromSource(BuiltinShaders::ShadowFragmentShader,
                                                           vk::ShaderStageFlagBits::eFragment, "shadow.frag"));
    
    // Particle shader
    auto* particleProgram = createProgram(PARTICLE_SHADER);
    particleProgram->modules.push_back(loadShaderFromSource(BuiltinShaders::ParticleVertexShader,
                                                             vk::ShaderStageFlagBits::eVertex, "particle.vert"));
    particleProgram->modules.push_back(loadShaderFromSource(BuiltinShaders::ParticleFragmentShader,
                                                             vk::ShaderStageFlagBits::eFragment, "particle.frag"));
    
    // Post-process shader
    auto* postProgram = createProgram(POSTPROCESS_SHADER);
    postProgram->modules.push_back(loadShaderFromSource(BuiltinShaders::PostProcessVertexShader,
                                                         vk::ShaderStageFlagBits::eVertex, "post.vert"));
    postProgram->modules.push_back(loadShaderFromSource(BuiltinShaders::FXAAFragmentShader,
                                                         vk::ShaderStageFlagBits::eFragment, "fxaa.frag"));
    
    Logger::info("Built-in shaders loaded");
}

ShaderProgram* ShaderManager::getChunkShader() { return getProgram(CHUNK_SHADER); }
ShaderProgram* ShaderManager::getEntityShader() { return getProgram(ENTITY_SHADER); }
ShaderProgram* ShaderManager::getUIShader() { return getProgram(UI_SHADER); }
ShaderProgram* ShaderManager::getPostProcessShader() { return getProgram(POSTPROCESS_SHADER); }
ShaderProgram* ShaderManager::getShadowShader() { return getProgram(SHADOW_SHADER); }
ShaderProgram* ShaderManager::getSkyShader() { return getProgram(SKY_SHADER); }
ShaderProgram* ShaderManager::getParticleShader() { return getProgram(PARTICLE_SHADER); }

void ShaderManager::createShaderModule(ShaderModule& shader, const std::vector<uint32_t>& spirv) {
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();
    
    shader.module = device->getDevice().createShaderModule(createInfo);
}

void ShaderManager::destroyShaderModule(ShaderModule& shader) {
    if (shader.module) {
        device->getDevice().destroyShaderModule(shader.module);
        shader.module = nullptr;
    }
}

std::string ShaderManager::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

std::string ShaderManager::preprocessShader(const std::string& source, const std::string& path) {
    std::string result;
    std::istringstream stream(source);
    std::string line;
    
    std::filesystem::path currentDir = std::filesystem::path(path).parent_path();
    
    while (std::getline(stream, line)) {
        if (line.substr(0, 8) == "#include") {
            // Parse include path
            size_t start = line.find('"');
            size_t end = line.find('"', start + 1);
            if (start != std::string::npos && end != std::string::npos) {
                std::string includePath = line.substr(start + 1, end - start - 1);
                
                // Try current directory first
                std::filesystem::path fullPath = currentDir / includePath;
                
                // Try include directories
                if (!std::filesystem::exists(fullPath)) {
                    for (const auto& incDir : settings.includeDirectories) {
                        fullPath = incDir / includePath;
                        if (std::filesystem::exists(fullPath)) break;
                    }
                }
                
                if (std::filesystem::exists(fullPath)) {
                    result += readFile(fullPath.string());
                    result += "\n";
                }
            }
        } else {
            result += line;
            result += "\n";
        }
    }
    
    return result;
}

bool ShaderManager::fileExists(const std::string& path) const {
    return std::filesystem::exists(path);
}

void ShaderManager::watchFile(const std::string& path) {
    if (watchedFiles.find(path) != watchedFiles.end()) return;
    
    FileWatch watch;
    watch.path = path;
    watch.lastWriteTime = std::filesystem::last_write_time(path);
    watchedFiles[path] = watch;
}

// Builtin shader implementations

namespace BuiltinShaders {

const char* ChunkVertexShader = R"(
#version 450
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in uint inTexIndex;
layout(location = 4) in uint inColor;
layout(location = 5) in uint inAO;

layout(set = 0, binding = 0) uniform ChunkUniforms {
    mat4 viewProj;
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
    float time;
    float fogStart;
    float fogEnd;
    vec4 fogColor;
    int renderDistance;
};

layout(push_constant) uniform PushConstants {
    mat4 model;
};

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out flat uint fragTexIndex;
layout(location = 4) out vec4 fragColor;
layout(location = 5) out float fragAO;
layout(location = 6) out float fragFog;

void main() {
    vec4 worldPos = model * vec4(inPosition, 1.0);
    gl_Position = viewProj * worldPos;
    
    fragPos = worldPos.xyz;
    fragNormal = mat3(model) * inNormal;
    fragTexCoord = inTexCoord;
    fragTexIndex = inTexIndex;
    
    // Unpack color
    fragColor = vec4(
        float((inColor >> 0) & 0xFF) / 255.0,
        float((inColor >> 8) & 0xFF) / 255.0,
        float((inColor >> 16) & 0xFF) / 255.0,
        float((inColor >> 24) & 0xFF) / 255.0
    );
    
    // Unpack AO
    fragAO = float(inAO & 0x3) / 3.0;
    
    // Calculate fog
    float dist = length(cameraPos - worldPos.xyz);
    fragFog = clamp((dist - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
}
)";

const char* ChunkFragmentShader = R"(
#version 450

layout(set = 0, binding = 1) uniform sampler2DArray texArray;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in flat uint fragTexIndex;
layout(location = 4) in vec4 fragColor;
layout(location = 5) in float fragAO;
layout(location = 6) in float fragFog;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texArray, vec3(fragTexCoord, float(fragTexIndex)));
    vec4 color = texColor * fragColor;
    
    // Apply ambient occlusion
    color.rgb *= mix(0.5, 1.0, fragAO);
    
    // Simple lighting
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(fragNormal), lightDir), 0.3);
    color.rgb *= diff;
    
    // Apply fog
    color.rgb = mix(color.rgb, vec3(0.6, 0.8, 1.0), fragFog);
    
    outColor = color;
}
)";

const char* EntityVertexShader = R"(
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;

layout(set = 0, binding = 0) uniform EntityUniforms {
    mat4 viewProj;
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
    float time;
    vec3 lightDir;
    float lightIntensity;
    uint enableShadows;
};

layout(push_constant) uniform PushConstants {
    mat4 model;
};

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec4 fragColor;

void main() {
    vec4 worldPos = model * vec4(inPosition, 1.0);
    gl_Position = viewProj * worldPos;
    
    fragPos = worldPos.xyz;
    fragNormal = mat3(model) * inNormal;
    fragTexCoord = inTexCoord;
    fragColor = inColor;
}
)";

const char* EntityFragmentShader = R"(
#version 450

layout(set = 0, binding = 2) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec4 color = texColor * fragColor;
    
    // Simple directional lighting
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(fragNormal), lightDir), 0.3);
    color.rgb *= diff;
    
    outColor = color;
}
)";

const char* UIVertexShader = R"(
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

layout(push_constant) uniform PushConstants {
    vec2 scale;
    vec2 translate;
};

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

void main() {
    gl_Position = vec4(inPosition * scale + translate, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragColor = inColor;
}
)";

const char* UIFragmentShader = R"(
#version 450

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    outColor = texColor * fragColor;
}
)";

const char* SkyVertexShader = R"(
#version 450

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform SkyUniforms {
    mat4 projection;
    mat4 view;
    float time;
};

layout(location = 0) out vec3 fragPos;

void main() {
    fragPos = inPosition;
    vec4 pos = projection * mat4(mat3(view)) * vec4(inPosition * 1000.0, 1.0);
    gl_Position = pos.xyww;  // Force max depth
}
)";

const char* SkyFragmentShader = R"(
#version 450

layout(location = 0) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 dir = normalize(fragPos);
    
    // Sky gradient
    float t = dir.y * 0.5 + 0.5;
    vec3 bottomColor = vec3(0.8, 0.9, 1.0);
    vec3 topColor = vec3(0.4, 0.6, 0.9);
    vec3 skyColor = mix(bottomColor, topColor, t);
    
    outColor = vec4(skyColor, 1.0);
}
)";

const char* PostProcessVertexShader = R"(
#version 450

layout(location = 0) out vec2 fragTexCoord;

void main() {
    vec2 positions[6] = vec2[](
        vec2(-1.0, -1.0),
        vec2(1.0, -1.0),
        vec2(1.0, 1.0),
        vec2(-1.0, -1.0),
        vec2(1.0, 1.0),
        vec2(-1.0, 1.0)
    );
    
    vec2 texCoords[6] = vec2[](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0)
    );
    
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragTexCoord = texCoords[gl_VertexIndex];
}
)";

const char* FXAAFragmentShader = R"(
#version 450

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

void main() {
    // Simple FXAA implementation
    vec2 texelSize = 1.0 / textureSize(texSampler, 0);
    
    vec3 color = texture(texSampler, fragTexCoord).rgb;
    vec3 colorN = texture(texSampler, fragTexCoord + vec2(0.0, -texelSize.y)).rgb;
    vec3 colorS = texture(texSampler, fragTexCoord + vec2(0.0, texelSize.y)).rgb;
    vec3 colorW = texture(texSampler, fragTexCoord + vec2(-texelSize.x, 0.0)).rgb;
    vec3 colorE = texture(texSampler, fragTexCoord + vec2(texelSize.x, 0.0)).rgb;
    
    vec3 blur = (color + colorN + colorS + colorW + colorE) / 5.0;
    
    outColor = vec4(blur, 1.0);
}
)";

const char* TAAFragmentShader = R"(
#version 450

layout(set = 0, binding = 0) uniform sampler2D currentFrame;
layout(set = 0, binding = 1) uniform sampler2D historyFrame;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 current = texture(currentFrame, fragTexCoord).rgb;
    vec3 history = texture(historyFrame, fragTexCoord).rgb;
    
    // Simple blend
    outColor = vec4(mix(current, history, 0.9), 1.0);
}
)";

const char* ShadowVertexShader = R"(
#version 450

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants {
    mat4 lightSpaceMatrix;
    mat4 model;
};

void main() {
    gl_Position = lightSpaceMatrix * model * vec4(inPosition, 1.0);
}
)";

const char* ShadowFragmentShader = R"(
#version 450

void main() {
    // Depth is written automatically
}
)";

const char* ParticleVertexShader = R"(
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inSize;

layout(set = 0, binding = 0) uniform ParticleUniforms {
    mat4 viewProj;
    vec3 cameraRight;
    vec3 cameraUp;
};

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 vertexPosition = inPosition 
        + cameraRight * inSize.x * (gl_VertexIndex == 0 || gl_VertexIndex == 2 ? -0.5 : 0.5)
        + cameraUp * inSize.y * (gl_VertexIndex == 0 || gl_VertexIndex == 1 ? -0.5 : 0.5);
    
    gl_Position = viewProj * vec4(vertexPosition, 1.0);
    fragColor = inColor;
}
)";

const char* ParticleFragmentShader = R"(
#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = fragColor;
}
)";

} // namespace BuiltinShaders

} // namespace VoxelForge
