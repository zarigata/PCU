/**
 * @file Block.hpp
 * @brief Block system for VoxelForge
 * 
 * Handles block states, properties, and behaviors.
 * Supports 65,536 unique block IDs with variable properties.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#include <functional>

namespace VoxelForge {

// Forward declarations
class World;
class Player;
class Entity;

// ============================================
// Type Definitions
// ============================================

using BlockID = uint16_t;
using BlockData = uint16_t;
constexpr BlockID AIR_BLOCK = 0;
constexpr BlockID MAX_BLOCK_ID = 65535;

// ============================================
// Material Types
// ============================================

enum class Material : uint8_t {
    Air,
    Stone,
    Dirt,
    Wood,
    Plant,
    Water,
    Lava,
    Sand,
    Metal,
    Glass,
    Wool,
    Clay,
    Snow,
    Ice,
    Netherrack,
    EndStone,
    Coral,
    Shulker,
    Decoration,
    Technical,
    
    // Custom materials for mods
    Custom1, Custom2, Custom3, Custom4, Custom5
};

// ============================================
// Tool Types
// ============================================

enum class ToolType : uint8_t {
    None,
    Pickaxe,
    Axe,
    Shovel,
    Hoe,
    Sword,
    Shears
};

// ============================================
// Tool Tiers
// ============================================

enum class ToolTier : uint8_t {
    None,
    Wood,
    Stone,
    Iron,
    Diamond,
    Netherite,
    Gold
};

// ============================================
// Render Types
// ============================================

enum class RenderType : uint8_t {
    Solid,          // Opaque blocks (stone, dirt)
    Cutout,         // Transparent with alpha test (glass, leaves)
    CutoutMipped,   // Cutout with mipmaps
    Translucent,    // Semi-transparent (water, stained glass)
    Tripwire,       // Special case for tripwire
    Air             // Invisible (air, barrier)
};

// ============================================
// Sound Groups
// ============================================

struct BlockSoundGroup {
    String breakSound;
    String stepSound;
    String placeSound;
    String hitSound;
    String fallSound;
    float pitch = 1.0f;
    float volume = 1.0f;
};

// ============================================
// Block Properties
// ============================================

struct BlockProperty {
    enum class Type { Bool, Int, Enum, Direction };
    
    String name;
    Type type;
    std::vector<String> values;  // For enums and directions
    int defaultValue = 0;
    int minValue = 0;            // For ints
    int maxValue = 0;            // For ints
};

// ============================================
// Block Properties Container
// ============================================

class PropertyContainer {
public:
    void setBool(const String& name, bool value);
    void setInt(const String& name, int value);
    void setEnum(const String& name, const String& value);
    void setDirection(const String& name, int direction);
    
    bool getBool(const String& name) const;
    int getInt(const String& name) const;
    String getEnum(const String& name) const;
    int getDirection(const String& name) const;
    
    bool has(const String& name) const;
    
    size_t getPropertyCount() const { return properties.size(); }
    
    // Serialization
    uint64_t toHash() const;
    static PropertyContainer fromHash(uint64_t hash, const std::vector<BlockProperty>& propertyDefs);
    
private:
    std::unordered_map<String, int> properties;
};

// ============================================
// Voxel Shape (Collision)
// ============================================

class VoxelShape {
public:
    VoxelShape() = default;
    VoxelShape(const std::vector<AABB>& boxes) : boxes(boxes) {}
    
    static VoxelShape fullCube() {
        return VoxelShape({ AABB(0, 0, 0, 1, 1, 1) });
    }
    
    static VoxelShape empty() {
        return VoxelShape({});
    }
    
    static VoxelShape cube(float x1, float y1, float z1, float x2, float y2, float z2) {
        return VoxelShape({ AABB(x1/16.0f, y1/16.0f, z1/16.0f, x2/16.0f, y2/16.0f, z2/16.0f) });
    }
    
    const std::vector<AABB>& getBoxes() const { return boxes; }
    bool isEmpty() const { return boxes.empty(); }
    
    VoxelShape offset(float dx, float dy, float dz) const;
    VoxelShape combine(const VoxelShape& other) const;
    
private:
    std::vector<AABB> boxes;
};

// ============================================
// Block Behavior Interface
// ============================================

class IBlockBehavior {
public:
    virtual ~IBlockBehavior() = default;
    
    // Placement and removal
    virtual void onPlace(World& world, const BlockPos& pos, BlockState state) {}
    virtual void onRemove(World& world, const BlockPos& pos, BlockState state) {}
    virtual bool canPlaceAt(World& world, const BlockPos& pos, BlockState state) { return true; }
    virtual bool canSurvive(World& world, const BlockPos& pos, BlockState state) { return true; }
    
    // Player interaction
    virtual void onUse(World& world, Player& player, const BlockPos& pos, BlockState state) {}
    virtual void onAttack(Player& player, const BlockPos& pos, BlockState state) {}
    virtual void onProjectileHit(World& world, const BlockPos& pos, Entity& projectile, BlockState state) {}
    
    // Block updates
    virtual void onNeighborUpdate(World& world, const BlockPos& pos, const BlockPos& neighborPos, BlockState state) {}
    virtual void onRandomTick(World& world, const BlockPos& pos, BlockState state) {}
    virtual void onScheduledTick(World& world, const BlockPos& pos, BlockState state) {}
    
    // Collision and shapes
    virtual VoxelShape getCollisionShape(BlockState state) const;
    virtual VoxelShape getOutlineShape(BlockState state) const;
    virtual VoxelShape getRaycastShape(BlockState state) const;
    
    // Lighting
    virtual int getLightEmission(BlockState state) const { return 0; }
    virtual int getLightOpacity(BlockState state) const { return 15; }
    virtual bool propagatesSkylightDown(BlockState state) const { return false; }
    
    // Redstone (will be expanded in redstone system)
    virtual int getWeakPower(BlockState state) const { return 0; }
    virtual int getStrongPower(BlockState state) const { return 0; }
    virtual bool canProvidePower(BlockState state) const { return false; }
    
    // Movement
    virtual float getSlipperiness(BlockState state) const { return 0.6f; }
    virtual float getVelocityMultiplier(BlockState state) const { return 1.0f; }
    virtual float getJumpVelocityMultiplier(BlockState state) const { return 1.0f; }
    
    // Fluids
    virtual bool isLiquid(BlockState state) const { return false; }
    virtual int getFluidLevel(BlockState state) const { return 0; }
    
    // Drops
    virtual std::vector<ItemStack> getDrops(World& world, const BlockPos& pos, BlockState state, 
                                            const Player* breaker, ToolType tool) const;
};

// ============================================
// Block Definition
// ============================================

struct BlockDefinition {
    String id;                          // e.g., "minecraft:stone"
    String name;                        // Display name
    Material material;
    float hardness = 1.0f;
    float blastResistance = 0.0f;
    float slipperiness = 0.6f;
    
    bool opaque = true;
    bool solid = true;
    bool replaceable = false;
    bool requiresTool = false;
    bool hasBlockEntity = false;
    
    ToolType requiredTool = ToolType::None;
    ToolTier minimumTier = ToolTier::None;
    RenderType renderType = RenderType::Solid;
    
    int lightEmission = 0;
    int lightOpacity = 15;
    
    BlockSoundGroup sounds;
    VoxelShape collisionShape = VoxelShape::fullCube();
    VoxelShape outlineShape = VoxelShape::fullCube();
    
    std::vector<BlockProperty> properties;
    std::unique_ptr<IBlockBehavior> behavior;
    
    // Loot table
    String lootTable;
    
    // Default state properties
    PropertyContainer defaultProperties;
    
    // State variants (pre-computed for performance)
    std::vector<BlockState> stateVariants;
};

// ============================================
// Block State (Forward Declaration)
// ============================================

class BlockState {
public:
    BlockState() : blockId(AIR_BLOCK), data(0) {}
    BlockState(BlockID id, uint64_t propertyHash = 0) 
        : blockId(id), propertyHash(propertyHash) {}
    
    BlockID getBlockId() const { return blockId; }
    uint64_t getPropertyHash() const { return propertyHash; }
    
    bool isAir() const { return blockId == AIR_BLOCK; }
    bool is(BlockID id) const { return blockId == id; }
    
    bool operator==(const BlockState& other) const {
        return blockId == other.blockId && propertyHash == other.propertyHash;
    }
    
    bool operator!=(const BlockState& other) const {
        return !(*this == other);
    }
    
    // Property access (requires BlockRegistry lookup)
    bool getBool(const String& name) const;
    int getInt(const String& name) const;
    String getEnum(const String& name) const;
    
    BlockState withProperty(const String& name, bool value) const;
    BlockState withProperty(const String& name, int value) const;
    BlockState withProperty(const String& name, const String& value) const;
    
    // Get block definition
    const BlockDefinition& getDefinition() const;
    
    // Convenience methods
    bool isOpaque() const;
    bool isSolid() const;
    int getLightEmission() const;
    int getLightOpacity() const;
    VoxelShape getCollisionShape() const;
    RenderType getRenderType() const;
    Material getMaterial() const;

private:
    BlockID blockId;
    uint64_t propertyHash = 0;
    uint16_t stateIndex = 0; // Index into state variants for fast lookup
};

// ============================================
// Block Registry
// ============================================

class BlockRegistry {
public:
    static BlockRegistry& get();
    
    // Registration
    BlockID registerBlock(const String& id, BlockDefinition definition);
    BlockID registerBlock(const String& id); // Simple registration with defaults
    
    // Lookup
    BlockID getBlockId(const String& id) const;
    const BlockDefinition& getDefinition(BlockID id) const;
    const BlockDefinition& getDefinition(const String& id) const;
    BlockState getDefaultState(BlockID id) const;
    BlockState getDefaultState(const String& id) const;
    
    // State management
    BlockState getState(BlockID id, const PropertyContainer& properties) const;
    const std::vector<BlockState>& getAllStates(BlockID id) const;
    std::vector<BlockState> getAllStates() const;
    
    // Iteration
    size_t getBlockCount() const { return blocks.size(); }
    auto begin() const { return blocks.begin(); }
    auto end() const { return blocks.end(); }
    
    // Built-in blocks registration
    void registerVanillaBlocks();
    
private:
    BlockRegistry();
    ~BlockRegistry() = default;
    
    std::vector<BlockDefinition> blocks;
    std::unordered_map<String, BlockID> idToIndex;
    std::unordered_map<BlockID, std::vector<BlockState>> blockStates;
    
    BlockID nextId = 1; // 0 is reserved for air
};

// ============================================
// Convenience Macros for Block Registration
// ============================================

#define VF_REGISTER_BLOCK(id, definition) \
    BlockRegistry::get().registerBlock(id, definition)

// ============================================
// Inline Implementations
// ============================================

inline const BlockDefinition& BlockState::getDefinition() const {
    return BlockRegistry::get().getDefinition(blockId);
}

inline bool BlockState::isOpaque() const {
    return getDefinition().opaque;
}

inline bool BlockState::isSolid() const {
    return getDefinition().solid;
}

inline int BlockState::getLightEmission() const {
    return getDefinition().lightEmission;
}

inline int BlockState::getLightOpacity() const {
    return getDefinition().lightOpacity;
}

inline VoxelShape BlockState::getCollisionShape() const {
    return getDefinition().collisionShape;
}

inline RenderType BlockState::getRenderType() const {
    return getDefinition().renderType;
}

inline Material BlockState::getMaterial() const {
    return getDefinition().material;
}

} // namespace VoxelForge

// Hash function for BlockState
namespace std {
    template<>
    struct hash<VoxelForge::BlockState> {
        size_t operator()(const VoxelForge::BlockState& state) const {
            return hash<uint64_t>()(state.getBlockId()) ^ 
                   (hash<uint64_t>()(state.getPropertyHash()) << 1);
        }
    };
}
