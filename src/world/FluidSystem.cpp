/**
 * @file FluidSystem.cpp
 * @brief Fluid physics simulation for VoxelForge
 *
 * Handles water and lava flow using a scheduled-tick approach.
 */

#include <VoxelForge/world/FluidSystem.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/world/BlockRegistry.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>
#include <glm/glm.hpp>

namespace VoxelForge {

// ============================================
// Constructor
// ============================================

FluidSystem::FluidSystem() {
    // Register default fluid types
    FluidProperties waterProps;
    waterProps.flowDistance = MAX_FLOW_DISTANCE_WATER;
    waterProps.flowSpeed = WATER_FLOW_SPEED;
    waterProps.canSetFire = false;
    waterProps.canExtinguish = true;
    waterProps.harmsEntities = false;
    waterProps.brightness = 0.0f;
    registerFluidType(FluidType::Water, waterProps);

    FluidProperties lavaProps;
    lavaProps.flowDistance = MAX_FLOW_DISTANCE_LAVA_OVERWORLD;
    lavaProps.flowSpeed = LAVA_FLOW_SPEED;
    lavaProps.canSetFire = true;
    lavaProps.canExtinguish = false;
    lavaProps.harmsEntities = true;
    lavaProps.brightness = 15.0f;
    registerFluidType(FluidType::Lava, lavaProps);
}

// ============================================
// Per-tick processing
// ============================================

void FluidSystem::tick(World& world) {
    std::unique_lock<std::shared_mutex> lock(mutex);

    int updatesThisTick = 0;
    while (!pendingUpdates.empty() && updatesThisTick < MAX_UPDATES_PER_TICK) {
        FluidUpdate update = pendingUpdates.front();
        pendingUpdates.pop();

        // Remove from scheduled set
        scheduledSet.erase(update.pos);

        // Remove from active fluids if still present
        auto it = activeFluids.find(update.pos);
        if (it != activeFluids.end()) {
            activeFluids.erase(it);
        }

        // Process the update
        processUpdate(world, update.pos);

        ++updatesThisTick;
    }

    SPDLOG_DEBUG("FluidSystem: Processed {} updates this tick, {} remaining in queue",
                updatesThisTick, pendingUpdates.size());
}

// ============================================
// Schedule updates
// ============================================

void FluidSystem::scheduleUpdate(const BlockPos& pos, int delay) {
    std::unique_lock<std::shared_mutex> lock(mutex);

    // Skip if already scheduled (avoid duplicates)
    if (scheduledSet.find(pos) != scheduledSet.end()) {
        return;
    }

    FluidUpdate update;
    update.pos = pos;
    update.ticksRemaining = delay;

    pendingUpdates.push(update);
    scheduledSet.insert(pos);
    activeFluids[pos] = static_cast<int>(delay);
}

// ============================================
// Query fluid state
// ============================================

FluidState FluidSystem::getFluid(World& world, const BlockPos& pos) const {
    BlockState state = world.getBlock(pos);

    if (state.getBlockId() == AIR_BLOCK) {
        return FluidState();
    }

    FluidState fluidState;
    fluidState.type = getFluidTypeFromBlock(state);
    fluidState.level = static_cast<uint8_t>(getFluidLevelFromBlock(state));
    fluidState.falling = isFallingFromBlock(state);

    return fluidState;
}

bool FluidSystem::isFluidSource(World& world, const BlockPos& pos) const {
    FluidState state = getFluid(world, pos);
    return state.isSource();
}

// ============================================
// Per-type properties
// ============================================

FluidSystem::FluidProperties FluidSystem::getProperties(FluidType type) const {
    std::shared_lock<std::shared_mutex> lock(mutex);

    auto it = fluidProperties.find(type);
    if (it != fluidProperties.end()) {
        return it->second;
    }

    FluidProperties defaultProps;
    return defaultProps;
}

void FluidSystem::registerFluidType(FluidType type, const FluidProperties& props) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    fluidProperties[type] = props;
}

// ============================================
// Core flow logic
// ============================================

void FluidSystem::flow(World& world, const BlockPos& pos, const FluidState& state) {
    if (state.isEmpty()) {
        return;
    }

    // Try to flow down first
    flowDown(world, pos, state);

    // Then try to flow sideways
    std::vector<BlockPos> directions = getFlowDirections(world, pos);
    flowSideways(world, pos, state, directions);
}

void FluidSystem::flowDown(World& world, const BlockPos& pos, const FluidState& state) {
    BlockPos below(pos.x, pos.y - 1, pos.z);

    if (canFlowInto(world, below)) {
        // Flowing down creates a falling fluid block
        BlockState newBlock = getFlowingBlockState(state.type, state.level, true);
        world.setBlock(below, newBlock);
        scheduleUpdate(below, getProperties(state.type).flowSpeed);
    }
}

void FluidSystem::flowSideways(World& world, const BlockPos& pos, const FluidState& state,
                                const std::vector<BlockPos>& directions) {
    if (state.level >= getProperties(state.type).flowDistance) {
        return; // Already at max flow distance
    }

    int newLevel = state.level + 1;
    BlockState newBlock = getFlowingBlockState(state.type, static_cast<uint8_t>(newLevel), false);

    for (const auto& direction : directions) {
        if (canFlowInto(world, direction)) {
            // Check for fluid mixing
            FluidState neighborFluid = getFluid(world, direction);
            if (!neighborFluid.isEmpty() && neighborFluid.type != state.type) {
                handleFluidMixing(world, direction, state.type);
                continue;
            }

            // Flow sideways
            world.setBlock(direction, newBlock);
            scheduleUpdate(direction, getProperties(state.type).flowSpeed);
        }
    }
}

bool FluidSystem::canFlowInto(World& world, const BlockPos& pos) const {
    BlockState state = world.getBlock(pos);
    BlockID id = state.getBlockId();

    // Can flow into air
    if (id == AIR_BLOCK) {
        return true;
    }

    // Get block definition
    const auto& def = BlockRegistry::get().getDefinition(id);

    // Can flow into replaceable blocks
    if (def.replaceable) {
        return true;
    }

    // Can flow into other fluids (will trigger mixing)
    if (def.material == Material::Water || def.material == Material::Lava) {
        return true;
    }

    return false;
}

// ============================================
// Fluid mixing (water + lava)
// ============================================

void FluidSystem::handleFluidMixing(World& world, const BlockPos& pos, FluidType incoming) {
    FluidState existing = getFluid(world, pos);

    if (existing.isEmpty()) {
        return;
    }

    // Water + Lava = Obsidian
    if (existing.type == FluidType::Water && incoming == FluidType::Lava) {
        BlockID obsidianId = BlockRegistry::get().getBlockId("poorcraftultra:obsidian");
        BlockState obsidian(obsidianId);
        world.setBlock(pos, obsidian);
        return;
    }

    if (existing.type == FluidType::Lava && incoming == FluidType::Water) {
        BlockID obsidianId = BlockRegistry::get().getBlockId("poorcraftultra:obsidian");
        BlockState obsidian(obsidianId);
        world.setBlock(pos, obsidian);
        return;
    }

    // Water flowing over lava = Cobblestone
    if (existing.type == FluidType::Water && incoming == FluidType::Water &&
        existing.isSource()) {
        // Check for lava below
        BlockPos below(pos.x, pos.y - 1, pos.z);
        if (isFluidSource(world, below) && getFluid(world, below).type == FluidType::Lava) {
            BlockID cobblestoneId = BlockRegistry::get().getBlockId("poorcraftultra:cobblestone");
            BlockState cobblestone(cobblestoneId);
            world.setBlock(pos, cobblestone);
        }
    }
}

// ============================================
// Block state helpers
// ============================================

BlockState FluidSystem::getFlowingBlockState(FluidType type, uint8_t level, bool falling) const {
    String blockId;

    if (type == FluidType::Water) {
        blockId = "poorcraftultra:flowing_water";
    } else if (type == FluidType::Lava) {
        blockId = "poorcraftultra:flowing_lava";
    } else {
        return BlockState(AIR_BLOCK);
    }

    BlockID id = BlockRegistry::get().getBlockId(blockId);
    BlockState state = BlockState(id);

    // Set level property if it exists
    if (level > 0) {
        state = state.withProperty("level", static_cast<int>(level));
    }

    // Note: Falling state would need to be a property, but for now we handle it via different block types
    // In a full implementation, we'd add a "falling" bool property

    return state;
}

BlockState FluidSystem::getSourceBlockState(FluidType type) const {
    String blockId;

    if (type == FluidType::Water) {
        blockId = "poorcraftultra:water";
    } else if (type == FluidType::Lava) {
        blockId = "poorcraftultra:lava";
    } else {
        return BlockState(AIR_BLOCK);
    }

    BlockID id = BlockRegistry::get().getBlockId(blockId);
    return BlockState(id);
}

// ============================================
// Statistics
// ============================================

size_t FluidSystem::getPendingUpdateCount() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return pendingUpdates.size();
}

size_t FluidSystem::getActiveFluidCount() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return activeFluids.size();
}

// ============================================
// Internal processing
// ============================================

void FluidSystem::processUpdate(World& world, const BlockPos& pos) {
    FluidState state = getFluid(world, pos);

    if (state.isEmpty()) {
        return;
    }

    // Schedule the next update for this fluid
    int flowSpeed = getProperties(state.type).flowSpeed;
    scheduleUpdate(pos, flowSpeed);

    // Perform flow logic
    flow(world, pos, state);
}

std::vector<BlockPos> FluidSystem::getFlowDirections(World& world, const BlockPos& pos) const {
    std::vector<BlockPos> directions;

    // Check all 4 horizontal neighbors
    BlockPos north(pos.x - 1, pos.y, pos.z);
    BlockPos south(pos.x + 1, pos.y, pos.z);
    BlockPos east(pos.x, pos.y, pos.z + 1);
    BlockPos west(pos.x, pos.y, pos.z - 1);

    // Collect valid flow directions
    if (canFlowInto(world, north)) directions.push_back(north);
    if (canFlowInto(world, south)) directions.push_back(south);
    if (canFlowInto(world, east)) directions.push_back(east);
    if (canFlowInto(world, west)) directions.push_back(west);

    return directions;
}

// ============================================
// Helper: Get fluid type from block
// ============================================

FluidType FluidSystem::getFluidTypeFromBlock(const BlockState& state) const {
    BlockID id = state.getBlockId();
    const auto& def = BlockRegistry::get().getDefinition(id);

    if (def.material == Material::Water) {
        return FluidType::Water;
    } else if (def.material == Material::Lava) {
        return FluidType::Lava;
    }

    return FluidType::Empty;
}

// ============================================
// Helper: Get fluid level from block
// ============================================

int FluidSystem::getFluidLevelFromBlock(const BlockState& state) const {
    BlockID id = state.getBlockId();
    const auto& def = BlockRegistry::get().getDefinition(id);

    // Source blocks have level 0
    if (def.id == "poorcraftultra:water" || def.id == "poorcraftultra:lava") {
        return 0;
    }

    // Flowing blocks have level property
    if (def.id == "poorcraftultra:flowing_water" || def.id == "poorcraftultra:flowing_lava") {
        int level = state.getInt("level");
        return level; // 0-7 (but for flowing, we'll interpret as 1-7)
    }

    return 0;
}

// ============================================
// Helper: Check if block is falling fluid
// ============================================

bool FluidSystem::isFallingFromBlock(const BlockState& state) const {
    // In a full implementation, this would check a "falling" property
    // For now, we return false since we don't have separate falling block types
    return false;
}

} // namespace VoxelForge
