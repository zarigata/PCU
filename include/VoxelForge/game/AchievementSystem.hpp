/**
 * @file AchievementSystem.hpp
 * @brief Achievement/Advancement system
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <glm/glm.hpp>

namespace VoxelForge {

// Forward declarations
class Player;
class World;

// Advancement display
struct AdvancementDisplay {
    std::string icon;           // Item ID for icon
    std::string title;
    std::string description;
    glm::vec3 backgroundColor = glm::vec3(0.0f);
    std::string frame = "task"; // task, challenge, goal
    bool showToast = true;
    bool announceToChat = true;
    bool hidden = false;
};

// Advancement criteria
struct Criterion {
    std::string id;
    std::string trigger;        // Trigger type
    std::unordered_map<std::string, std::string> conditions;
    bool achieved = false;
    int64_t achievedTime = 0;
};

// Advancement rewards
struct AdvancementRewards {
    int experience = 0;
    std::vector<std::string> loot;
    std::vector<std::string> recipes;
    std::string function;
};

// Advancement state
enum class AdvancementState {
    Locked,
    Available,
    InProgress,
    Completed
};

// Advancement
class Advancement {
public:
    std::string id;
    std::string parent;
    std::shared_ptr<AdvancementDisplay> display;
    std::vector<std::shared_ptr<Criterion>> criteria;
    AdvancementRewards rewards;
    std::unordered_map<std::string, std::vector<std::string>> requirements;
    
    // Check if complete
    bool isComplete() const;
    float getProgress() const;  // 0.0 to 1.0
    int getCompletedCriteria() const;
    int getTotalCriteria() const;
    
    // Criteria management
    void grantCriterion(const std::string& criterionId);
    void revokeCriterion(const std::string& criterionId);
    bool hasCriterion(const std::string& criterionId) const;
    
    // State
    AdvancementState getState() const;
    
    // Serialization
    nlohmann::json toJson() const;
    static std::unique_ptr<Advancement> fromJson(const nlohmann::json& json, const std::string& id);
};

// Player advancement progress
struct PlayerAdvancementProgress {
    std::string advancementId;
    bool achieved = false;
    int64_t achievedTime = 0;
    std::unordered_map<std::string, bool> criteriaProgress;
    int earnedExperience = 0;
};

// Advancement tab
struct AdvancementTab {
    std::string id;
    std::string title;
    std::shared_ptr<Advancement> root;
    std::vector<std::shared_ptr<Advancement>> advancements;
    glm::vec2 scrollPosition = glm::vec2(0.0f);
};

// Trigger types
enum class TriggerType {
    Impossible,
    Location,
    EntityHurtPlayer,
    PlayerHurtEntity,
    PlayerKilledEntity,
    EntityKilledPlayer,
    InventoryChanged,
    RecipeUnlocked,
    ItemUsedOnBlock,
    PlacedBlock,
    DestroyBlock,
    EnterBlock,
    Tick,
    SummonedEntity,
    BredAnimals,
    ConstructBeacon,
    UsedEnderEye,
    UsedTotem,
    NetherTravel,
    ChangedDimension,
    FishingRodHook,
    ChanneledLightning,
    ShotCrossbow,
    KilledByCrossbow,
    HeroOfTheVillage,
    VoluntaryExile
};

// Trigger handler
class TriggerHandler {
public:
    virtual ~TriggerHandler() = default;
    virtual bool test(Player* player, const std::unordered_map<std::string, std::string>& conditions) = 0;
};

// Achievement/Advancement system
class AchievementSystem {
public:
    AchievementSystem();
    ~AchievementSystem();
    
    // Advancement registration
    void registerAdvancement(std::unique_ptr<Advancement> advancement);
    void unregisterAdvancement(const std::string& id);
    void loadAdvancements(const std::string& directory);
    void clear();
    
    // Query
    const Advancement* getAdvancement(const std::string& id) const;
    std::vector<const Advancement*> getAdvancements() const;
    std::vector<const Advancement*> getRootAdvancements() const;
    std::vector<const Advancement*> getChildren(const std::string& parentId) const;
    std::vector<const Advancement*> getTabAdvancements(const std::string& tabId) const;
    
    // Player progress
    void loadPlayerProgress(uint32_t playerId);
    void savePlayerProgress(uint32_t playerId);
    void clearPlayerProgress(uint32_t playerId);
    
    PlayerAdvancementProgress* getProgress(uint32_t playerId, const std::string& advancementId);
    const PlayerAdvancementProgress* getProgress(uint32_t playerId, const std::string& advancementId) const;
    
    // Grant/revoke
    bool grantAdvancement(uint32_t playerId, const std::string& id);
    bool grantCriterion(uint32_t playerId, const std::string& advancementId, const std::string& criterionId);
    bool revokeAdvancement(uint32_t playerId, const std::string& id);
    bool revokeCriterion(uint32_t playerId, const std::string& advancementId, const std::string& criterionId);
    
    // Check progress
    bool isComplete(uint32_t playerId, const std::string& id) const;
    float getProgress(uint32_t playerId, const std::string& id) const;
    int getEarnedExperience(uint32_t playerId) const;
    
    // Triggers
    void triggerEvent(uint32_t playerId, TriggerType type, const std::unordered_map<std::string, std::string>& data);
    void registerTriggerHandler(TriggerType type, std::unique_ptr<TriggerHandler> handler);
    
    // Callbacks
    using AdvancementCallback = std::function<void(uint32_t playerId, const std::string& advancementId)>;
    void setOnAdvancementGranted(AdvancementCallback callback);
    void setOnAdvancementRevoked(AdvancementCallback callback);
    
    // Vanilla advancements
    void registerVanillaAdvancements();
    
    // Stats
    size_t getAdvancementCount() const { return advancements.size(); }
    size_t getPlayerCount() const { return playerProgress.size(); }
    
private:
    void checkAndGrant(uint32_t playerId, const std::string& advancementId);
    void grantRewards(uint32_t playerId, const Advancement& advancement);
    void notifyParent(uint32_t playerId, const std::string& childId);
    
    std::unordered_map<std::string, std::unique_ptr<Advancement>> advancements;
    std::unordered_map<std::string, std::vector<std::string>> children;  // parent -> children
    std::vector<std::string> rootAdvancements;
    
    std::unordered_map<uint32_t, std::unordered_map<std::string, PlayerAdvancementProgress>> playerProgress;
    
    std::unordered_map<TriggerType, std::unique_ptr<TriggerHandler>> triggerHandlers;
    
    AdvancementCallback onAdvancementGranted;
    AdvancementCallback onAdvancementRevoked;
};

} // namespace VoxelForge
