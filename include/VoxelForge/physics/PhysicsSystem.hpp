/**
 * @file PhysicsSystem.hpp
 * @brief PhysX-based physics system
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

// Forward declarations for PhysX
namespace physx {
    class PxPhysics;
    class PxFoundation;
    class PxScene;
    class PxDefaultCpuDispatcher;
    class PxMaterial;
    class PxRigidActor;
    class PxRigidDynamic;
    class PxRigidStatic;
    class PxShape;
    class PxControllerManager;
    class PxController;
    class PxCooking;
    struct PxFilterData;
}

namespace VoxelForge {

class World;
class Entity;
class Chunk;

// Physics settings
struct PhysicsSettings {
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f * 20.0f, 0.0f);  // Minecraft gravity
    uint32_t cpuThreads = 2;
    float fixedTimeStep = 1.0f / 60.0f;
    float maxTimeStep = 1.0f / 30.0f;
    bool enableCCD = false;  // Continuous collision detection
    bool enableGPUPhysics = false;
};

// Collision groups
enum class CollisionGroup : uint16_t {
    None = 0,
    Terrain = 1 << 0,
    Player = 1 << 1,
    Mob = 1 << 2,
    Item = 1 << 3,
    Projectile = 1 << 4,
    Trigger = 1 << 5,
    All = 0xFFFF
};

// Collision flags
enum class CollisionFlags {
    None = 0,
    Solid = 1 << 0,
    Liquid = 1 << 1,
    Ladder = 1 << 2,
    Fire = 1 << 3,
    Portal = 1 << 4
};

// Physics material
struct PhysicsMaterial {
    float staticFriction = 0.5f;
    float dynamicFriction = 0.5f;
    float restitution = 0.0f;  // Bounciness
    
    // Presets
    static PhysicsMaterial Stone() { return {0.6f, 0.5f, 0.0f}; }
    static PhysicsMaterial Dirt() { return {0.5f, 0.4f, 0.0f}; }
    static PhysicsMaterial Ice() { return {0.1f, 0.05f, 0.1f}; }
    static PhysicsMaterial Slime() { return {0.8f, 0.6f, 0.8f}; }
    static PhysicsMaterial Water() { return {0.0f, 0.0f, 0.0f}; }
};

// Raycast hit result
struct RaycastHit {
    bool hit = false;
    glm::vec3 point;
    glm::vec3 normal;
    float distance;
    uint32_t entityId = 0;
    glm::ivec3 blockPos;
    bool isBlock = false;
    bool isEntity = false;
};

// Collision info
struct CollisionInfo {
    uint32_t otherEntityId;
    glm::vec3 contactPoint;
    glm::vec3 normal;
    float penetration;
};

// Physics actor wrapper
class PhysicsActor {
public:
    PhysicsActor() = default;
    virtual ~PhysicsActor() = default;
    
    virtual void setPosition(const glm::vec3& pos) = 0;
    virtual glm::vec3 getPosition() const = 0;
    
    virtual void setRotation(const glm::quat& rot) = 0;
    virtual glm::quat getRotation() const = 0;
    
    virtual void setLinearVelocity(const glm::vec3& vel) = 0;
    virtual glm::vec3 getLinearVelocity() const = 0;
    
    virtual void setAngularVelocity(const glm::vec3& vel) = 0;
    virtual glm::vec3 getAngularVelocity() const = 0;
    
    virtual void addForce(const glm::vec3& force) = 0;
    virtual void addImpulse(const glm::vec3& impulse) = 0;
    virtual void addTorque(const glm::vec3& torque) = 0;
    
    virtual void setGravityEnabled(bool enabled) = 0;
    virtual bool isGravityEnabled() const = 0;
    
    virtual void setCollisionGroup(CollisionGroup group) = 0;
    virtual CollisionGroup getCollisionGroup() const = 0;
    
    virtual void setCollisionMask(uint16_t mask) = 0;
    
    virtual physx::PxRigidActor* getActor() const = 0;
    
    // Callbacks
    std::function<void(const CollisionInfo&)> onCollisionEnter;
    std::function<void(const CollisionInfo&)> onCollisionExit;
    std::function<void(const CollisionInfo&)> onTriggerEnter;
    std::function<void(const CollisionInfo&)> onTriggerExit;
};

// Dynamic actor (can move)
class DynamicActor : public PhysicsActor {
public:
    DynamicActor();
    ~DynamicActor() override;
    
    void init(class PhysicsSystem* system, const glm::vec3& pos, 
              const glm::vec3& halfExtents, float mass = 1.0f);
    void cleanup();
    
    void setPosition(const glm::vec3& pos) override;
    glm::vec3 getPosition() const override;
    
    void setRotation(const glm::quat& rot) override;
    glm::quat getRotation() const override;
    
    void setLinearVelocity(const glm::vec3& vel) override;
    glm::vec3 getLinearVelocity() const override;
    
    void setAngularVelocity(const glm::vec3& vel) override;
    glm::vec3 getAngularVelocity() const override;
    
    void addForce(const glm::vec3& force) override;
    void addImpulse(const glm::vec3& impulse) override;
    void addTorque(const glm::vec3& torque) override;
    
    void setGravityEnabled(bool enabled) override;
    bool isGravityEnabled() const override;
    
    void setCollisionGroup(CollisionGroup group) override;
    CollisionGroup getCollisionGroup() const override;
    
    void setCollisionMask(uint16_t mask) override;
    
    void setMass(float mass);
    float getMass() const;
    
    void setKinematic(bool kinematic);
    bool isKinematic() const;
    
    void lockRotation(bool lockX, bool lockY, bool lockZ);
    
    physx::PxRigidActor* getActor() const override;
    physx::PxRigidDynamic* getDynamicActor() const { return rigidBody; }
    
private:
    physx::PxRigidDynamic* rigidBody = nullptr;
    physx::PxShape* shape = nullptr;
    CollisionGroup collisionGroup = CollisionGroup::None;
};

// Static actor (cannot move)
class StaticActor : public PhysicsActor {
public:
    StaticActor();
    ~StaticActor() override;
    
    void init(class PhysicsSystem* system, const glm::vec3& pos,
              const glm::vec3& halfExtents);
    void cleanup();
    
    void setPosition(const glm::vec3& pos) override;
    glm::vec3 getPosition() const override;
    
    void setRotation(const glm::quat& rot) override;
    glm::quat getRotation() const override;
    
    void setLinearVelocity(const glm::vec3& vel) override { /* no-op */ }
    glm::vec3 getLinearVelocity() const override { return glm::vec3(0.0f); }
    
    void setAngularVelocity(const glm::vec3& vel) override { /* no-op */ }
    glm::vec3 getAngularVelocity() const override { return glm::vec3(0.0f); }
    
    void addForce(const glm::vec3& force) override { /* no-op */ }
    void addImpulse(const glm::vec3& impulse) override { /* no-op */ }
    void addTorque(const glm::vec3& torque) override { /* no-op */ }
    
    void setGravityEnabled(bool enabled) override { /* no-op */ }
    bool isGravityEnabled() const override { return false; }
    
    void setCollisionGroup(CollisionGroup group) override;
    CollisionGroup getCollisionGroup() const override;
    
    void setCollisionMask(uint16_t mask) override;
    
    physx::PxRigidActor* getActor() const override;
    
private:
    physx::PxRigidStatic* rigidBody = nullptr;
    physx::PxShape* shape = nullptr;
    CollisionGroup collisionGroup = CollisionGroup::None;
};

// Main physics system
class PhysicsSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();
    
    // No copy
    PhysicsSystem(const PhysicsSystem&) = delete;
    PhysicsSystem& operator=(const PhysicsSystem&) = delete;
    
    void init(const PhysicsSettings& settings = {});
    void shutdown();
    
    // Simulation
    void simulate(float deltaTime);
    void fetchResults();
    void step(float deltaTime);  // Combined simulate + fetch
    
    // Actor creation
    std::unique_ptr<DynamicActor> createDynamicActor(
        const glm::vec3& pos, const glm::vec3& halfExtents, float mass = 1.0f);
    std::unique_ptr<StaticActor> createStaticActor(
        const glm::vec3& pos, const glm::vec3& halfExtents);
    
    // Terrain physics
    void addChunkCollider(Chunk* chunk);
    void removeChunkCollider(const glm::ivec3& chunkPos);
    void updateChunkCollider(Chunk* chunk);
    
    // Queries
    RaycastHit raycast(const glm::vec3& origin, const glm::vec3& direction, 
                       float maxDistance = 100.0f,
                       uint16_t collisionMask = static_cast<uint16_t>(CollisionGroup::All));
    std::vector<RaycastHit> raycastAll(const glm::vec3& origin, const glm::vec3& direction,
                                        float maxDistance = 100.0f,
                                        uint16_t collisionMask = static_cast<uint16_t>(CollisionGroup::All));
    bool overlapSphere(const glm::vec3& center, float radius,
                       std::vector<uint32_t>& outEntities,
                       uint16_t collisionMask = static_cast<uint16_t>(CollisionGroup::All));
    bool overlapBox(const glm::vec3& center, const glm::vec3& halfExtents,
                    std::vector<uint32_t>& outEntities,
                    uint16_t collisionMask = static_cast<uint16_t>(CollisionGroup::All));
    
    // Character controllers
    class CharacterController* createCharacterController(
        const glm::vec3& pos, float height, float radius);
    void destroyCharacterController(CharacterController* controller);
    
    // Materials
    physx::PxMaterial* createMaterial(const PhysicsMaterial& material);
    physx::PxMaterial* getDefaultMaterial() const { return defaultMaterial; }
    
    // Settings
    void setGravity(const glm::vec3& gravity);
    glm::vec3 getGravity() const;
    
    // Getters
    physx::PxPhysics* getPhysics() const { return physics; }
    physx::PxScene* getScene() const { return scene; }
    physx::PxCooking* getCooking() const { return cooking; }
    const PhysicsSettings& getSettings() const { return settings; }
    
    // Collision callbacks
    void setCollisionCallback(std::function<void(uint32_t, uint32_t, const CollisionInfo&)> callback);
    
private:
    void setupFiltering(physx::PxShape* shape, CollisionGroup group, uint16_t mask);
    
    PhysicsSettings settings;
    
    // PhysX objects
    physx::PxFoundation* foundation = nullptr;
    physx::PxPhysics* physics = nullptr;
    physx::PxScene* scene = nullptr;
    physx::PxDefaultCpuDispatcher* dispatcher = nullptr;
    physx::PxMaterial* defaultMaterial = nullptr;
    physx::PxControllerManager* controllerManager = nullptr;
    physx::PxCooking* cooking = nullptr;
    
    // Chunk colliders
    struct ChunkCollider {
        physx::PxRigidStatic* actor = nullptr;
        std::vector<physx::PxShape*> shapes;
    };
    std::unordered_map<glm::ivec3, ChunkCollider, glm::ivec3Hash> chunkColliders;
    
    // Collision callback
    std::function<void(uint32_t, uint32_t, const CollisionInfo&)> collisionCallback;
    
    // Time accumulator for fixed timestep
    float timeAccumulator = 0.0f;
};

} // namespace VoxelForge
