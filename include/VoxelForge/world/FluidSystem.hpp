/**
 * @file FluidSystem.hpp
 * @brief Fluid physics simulation for VoxelForge
 *
 * Handles water and lava flow using a scheduled-tick approach.
 */

#pragma once

#include <VoxelForge/Engine.hpp>
#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/world/FluidTypes.hpp>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <mutex>
#include <shared_mutex>

namespace VoxelForge {

// ============================================
// Fluid State
// ============================================

struct FluidState {
    FluidType type = FluidType::Empty;
    uint8_t level = 0;       // 0 = source, 1-7 = flowing (higher = less fluid)
    bool falling = false;    // True when flowing straight down

    bool isEmpty() const { return type == FluidType::Empty; }
    bool isSource() const { return level == 0; }
    int getFlowDistance() const { return static_cast<int>(level); }
};

// ============================================
// Fluid System
// ============================================

class FluidSystem {
public:
    explicit FluidSystem();
    ~FluidSystem() = default;

    // Per-tick processing
    void tick(class World& world);

    // Schedule a fluid update at a world position
    void scheduleUpdate(const BlockPos& pos, int delay = 0);

    // Query fluid state
    FluidState getFluid(class World& world, const BlockPos& pos) const;
    bool isFluidSource(class World& world, const BlockPos& pos) const;

    // Per-type flow parameters
    struct FluidProperties {
        int flowDistance = 7;
        int flowSpeed = 4;          // Ticks between flow steps
        bool canSetFire = false;
        bool canExtinguish = true;
        bool harmsEntities = false;
        float brightness = 0.0f;
    };

    FluidProperties getProperties(FluidType type) const;
    void registerFluidType(FluidType type, const FluidProperties& props);

    // Core flow logic
    void flow(class World& world, const BlockPos& pos, const FluidState& state);
    void flowDown(class World& world, const BlockPos& pos, const FluidState& state);
    void flowSideways(class World& world, const BlockPos& pos, const FluidState& state,
                      const std::vector<BlockPos>& directions);

    bool canFlowInto(class World& world, const BlockPos& pos) const;

    // Fluid interactions (water + lava)
    void handleFluidMixing(class World& world, const BlockPos& pos, FluidType incoming);

    // Block state helpers
    BlockState getFlowingBlockState(FluidType type, uint8_t level, bool falling) const;
    BlockState getSourceBlockState(FluidType type) const;

    // Statistics
    size_t getPendingUpdateCount() const;
    size_t getActiveFluidCount() const;

private:
    struct FluidUpdate {
        BlockPos pos;
        int ticksRemaining;
    };

    // BlockPos hasher — reuses std::hash<glm::ivec3> defined in ChunkPos.hpp
    struct BlockPosHash {
        size_t operator()(const BlockPos& p) const {
            return std::hash<glm::ivec3>()(p);
        }
    };

    // Pending updates (FIFO)
    std::queue<FluidUpdate> pendingUpdates;

    // Tracks positions that already have a pending update to avoid duplicates
    std::unordered_set<BlockPos, BlockPosHash> scheduledSet;

    // Active fluid positions (for statistics / debugging)
    std::unordered_map<BlockPos, int, BlockPosHash> activeFluids;

    // Per-type properties
    std::unordered_map<FluidType, FluidProperties> fluidProperties;

    // Thread safety
    mutable std::shared_mutex mutex;

    // Internal processing
    void processUpdate(class World& world, const BlockPos& pos);
    std::vector<BlockPos> getFlowDirections(class World& world, const BlockPos& pos) const;

    // Block type helpers
    FluidType getFluidTypeFromBlock(const BlockState& state) const;
    int getFluidLevelFromBlock(const BlockState& state) const;
    bool isFallingFromBlock(const BlockState& state) const;

    // Constants
    static constexpr int MAX_UPDATES_PER_TICK = 1000;
    static constexpr int MAX_FLOW_DISTANCE_WATER = 7;
    static constexpr int MAX_FLOW_DISTANCE_LAVA_OVERWORLD = 4;
    static constexpr int WATER_FLOW_SPEED = 4;
    static constexpr int LAVA_FLOW_SPEED = 30;
};

} // namespace VoxelForge
