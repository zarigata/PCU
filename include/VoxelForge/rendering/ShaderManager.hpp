/**
 * @file ShaderManager.hpp
 * @brief Shader loading and management
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>

namespace VoxelForge {

class VulkanDevice;

// Shader module wrapper
struct ShaderModule {
    vk::ShaderModule module = VK_NULL_HANDLE;
    vk::ShaderStageFlagBits stage;
    std::string entryPoint = "main";
    std::string filePath;
    std::string sourceCode;  // For hot reload
    
    operator bool() const { return module != VK_NULL_HANDLE; }
};

// Shader program (collection of modules)
struct ShaderProgram {
    std::string name;
    std::vector<ShaderModule> modules;
    vk::PipelineLayout pipelineLayout = VK_NULL_HANDLE;
    std::vector<vk::DescriptorSetLayout> descriptorLayouts;
    
    // Specialization constants
    std::unordered_map<uint32_t, uint32_t> specializationConstants;
    
    operator bool() const { return !modules.empty(); }
};

// Shader include handler
struct ShaderInclude {
    std::string path;
    std::string content;
};

// Shader compilation result
struct ShaderCompileResult {
    bool success = false;
    std::string errorMessage;
    std::vector<uint32_t> spirv;
};

// Shader manager settings
struct ShaderManagerSettings {
    std::filesystem::path shaderDirectory = "shaders";
    bool enableHotReload = true;
    bool enableOptimization = true;
    bool includeDebugInfo = false;
    std::vector<std::filesystem::path> includeDirectories;
};

class ShaderManager {
public:
    ShaderManager();
    ~ShaderManager();
    
    // No copy
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    
    void init(VulkanDevice* device, const ShaderManagerSettings& settings = {});
    void cleanup();
    
    // Shader loading
    ShaderModule loadShader(const std::string& path, vk::ShaderStageFlagBits stage);
    ShaderModule loadShaderFromSource(const std::string& source, 
                                       vk::ShaderStageFlagBits stage,
                                       const std::string& name = "");
    ShaderModule loadShaderFromSPIRV(const std::vector<uint32_t>& spirv,
                                      vk::ShaderStageFlagBits stage,
                                      const std::string& name = "");
    
    // Program management
    ShaderProgram* createProgram(const std::string& name);
    ShaderProgram* getProgram(const std::string& name);
    void destroyProgram(const std::string& name);
    
    // Add shaders to program
    void addVertexShader(ShaderProgram& program, const std::string& path);
    void addFragmentShader(ShaderProgram& program, const std::string& path);
    void addGeometryShader(ShaderProgram& program, const std::string& path);
    void addComputeShader(ShaderProgram& program, const std::string& path);
    void addTessControlShader(ShaderProgram& program, const std::string& path);
    void addTessEvalShader(ShaderProgram& program, const std::string& path);
    
    // Compilation
    ShaderCompileResult compileGLSL(const std::string& source, 
                                     vk::ShaderStageFlagBits stage,
                                     const std::string& name = "");
    
    // Hot reload
    void checkForChanges();
    void reloadShader(const std::string& path);
    void reloadAll();
    
    // Get shader stages for pipeline creation
    std::vector<vk::PipelineShaderStageCreateInfo> getShaderStages(const ShaderProgram& program);
    
    // Specialization constants
    void setSpecializationConstant(ShaderProgram& program, uint32_t constantId, uint32_t value);
    
    // Include directories
    void addIncludeDirectory(const std::filesystem::path& path);
    
    // Reflection
    struct DescriptorBindingInfo {
        uint32_t set;
        uint32_t binding;
        vk::DescriptorType type;
        uint32_t count;
        vk::ShaderStageFlags stages;
        std::string name;
    };
    
    std::vector<DescriptorBindingInfo> reflectDescriptorBindings(const ShaderProgram& program);
    
    // Built-in shaders
    void loadBuiltinShaders();
    ShaderProgram* getChunkShader();
    ShaderProgram* getEntityShader();
    ShaderProgram* getUIShader();
    ShaderProgram* getPostProcessShader();
    ShaderProgram* getShadowShader();
    ShaderProgram* getSkyShader();
    ShaderProgram* getParticleShader();
    
private:
    void createShaderModule(ShaderModule& shader, const std::vector<uint32_t>& spirv);
    void destroyShaderModule(ShaderModule& shader);
    std::string readFile(const std::string& path);
    std::string preprocessShader(const std::string& source, const std::string& path);
    bool fileExists(const std::string& path) const;
    void watchFile(const std::string& path);
    
    VulkanDevice* device = nullptr;
    ShaderManagerSettings settings;
    
    // Loaded shaders
    std::unordered_map<std::string, ShaderModule> shaderModules;
    std::unordered_map<std::string, ShaderProgram> programs;
    
    // File watching for hot reload
    struct FileWatch {
        std::string path;
        std::filesystem::file_time_type lastWriteTime;
        std::vector<std::string> dependentPrograms;
    };
    std::unordered_map<std::string, FileWatch> watchedFiles;
    
    // Built-in shader names
    static constexpr const char* CHUNK_SHADER = "chunk";
    static constexpr const char* ENTITY_SHADER = "entity";
    static constexpr const char* UI_SHADER = "ui";
    static constexpr const char* POSTPROCESS_SHADER = "postprocess";
    static constexpr const char* SHADOW_SHADER = "shadow";
    static constexpr const char* SKY_SHADER = "sky";
    static constexpr const char* PARTICLE_SHADER = "particle";
};

// Inline shader sources for built-in shaders
namespace BuiltinShaders {

// Chunk vertex shader (GLSL)
extern const char* ChunkVertexShader;

// Chunk fragment shader (GLSL)
extern const char* ChunkFragmentShader;

// Entity vertex shader (GLSL)
extern const char* EntityVertexShader;

// Entity fragment shader (GLSL)
extern const char* EntityFragmentShader;

// UI vertex shader (GLSL)
extern const char* UIVertexShader;

// UI fragment shader (GLSL)
extern const char* UIFragmentShader;

// Sky vertex shader (GLSL)
extern const char* SkyVertexShader;

// Sky fragment shader (GLSL)
extern const char* SkyFragmentShader;

// Post-process vertex shader (GLSL)
extern const char* PostProcessVertexShader;

// Post-process fragment shader (FXAA)
extern const char* FXAAFragmentShader;

// Post-process fragment shader (TAA)
extern const char* TAAFragmentShader;

// Shadow vertex shader (GLSL)
extern const char* ShadowVertexShader;

// Shadow fragment shader (GLSL)
extern const char* ShadowFragmentShader;

// Particle vertex shader (GLSL)
extern const char* ParticleVertexShader;

// Particle fragment shader (GLSL)
extern const char* ParticleFragmentShader;

} // namespace BuiltinShaders

} // namespace VoxelForge
