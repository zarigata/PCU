/**
 * @file Entity.hpp
 * @brief Entity base classes
 */

#pragma once

#include <VoxelForge/core/ECS.hpp>
#include <VoxelForge/world/Block.hpp>
#include <cstdint>
#include <string>

namespace VoxelForge {

// Forward declarations
struct ItemStack;

// EntityID types
enum class EntityType : uint8_t {
    Generic,
    Player,
    Mob,
    Animal,
    Monster,
    Projectile,
    Item,
    Vehicle,
    BlockEntity
};

// EntityID tags for behavior
enum class EntityTag : uint32_t {
    None = 0,
    CanSwim = 1 << 0,
    CanFly = 1 << 1,
    CanClimb = 1 << 2,
    OnFire = 1 << 3,
    Wet = 1 << 4,
    Invisible = 1 << 5,
    Invulnerable = 1 << 6,
    NoGravity = 1 << 7,
    NoClip = 1 << 8,
    Silent = 1 << 9
};

// Components

struct EntityBaseComponent {
    UUID uuid;
    uint32_t networkId = 0;
    EntityType type = EntityType::Generic;
    uint32_t tags = 0;
    bool isAlive = true;
    bool isOnGround = false;
    bool isInWater = false;
    bool isInLava = false;
    int ticksAlive = 0;
};

struct LivingComponent {
    float health = 20.0f;
    float maxHealth = 20.0f;
    float armor = 0.0f;
    float armorToughness = 0.0f;
    
    // Status effects
    struct ActiveEffect {
        enum class Type {
            Speed, Slowness, Haste, MiningFatigue,
            Strength, InstantHealth, InstantDamage,
            JumpBoost, Nausea, Regeneration,
            Resistance, FireResistance, WaterBreathing,
            Invisibility, Blindness, NightVision,
            Hunger, Weakness, Poison,
            Wither, HealthBoost, Absorption,
            Saturation, Glowing, Levitation, Luck,
            BadLuck, SlowFalling, ConduitPower,
            DolphinsGrace, BadOmen, HeroOfTheVillage,
            Darkness
        } type;
        int duration;  // In ticks
        int amplifier;
    };
    
    std::vector<ActiveEffect> effects;
    
    bool hasEffect(ActiveEffect::Type type) const;
    void addEffect(ActiveEffect::Type type, int duration, int amplifier);
    void removeEffect(ActiveEffect::Type type);
    void tickEffects();
};

struct AgeComponent {
    int age = 0;      // For baby animals
    int maxAge = 0;
    bool isBaby = false;
    int breedingCooldown = 0;
    int inLove = 0;
};

struct OwnerComponent {
    EntityID owner = INVALID_ENTITY;
    UUID ownerUUID;
};

struct TargetComponent {
    EntityID target = INVALID_ENTITY;
    BlockPos targetPos;
    float targetDistance = 0.0f;
};

struct MovementComponent {
    float moveForward = 0.0f;
    float moveStrafe = 0.0f;
    float jumpStrength = 0.0f;
    bool wantsToJump = false;
    bool isSprinting = false;
    bool isSneaking = false;
    bool isSwimming = false;
};

struct AIComponent {
    // AI brain state
    enum class State {
        Idle,
        Wander,
        Follow,
        Flee,
        Attack,
        Eat,
        Sleep,
        Breed,
        Work
    };
    
    State currentState = State::Idle;
    int stateTimer = 0;
    BlockPos targetBlock;
    
    // Memory
    std::vector<BlockPos> path;
    BlockPos lastKnownTargetPos;
    float lastKnownTargetTime = 0.0f;
};

struct CollisionComponent {
    float width = 0.6f;
    float height = 1.8f;
    float eyeHeight = 1.62f;
    bool isColliding = false;
    BlockPos collidingBlock;
};

struct ProjectileComponent {
    EntityID owner = INVALID_ENTITY;
    bool inGround = false;
    int inGroundTime = 0;
    float damage = 0.0f;
    bool pierce = false;
    int pierceLevel = 0;
};

struct ItemEntityComponent {
    std::shared_ptr<ItemStack> stack;
    int pickupDelay = 0;
    int lifespan = 6000;  // 5 minutes in ticks
    float hoverStart = 0.0f;
};

// EntityID creation helpers
namespace EntityFactory {
    EntityID createPlayer(ECSWorld& world, const Vec3& position);
    EntityID createItem(ECSWorld& world, const Vec3& position, const ItemStack& stack);
    EntityID createZombie(ECSWorld& world, const Vec3& position);
    EntityID createSkeleton(ECSWorld& world, const Vec3& position);
    EntityID createCreeper(ECSWorld& world, const Vec3& position);
    EntityID createCow(ECSWorld& world, const Vec3& position);
    EntityID createPig(ECSWorld& world, const Vec3& position);
    EntityID createSheep(ECSWorld& world, const Vec3& position);
    EntityID createChicken(ECSWorld& world, const Vec3& position);
    EntityID createArrow(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner);
    EntityID createSnowball(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner);
}

} // namespace VoxelForge
