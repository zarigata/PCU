/**
 * @file MobBase.cpp
 * @brief Base mob AI implementation
 */

#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <random>

namespace VoxelForge {

MobAISystem::MobAISystem() {
    LOG_INFO("MobAISystem created");
}

void MobAISystem::update(ECSWorld& world, float deltaTime) {
    auto view = world.view<AIComponent, MovementComponent, EntityBaseComponent>();
    
    for (auto entity : view) {
        auto& ai = view.get<AIComponent>(entity);
        auto& movement = view.get<MovementComponent>(entity);
        auto& base = view.get<EntityBaseComponent>(entity);
        
        if (!base.isAlive) continue;
        
        // Update AI state timer
        if (ai.stateTimer > 0) {
            ai.stateTimer -= static_cast<int>(deltaTime * 20.0f);
        }
        
        // Execute current AI state
        switch (ai.currentState) {
            case AIComponent::State::Idle:
                updateIdle(ai, movement, deltaTime);
                break;
            case AIComponent::State::Wander:
                updateWander(ai, movement, deltaTime);
                break;
            case AIComponent::State::Follow:
                updateFollow(ai, movement, deltaTime);
                break;
            case AIComponent::State::Flee:
                updateFlee(ai, movement, deltaTime);
                break;
            case AIComponent::State::Attack:
                updateAttack(ai, movement, deltaTime);
                break;
            case AIComponent::State::Eat:
                updateEat(ai, movement, deltaTime);
                break;
            case AIComponent::State::Sleep:
                updateSleep(ai, movement, deltaTime);
                break;
            case AIComponent::State::Breed:
                updateBreed(ai, movement, deltaTime);
                break;
            case AIComponent::State::Work:
                updateWork(ai, movement, deltaTime);
                break;
        }
    }
}

void MobAISystem::updateIdle(AIComponent& ai, MovementComponent& movement, float deltaTime) {
    movement.moveForward = 0.0f;
    movement.moveStrafe = 0.0f;
    
    // Random chance to start wandering
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1, 100);
    
    if (dis(gen) <= 1) { // 1% chance per tick
        ai.currentState = AIComponent::State::Wander;
        ai.stateTimer = 60 + dis(gen) % 60; // 3-6 seconds
    }
}

void MobAISystem::updateWander(AIComponent& ai, MovementComponent& movement, float deltaTime) {
    if (ai.stateTimer <= 0) {
        ai.currentState = AIComponent::State::Idle;
        movement.moveForward = 0.0f;
        movement.moveStrafe = 0.0f;
        return;
    }
    
    // Random movement direction
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(-1.0, 1.0);
    
    if (ai.stateTimer % 20 == 0) { // Change direction every second
        movement.moveForward = static_cast<float>(dis(gen));
        movement.moveStrafe = static_cast<float>(dis(gen));
    }
}

void MobAISystem::updateFollow(AIComponent& ai, MovementComponent& movement, float deltaTime) {
    // Move towards target
    // TODO: Implement pathfinding
}

void MobAISystem::updateFlee(AIComponent& ai, MovementComponent& movement, float deltaTime) {
    // Move away from threat
    // TODO: Implement flee behavior
}

void MobAISystem::updateAttack(AIComponent& ai, MovementComponent& movement, float deltaTime) {
    // Attack target
    // TODO: Implement attack behavior
}

void MobAISystem::updateEat(AIComponent& ai, MovementComponent& movement, float deltaTime) {
    // Eating animation and logic
    movement.moveForward = 0.0f;
    movement.moveStrafe = 0.0f;
}

void MobAISystem::updateSleep(AIComponent& ai, MovementComponent& movement, float deltaTime) {
    // Sleeping logic
    movement.moveForward = 0.0f;
    movement.moveStrafe = 0.0f;
}

void MobAISystem::updateBreed(AIComponent& ai, MovementComponent& movement, float deltaTime) {
    // Breeding behavior
    // TODO: Implement breeding
}

void MobAISystem::updateWork(AIComponent& ai, MovementComponent& movement, float deltaTime) {
    // Villager work behavior
    // TODO: Implement work behavior
}

// ============================================================================
// Mob Factory Functions
// ============================================================================

namespace MobFactory {

Entity createZombie(ECSWorld& world, const Vec3& position) {
    return EntityFactory::createZombie(world, position);
}

Entity createSkeleton(ECSWorld& world, const Vec3& position) {
    return EntityFactory::createSkeleton(world, position);
}

Entity createCreeper(ECSWorld& world, const Vec3& position) {
    return EntityFactory::createCreeper(world, position);
}

Entity createSpider(ECSWorld& world, const Vec3& position) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Monster;
    base.isAlive = true;
    
    auto& living = world.addComponent<LivingComponent>(entity);
    living.health = 16.0f;
    living.maxHealth = 16.0f;
    
    world.addComponent<MovementComponent>(entity);
    world.addComponent<AIComponent>(entity);
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 1.4f;
    collision.height = 0.9f;
    collision.eyeHeight = 0.65f;
    
    return entity;
}

Entity createEnderman(ECSWorld& world, const Vec3& position) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Monster;
    base.isAlive = true;
    
    auto& living = world.addComponent<LivingComponent>(entity);
    living.health = 40.0f;
    living.maxHealth = 40.0f;
    
    world.addComponent<MovementComponent>(entity);
    world.addComponent<AIComponent>(entity);
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.6f;
    collision.height = 2.9f;
    collision.eyeHeight = 2.55f;
    
    return entity;
}

Entity createBlaze(ECSWorld& world, const Vec3& position) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Monster;
    base.isAlive = true;
    base.tags |= static_cast<uint32_t>(EntityTag::CanFly);
    
    auto& living = world.addComponent<LivingComponent>(entity);
    living.health = 20.0f;
    living.maxHealth = 20.0f;
    
    world.addComponent<MovementComponent>(entity);
    world.addComponent<AIComponent>(entity);
    
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = 0.6f;
    collision.height = 1.8f;
    collision.eyeHeight = 1.53f;
    
    return entity;
}

Entity createSlime(ECSWorld& world, const Vec3& position, int size) {
    Entity entity = world.createEntity();
    
    auto& base = world.addComponent<EntityBaseComponent>(entity);
    base.type = EntityType::Monster;
    base.isAlive = true;
    
    float health = size * size; // Size 1 = 1 HP, size 2 = 4 HP, size 4 = 16 HP
    auto& living = world.addComponent<LivingComponent>(entity);
    living.health = health;
    living.maxHealth = health;
    
    world.addComponent<MovementComponent>(entity);
    world.addComponent<AIComponent>(entity);
    
    float dimension = 0.51f * size;
    auto& collision = world.addComponent<CollisionComponent>(entity);
    collision.width = dimension;
    collision.height = dimension;
    collision.eyeHeight = dimension * 0.8f;
    
    return entity;
}

} // namespace MobFactory

} // namespace VoxelForge
