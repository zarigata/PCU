/**
 * @file Config.cpp
 * @brief Configuration system implementation
 */

#include <VoxelForge/engine/Config.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <fstream>
#include <filesystem>

namespace VoxelForge {

// ============================================
// Config
// ============================================

Config::Config(const std::string& filepath) {
    load(filepath);
}

bool Config::load(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            VF_CORE_WARN("Could not open config file: {}", filepath);
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        
        values.clear();
        for (auto& [key, val] : j.items()) {
            values[key] = jsonToValue(val);
        }
        
        VF_CORE_INFO("Loaded config from: {}", filepath);
        return true;
    }
    catch (const std::exception& e) {
        VF_CORE_ERROR("Error loading config: {}", e.what());
        return false;
    }
}

bool Config::save(const std::string& filepath) {
    try {
        nlohmann::json j;
        for (const auto& [key, val] : values) {
            j[key] = valueToJson(val);
        }
        
        // Create directory if needed
        std::filesystem::path path(filepath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        
        std::ofstream file(filepath);
        file << j.dump(4);
        
        VF_CORE_INFO("Saved config to: {}", filepath);
        return true;
    }
    catch (const std::exception& e) {
        VF_CORE_ERROR("Error saving config: {}", e.what());
        return false;
    }
}

bool Config::getBool(const std::string& key, bool defaultVal) const {
    return get<bool>(key, defaultVal);
}

int64_t Config::getInt(const std::string& key, int64_t defaultVal) const {
    return get<int64_t>(key, defaultVal);
}

double Config::getDouble(const std::string& key, double defaultVal) const {
    return get<double>(key, defaultVal);
}

std::string Config::getString(const std::string& key, const std::string& defaultVal) const {
    return get<std::string>(key, defaultVal);
}

void Config::set(const std::string& key, const Value& value) {
    values[key] = value;
}

void Config::setBool(const std::string& key, bool value) {
    values[key] = value;
}

void Config::setInt(const std::string& key, int64_t value) {
    values[key] = value;
}

void Config::setDouble(const std::string& key, double value) {
    values[key] = value;
}

void Config::setString(const std::string& key, const std::string& value) {
    values[key] = value;
}

bool Config::has(const std::string& key) const {
    return values.find(key) != values.end();
}

void Config::remove(const std::string& key) {
    values.erase(key);
}

void Config::clear() {
    values.clear();
}

void Config::merge(const Config& other) {
    for (const auto& [key, val] : other.values) {
        values[key] = val;
    }
}

Config::Value Config::jsonToValue(const nlohmann::json& j) {
    if (j.is_null()) return std::monostate{};
    if (j.is_boolean()) return j.get<bool>();
    if (j.is_number_integer()) return j.get<int64_t>();
    if (j.is_number_float()) return j.get<double>();
    if (j.is_string()) return j.get<std::string>();
    if (j.is_array()) {
        if (j.empty()) return std::monostate{};
        
        if (j[0].is_boolean()) {
            return j.get<std::vector<bool>>();
        } else if (j[0].is_number_integer()) {
            return j.get<std::vector<int64_t>>();
        } else if (j[0].is_number_float()) {
            return j.get<std::vector<double>>();
        } else if (j[0].is_string()) {
            return j.get<std::vector<std::string>>();
        }
    }
    return std::monostate{};
}

nlohmann::json Config::valueToJson(const Value& v) {
    if (std::holds_alternative<std::monostate>(v)) {
        return nullptr;
    } else if (std::holds_alternative<bool>(v)) {
        return std::get<bool>(v);
    } else if (std::holds_alternative<int64_t>(v)) {
        return std::get<int64_t>(v);
    } else if (std::holds_alternative<double>(v)) {
        return std::get<double>(v);
    } else if (std::holds_alternative<std::string>(v)) {
        return std::get<std::string>(v);
    } else if (std::holds_alternative<std::vector<bool>>(v)) {
        return std::get<std::vector<bool>>(v);
    } else if (std::holds_alternative<std::vector<int64_t>>(v)) {
        return std::get<std::vector<int64_t>>(v);
    } else if (std::holds_alternative<std::vector<double>>(v)) {
        return std::get<std::vector<double>>(v);
    } else if (std::holds_alternative<std::vector<std::string>>(v)) {
        return std::get<std::vector<std::string>>(v);
    }
    return nullptr;
}

// ============================================
// Global Game Config
// ============================================

namespace GameConfig {

static Config globalConfig;

Config& get() {
    return globalConfig;
}

bool load() {
    return get().load("config/game.json");
}

bool save() {
    return get().save("config/game.json");
}

int getRenderDistance() {
    return static_cast<int>(get().getInt("render_distance", 8));
}

int getSimulationDistance() {
    return static_cast<int>(get().getInt("simulation_distance", 6));
}

float getFOV() {
    return static_cast<float>(get().getDouble("fov", 70.0));
}

float getMouseSensitivity() {
    return static_cast<float>(get().getDouble("mouse_sensitivity", 0.1));
}

bool isVSync() {
    return get().getBool("vsync", true);
}

bool isFullscreen() {
    return get().getBool("fullscreen", false);
}

int getMaxFPS() {
    return static_cast<int>(get().getInt("max_fps", 120));
}

void setRenderDistance(int distance) {
    get().setInt("render_distance", distance);
}

void setSimulationDistance(int distance) {
    get().setInt("simulation_distance", distance);
}

void setFOV(float fov) {
    get().setDouble("fov", fov);
}

void setMouseSensitivity(float sensitivity) {
    get().setDouble("mouse_sensitivity", sensitivity);
}

void setVSync(bool enabled) {
    get().setBool("vsync", enabled);
}

void setFullscreen(bool enabled) {
    get().setBool("fullscreen", enabled);
}

void setMaxFPS(int fps) {
    get().setInt("max_fps", fps);
}

} // namespace GameConfig

} // namespace VoxelForge
