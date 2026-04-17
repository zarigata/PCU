/**
 * @file AudioSystem.cpp
 * @brief FMOD-based audio system implementation
 */

#include "VoxelForge/audio/AudioSystem.hpp"
#include <VoxelForge/core/Logger.hpp>

#ifdef USE_FMOD
#include <fmod.hpp>
#include <fmod_errors.h>
#endif

namespace VoxelForge {

#ifdef USE_FMOD
#define FMOD_CHECK(result) \
    if (result != FMOD_OK) { \
        VF_ERROR("FMOD Error: {} ({})", FMOD_ErrorString(result), result); \
        return false; \
    }
#else
#define FMOD_CHECK(result) ((void)0)
#endif

ReverbPreset ReverbPreset::Cave() {
    ReverbPreset p;
    p.name = "Cave";
    p.decayTime = 2.0f;
    p.earlyDelay = 0.02f;
    p.lateDelay = 0.03f;
    p.diffusion = 0.9f;
    p.density = 0.9f;
    p.wetLevel = -3.0f;
    return p;
}

ReverbPreset ReverbPreset::Underwater() {
    ReverbPreset p;
    p.name = "Underwater";
    p.decayTime = 3.0f;
    p.earlyDelay = 0.05f;
    p.lateDelay = 0.1f;
    p.hfDecayRatio = 0.1f;
    p.diffusion = 0.5f;
    p.density = 0.5f;
    p.wetLevel = -6.0f;
    return p;
}

ReverbPreset ReverbPreset::Nether() {
    ReverbPreset p;
    p.name = "Nether";
    p.decayTime = 1.5f;
    p.earlyDelay = 0.01f;
    p.lateDelay = 0.02f;
    p.diffusion = 0.95f;
    p.density = 0.95f;
    p.wetLevel = -4.0f;
    return p;
}

ReverbPreset ReverbPreset::End() {
    ReverbPreset p;
    p.name = "End";
    p.decayTime = 5.0f;
    p.earlyDelay = 0.1f;
    p.lateDelay = 0.2f;
    p.diffusion = 0.8f;
    p.density = 0.6f;
    p.wetLevel = -8.0f;
    return p;
}

ReverbPreset ReverbPreset::LargeRoom() {
    ReverbPreset p;
    p.name = "LargeRoom";
    p.decayTime = 1.8f;
    p.earlyDelay = 0.02f;
    p.lateDelay = 0.03f;
    p.diffusion = 0.85f;
    p.density = 0.85f;
    p.wetLevel = -6.0f;
    return p;
}

ReverbPreset ReverbPreset::SmallRoom() {
    ReverbPreset p;
    p.name = "SmallRoom";
    p.decayTime = 0.8f;
    p.earlyDelay = 0.01f;
    p.lateDelay = 0.015f;
    p.diffusion = 0.9f;
    p.density = 0.9f;
    p.wetLevel = -9.0f;
    return p;
}

ReverbPreset ReverbPreset::Outdoor() {
    ReverbPreset p;
    p.name = "Outdoor";
    p.decayTime = 0.3f;
    p.earlyDelay = 0.0f;
    p.lateDelay = 0.01f;
    p.diffusion = 0.3f;
    p.density = 0.3f;
    p.wetLevel = -12.0f;
    return p;
}

#ifdef USE_FMOD
// FMOD implementations go here
#else

AudioSystem::AudioSystem() {}
AudioSystem::~AudioSystem() { shutdown(); }

void AudioSystem::init(const AudioSettings& settings) {
    this->settings = settings;
    VF_INFO("AudioSystem initialized (stub - FMOD not available)");
}

void AudioSystem::shutdown() {}
void AudioSystem::update() {}
void AudioSystem::update3DListener(const glm::vec3&, const glm::vec3&, const glm::vec3&, const glm::vec3&) {}
bool AudioSystem::loadSound(const SoundInfo&) { return false; }
bool AudioSystem::loadSound(const std::string&, const std::string&, bool, bool) { return false; }
void AudioSystem::unloadSound(const std::string&) {}
void AudioSystem::unloadAllSounds() {}
bool AudioSystem::isSoundLoaded(const std::string&) const { return false; }
uint32_t AudioSystem::play(const std::string&, SoundCategory) { return 0; }
uint32_t AudioSystem::play3D(const std::string&, const glm::vec3&, SoundCategory) { return 0; }
uint32_t AudioSystem::playMusic(const std::string&) { return 0; }
uint32_t AudioSystem::playAmbient(const std::string&) { return 0; }
void AudioSystem::stop(uint32_t) {}
void AudioSystem::stopAll() {}
void AudioSystem::stopCategory(SoundCategory) {}
void AudioSystem::pause(uint32_t) {}
void AudioSystem::resume(uint32_t) {}
void AudioSystem::pauseAll() {}
void AudioSystem::resumeAll() {}
void AudioSystem::setVolume(uint32_t, float) {}
void AudioSystem::setPitch(uint32_t, float) {}
void AudioSystem::set3DAttributes(uint32_t, const Sound3DAttributes&) {}
void AudioSystem::setLooping(uint32_t, bool) {}
bool AudioSystem::isPlaying(uint32_t) const { return false; }
bool AudioSystem::isPaused(uint32_t) const { return false; }
void AudioSystem::setMasterVolume(float v) { settings.masterVolume = v; }
void AudioSystem::setMusicVolume(float v) { settings.musicVolume = v; }
void AudioSystem::setSFXVolume(float v) { settings.sfxVolume = v; }
void AudioSystem::setAmbientVolume(float v) { settings.ambientVolume = v; }
void AudioSystem::setVoiceVolume(float v) { settings.voiceVolume = v; }
void AudioSystem::setUIVolume(float) {}
void AudioSystem::setReverbEnabled(bool) {}
void AudioSystem::setReverbPreset(const ReverbPreset&) {}
void AudioSystem::setReverbZone(const glm::vec3&, float, const ReverbPreset&) {}
void AudioSystem::clearReverbZones() {}
void AudioSystem::enableLowPassFilter(bool, float) {}
void AudioSystem::enableHighPassFilter(bool, float) {}
void AudioSystem::enableEcho(bool, float, float) {}
void AudioSystem::enableDistortion(bool, float) {}
void AudioSystem::setUnderwaterEffect(bool) {}
SoundInstance* AudioSystem::getInstance(uint32_t) { return nullptr; }
const SoundInstance* AudioSystem::getInstance(uint32_t) const { return nullptr; }
std::vector<uint32_t> AudioSystem::getActiveInstances() const { return {}; }
float AudioSystem::getSoundDuration(const std::string&) const { return 0.0f; }
float AudioSystem::getInstancePosition(uint32_t) const { return 0.0f; }
uint32_t AudioSystem::generateInstanceId() { return nextInstanceId++; }
void AudioSystem::cleanupFinishedInstances() {}

SoundManager::SoundManager() = default;
SoundManager::~SoundManager() { shutdown(); }
void SoundManager::init(AudioSystem* audio) { this->audio = audio; }
void SoundManager::shutdown() { audio = nullptr; }
void SoundManager::loadBlockSounds() {}
void SoundManager::loadEntitySounds() {}
void SoundManager::loadAmbientSounds() {}
void SoundManager::loadMusic() {}
void SoundManager::loadUISounds() {}
void SoundManager::loadAllSounds() {}
uint32_t SoundManager::playBlockSound(const std::string&, const std::string&, const glm::vec3&) { return 0; }
uint32_t SoundManager::playEntitySound(const std::string&, const std::string&, const glm::vec3&) { return 0; }
uint32_t SoundManager::playRandomBlockSound(const std::string&, const std::string&, const glm::vec3&) { return 0; }
uint32_t SoundManager::playRandomEntitySound(const std::string&, const std::string&, const glm::vec3&) { return 0; }
void SoundManager::playBiomeAmbient(const std::string&) {}
void SoundManager::playWeatherAmbient(const std::string&) {}
void SoundManager::stopAmbient() {}
void SoundManager::playRandomMusic() {}
void SoundManager::playMenuMusic() {}
void SoundManager::stopMusic() {}
void SoundManager::update(float) {}
void SoundManager::setSoundPack(const std::string& pack) { currentSoundPack = pack; }
std::string SoundManager::getRandomVariant(const std::string& baseName, int variants) {
    return baseName + "." + std::to_string(rand() % variants + 1);
}

#endif

} // namespace VoxelForge
