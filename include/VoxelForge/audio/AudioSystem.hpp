/**
 * @file AudioSystem.hpp
 * @brief FMOD-based audio system
 */

#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace FMOD {
    class System;
    class Sound;
    class Channel;
    class ChannelGroup;
    class DSP;
    class Reverb3D;
    struct VECTOR;
}

namespace VoxelForge {

// Forward declarations
class World;

// Audio settings
struct AudioSettings {
    int frequency = 48000;
    int maxChannels = 256;
    int virtualChannels = 1024;
    float masterVolume = 1.0f;
    float musicVolume = 0.7f;
    float sfxVolume = 1.0f;
    float ambientVolume = 0.8f;
    float voiceVolume = 1.0f;
    bool enableReverb = true;
    bool enable3D = true;
    float dopplerScale = 1.0f;
    float distanceFactor = 1.0f;  // 1 unit = 1 meter
    float rolloffScale = 1.0f;
};

// Sound loading info
struct SoundInfo {
    std::string name;
    std::string path;
    bool stream = false;        // Stream from disk (for music)
    bool loop = false;
    bool is3D = false;
    float minDistance = 1.0f;   // Distance at which volume starts attenuating
    float maxDistance = 100.0f; // Distance at which volume is silent
    float volume = 1.0f;
    float pitch = 1.0f;
};

// Sound category
enum class SoundCategory {
    Master,
    Music,
    SFX,
    Ambient,
    Voice,
    UI
};

// 3D attributes for sound
struct Sound3DAttributes {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
};

// Playing sound instance
struct SoundInstance {
    uint32_t id = 0;
    std::string name;
    FMOD::Channel* channel = nullptr;
    SoundCategory category = SoundCategory::SFX;
    bool isPlaying = false;
    bool isPaused = false;
    float volume = 1.0f;
    float pitch = 1.0f;
    bool is3D = false;
    Sound3DAttributes attributes;
};

// Reverb preset
struct ReverbPreset {
    std::string name;
    float decayTime = 1.49f;
    float earlyDelay = 0.007f;
    float lateDelay = 0.011f;
    float hfReference = 5000.0f;
    float hfDecayRatio = 0.83f;
    float diffusion = 1.0f;
    float density = 1.0f;
    float lowShelfFrequency = 250.0f;
    float lowShelfGain = 0.0f;
    float highCut = 10000.0f;
    float earlyLateMix = 0.96f;
    float wetLevel = -6.0f;
    float dryLevel = 0.0f;
    
    // Minecraft-inspired presets
    static ReverbPreset Cave();
    static ReverbPreset Underwater();
    static ReverbPreset Nether();
    static ReverbPreset End();
    static ReverbPreset LargeRoom();
    static ReverbPreset SmallRoom();
    static ReverbPreset Outdoor();
};

// Audio system
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();
    
    // No copy
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    
    void init(const AudioSettings& settings = {});
    void shutdown();
    
    // Update (call once per frame)
    void update();
    void update3DListener(const glm::vec3& position, const glm::vec3& velocity,
                          const glm::vec3& forward, const glm::vec3& up);
    
    // Sound loading
    bool loadSound(const SoundInfo& info);
    bool loadSound(const std::string& name, const std::string& path, 
                   bool stream = false, bool is3D = false);
    void unloadSound(const std::string& name);
    void unloadAllSounds();
    bool isSoundLoaded(const std::string& name) const;
    
    // Sound playback
    uint32_t play(const std::string& name, SoundCategory category = SoundCategory::SFX);
    uint32_t play3D(const std::string& name, const glm::vec3& position,
                    SoundCategory category = SoundCategory::SFX);
    uint32_t playMusic(const std::string& name);
    uint32_t playAmbient(const std::string& name);
    
    void stop(uint32_t instanceId);
    void stopAll();
    void stopCategory(SoundCategory category);
    void pause(uint32_t instanceId);
    void resume(uint32_t instanceId);
    void pauseAll();
    void resumeAll();
    
    // Instance control
    void setVolume(uint32_t instanceId, float volume);
    void setPitch(uint32_t instanceId, float pitch);
    void set3DAttributes(uint32_t instanceId, const Sound3DAttributes& attributes);
    void setLooping(uint32_t instanceId, bool loop);
    
    bool isPlaying(uint32_t instanceId) const;
    bool isPaused(uint32_t instanceId) const;
    
    // Category volume
    void setMasterVolume(float volume);
    void setMusicVolume(float volume);
    void setSFXVolume(float volume);
    void setAmbientVolume(float volume);
    void setVoiceVolume(float volume);
    void setUIVolume(float volume);
    
    float getMasterVolume() const { return settings.masterVolume; }
    float getMusicVolume() const { return settings.musicVolume; }
    float getSFXVolume() const { return settings.sfxVolume; }
    float getAmbientVolume() const { return settings.ambientVolume; }
    float getVoiceVolume() const { return settings.voiceVolume; }
    
    // Reverb
    void setReverbEnabled(bool enabled);
    void setReverbPreset(const ReverbPreset& preset);
    void setReverbZone(const glm::vec3& position, float radius, const ReverbPreset& preset);
    void clearReverbZones();
    
    // DSP effects
    void enableLowPassFilter(bool enabled, float cutoff = 5000.0f);
    void enableHighPassFilter(bool enabled, float cutoff = 100.0f);
    void enableEcho(bool enabled, float delay = 500.0f, float decay = 0.5f);
    void enableDistortion(bool enabled, float level = 0.5f);
    
    // Underwater effect
    void setUnderwaterEffect(bool enabled);
    
    // Sound query
    SoundInstance* getInstance(uint32_t instanceId);
    const SoundInstance* getInstance(uint32_t instanceId) const;
    std::vector<uint32_t> getActiveInstances() const;
    
    // Utility
    float getSoundDuration(const std::string& name) const;
    float getInstancePosition(uint32_t instanceId) const;  // In seconds
    
    // Getters
    FMOD::System* getSystem() const { return system; }
    const AudioSettings& getSettings() const { return settings; }
    
private:
    FMOD::VECTOR toFMODVector(const glm::vec3& v) const;
    uint32_t generateInstanceId();
    void cleanupFinishedInstances();
    
    AudioSettings settings;
    
    FMOD::System* system = nullptr;
    FMOD::ChannelGroup* masterGroup = nullptr;
    FMOD::ChannelGroup* musicGroup = nullptr;
    FMOD::ChannelGroup* sfxGroup = nullptr;
    FMOD::ChannelGroup* ambientGroup = nullptr;
    FMOD::ChannelGroup* voiceGroup = nullptr;
    FMOD::ChannelGroup* uiGroup = nullptr;
    
    // Loaded sounds
    std::unordered_map<std::string, FMOD::Sound*> sounds;
    
    // Active sound instances
    std::unordered_map<uint32_t, std::unique_ptr<SoundInstance>> instances;
    uint32_t nextInstanceId = 1;
    
    // DSP effects
    FMOD::DSP* lowPassDSP = nullptr;
    FMOD::DSP* highPassDSP = nullptr;
    FMOD::DSP* echoDSP = nullptr;
    FMOD::DSP* distortionDSP = nullptr;
    
    // Reverb zones
    std::vector<FMOD::Reverb3D*> reverbZones;
    
    // Listener position for 3D
    Sound3DAttributes listenerAttributes;
};

// Sound manager for game-specific sounds
class SoundManager {
public:
    SoundManager();
    ~SoundManager();
    
    void init(AudioSystem* audio);
    void shutdown();
    
    // Load game sounds
    void loadBlockSounds();
    void loadEntitySounds();
    void loadAmbientSounds();
    void loadMusic();
    void loadUISounds();
    void loadAllSounds();
    
    // Play specific sounds
    uint32_t playBlockSound(const std::string& blockName, const std::string& action,
                           const glm::vec3& position);
    uint32_t playEntitySound(const std::string& entityName, const std::string& action,
                            const glm::vec3& position);
    uint32_t playRandomBlockSound(const std::string& blockName, const std::string& action,
                                  const glm::vec3& position);
    uint32_t playRandomEntitySound(const std::string& entityName, const std::string& action,
                                   const glm::vec3& position);
    
    // Ambient sounds
    void playBiomeAmbient(const std::string& biomeName);
    void playWeatherAmbient(const std::string& weather);
    void stopAmbient();
    
    // Music
    void playRandomMusic();
    void playMenuMusic();
    void stopMusic();
    
    // Update
    void update(float deltaTime);
    
    // Settings
    void setSoundPack(const std::string& packName);
    std::string getSoundPack() const { return currentSoundPack; }
    
private:
    std::string getRandomVariant(const std::string& baseName, int variants);
    
    AudioSystem* audio = nullptr;
    std::string currentSoundPack = "default";
    
    // Sound mappings
    std::unordered_map<std::string, std::vector<std::string>> blockSounds;
    std::unordered_map<std::string, std::vector<std::string>> entitySounds;
    std::vector<std::string> musicTracks;
    
    // Current ambient
    uint32_t currentAmbientId = 0;
    uint32_t currentMusicId = 0;
};

} // namespace VoxelForge
