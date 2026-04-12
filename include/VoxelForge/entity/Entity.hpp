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
struct InventoryComponent;
struct PlayerComponent;
class PlayerInventory;

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

// ============================================
// Entity Systems
// ============================================

class LivingEntitySystem {
public:
    LivingEntitySystem();
    
    void update(ECSWorld& world, float deltaTime);
    void applyEffectModifiers(LivingComponent& living, float deltaTime);
    void onDeath(ECSWorld& world, EntityID entity, LivingComponent& living, EntityBaseComponent& base);
    float calculateDamage(LivingComponent& living, float baseDamage, int type); // DamageType enum from Player.hpp
    void heal(LivingComponent& living, float amount);
    void damage(ECSWorld& world, EntityID entity, LivingComponent& living, float amount, int type); // DamageType enum from Player.hpp
};

class MobAISystem {
public:
    MobAISystem();
    
    void update(ECSWorld& world, float deltaTime);
    
    // AI state updates
    void updateIdle(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateWander(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateFollow(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateFlee(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateAttack(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateEat(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateSleep(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateBreed(AIComponent& ai, MovementComponent& movement, float deltaTime);
    void updateWork(AIComponent& ai, MovementComponent& movement, float deltaTime);
};

class ProjectileSystem {
public:
    ProjectileSystem();
    void update(ECSWorld& world, float deltaTime);
    void onHitBlock(ECSWorld& world, EntityID entity, ProjectileComponent& projectile, const BlockPos& pos);
    void onHitEntity(ECSWorld& world, EntityID projectileEntity, ProjectileComponent& projectile, EntityID target, LivingComponent& targetLiving);
};

class ItemEntitySystem {
public:
    ItemEntitySystem();
    void update(ECSWorld& world, float deltaTime);
    bool canPickup(const ItemEntityComponent& item, EntityID player);
    void onPickup(ECSWorld& world, EntityID itemEntity, ItemEntityComponent& item, EntityID player, PlayerComponent& playerComp);
    int addToInventory(PlayerInventory& inventory, ItemStack& stack);
    void mergeItems(ECSWorld& world, EntityID entity1, ItemEntityComponent& item1, EntityID entity2, ItemEntityComponent& item2);
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

// Mob factory helpers
namespace MobFactory {
    EntityID createZombie(ECSWorld& world, const Vec3& position);
    EntityID createSkeleton(ECSWorld& world, const Vec3& position);
    EntityID createCreeper(ECSWorld& world, const Vec3& position);
    EntityID createSpider(ECSWorld& world, const Vec3& position);
    EntityID createEnderman(ECSWorld& world, const Vec3& position);
    EntityID createBlaze(ECSWorld& world, const Vec3& position);
    EntityID createSlime(ECSWorld& world, const Vec3& position, int size);
}

// Projectile factory helpers
namespace ProjectileFactory {
    EntityID createArrow(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner);
    EntityID createSpectralArrow(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner);
    EntityID createSnowball(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner);
    EntityID createEgg(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner);
    EntityID createEnderPearl(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner);
    EntityID createFireball(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner);
    EntityID createSmallFireball(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner);
    EntityID createDragonFireball(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner);
    EntityID createWitherSkull(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner);
    EntityID createShulkerBullet(ECSWorld& world, const Vec3& position, EntityID owner, EntityID target);
    EntityID createFishingBobber(ECSWorld& world, const Vec3& position, EntityID owner);
    EntityID createLlamaSpit(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner);
    EntityID createTrident(ECSWorld& world, const Vec3& position, const Vec3& velocity, EntityID owner);
}

} // namespace VoxelForge
