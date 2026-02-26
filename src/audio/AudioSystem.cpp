/**
 * @file AudioSystem.cpp
 * @brief FMOD-based audio system implementation
 */

#include "AudioSystem.hpp"
#include <VoxelForge/core/Logger.hpp>

// FMOD includes
#include <fmod.hpp>
#include <fmod_errors.h>

namespace VoxelForge {

// Helper to check FMOD results
#define FMOD_CHECK(result) \
    if (result != FMOD_OK) { \
        Logger::error("FMOD Error: {} ({})", FMOD_ErrorString(result), result); \
        return false; \
    }

// Reverb presets
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

// ============== AudioSystem ==============

AudioSystem::AudioSystem() = default;

AudioSystem::~AudioSystem() {
    shutdown();
}

void AudioSystem::init(const AudioSettings& settings) {
    this->settings = settings;
    
    // Create FMOD system
    FMOD_RESULT result = FMOD::System_Create(&system);
    FMOD_CHECK(result);
    
    // Initialize
    result = system->init(settings.maxChannels, FMOD_INIT_3D_RIGHTHANDED, nullptr);
    FMOD_CHECK(result);
    
    // Create channel groups
    result = system->getMasterChannelGroup(&masterGroup);
    FMOD_CHECK(result);
    
    result = system->createChannelGroup("music", &musicGroup);
    FMOD_CHECK(result);
    masterGroup->addGroup(musicGroup);
    
    result = system->createChannelGroup("sfx", &sfxGroup);
    FMOD_CHECK(result);
    masterGroup->addGroup(sfxGroup);
    
    result = system->createChannelGroup("ambient", &ambientGroup);
    FMOD_CHECK(result);
    masterGroup->addGroup(ambientGroup);
    
    result = system->createChannelGroup("voice", &voiceGroup);
    FMOD_CHECK(result);
    masterGroup->addGroup(voiceGroup);
    
    result = system->createChannelGroup("ui", &uiGroup);
    FMOD_CHECK(result);
    masterGroup->addGroup(uiGroup);
    
    // Set 3D settings
    result = system->set3DSettings(settings.dopplerScale, 
                                   settings.distanceFactor,
                                   settings.rolloffScale);
    FMOD_CHECK(result);
    
    // Create DSP effects
    system->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &lowPassDSP);
    system->createDSPByType(FMOD_DSP_TYPE_HIGHPASS, &highPassDSP);
    system->createDSPByType(FMOD_DSP_TYPE_ECHO, &echoDSP);
    system->createDSPByType(FMOD_DSP_TYPE_DISTORTION, &distortionDSP);
    
    // Set initial volumes
    setMasterVolume(settings.masterVolume);
    setMusicVolume(settings.musicVolume);
    setSFXVolume(settings.sfxVolume);
    setAmbientVolume(settings.ambientVolume);
    setVoiceVolume(settings.voiceVolume);
    
    Logger::info("AudioSystem initialized with {} channels", settings.maxChannels);
}

void AudioSystem::shutdown() {
    if (!system) return;
    
    // Stop all sounds
    stopAll();
    
    // Unload all sounds
    unloadAllSounds();
    
    // Release DSP effects
    if (lowPassDSP) { lowPassDSP->release(); lowPassDSP = nullptr; }
    if (highPassDSP) { highPassDSP->release(); highPassDSP = nullptr; }
    if (echoDSP) { echoDSP->release(); echoDSP = nullptr; }
    if (distortionDSP) { distortionDSP->release(); distortionDSP = nullptr; }
    
    // Clear reverb zones
    clearReverbZones();
    
    // Release channel groups
    if (uiGroup) { uiGroup->release(); uiGroup = nullptr; }
    if (voiceGroup) { voiceGroup->release(); voiceGroup = nullptr; }
    if (ambientGroup) { ambientGroup->release(); ambientGroup = nullptr; }
    if (sfxGroup) { sfxGroup->release(); sfxGroup = nullptr; }
    if (musicGroup) { musicGroup->release(); musicGroup = nullptr; }
    masterGroup = nullptr;
    
    // Close and release system
    system->close();
    system->release();
    system = nullptr;
    
    Logger::info("AudioSystem shutdown");
}

void AudioSystem::update() {
    if (!system) return;
    
    // Update listener position
    FMOD_VECTOR pos = {listenerAttributes.position.x, listenerAttributes.position.y, 
                       listenerAttributes.position.z};
    FMOD_VECTOR vel = {listenerAttributes.velocity.x, listenerAttributes.velocity.y,
                       listenerAttributes.velocity.z};
    FMOD_VECTOR forward = {listenerAttributes.forward.x, listenerAttributes.forward.y,
                           listenerAttributes.forward.z};
    FMOD_VECTOR up = {listenerAttributes.up.x, listenerAttributes.up.y, 
                      listenerAttributes.up.z};
    
    system->set3DListenerAttributes(0, &pos, &vel, &forward, &up);
    
    // Update FMOD
    system->update();
    
    // Cleanup finished instances
    cleanupFinishedInstances();
}

void AudioSystem::update3DListener(const glm::vec3& position, const glm::vec3& velocity,
                                    const glm::vec3& forward, const glm::vec3& up) {
    listenerAttributes.position = position;
    listenerAttributes.velocity = velocity;
    listenerAttributes.forward = forward;
    listenerAttributes.up = up;
}

bool AudioSystem::loadSound(const SoundInfo& info) {
    if (!system) return false;
    
    FMOD_MODE mode = FMOD_DEFAULT;
    
    if (info.stream) {
        mode |= FMOD_CREATESTREAM;
    }
    if (info.loop) {
        mode |= FMOD_LOOP_NORMAL;
    }
    if (info.is3D) {
        mode |= FMOD_3D;
        mode |= FMOD_3D_INVERSEROLLOFF;
    }
    
    FMOD::Sound* sound = nullptr;
    FMOD_RESULT result = system->createSound(info.path.c_str(), mode, nullptr, &sound);
    
    if (result != FMOD_OK) {
        Logger::error("Failed to load sound: {} ({})", info.path, FMOD_ErrorString(result));
        return false;
    }
    
    if (info.is3D) {
        sound->set3DMinMaxDistance(info.minDistance, info.maxDistance);
    }
    
    sound->setDefaults(0.0f, info.volume);
    
    sounds[info.name] = sound;
    Logger::debug("Loaded sound: {}", info.name);
    return true;
}

bool AudioSystem::loadSound(const std::string& name, const std::string& path,
                             bool stream, bool is3D) {
    SoundInfo info;
    info.name = name;
    info.path = path;
    info.stream = stream;
    info.is3D = is3D;
    return loadSound(info);
}

void AudioSystem::unloadSound(const std::string& name) {
    auto it = sounds.find(name);
    if (it != sounds.end()) {
        it->second->release();
        sounds.erase(it);
    }
}

void AudioSystem::unloadAllSounds() {
    for (auto& [name, sound] : sounds) {
        sound->release();
    }
    sounds.clear();
}

bool AudioSystem::isSoundLoaded(const std::string& name) const {
    return sounds.find(name) != sounds.end();
}

uint32_t AudioSystem::play(const std::string& name, SoundCategory category) {
    auto soundIt = sounds.find(name);
    if (soundIt == sounds.end()) {
        Logger::warn("Sound not found: {}", name);
        return 0;
    }
    
    FMOD::ChannelGroup* group = sfxGroup;
    switch (category) {
        case SoundCategory::Music: group = musicGroup; break;
        case SoundCategory::Ambient: group = ambientGroup; break;
        case SoundCategory::Voice: group = voiceGroup; break;
        case SoundCategory::UI: group = uiGroup; break;
        default: break;
    }
    
    FMOD::Channel* channel = nullptr;
    FMOD_RESULT result = system->playSound(soundIt->second, group, false, &channel);
    
    if (result != FMOD_OK) {
        Logger::error("Failed to play sound: {}", name);
        return 0;
    }
    
    uint32_t instanceId = generateInstanceId();
    
    auto instance = std::make_unique<SoundInstance>();
    instance->id = instanceId;
    instance->name = name;
    instance->channel = channel;
    instance->category = category;
    instance->isPlaying = true;
    instance->is3D = false;
    
    instances[instanceId] = std::move(instance);
    
    return instanceId;
}

uint32_t AudioSystem::play3D(const std::string& name, const glm::vec3& position,
                              SoundCategory category) {
    uint32_t instanceId = play(name, category);
    
    if (instanceId != 0) {
        auto* instance = getInstance(instanceId);
        if (instance && instance->channel) {
            instance->is3D = true;
            instance->attributes.position = position;
            
            FMOD_VECTOR pos = {position.x, position.y, position.z};
            FMOD_VECTOR vel = {0.0f, 0.0f, 0.0f};
            instance->channel->set3DAttributes(&pos, &vel);
        }
    }
    
    return instanceId;
}

uint32_t AudioSystem::playMusic(const std::string& name) {
    return play(name, SoundCategory::Music);
}

uint32_t AudioSystem::playAmbient(const std::string& name) {
    return play(name, SoundCategory::Ambient);
}

void AudioSystem::stop(uint32_t instanceId) {
    auto it = instances.find(instanceId);
    if (it != instances.end() && it->second->channel) {
        it->second->channel->stop();
        instances.erase(it);
    }
}

void AudioSystem::stopAll() {
    for (auto& [id, instance] : instances) {
        if (instance->channel) {
            instance->channel->stop();
        }
    }
    instances.clear();
}

void AudioSystem::stopCategory(SoundCategory category) {
    std::vector<uint32_t> toRemove;
    
    for (auto& [id, instance] : instances) {
        if (instance->category == category) {
            if (instance->channel) {
                instance->channel->stop();
            }
            toRemove.push_back(id);
        }
    }
    
    for (uint32_t id : toRemove) {
        instances.erase(id);
    }
}

void AudioSystem::pause(uint32_t instanceId) {
    auto* instance = getInstance(instanceId);
    if (instance && instance->channel) {
        instance->channel->setPaused(true);
        instance->isPaused = true;
    }
}

void AudioSystem::resume(uint32_t instanceId) {
    auto* instance = getInstance(instanceId);
    if (instance && instance->channel) {
        instance->channel->setPaused(false);
        instance->isPaused = false;
    }
}

void AudioSystem::pauseAll() {
    if (masterGroup) {
        masterGroup->setPaused(true);
    }
}

void AudioSystem::resumeAll() {
    if (masterGroup) {
        masterGroup->setPaused(false);
    }
}

void AudioSystem::setVolume(uint32_t instanceId, float volume) {
    auto* instance = getInstance(instanceId);
    if (instance && instance->channel) {
        instance->channel->setVolume(volume);
        instance->volume = volume;
    }
}

void AudioSystem::setPitch(uint32_t instanceId, float pitch) {
    auto* instance = getInstance(instanceId);
    if (instance && instance->channel) {
        instance->channel->setPitch(pitch);
        instance->pitch = pitch;
    }
}

void AudioSystem::set3DAttributes(uint32_t instanceId, const Sound3DAttributes& attributes) {
    auto* instance = getInstance(instanceId);
    if (instance && instance->channel && instance->is3D) {
        instance->attributes = attributes;
        
        FMOD_VECTOR pos = {attributes.position.x, attributes.position.y, attributes.position.z};
        FMOD_VECTOR vel = {attributes.velocity.x, attributes.velocity.y, attributes.velocity.z};
        instance->channel->set3DAttributes(&pos, &vel);
    }
}

void AudioSystem::setLooping(uint32_t instanceId, bool loop) {
    auto* instance = getInstance(instanceId);
    if (instance && instance->channel) {
        instance->channel->setMode(loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
    }
}

bool AudioSystem::isPlaying(uint32_t instanceId) const {
    auto it = instances.find(instanceId);
    if (it != instances.end() && it->second->channel) {
        bool playing = false;
        it->second->channel->isPlaying(&playing);
        return playing;
    }
    return false;
}

bool AudioSystem::isPaused(uint32_t instanceId) const {
    auto it = instances.find(instanceId);
    if (it != instances.end()) {
        return it->second->isPaused;
    }
    return false;
}

void AudioSystem::setMasterVolume(float volume) {
    settings.masterVolume = volume;
    if (masterGroup) {
        masterGroup->setVolume(volume);
    }
}

void AudioSystem::setMusicVolume(float volume) {
    settings.musicVolume = volume;
    if (musicGroup) {
        musicGroup->setVolume(volume);
    }
}

void AudioSystem::setSFXVolume(float volume) {
    settings.sfxVolume = volume;
    if (sfxGroup) {
        sfxGroup->setVolume(volume);
    }
}

void AudioSystem::setAmbientVolume(float volume) {
    settings.ambientVolume = volume;
    if (ambientGroup) {
        ambientGroup->setVolume(volume);
    }
}

void AudioSystem::setVoiceVolume(float volume) {
    settings.voiceVolume = volume;
    if (voiceGroup) {
        voiceGroup->setVolume(volume);
    }
}

void AudioSystem::setUIVolume(float volume) {
    if (uiGroup) {
        uiGroup->setVolume(volume);
    }
}

void AudioSystem::setReverbEnabled(bool enabled) {
    settings.enableReverb = enabled;
}

void AudioSystem::setReverbPreset(const ReverbPreset& preset) {
    // Apply reverb settings to master channel
}

void AudioSystem::setReverbZone(const glm::vec3& position, float radius, 
                                 const ReverbPreset& preset) {
    // Create 3D reverb zone
}

void AudioSystem::clearReverbZones() {
    for (auto* reverb : reverbZones) {
        if (reverb) {
            reverb->release();
        }
    }
    reverbZones.clear();
}

void AudioSystem::enableLowPassFilter(bool enabled, float cutoff) {
    if (enabled && lowPassDSP) {
        lowPassDSP->setParameter(FMOD_DSP_LOWPASS_CUTOFF, cutoff);
        masterGroup->addDSP(0, lowPassDSP);
    } else if (lowPassDSP) {
        masterGroup->removeDSP(lowPassDSP);
    }
}

void AudioSystem::enableHighPassFilter(bool enabled, float cutoff) {
    if (enabled && highPassDSP) {
        highPassDSP->setParameter(FMOD_DSP_HIGHPASS_CUTOFF, cutoff);
        masterGroup->addDSP(0, highPassDSP);
    } else if (highPassDSP) {
        masterGroup->removeDSP(highPassDSP);
    }
}

void AudioSystem::enableEcho(bool enabled, float delay, float decay) {
    if (enabled && echoDSP) {
        echoDSP->setParameter(FMOD_DSP_ECHO_DELAY, delay);
        echoDSP->setParameter(FMOD_DSP_ECHO_DECAYRATIO, decay);
        masterGroup->addDSP(0, echoDSP);
    } else if (echoDSP) {
        masterGroup->removeDSP(echoDSP);
    }
}

void AudioSystem::enableDistortion(bool enabled, float level) {
    if (enabled && distortionDSP) {
        distortionDSP->setParameter(FMOD_DSP_DISTORTION_LEVEL, level);
        masterGroup->addDSP(0, distortionDSP);
    } else if (distortionDSP) {
        masterGroup->removeDSP(distortionDSP);
    }
}

void AudioSystem::setUnderwaterEffect(bool enabled) {
    if (enabled) {
        enableLowPassFilter(true, 1000.0f);
        setReverbPreset(ReverbPreset::Underwater());
    } else {
        enableLowPassFilter(false);
        setReverbPreset(ReverbPreset::Outdoor());
    }
}

SoundInstance* AudioSystem::getInstance(uint32_t instanceId) {
    auto it = instances.find(instanceId);
    if (it != instances.end()) {
        return it->second.get();
    }
    return nullptr;
}

const SoundInstance* AudioSystem::getInstance(uint32_t instanceId) const {
    auto it = instances.find(instanceId);
    if (it != instances.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<uint32_t> AudioSystem::getActiveInstances() const {
    std::vector<uint32_t> result;
    for (const auto& [id, instance] : instances) {
        result.push_back(id);
    }
    return result;
}

float AudioSystem::getSoundDuration(const std::string& name) const {
    auto it = sounds.find(name);
    if (it != sounds.end()) {
        unsigned int length = 0;
        it->second->getLength(&length, FMOD_TIMEUNIT_MS);
        return static_cast<float>(length) / 1000.0f;
    }
    return 0.0f;
}

float AudioSystem::getInstancePosition(uint32_t instanceId) const {
    auto* instance = getInstance(instanceId);
    if (instance && instance->channel) {
        unsigned int position = 0;
        instance->channel->getPosition(&position, FMOD_TIMEUNIT_MS);
        return static_cast<float>(position) / 1000.0f;
    }
    return 0.0f;
}

FMOD::VECTOR AudioSystem::toFMODVector(const glm::vec3& v) const {
    return {v.x, v.y, v.z};
}

uint32_t AudioSystem::generateInstanceId() {
    return nextInstanceId++;
}

void AudioSystem::cleanupFinishedInstances() {
    std::vector<uint32_t> toRemove;
    
    for (auto& [id, instance] : instances) {
        if (instance->channel) {
            bool playing = false;
            instance->channel->isPlaying(&playing);
            if (!playing) {
                toRemove.push_back(id);
            }
        }
    }
    
    for (uint32_t id : toRemove) {
        instances.erase(id);
    }
}

// ============== SoundManager ==============

SoundManager::SoundManager() = default;

SoundManager::~SoundManager() {
    shutdown();
}

void SoundManager::init(AudioSystem* audio) {
    this->audio = audio;
}

void SoundManager::shutdown() {
    audio = nullptr;
}

void SoundManager::loadBlockSounds() {
    // Load standard block sounds
    // dig.stone, dig.dirt, dig.grass, etc.
}

void SoundManager::loadEntitySounds() {
    // Load entity sounds
    // mob.pig.say, mob.cow.say, etc.
}

void SoundManager::loadAmbientSounds() {
    // Load ambient sounds
    // ambient.cave, ambient.weather.rain, etc.
}

void SoundManager::loadMusic() {
    // Load music tracks
    // menu music, game music, creative music, etc.
}

void SoundManager::loadUISounds() {
    // Load UI sounds
    // click, pop, etc.
}

void SoundManager::loadAllSounds() {
    loadBlockSounds();
    loadEntitySounds();
    loadAmbientSounds();
    loadMusic();
    loadUISounds();
}

uint32_t SoundManager::playBlockSound(const std::string& blockName, const std::string& action,
                                       const glm::vec3& position) {
    std::string soundName = "block." + blockName + "." + action;
    
    auto it = blockSounds.find(soundName);
    if (it != blockSounds.end() && !it->second.empty()) {
        return audio->play3D(it->second[0], position);
    }
    
    return 0;
}

uint32_t SoundManager::playEntitySound(const std::string& entityName, const std::string& action,
                                        const glm::vec3& position) {
    std::string soundName = "entity." + entityName + "." + action;
    
    auto it = entitySounds.find(soundName);
    if (it != entitySounds.end() && !it->second.empty()) {
        return audio->play3D(it->second[0], position);
    }
    
    return 0;
}

uint32_t SoundManager::playRandomBlockSound(const std::string& blockName, const std::string& action,
                                             const glm::vec3& position) {
    std::string soundName = "block." + blockName + "." + action;
    
    auto it = blockSounds.find(soundName);
    if (it != blockSounds.end() && !it->second.empty()) {
        int index = rand() % it->second.size();
        return audio->play3D(it->second[index], position);
    }
    
    return 0;
}

uint32_t SoundManager::playRandomEntitySound(const std::string& entityName, const std::string& action,
                                              const glm::vec3& position) {
    std::string soundName = "entity." + entityName + "." + action;
    
    auto it = entitySounds.find(soundName);
    if (it != entitySounds.end() && !it->second.empty()) {
        int index = rand() % it->second.size();
        return audio->play3D(it->second[index], position);
    }
    
    return 0;
}

void SoundManager::playBiomeAmbient(const std::string& biomeName) {
    stopAmbient();
    std::string soundName = "ambient." + biomeName;
    currentAmbientId = audio->playAmbient(soundName);
}

void SoundManager::playWeatherAmbient(const std::string& weather) {
    std::string soundName = "ambient.weather." + weather;
    audio->playAmbient(soundName);
}

void SoundManager::stopAmbient() {
    if (currentAmbientId != 0) {
        audio->stop(currentAmbientId);
        currentAmbientId = 0;
    }
}

void SoundManager::playRandomMusic() {
    if (musicTracks.empty()) return;
    
    stopMusic();
    int index = rand() % musicTracks.size();
    currentMusicId = audio->playMusic(musicTracks[index]);
}

void SoundManager::playMenuMusic() {
    stopMusic();
    currentMusicId = audio->playMusic("music.menu");
}

void SoundManager::stopMusic() {
    if (currentMusicId != 0) {
        audio->stop(currentMusicId);
        currentMusicId = 0;
    }
}

void SoundManager::update(float deltaTime) {
    // Check if ambient or music finished
    if (currentAmbientId != 0 && !audio->isPlaying(currentAmbientId)) {
        // Loop ambient
    }
    
    if (currentMusicId != 0 && !audio->isPlaying(currentMusicId)) {
        playRandomMusic();
    }
}

void SoundManager::setSoundPack(const std::string& packName) {
    currentSoundPack = packName;
    // Reload all sounds from new pack
    audio->unloadAllSounds();
    loadAllSounds();
}

std::string SoundManager::getRandomVariant(const std::string& baseName, int variants) {
    return baseName + "." + std::to_string(rand() % variants + 1);
}

} // namespace VoxelForge
