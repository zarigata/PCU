/**
 * @file AchievementSystem.cpp
 * @brief Achievement/Advancement system implementation
 */

#include <VoxelForge/game/AchievementSystem.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <chrono>

namespace VoxelForge {

// ============================================================================
// Advancement Implementation
// ============================================================================

bool Advancement::isComplete() const {
    for (const auto& criterion : criteria) {
        if (!criterion->achieved) return false;
    }
    return !criteria.empty();
}

float Advancement::getProgress() const {
    if (criteria.empty()) return 0.0f;
    return static_cast<float>(getCompletedCriteria()) / static_cast<float>(criteria.size());
}

int Advancement::getCompletedCriteria() const {
    int count = 0;
    for (const auto& criterion : criteria) {
        if (criterion->achieved) count++;
    }
    return count;
}

int Advancement::getTotalCriteria() const {
    return static_cast<int>(criteria.size());
}

void Advancement::grantCriterion(const std::string& criterionId) {
    for (auto& criterion : criteria) {
        if (criterion->id == criterionId) {
            criterion->achieved = true;
            criterion->achievedTime = std::chrono::system_clock::now().time_since_epoch().count();
        }
    }
}

void Advancement::revokeCriterion(const std::string& criterionId) {
    for (auto& criterion : criteria) {
        if (criterion->id == criterionId) {
            criterion->achieved = false;
            criterion->achievedTime = 0;
        }
    }
}

bool Advancement::hasCriterion(const std::string& criterionId) const {
    for (const auto& criterion : criteria) {
        if (criterion->id == criterionId && criterion->achieved) {
            return true;
        }
    }
    return false;
}

AdvancementState Advancement::getState() const {
    if (isComplete()) return AdvancementState::Completed;
    if (getCompletedCriteria() > 0) return AdvancementState::InProgress;
    return AdvancementState::Locked;
}

nlohmann::json Advancement::toJson() const {
    nlohmann::json j;
    j["id"] = id;
    j["parent"] = parent;
    
    if (display) {
        j["display"]["icon"] = display->icon;
        j["display"]["title"] = display->title;
        j["display"]["description"] = display->description;
        j["display"]["frame"] = display->frame;
        j["display"]["show_toast"] = display->showToast;
        j["display"]["announce_to_chat"] = display->announceToChat;
        j["display"]["hidden"] = display->hidden;
    }
    
    // Criteria
    nlohmann::json criteriaJson;
    for (const auto& criterion : criteria) {
        criteriaJson[criterion->id] = {
            {"trigger", criterion->trigger},
            {"conditions", criterion->conditions}
        };
    }
    j["criteria"] = criteriaJson;
    
    // Rewards
    j["rewards"]["experience"] = rewards.experience;
    j["rewards"]["loot"] = rewards.loot;
    j["rewards"]["recipes"] = rewards.recipes;
    j["rewards"]["function"] = rewards.function;
    
    return j;
}

std::unique_ptr<Advancement> Advancement::fromJson(const nlohmann::json& json, const std::string& id) {
    auto adv = std::make_unique<Advancement>();
    adv->id = id;
    adv->parent = json.value("parent", "");
    
    if (json.contains("display")) {
        adv->display = std::make_shared<AdvancementDisplay>();
        adv->display->icon = json["display"].value("icon", "");
        adv->display->title = json["display"].value("title", "");
        adv->display->description = json["display"].value("description", "");
        adv->display->frame = json["display"].value("frame", "task");
        adv->display->showToast = json["display"].value("show_toast", true);
        adv->display->announceToChat = json["display"].value("announce_to_chat", true);
        adv->display->hidden = json["display"].value("hidden", false);
    }
    
    if (json.contains("criteria")) {
        for (auto& [key, value] : json["criteria"].items()) {
            auto criterion = std::make_shared<Criterion>();
            criterion->id = key;
            criterion->trigger = value.value("trigger", "minecraft:impossible");
            if (value.contains("conditions")) {
                for (auto& [k, v] : value["conditions"].items()) {
                    criterion->conditions[k] = v.get<std::string>();
                }
            }
            adv->criteria.push_back(criterion);
        }
    }
    
    if (json.contains("rewards")) {
        adv->rewards.experience = json["rewards"].value("experience", 0);
        if (json["rewards"].contains("loot")) {
            adv->rewards.loot = json["rewards"]["loot"].get<std::vector<std::string>>();
        }
        if (json["rewards"].contains("recipes")) {
            adv->rewards.recipes = json["rewards"]["recipes"].get<std::vector<std::string>>();
        }
        adv->rewards.function = json["rewards"].value("function", "");
    }
    
    return adv;
}

// ============================================================================
// AchievementSystem Implementation
// ============================================================================

AchievementSystem::AchievementSystem() {
    LOG_INFO("AchievementSystem created");
}

AchievementSystem::~AchievementSystem() {
    LOG_INFO("AchievementSystem destroyed");
}

void AchievementSystem::registerAdvancement(std::unique_ptr<Advancement> advancement) {
    if (!advancement || advancement->id.empty()) {
        LOG_ERROR("Cannot register null or empty advancement");
        return;
    }
    
    std::string id = advancement->id;
    
    // Track children
    if (!advancement->parent.empty()) {
        children[advancement->parent].push_back(id);
    } else {
        rootAdvancements.push_back(id);
    }
    
    advancements[id] = std::move(advancement);
    LOG_DEBUG("Registered advancement: {}", id);
}

void AchievementSystem::unregisterAdvancement(const std::string& id) {
    auto it = advancements.find(id);
    if (it == advancements.end()) return;
    
    // Remove from parent's children
    if (!it->second->parent.empty()) {
        auto& siblings = children[it->second->parent];
        siblings.erase(std::remove(siblings.begin(), siblings.end(), id), siblings.end());
    } else {
        rootAdvancements.erase(std::remove(rootAdvancements.begin(), rootAdvancements.end(), id), rootAdvancements.end());
    }
    
    // Remove children tracking
    children.erase(id);
    
    advancements.erase(it);
}

void AchievementSystem::loadAdvancements(const std::string& directory) {
    LOG_INFO("Loading advancements from: {}", directory);
    // TODO: Load from JSON files
}

void AchievementSystem::clear() {
    advancements.clear();
    children.clear();
    rootAdvancements.clear();
    playerProgress.clear();
}

const Advancement* AchievementSystem::getAdvancement(const std::string& id) const {
    auto it = advancements.find(id);
    return it != advancements.end() ? it->second.get() : nullptr;
}

std::vector<const Advancement*> AchievementSystem::getAdvancements() const {
    std::vector<const Advancement*> result;
    for (const auto& [id, adv] : advancements) {
        result.push_back(adv.get());
    }
    return result;
}

std::vector<const Advancement*> AchievementSystem::getRootAdvancements() const {
    std::vector<const Advancement*> result;
    for (const auto& id : rootAdvancements) {
        auto it = advancements.find(id);
        if (it != advancements.end()) {
            result.push_back(it->second.get());
        }
    }
    return result;
}

std::vector<const Advancement*> AchievementSystem::getChildren(const std::string& parentId) const {
    std::vector<const Advancement*> result;
    auto it = children.find(parentId);
    if (it != children.end()) {
        for (const auto& childId : it->second) {
            auto advIt = advancements.find(childId);
            if (advIt != advancements.end()) {
                result.push_back(advIt->second.get());
            }
        }
    }
    return result;
}

std::vector<const Advancement*> AchievementSystem::getTabAdvancements(const std::string& tabId) const {
    // Find root advancement and return all descendants
    std::vector<const Advancement*> result;
    std::function<void(const std::string&)> collect;
    collect = [&](const std::string& id) {
        auto it = advancements.find(id);
        if (it != advancements.end()) {
            result.push_back(it->second.get());
            auto childIt = children.find(id);
            if (childIt != children.end()) {
                for (const auto& childId : childIt->second) {
                    collect(childId);
                }
            }
        }
    };
    collect(tabId);
    return result;
}

void AchievementSystem::loadPlayerProgress(uint32_t playerId) {
    // TODO: Load from file
    if (playerProgress.find(playerId) == playerProgress.end()) {
        playerProgress[playerId] = {};
    }
}

void AchievementSystem::savePlayerProgress(uint32_t playerId) {
    // TODO: Save to file
}

void AchievementSystem::clearPlayerProgress(uint32_t playerId) {
    playerProgress.erase(playerId);
}

PlayerAdvancementProgress* AchievementSystem::getProgress(uint32_t playerId, const std::string& advancementId) {
    auto playerIt = playerProgress.find(playerId);
    if (playerIt == playerProgress.end()) return nullptr;
    
    auto progressIt = playerIt->second.find(advancementId);
    if (progressIt == playerIt->second.end()) return nullptr;
    
    return &progressIt->second;
}

const PlayerAdvancementProgress* AchievementSystem::getProgress(uint32_t playerId, const std::string& advancementId) const {
    auto playerIt = playerProgress.find(playerId);
    if (playerIt == playerProgress.end()) return nullptr;
    
    auto progressIt = playerIt->second.find(advancementId);
    if (progressIt == playerIt->second.end()) return nullptr;
    
    return &progressIt->second;
}

bool AchievementSystem::grantAdvancement(uint32_t playerId, const std::string& id) {
    const Advancement* adv = getAdvancement(id);
    if (!adv) return false;
    
    // Create progress if not exists
    auto& progress = playerProgress[playerId][id];
    progress.advancementId = id;
    
    // Grant all criteria
    for (const auto& criterion : adv->criteria) {
        progress.criteriaProgress[criterion->id] = true;
    }
    
    progress.achieved = true;
    progress.achievedTime = std::chrono::system_clock::now().time_since_epoch().count();
    
    // Grant rewards
    grantRewards(playerId, *adv);
    
    // Notify callback
    if (onAdvancementGranted) {
        onAdvancementGranted(playerId, id);
    }
    
    return true;
}

bool AchievementSystem::grantCriterion(uint32_t playerId, const std::string& advancementId, const std::string& criterionId) {
    const Advancement* adv = getAdvancement(advancementId);
    if (!adv) return false;
    
    // Check parent is complete
    if (!adv->parent.empty()) {
        if (!isComplete(playerId, adv->parent)) {
            return false;
        }
    }
    
    auto& progress = playerProgress[playerId][advancementId];
    progress.advancementId = advancementId;
    progress.criteriaProgress[criterionId] = true;
    
    // Check if advancement is now complete
    checkAndGrant(playerId, advancementId);
    
    return true;
}

bool AchievementSystem::revokeAdvancement(uint32_t playerId, const std::string& id) {
    auto playerIt = playerProgress.find(playerId);
    if (playerIt == playerProgress.end()) return false;
    
    auto progressIt = playerIt->second.find(id);
    if (progressIt == playerIt->second.end()) return false;
    
    progressIt->second.achieved = false;
    progressIt->second.achievedTime = 0;
    progressIt->second.criteriaProgress.clear();
    
    if (onAdvancementRevoked) {
        onAdvancementRevoked(playerId, id);
    }
    
    return true;
}

bool AchievementSystem::revokeCriterion(uint32_t playerId, const std::string& advancementId, const std::string& criterionId) {
    auto playerIt = playerProgress.find(playerId);
    if (playerIt == playerProgress.end()) return false;
    
    auto progressIt = playerIt->second.find(advancementId);
    if (progressIt == playerIt->second.end()) return false;
    
    progressIt->second.criteriaProgress[criterionId] = false;
    progressIt->second.achieved = false;
    
    return true;
}

bool AchievementSystem::isComplete(uint32_t playerId, const std::string& id) const {
    auto progress = getProgress(playerId, id);
    return progress && progress->achieved;
}

float AchievementSystem::getProgress(uint32_t playerId, const std::string& id) const {
    const Advancement* adv = getAdvancement(id);
    if (!adv) return 0.0f;
    
    auto progress = getProgress(playerId, id);
    if (!progress) return 0.0f;
    
    int completed = 0;
    for (const auto& criterion : adv->criteria) {
        auto it = progress->criteriaProgress.find(criterion->id);
        if (it != progress->criteriaProgress.end() && it->second) {
            completed++;
        }
    }
    
    return adv->criteria.empty() ? 0.0f : static_cast<float>(completed) / static_cast<float>(adv->criteria.size());
}

int AchievementSystem::getEarnedExperience(uint32_t playerId) const {
    int total = 0;
    auto playerIt = playerProgress.find(playerId);
    if (playerIt != playerProgress.end()) {
        for (const auto& [id, progress] : playerIt->second) {
            if (progress.achieved) {
                total += progress.earnedExperience;
            }
        }
    }
    return total;
}

void AchievementSystem::triggerEvent(uint32_t playerId, TriggerType type, const std::unordered_map<std::string, std::string>& data) {
    auto handlerIt = triggerHandlers.find(type);
    if (handlerIt == triggerHandlers.end()) return;
    
    for (const auto& [id, adv] : advancements) {
        for (const auto& criterion : adv->criteria) {
            // Check if trigger matches
            if (handlerIt->second->test(nullptr, criterion->conditions)) {
                grantCriterion(playerId, id, criterion->id);
            }
        }
    }
}

void AchievementSystem::registerTriggerHandler(TriggerType type, std::unique_ptr<TriggerHandler> handler) {
    triggerHandlers[type] = std::move(handler);
}

void AchievementSystem::setOnAdvancementGranted(AdvancementCallback callback) {
    onAdvancementGranted = std::move(callback);
}

void AchievementSystem::setOnAdvancementRevoked(AdvancementCallback callback) {
    onAdvancementRevoked = std::move(callback);
}

void AchievementSystem::registerVanillaAdvancements() {
    LOG_INFO("Registering vanilla advancements...");
    
    // Stone Age
    auto stoneAge = std::make_unique<Advancement>();
    stoneAge->id = "minecraft:story/stone_age";
    stoneAge->display = std::make_shared<AdvancementDisplay>();
    stoneAge->display->icon = "minecraft:cobblestone";
    stoneAge->display->title = "Stone Age";
    stoneAge->display->description = "Mine stone with your new pickaxe";
    stoneAge->display->frame = "task";
    
    auto criterion = std::make_shared<Criterion>();
    criterion->id = "get_cobblestone";
    criterion->trigger = "minecraft:inventory_changed";
    stoneAge->criteria.push_back(criterion);
    
    registerAdvancement(std::move(stoneAge));
    
    // Iron Age
    auto ironAge = std::make_unique<Advancement>();
    ironAge->id = "minecraft:story/iron_age";
    ironAge->parent = "minecraft:story/stone_age";
    ironAge->display = std::make_shared<AdvancementDisplay>();
    ironAge->display->icon = "minecraft:iron_ingot";
    ironAge->display->title = "Isn't It Iron Pick";
    ironAge->display->description = "Upgrade your pickaxe";
    ironAge->display->frame = "task";
    ironAge->rewards.experience = 50;
    
    criterion = std::make_shared<Criterion>();
    criterion->id = "get_iron_ingot";
    criterion->trigger = "minecraft:inventory_changed";
    ironAge->criteria.push_back(criterion);
    
    registerAdvancement(std::move(ironAge));
    
    LOG_INFO("Registered {} advancements", advancements.size());
}

void AchievementSystem::checkAndGrant(uint32_t playerId, const std::string& advancementId) {
    const Advancement* adv = getAdvancement(advancementId);
    if (!adv) return;
    
    auto& progress = playerProgress[playerId][advancementId];
    
    // Check all criteria
    bool allComplete = true;
    for (const auto& criterion : adv->criteria) {
        auto it = progress.criteriaProgress.find(criterion->id);
        if (it == progress.criteriaProgress.end() || !it->second) {
            allComplete = false;
            break;
        }
    }
    
    if (allComplete && !progress.achieved) {
        progress.achieved = true;
        progress.achievedTime = std::chrono::system_clock::now().time_since_epoch().count();
        
        // Grant rewards
        grantRewards(playerId, *adv);
        
        // Notify
        if (onAdvancementGranted) {
            onAdvancementGranted(playerId, advancementId);
        }
    }
}

void AchievementSystem::grantRewards(uint32_t playerId, const Advancement& advancement) {
    if (advancement.rewards.experience > 0) {
        // TODO: Grant experience to player
        LOG_DEBUG("Granting {} XP to player {}", advancement.rewards.experience, playerId);
    }
    
    if (!advancement.rewards.recipes.empty()) {
        // TODO: Unlock recipes
    }
    
    if (!advancement.rewards.function.empty()) {
        // TODO: Execute function
    }
}

} // namespace VoxelForge
