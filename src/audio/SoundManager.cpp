/**
 * @file SoundManager.cpp
 * @brief Game-specific sound management implementation
 */

#include <VoxelForge/audio/AudioSystem.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

// ============================================================================
// SoundManager Implementation
// ============================================================================

SoundManager::SoundManager() {
    LOG_INFO("SoundManager created");
}

SoundManager::~SoundManager() {
    shutdown();
}

void SoundManager::init(AudioSystem* audioSystem) {
    audio = audioSystem;
    if (!audio) {
        LOG_ERROR("SoundManager: Invalid audio system");
        return;
    }
    
    loadAllSounds();
    LOG_INFO("SoundManager initialized");
}

void SoundManager::shutdown() {
    stopMusic();
    stopAmbient();
    
    blockSounds.clear();
    entitySounds.clear();
    musicTracks.clear();
    
    audio = nullptr;
    LOG_INFO("SoundManager shutdown");
}

void SoundManager::loadBlockSounds() {
    if (!audio) return;
    
    // Stone sounds
    audio->loadSound("block.stone.break", "sounds/block/stone/break.ogg", false, true);
    audio->loadSound("block.stone.place", "sounds/block/stone/place.ogg", false, true);
    audio->loadSound("block.stone.step", "sounds/block/stone/step.ogg", false, true);
    audio->loadSound("block.stone.hit", "sounds/block/stone/hit.ogg", false, true);
    blockSounds["minecraft:stone"] = {"block.stone.break", "block.stone.place", "block.stone.step"};
    
    // Dirt sounds
    audio->loadSound("block.dirt.break", "sounds/block/dirt/break.ogg", false, true);
    audio->loadSound("block.dirt.place", "sounds/block/dirt/place.ogg", false, true);
    audio->loadSound("block.dirt.step", "sounds/block/dirt/step.ogg", false, true);
    blockSounds["minecraft:dirt"] = {"block.dirt.break", "block.dirt.place", "block.dirt.step"};
    
    // Grass sounds
    audio->loadSound("block.grass.break", "sounds/block/grass/break.ogg", false, true);
    audio->loadSound("block.grass.place", "sounds/block/grass/place.ogg", false, true);
    audio->loadSound("block.grass.step", "sounds/block/grass/step.ogg", false, true);
    blockSounds["minecraft:grass_block"] = {"block.grass.break", "block.grass.place", "block.grass.step"};
    
    // Wood sounds
    audio->loadSound("block.wood.break", "sounds/block/wood/break.ogg", false, true);
    audio->loadSound("block.wood.place", "sounds/block/wood/place.ogg", false, true);
    audio->loadSound("block.wood.step", "sounds/block/wood/step.ogg", false, true);
    blockSounds["minecraft:oak_planks"] = {"block.wood.break", "block.wood.place", "block.wood.step"};
    
    // Sand sounds
    audio->loadSound("block.sand.break", "sounds/block/sand/break.ogg", false, true);
    audio->loadSound("block.sand.place", "sounds/block/sand/place.ogg", false, true);
    audio->loadSound("block.sand.step", "sounds/block/sand/step.ogg", false, true);
    blockSounds["minecraft:sand"] = {"block.sand.break", "block.sand.place", "block.sand.step"};
    
    // Glass sounds
    audio->loadSound("block.glass.break", "sounds/block/glass/break.ogg", false, true);
    audio->loadSound("block.glass.place", "sounds/block/glass/place.ogg", false, true);
    blockSounds["minecraft:glass"] = {"block.glass.break", "block.glass.place"};
    
    // Metal sounds
    audio->loadSound("block.metal.break", "sounds/block/metal/break.ogg", false, true);
    audio->loadSound("block.metal.place", "sounds/block/metal/place.ogg", false, true);
    audio->loadSound("block.metal.step", "sounds/block/metal/step.ogg", false, true);
    blockSounds["minecraft:iron_block"] = {"block.metal.break", "block.metal.place", "block.metal.step"};
    
    LOG_INFO("Loaded block sounds");
}

void SoundManager::loadEntitySounds() {
    if (!audio) return;
    
    // Player sounds
    audio->loadSound("entity.player.hurt", "sounds/entity/player/hurt.ogg", false, true);
    audio->loadSound("entity.player.death", "sounds/entity/player/death.ogg", false, true);
    audio->loadSound("entity.player.burp", "sounds/entity/player/burp.ogg", false, true);
    audio->loadSound("entity.player.breath", "sounds/entity/player/breath.ogg", false, true);
    entitySounds["minecraft:player"] = {"entity.player.hurt", "entity.player.death"};
    
    // Zombie sounds
    audio->loadSound("entity.zombie.ambient", "sounds/entity/zombie/ambient.ogg", false, true);
    audio->loadSound("entity.zombie.hurt", "sounds/entity/zombie/hurt.ogg", false, true);
    audio->loadSound("entity.zombie.death", "sounds/entity/zombie/death.ogg", false, true);
    entitySounds["minecraft:zombie"] = {"entity.zombie.ambient", "entity.zombie.hurt", "entity.zombie.death"};
    
    // Skeleton sounds
    audio->loadSound("entity.skeleton.ambient", "sounds/entity/skeleton/ambient.ogg", false, true);
    audio->loadSound("entity.skeleton.hurt", "sounds/entity/skeleton/hurt.ogg", false, true);
    audio->loadSound("entity.skeleton.death", "sounds/entity/skeleton/death.ogg", false, true);
    entitySounds["minecraft:skeleton"] = {"entity.skeleton.ambient", "entity.skeleton.hurt", "entity.skeleton.death"};
    
    // Creeper sounds
    audio->loadSound("entity.creeper.primed", "sounds/entity/creeper/primed.ogg", false, true);
    audio->loadSound("entity.creeper.hurt", "sounds/entity/creeper/hurt.ogg", false, true);
    audio->loadSound("entity.creeper.death", "sounds/entity/creeper/death.ogg", false, true);
    entitySounds["minecraft:creeper"] = {"entity.creeper.primed", "entity.creeper.hurt", "entity.creeper.death"};
    
    // Cow sounds
    audio->loadSound("entity.cow.ambient", "sounds/entity/cow/ambient.ogg", false, true);
    audio->loadSound("entity.cow.hurt", "sounds/entity/cow/hurt.ogg", false, true);
    audio->loadSound("entity.cow.death", "sounds/entity/cow/death.ogg", false, true);
    entitySounds["minecraft:cow"] = {"entity.cow.ambient", "entity.cow.hurt", "entity.cow.death"};
    
    // Pig sounds
    audio->loadSound("entity.pig.ambient", "sounds/entity/pig/ambient.ogg", false, true);
    audio->loadSound("entity.pig.hurt", "sounds/entity/pig/hurt.ogg", false, true);
    audio->loadSound("entity.pig.death", "sounds/entity/pig/death.ogg", false, true);
    entitySounds["minecraft:pig"] = {"entity.pig.ambient", "entity.pig.hurt", "entity.pig.death"};
    
    // Chicken sounds
    audio->loadSound("entity.chicken.ambient", "sounds/entity/chicken/ambient.ogg", false, true);
    audio->loadSound("entity.chicken.hurt", "sounds/entity/chicken/hurt.ogg", false, true);
    audio->loadSound("entity.chicken.death", "sounds/entity/chicken/death.ogg", false, true);
    entitySounds["minecraft:chicken"] = {"entity.chicken.ambient", "entity.chicken.hurt", "entity.chicken.death"};
    
    LOG_INFO("Loaded entity sounds");
}

void SoundManager::loadAmbientSounds() {
    if (!audio) return;
    
    // Biome ambient
    audio->loadSound("ambient.plains", "sounds/ambient/plains.ogg", true, true);
    audio->loadSound("ambient.forest", "sounds/ambient/forest.ogg", true, true);
    audio->loadSound("ambient.desert", "sounds/ambient/desert.ogg", true, true);
    audio->loadSound("ambient.ocean", "sounds/ambient/ocean.ogg", true, true);
    audio->loadSound("ambient.cave", "sounds/ambient/cave.ogg", true, true);
    audio->loadSound("ambient.nether", "sounds/ambient/nether.ogg", true, true);
    audio->loadSound("ambient.end", "sounds/ambient/end.ogg", true, true);
    
    // Weather
    audio->loadSound("weather.rain", "sounds/weather/rain.ogg", true, true);
    audio->loadSound("weather.thunder", "sounds/weather/thunder.ogg", false, true);
    
    LOG_INFO("Loaded ambient sounds");
}

void SoundManager::loadMusic() {
    if (!audio) return;
    
    musicTracks = {
        "music.c418.calm1",
        "music.c418.calm2",
        "music.c418.calm3",
        "music.c418.hal1",
        "music.c418.hal2",
        "music.c418.hal3",
        "music.c418.hal4",
        "music.menu"
    };
    
    // Load as streams for memory efficiency
    for (const auto& track : musicTracks) {
        std::string path = "sounds/" + track + ".ogg";
        audio->loadSound(track, path, true, false);
    }
    
    LOG_INFO("Loaded {} music tracks", musicTracks.size());
}

void SoundManager::loadUISounds() {
    if (!audio) return;
    
    audio->loadSound("ui.button.click", "sounds/ui/button/click.ogg", false, false);
    audio->loadSound("ui.button.hover", "sounds/ui/button/hover.ogg", false, false);
    audio->loadSound("ui.inventory.open", "sounds/ui/inventory/open.ogg", false, false);
    audio->loadSound("ui.inventory.close", "sounds/ui/inventory/close.ogg", false, false);
    audio->loadSound("ui.chat.message", "sounds/ui/chat/message.ogg", false, false);
    audio->loadSound("ui.levelup", "sounds/ui/levelup.ogg", false, false);
    
    LOG_INFO("Loaded UI sounds");
}

void SoundManager::loadAllSounds() {
    loadBlockSounds();
    loadEntitySounds();
    loadAmbientSounds();
    loadMusic();
    loadUISounds();
    
    LOG_INFO("All sounds loaded");
}

uint32_t SoundManager::playBlockSound(const std::string& blockName, const std::string& action,
                                       const glm::vec3& position) {
    if (!audio) return 0;
    
    auto it = blockSounds.find(blockName);
    if (it == blockSounds.end() || it->second.empty()) {
        // Use generic sound
        return audio->play3D("block.stone." + action, position);
    }
    
    // Find appropriate sound based on action
    std::string soundName;
    for (const auto& sound : it->second) {
        if (sound.find(action) != std::string::npos) {
            soundName = sound;
            break;
        }
    }
    
    if (soundName.empty()) {
        soundName = it->second[0];
    }
    
    return audio->play3D(soundName, position);
}

uint32_t SoundManager::playEntitySound(const std::string& entityName, const std::string& action,
                                         const glm::vec3& position) {
    if (!audio) return 0;
    
    auto it = entitySounds.find(entityName);
    if (it == entitySounds.end() || it->second.empty()) {
        return 0;
    }
    
    std::string soundName;
    for (const auto& sound : it->second) {
        if (sound.find(action) != std::string::npos) {
            soundName = sound;
            break;
        }
    }
    
    if (soundName.empty()) {
        soundName = it->second[0];
    }
    
    return audio->play3D(soundName, position);
}

uint32_t SoundManager::playRandomBlockSound(const std::string& blockName, const std::string& action,
                                             const glm::vec3& position) {
    // Play with random variant (1, 2, 3, etc.)
    int variant = (rand() % 3) + 1;
    std::string variantName = blockName + "." + action + "." + std::to_string(variant);
    
    if (audio && audio->isSoundLoaded(variantName)) {
        return audio->play3D(variantName, position);
    }
    
    return playBlockSound(blockName, action, position);
}

uint32_t SoundManager::playRandomEntitySound(const std::string& entityName, const std::string& action,
                                              const glm::vec3& position) {
    int variant = (rand() % 3) + 1;
    std::string variantName = entityName + "." + action + "." + std::to_string(variant);
    
    if (audio && audio->isSoundLoaded(variantName)) {
        return audio->play3D(variantName, position);
    }
    
    return playEntitySound(entityName, action, position);
}

void SoundManager::playBiomeAmbient(const std::string& biomeName) {
    if (!audio) return;
    
    stopAmbient();
    
    std::string soundName = "ambient." + biomeName;
    if (audio->isSoundLoaded(soundName)) {
        currentAmbientId = audio->playAmbient(soundName);
    }
}

void SoundManager::playWeatherAmbient(const std::string& weather) {
    if (!audio) return;
    
    std::string soundName = "weather." + weather;
    if (audio->isSoundLoaded(soundName)) {
        audio->playAmbient(soundName);
    }
}

void SoundManager::stopAmbient() {
    if (audio && currentAmbientId != 0) {
        audio->stop(currentAmbientId);
        currentAmbientId = 0;
    }
}

void SoundManager::playRandomMusic() {
    if (!audio || musicTracks.empty()) return;
    
    stopMusic();
    
    int index = rand() % musicTracks.size();
    currentMusicId = audio->playMusic(musicTracks[index]);
}

void SoundManager::playMenuMusic() {
    if (!audio) return;
    
    stopMusic();
    
    if (audio->isSoundLoaded("music.menu")) {
        currentMusicId = audio->playMusic("music.menu");
    }
}

void SoundManager::stopMusic() {
    if (audio && currentMusicId != 0) {
        audio->stop(currentMusicId);
        currentMusicId = 0;
    }
}

void SoundManager::update(float deltaTime) {
    if (!audio) return;
    
    // Check if music has ended, play next track
    if (currentMusicId != 0 && !audio->isPlaying(currentMusicId)) {
        playRandomMusic();
    }
}

void SoundManager::setSoundPack(const std::string& packName) {
    currentSoundPack = packName;
    // TODO: Reload all sounds from new pack
    LOG_INFO("Switched to sound pack: {}", packName);
}

std::string SoundManager::getRandomVariant(const std::string& baseName, int variants) {
    return baseName + "." + std::to_string((rand() % variants) + 1);
}

} // namespace VoxelForge
