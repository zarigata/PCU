/**
 * @file Statistics.cpp
 * @brief Statistics system implementation
 */

#include <VoxelForge/game/Statistics.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>
#include <fstream>

namespace VoxelForge {

// ============================================================================
// Leaderboard Implementation
// ============================================================================

Leaderboard::Leaderboard(const std::string& statId) : statId(statId) {}

void Leaderboard::update(uint32_t playerId, const std::string& name, int64_t value) {
    auto it = playerIndex.find(playerId);
    if (it != playerIndex.end()) {
        entries[it->second].value = value;
    } else {
        LeaderboardEntry entry;
        entry.playerId = playerId;
        entry.playerName = name;
        entry.value = value;
        entries.push_back(entry);
        playerIndex[playerId] = entries.size() - 1;
    }
    sortEntries();
}

void Leaderboard::remove(uint32_t playerId) {
    auto it = playerIndex.find(playerId);
    if (it == playerIndex.end()) return;
    
    size_t idx = it->second;
    entries.erase(entries.begin() + idx);
    playerIndex.erase(it);
    
    // Rebuild index
    playerIndex.clear();
    for (size_t i = 0; i < entries.size(); ++i) {
        playerIndex[entries[i].playerId] = i;
    }
}

void Leaderboard::rebuild(const std::unordered_map<uint32_t, std::pair<std::string, int64_t>>& data) {
    entries.clear();
    playerIndex.clear();
    
    for (const auto& [playerId, info] : data) {
        LeaderboardEntry entry;
        entry.playerId = playerId;
        entry.playerName = info.first;
        entry.value = info.second;
        entries.push_back(entry);
        playerIndex[playerId] = entries.size() - 1;
    }
    sortEntries();
}

std::vector<LeaderboardEntry> Leaderboard::getTop(size_t count) const {
    std::vector<LeaderboardEntry> result;
    size_t n = std::min(count, entries.size());
    for (size_t i = 0; i < n; ++i) {
        result.push_back(entries[i]);
    }
    return result;
}

int Leaderboard::getRank(uint32_t playerId) const {
    auto it = playerIndex.find(playerId);
    if (it == playerIndex.end()) return -1;
    return static_cast<int>(it->second) + 1;
}

int64_t Leaderboard::getValue(uint32_t playerId) const {
    auto it = playerIndex.find(playerId);
    if (it == playerIndex.end()) return 0;
    return entries[it->second].value;
}

void Leaderboard::sortEntries() {
    std::sort(entries.begin(), entries.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
        return a.value > b.value;
    });
    
    // Rebuild index and ranks
    playerIndex.clear();
    for (size_t i = 0; i < entries.size(); ++i) {
        entries[i].rank = static_cast<int>(i) + 1;
        playerIndex[entries[i].playerId] = i;
    }
}

// ============================================================================
// Statistics Implementation
// ============================================================================

Statistics::Statistics() {
    LOG_INFO("Statistics system created");
    loadVanillaStats();
}

Statistics::~Statistics() {
    LOG_INFO("Statistics system destroyed");
}

void Statistics::registerStat(const StatType& stat) {
    stats[stat.id] = stat;
    leaderboards[stat.id] = std::make_unique<Leaderboard>(stat.id);
}

void Statistics::unregisterStat(const std::string& id) {
    stats.erase(id);
    leaderboards.erase(id);
}

void Statistics::loadVanillaStats() {
    LOG_INFO("Loading vanilla stats...");
    
    // General stats
    registerStat({VanillaStats::LEAVE_GAME, "Leave Game", StatCategory::General, "times"});
    registerStat({VanillaStats::PLAY_TIME, "Play Time", StatCategory::Time, "ticks"});
    registerStat({VanillaStats::TOTAL_WORLD_TIME, "Total World Time", StatCategory::Time, "ticks"});
    registerStat({VanillaStats::TIME_SINCE_DEATH, "Time Since Death", StatCategory::Time, "ticks"});
    registerStat({VanillaStats::TIME_SINCE_REST, "Time Since Rest", StatCategory::Time, "ticks"});
    registerStat({VanillaStats::SNEAK_TIME, "Sneak Time", StatCategory::Time, "ticks"});
    
    // Distance stats
    registerStat({VanillaStats::WALK_ONE_CM, "Walk Distance", StatCategory::Distance, "cm"});
    registerStat({VanillaStats::CROUCH_ONE_CM, "Crouch Distance", StatCategory::Distance, "cm"});
    registerStat({VanillaStats::SPRINT_ONE_CM, "Sprint Distance", StatCategory::Distance, "cm"});
    registerStat({VanillaStats::WALK_ON_WATER_ONE_CM, "Walk on Water Distance", StatCategory::Distance, "cm"});
    registerStat({VanillaStats::FALL_ONE_CM, "Fall Distance", StatCategory::Distance, "cm"});
    registerStat({VanillaStats::CLIMB_ONE_CM, "Climb Distance", StatCategory::Distance, "cm"});
    registerStat({VanillaStats::FLY_ONE_CM, "Fly Distance", StatCategory::Distance, "cm"});
    registerStat({VanillaStats::WALK_UNDER_WATER_ONE_CM, "Walk Under Water Distance", StatCategory::Distance, "cm"});
    registerStat({VanillaStats::SWIM_ONE_CM, "Swim Distance", StatCategory::Distance, "cm"});
    
    // Vehicle distance
    registerStat({VanillaStats::MINECART_ONE_CM, "Minecart Distance", StatCategory::Distance, "cm"});
    registerStat({VanillaStats::BOAT_ONE_CM, "Boat Distance", StatCategory::Distance, "cm"});
    registerStat({VanillaStats::PIG_ONE_CM, "Pig Distance", StatCategory::Distance, "cm"});
    registerStat({VanillaStats::HORSE_ONE_CM, "Horse Distance", StatCategory::Distance, "cm"});
    registerStat({VanillaStats::AVIATE_ONE_CM, "Elytra Distance", StatCategory::Distance, "cm"});
    registerStat({VanillaStats::STRIDER_ONE_CM, "Strider Distance", StatCategory::Distance, "cm"});
    
    // Actions
    registerStat({VanillaStats::JUMP, "Jump", StatCategory::General, "times"});
    registerStat({VanillaStats::DROP, "Items Dropped", StatCategory::General, "items"});
    
    // Combat
    registerStat({VanillaStats::DAMAGE_DEALT, "Damage Dealt", StatCategory::General, "damage"});
    registerStat({VanillaStats::DAMAGE_TAKEN, "Damage Taken", StatCategory::General, "damage"});
    registerStat({VanillaStats::DEATHS, "Deaths", StatCategory::General, "times"});
    registerStat({VanillaStats::MOB_KILLS, "Mob Kills", StatCategory::General, "kills"});
    registerStat({VanillaStats::PLAYER_KILLS, "Player Kills", StatCategory::General, "kills"});
    
    // Animals
    registerStat({VanillaStats::ANIMALS_BRED, "Animals Bred", StatCategory::General, "times"});
    registerStat({VanillaStats::FISH_CAUGHT, "Fish Caught", StatCategory::General, "fish"});
    
    // Interactions
    registerStat({VanillaStats::TALKED_TO_VILLAGER, "Villager Talks", StatCategory::General, "times"});
    registerStat({VanillaStats::TRADED_WITH_VILLAGER, "Villager Trades", StatCategory::General, "times"});
    
    // Block interactions
    registerStat({VanillaStats::CHEST_OPENED, "Chests Opened", StatCategory::General, "times"});
    registerStat({VanillaStats::ENDERCHEST_OPENED, "Ender Chests Opened", StatCategory::General, "times"});
    registerStat({VanillaStats::SHULKER_BOX_OPENED, "Shulker Boxes Opened", StatCategory::General, "times"});
    registerStat({VanillaStats::CRAFTING_TABLE_INTERACTION, "Crafting Tables Used", StatCategory::General, "times"});
    registerStat({VanillaStats::FURNACE_INTERACTION, "Furnaces Used", StatCategory::General, "times"});
    registerStat({VanillaStats::ANVIL_INTERACTION, "Anvils Used", StatCategory::General, "times"});
    registerStat({VanillaStats::BREWINGSTAND_INTERACTION, "Brewing Stands Used", StatCategory::General, "times"});
    registerStat({VanillaStats::BEACON_INTERACTION, "Beacons Used", StatCategory::General, "times"});
    
    LOG_INFO("Loaded {} vanilla stats", stats.size());
}

const StatType* Statistics::getStat(const std::string& id) const {
    auto it = stats.find(id);
    return it != stats.end() ? &it->second : nullptr;
}

std::vector<const StatType*> Statistics::getStatsByCategory(StatCategory category) const {
    std::vector<const StatType*> result;
    for (const auto& [id, stat] : stats) {
        if (stat.category == category) {
            result.push_back(&stat);
        }
    }
    return result;
}

std::vector<std::string> Statistics::getAllStatIds() const {
    std::vector<std::string> result;
    for (const auto& [id, stat] : stats) {
        result.push_back(id);
    }
    return result;
}

void Statistics::createPlayerStats(uint32_t playerId) {
    if (playerStats.find(playerId) == playerStats.end()) {
        playerStats[playerId] = {};
    }
}

void Statistics::removePlayerStats(uint32_t playerId) {
    playerStats.erase(playerId);
    
    // Remove from all leaderboards
    for (auto& [id, leaderboard] : leaderboards) {
        leaderboard->remove(playerId);
    }
}

void Statistics::clearPlayerStats(uint32_t playerId) {
    playerStats[playerId].clear();
}

int64_t Statistics::getStat(uint32_t playerId, const std::string& statId) const {
    auto playerIt = playerStats.find(playerId);
    if (playerIt == playerStats.end()) return 0;
    
    auto statIt = playerIt->second.find(statId);
    if (statIt == playerIt->second.end()) return 0;
    
    return statIt->second.value;
}

void Statistics::setStat(uint32_t playerId, const std::string& statId, int64_t value) {
    auto& statValue = playerStats[playerId][statId];
    int64_t oldValue = statValue.value;
    statValue.value = value;
    statValue.lastUpdated = std::chrono::system_clock::now().time_since_epoch().count();
    
    // Update leaderboard
    if (leaderboards.find(statId) != leaderboards.end()) {
        // leaderboard->update(playerId, playerName, value);
    }
    
    // Callback
    if (onStatChanged) {
        onStatChanged(playerId, statId, oldValue, value);
    }
    
    // Check milestones
    checkMilestones(playerId, statId, value);
}

void Statistics::incrementStat(uint32_t playerId, const std::string& statId, int64_t amount) {
    int64_t current = getStat(playerId, statId);
    setStat(playerId, statId, current + amount);
}

void Statistics::decrementStat(uint32_t playerId, const std::string& statId, int64_t amount) {
    int64_t current = getStat(playerId, statId);
    setStat(playerId, statId, current - amount);
}

void Statistics::incrementBatch(uint32_t playerId, const std::unordered_map<std::string, int64_t>& statUpdates) {
    for (const auto& [statId, amount] : statUpdates) {
        incrementStat(playerId, statId, amount);
    }
}

void Statistics::startSession(uint32_t playerId) {
    createPlayerStats(playerId);
    for (auto& [statId, statValue] : playerStats[playerId]) {
        statValue.sessionValue = 0;
    }
}

void Statistics::endSession(uint32_t playerId) {
    // Save stats on session end
    savePlayerStats(playerId, "stats/" + std::to_string(playerId) + ".json");
}

int64_t Statistics::getSessionValue(uint32_t playerId, const std::string& statId) const {
    auto playerIt = playerStats.find(playerId);
    if (playerIt == playerStats.end()) return 0;
    
    auto statIt = playerIt->second.find(statId);
    if (statIt == playerIt->second.end()) return 0;
    
    return statIt->second.sessionValue;
}

void Statistics::loadPlayerStats(uint32_t playerId, const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return;
    
    try {
        nlohmann::json j;
        file >> j;
        
        for (auto& [statId, value] : j.items()) {
            playerStats[playerId][statId].value = value.get<int64_t>();
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load player stats: {}", e.what());
    }
}

void Statistics::savePlayerStats(uint32_t playerId, const std::string& path) {
    nlohmann::json j;
    
    auto playerIt = playerStats.find(playerId);
    if (playerIt != playerStats.end()) {
        for (const auto& [statId, statValue] : playerIt->second) {
            j[statId] = statValue.value;
        }
    }
    
    std::ofstream file(path);
    if (file.is_open()) {
        file << j.dump(2);
    }
}

void Statistics::loadAllPlayers(const std::string& directory) {
    // TODO: Load all player stat files from directory
}

void Statistics::saveAllPlayers(const std::string& directory) {
    for (const auto& [playerId, stats] : playerStats) {
        savePlayerStats(playerId, directory + "/" + std::to_string(playerId) + ".json");
    }
}

Leaderboard* Statistics::getLeaderboard(const std::string& statId) {
    auto it = leaderboards.find(statId);
    return it != leaderboards.end() ? it->second.get() : nullptr;
}

const Leaderboard* Statistics::getLeaderboard(const std::string& statId) const {
    auto it = leaderboards.find(statId);
    return it != leaderboards.end() ? it->second.get() : nullptr;
}

void Statistics::updateLeaderboards() {
    for (auto& [statId, leaderboard] : leaderboards) {
        std::unordered_map<uint32_t, std::pair<std::string, int64_t>> data;
        for (const auto& [playerId, stats] : playerStats) {
            auto statIt = stats.find(statId);
            if (statIt != stats.end()) {
                data[playerId] = {"Player" + std::to_string(playerId), statIt->second.value};
            }
        }
        leaderboard->rebuild(data);
    }
}

std::vector<LeaderboardEntry> Statistics::getTopPlayers(const std::string& statId, size_t count) const {
    auto leaderboard = getLeaderboard(statId);
    return leaderboard ? leaderboard->getTop(count) : std::vector<LeaderboardEntry>();
}

void Statistics::setOnStatChanged(StatCallback callback) {
    onStatChanged = std::move(callback);
}

void Statistics::addMilestone(const Milestone& milestone) {
    milestones.push_back(milestone);
}

void Statistics::checkMilestones(uint32_t playerId, const std::string& statId, int64_t value) {
    for (const auto& milestone : milestones) {
        if (milestone.statId == statId && value >= milestone.targetValue) {
            // Check if already achieved
            // TODO: Track achieved milestones
            LOG_INFO("Player {} reached milestone: {} = {}", playerId, statId, value);
        }
    }
}

void Statistics::saveServerStats(const std::string& path) {
    nlohmann::json j;
    j["total_playtime"] = serverStats.totalPlaytime;
    j["total_blocks_broken"] = serverStats.totalBlocksBroken;
    j["total_blocks_placed"] = serverStats.totalBlocksPlaced;
    j["total_deaths"] = serverStats.totalDeaths;
    j["total_mob_kills"] = serverStats.totalMobKills;
    j["total_items_crafted"] = serverStats.totalItemsCrafted;
    j["total_distance_traveled"] = serverStats.totalDistanceTraveled;
    j["player_joins"] = serverStats.playerJoins;
    j["player_leaves"] = serverStats.playerLeaves;
    j["highest_player_count"] = serverStats.highestPlayerCount;
    j["server_start_time"] = serverStats.serverStartTime;
    
    std::ofstream file(path);
    if (file.is_open()) {
        file << j.dump(2);
    }
}

void Statistics::loadServerStats(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return;
    
    try {
        nlohmann::json j;
        file >> j;
        
        serverStats.totalPlaytime = j.value("total_playtime", 0LL);
        serverStats.totalBlocksBroken = j.value("total_blocks_broken", 0LL);
        serverStats.totalBlocksPlaced = j.value("total_blocks_placed", 0LL);
        serverStats.totalDeaths = j.value("total_deaths", 0LL);
        serverStats.totalMobKills = j.value("total_mob_kills", 0LL);
        serverStats.totalItemsCrafted = j.value("total_items_crafted", 0LL);
        serverStats.totalDistanceTraveled = j.value("total_distance_traveled", 0LL);
        serverStats.playerJoins = j.value("player_joins", 0LL);
        serverStats.playerLeaves = j.value("player_leaves", 0LL);
        serverStats.highestPlayerCount = j.value("highest_player_count", 0LL);
        serverStats.serverStartTime = j.value("server_start_time", 0LL);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load server stats: {}", e.what());
    }
}

} // namespace VoxelForge
