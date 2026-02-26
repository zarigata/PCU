/**
 * @file Statistics.hpp
 * @brief Player and server statistics tracking
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <cstdint>

namespace VoxelForge {

// Stat category
enum class StatCategory {
    General,
    Mining,
    Crafting,
    Used,
    Broken,
    PickedUp,
    Dropped,
    Killed,
    KilledBy,
    Custom,
    Distance,
    Time
};

// Stat type
struct StatType {
    std::string id;
    std::string name;
    StatCategory category;
    std::string unit;           // blocks, items, cm, ticks, etc.
    bool hidden = false;
};

// Stat value
struct StatValue {
    int64_t value = 0;
    int64_t sessionValue = 0;   // Value since last save/login
    int64_t maxValue = 0;       // For tracking records
    int64_t firstAchieved = 0;  // Timestamp
    int64_t lastUpdated = 0;    // Timestamp
};

// Leaderboard entry
struct LeaderboardEntry {
    uint32_t playerId;
    std::string playerName;
    int64_t value;
    int rank;
};

// Leaderboard
class Leaderboard {
public:
    explicit Leaderboard(const std::string& statId);
    
    void update(uint32_t playerId, const std::string& name, int64_t value);
    void remove(uint32_t playerId);
    void rebuild(const std::unordered_map<uint32_t, std::pair<std::string, int64_t>>& data);
    
    std::vector<LeaderboardEntry> getTop(size_t count) const;
    int getRank(uint32_t playerId) const;
    int64_t getValue(uint32_t playerId) const;
    
    const std::string& getStatId() const { return statId; }
    size_t getSize() const { return entries.size(); }
    
private:
    void sortEntries();
    
    std::string statId;
    std::vector<LeaderboardEntry> entries;
    std::unordered_map<uint32_t, size_t> playerIndex;
};

// Statistics system
class Statistics {
public:
    Statistics();
    ~Statistics();
    
    // Stat registration
    void registerStat(const StatType& stat);
    void unregisterStat(const std::string& id);
    void loadVanillaStats();
    
    // Query
    const StatType* getStat(const std::string& id) const;
    std::vector<const StatType*> getStatsByCategory(StatCategory category) const;
    std::vector<std::string> getAllStatIds() const;
    
    // Player stats
    void createPlayerStats(uint32_t playerId);
    void removePlayerStats(uint32_t playerId);
    void clearPlayerStats(uint32_t playerId);
    
    // Get/Set stats
    int64_t getStat(uint32_t playerId, const std::string& statId) const;
    void setStat(uint32_t playerId, const std::string& statId, int64_t value);
    void incrementStat(uint32_t playerId, const std::string& statId, int64_t amount = 1);
    void decrementStat(uint32_t playerId, const std::string& statId, int64_t amount = 1);
    
    // Batch operations
    void incrementBatch(uint32_t playerId, const std::unordered_map<std::string, int64_t>& stats);
    
    // Session tracking
    void startSession(uint32_t playerId);
    void endSession(uint32_t playerId);
    int64_t getSessionValue(uint32_t playerId, const std::string& statId) const;
    
    // Persistence
    void loadPlayerStats(uint32_t playerId, const std::string& path);
    void savePlayerStats(uint32_t playerId, const std::string& path);
    void loadAllPlayers(const std::string& directory);
    void saveAllPlayers(const std::string& directory);
    
    // Leaderboards
    Leaderboard* getLeaderboard(const std::string& statId);
    const Leaderboard* getLeaderboard(const std::string& statId) const;
    void updateLeaderboards();
    std::vector<LeaderboardEntry> getTopPlayers(const std::string& statId, size_t count) const;
    
    // Callbacks
    using StatCallback = std::function<void(uint32_t playerId, const std::string& statId, int64_t oldValue, int64_t newValue)>;
    void setOnStatChanged(StatCallback callback);
    
    // Milestones
    struct Milestone {
        std::string statId;
        int64_t targetValue;
        std::string reward;       // Achievement or reward
        bool announce = true;
    };
    
    void addMilestone(const Milestone& milestone);
    void checkMilestones(uint32_t playerId, const std::string& statId, int64_t value);
    
    // Server stats
    struct ServerStats {
        int64_t totalPlaytime = 0;
        int64_t totalBlocksBroken = 0;
        int64_t totalBlocksPlaced = 0;
        int64_t totalDeaths = 0;
        int64_t totalMobKills = 0;
        int64_t totalItemsCrafted = 0;
        int64_t totalDistanceTraveled = 0;
        int64_t playerJoins = 0;
        int64_t playerLeaves = 0;
        int64_t highestPlayerCount = 0;
        int64_t serverStartTime = 0;
    };
    
    ServerStats& getServerStats() { return serverStats; }
    const ServerStats& getServerStats() const { return serverStats; }
    void saveServerStats(const std::string& path);
    void loadServerStats(const std::string& path);
    
    // Stats count
    size_t getStatCount() const { return stats.size(); }
    size_t getPlayerCount() const { return playerStats.size(); }
    
private:
    std::unordered_map<std::string, StatType> stats;
    std::unordered_map<uint32_t, std::unordered_map<std::string, StatValue>> playerStats;
    std::unordered_map<std::string, std::unique_ptr<Leaderboard>> leaderboards;
    std::vector<Milestone> milestones;
    
    StatCallback onStatChanged;
    ServerStats serverStats;
};

// Built-in stat definitions
namespace VanillaStats {
    // General
    constexpr const char* LEAVE_GAME = "minecraft:leave_game";
    constexpr const char* PLAY_TIME = "minecraft:play_time";
    constexpr const char* TOTAL_WORLD_TIME = "minecraft:total_world_time";
    constexpr const char* TIME_SINCE_DEATH = "minecraft:time_since_death";
    constexpr const char* TIME_SINCE_REST = "minecraft:time_since_rest";
    constexpr const char* SNEAK_TIME = "minecraft:sneak_time";
    constexpr const char* WALK_ONE_CM = "minecraft:walk_one_cm";
    constexpr const char* CROUCH_ONE_CM = "minecraft:crouch_one_cm";
    constexpr const char* SPRINT_ONE_CM = "minecraft:sprint_one_cm";
    constexpr const char* WALK_ON_WATER_ONE_CM = "minecraft:walk_on_water_one_cm";
    constexpr const char* FALL_ONE_CM = "minecraft:fall_one_cm";
    constexpr const char* CLIMB_ONE_CM = "minecraft:climb_one_cm";
    constexpr const char* FLY_ONE_CM = "minecraft:fly_one_cm";
    constexpr const char* WALK_UNDER_WATER_ONE_CM = "minecraft:walk_under_water_one_cm";
    constexpr const char* MINECART_ONE_CM = "minecraft:minecart_one_cm";
    constexpr const char* BOAT_ONE_CM = "minecraft:boat_one_cm";
    constexpr const char* PIG_ONE_CM = "minecraft:pig_one_cm";
    constexpr const char* HORSE_ONE_CM = "minecraft:horse_one_cm";
    constexpr const char* AVIATE_ONE_CM = "minecraft:aviate_one_cm";
    constexpr const char* SWIM_ONE_CM = "minecraft:swim_one_cm";
    constexpr const char* STRIDER_ONE_CM = "minecraft:strider_one_cm";
    constexpr const char* JUMP = "minecraft:jump";
    constexpr const char* DROP = "minecraft:drop";
    constexpr const char* DAMAGE_DEALT = "minecraft:damage_dealt";
    constexpr const char* DAMAGE_TAKEN = "minecraft:damage_taken";
    constexpr const char* DEATHS = "minecraft:deaths";
    constexpr const char* MOB_KILLS = "minecraft:mob_kills";
    constexpr const char* ANIMALS_BRED = "minecraft:animals_bred";
    constexpr const char* PLAYER_KILLS = "minecraft:player_kills";
    constexpr const char* FISH_CAUGHT = "minecraft:fish_caught";
    constexpr const char* TALKED_TO_VILLAGER = "minecraft:talked_to_villager";
    constexpr const char* TRADED_WITH_VILLAGER = "minecraft:traded_with_villager";
    constexpr const char* CAKES_EATEN = "minecraft:cakes_eaten";
    constexpr const char* CAULDRON_FILLED = "minecraft:cauldron_filled";
    constexpr const char* CAULDRON_USED = "minecraft:cauldron_used";
    constexpr const char* ARMOR_CLEANED = "minecraft:armor_cleaned";
    constexpr const char* BANNER_CLEANED = "minecraft:banner_cleaned";
    constexpr const char* BREWINGSTAND_INTERACTION = "minecraft:brewingstand_interaction";
    constexpr const char* BEACON_INTERACTION = "minecraft:beacon_interaction";
    constexpr const char* ANVIL_INTERACTION = "minecraft:anvil_interaction";
    constexpr const char* CRAFTING_TABLE_INTERACTION = "minecraft:crafting_table_interaction";
    constexpr const char* FURNACE_INTERACTION = "minecraft:furnace_interaction";
    constexpr const char* CHEST_OPENED = "minecraft:chest_opened";
    constexpr const char* ENDERCHEST_OPENED = "minecraft:enderchest_opened";
    constexpr const char* SHULKER_BOX_OPENED = "minecraft:shulker_box_opened";
}

} // namespace VoxelForge
