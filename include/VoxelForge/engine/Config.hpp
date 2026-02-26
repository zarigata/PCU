/**
 * @file Config.hpp
 * @brief Configuration management system
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <fstream>
#include <nlohmann/json.hpp>

namespace VoxelForge {

class Config {
public:
    using Value = std::variant<
        std::monostate,
        bool,
        int64_t,
        double,
        std::string,
        std::vector<bool>,
        std::vector<int64_t>,
        std::vector<double>,
        std::vector<std::string>
    >;
    
    Config() = default;
    explicit Config(const std::string& filepath);
    
    // Load/save
    bool load(const std::string& filepath);
    bool save(const std::string& filepath);
    
    // Getters
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T{}) const {
        auto it = values.find(key);
        if (it == values.end()) return defaultValue;
        
        if (const T* val = std::get_if<T>(&it->second)) {
            return *val;
        }
        return defaultValue;
    }
    
    bool getBool(const std::string& key, bool defaultVal = false) const;
    int64_t getInt(const std::string& key, int64_t defaultVal = 0) const;
    double getDouble(const std::string& key, double defaultVal = 0.0) const;
    std::string getString(const std::string& key, const std::string& defaultVal = "") const;
    
    // Setters
    void set(const std::string& key, const Value& value);
    void setBool(const std::string& key, bool value);
    void setInt(const std::string& key, int64_t value);
    void setDouble(const std::string& key, double value);
    void setString(const std::string& key, const std::string& value);
    
    // Check
    bool has(const std::string& key) const;
    void remove(const std::string& key);
    void clear();
    
    // Merge
    void merge(const Config& other);
    
    // Access
    const std::unordered_map<std::string, Value>& getAll() const { return values; }
    size_t size() const { return values.size(); }
    bool empty() const { return values.empty(); }
    
private:
    std::unordered_map<std::string, Value> values;
    
    static Value jsonToValue(const nlohmann::json& j);
    static nlohmann::json valueToJson(const Value& v);
};

// Global config
namespace GameConfig {
    Config& get();
    
    bool load();
    bool save();
    
    // Convenience getters
    int getRenderDistance();
    int getSimulationDistance();
    float getFOV();
    float getMouseSensitivity();
    bool isVSync();
    bool isFullscreen();
    int getMaxFPS();
    
    // Convenience setters
    void setRenderDistance(int distance);
    void setSimulationDistance(int distance);
    void setFOV(float fov);
    void setMouseSensitivity(float sensitivity);
    void setVSync(bool enabled);
    void setFullscreen(bool enabled);
    void setMaxFPS(int fps);
}

} // namespace VoxelForge
