/**
 * @file PhysicsSystem.cpp
 * @brief PhysX-based physics system implementation
 */

#include <VoxelForge/physics/PhysicsSystem.hpp>
#include <VoxelForge/physics/CharacterController.hpp>
#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/core/Logger.hpp>

// TODO: PhysX headers need to be added to repository
// PhysX includes commented out:
// #include <PxPhysicsAPI.h>
// #include <cooking/PxCooking.h>

// using namespace physx;

namespace VoxelForge {

// Custom error callback
class VoxelForgeErrorCallback : public PxErrorCallback {
public:
    void reportError(PxErrorCode::Enum code, const char* message, 
                     const char* file, int line) override {
        Logger::error("PhysX Error ({}): {} at {}:{}", 
                      static_cast<int>(code), message, file, line);
    }
};

// Custom allocator
class VoxelForgeAllocator : public PxAllocatorCallback {
public:
    void* allocate(size_t size, const char* typeName, const char* filename, 
                   int line) override {
        void* ptr = aligned_alloc(16, size);
        if (!ptr) {
            throw std::bad_alloc();
        }
        return ptr;
    }
    
    void deallocate(void* ptr) override {
        if (ptr) {
            aligned_free(ptr);
        }
    }
};

// Custom simulation filter shader
PxFilterFlags VoxelForgeFilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    // Check if groups should collide
    if (!(filterData0.word0 & filterData1.word1) ||
        !(filterData1.word0 & filterData0.word1)) {
        return PxFilterFlag::eKILL;
    }
    
    // Let triggers through
    if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1)) {
        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;
    }
    
    // Generate contacts
    pairFlags = PxPairFlag::eCONTACT_DEFAULT;
    pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
    pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;
    
    return PxFilterFlag::eDEFAULT;
}

// Static callbacks
static VoxelForgeErrorCallback gErrorCallback;
static VoxelForgeAllocator gAllocator;

// ============== PhysicsSystem ==============

PhysicsSystem::PhysicsSystem() = default;

PhysicsSystem::~PhysicsSystem() {
    shutdown();
}

void PhysicsSystem::init(const PhysicsSettings& settings) {
    // TODO: PhysX implementation not available - headers not in repository
    VF_ERROR("PhysicsSystem not implemented - PhysX headers not available");
    (void)settings; // Suppress unused warning
}

void PhysicsSystem::shutdown() {
    // TODO: PhysX implementation not available
    chunkColliders.clear();
    characterControllers.clear();
}

void PhysicsSystem::shutdown() {
    // Clean up chunk colliders
    for (auto& [pos, collider] : chunkColliders) {
        for (auto* shape : collider.shapes) {
            if (shape) collider.actor->detachShape(*shape);
        }
        if (collider.actor) {
            collider.actor->release();
        }
    }
    chunkColliders.clear();
    
    // Clean up controller manager
    if (controllerManager) {
        controllerManager->purgeControllers();
        controllerManager->release();
        controllerManager = nullptr;
    }
    
    // Clean up PhysX
    if (defaultMaterial) {
        defaultMaterial->release();
        defaultMaterial = nullptr;
    }
    
    if (scene) {
        scene->release();
        scene = nullptr;
    }
    
    if (dispatcher) {
        dispatcher->release();
        dispatcher = nullptr;
    }
    
    if (cooking) {
        cooking->release();
        cooking = nullptr;
    }
    
    if (physics) {
        physics->release();
        physics = nullptr;
    }
    
    if (foundation) {
        foundation->release();
        foundation = nullptr;
    }
}

void PhysicsSystem::simulate(float deltaTime) {
    if (!scene) return;
    
    // Use fixed timestep with accumulator
    timeAccumulator += deltaTime;
    
    while (timeAccumulator >= settings.fixedTimeStep) {
        scene->simulate(settings.fixedTimeStep);
        fetchResults();
        timeAccumulator -= settings.fixedTimeStep;
    }
}

void PhysicsSystem::simulate(float deltaTime) {
    // TODO: PhysX implementation not available
    (void)deltaTime;
}

void PhysicsSystem::fetchResults() {
    // TODO: PhysX implementation not available
}

void PhysicsSystem::step(float deltaTime) {
    simulate(deltaTime);
    fetchResults();
}

std::unique_ptr<DynamicActor> PhysicsSystem::createDynamicActor(
    const glm::vec3& pos, const glm::vec3& halfExtents, float mass) {
    VF_ERROR("PhysicsSystem::createDynamicActor not implemented - PhysX not available");
    (void)pos; (void)halfExtents; (void)mass;
    return nullptr;
}

std::unique_ptr<StaticActor> PhysicsSystem::createStaticActor(
    const glm::vec3& pos, const glm::vec3& halfExtents) {
    VF_ERROR("PhysicsSystem::createStaticActor not implemented - PhysX not available");
    (void)pos; (void)halfExtents;
    return nullptr;
}

void PhysicsSystem::addChunkCollider(Chunk* chunk) {
    if (!chunk) return;
    
    glm::ivec3 chunkPos = chunk->getPosition();
    
    // Remove existing if any
    removeChunkCollider(chunkPos);
    
    ChunkCollider collider;
    
    // Create static actor for chunk
    PxTransform transform(PxVec3(chunkPos.x * 16 + 8, chunkPos.y * 16 + 8, chunkPos.z * 16 + 8));
    collider.actor = physics->createRigidStatic(transform);
    
    // Generate collision shapes from solid blocks
    // For efficiency, we use a simplified approach with larger boxes
    // A full implementation would use triangle meshes or heightfields
    
    // Group adjacent solid blocks into larger boxes
    std::vector<std::pair<glm::ivec3, glm::ivec3>> boxRegions;
    
    // Simple greedy meshing for collision
    // TODO: Implement proper greedy meshing
    
    // For now, create individual box shapes for each solid block
    // This is not efficient but works for testing
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            for (int z = 0; z < 16; z++) {
                // Check if block is solid
                // if (chunk->getBlock(x, y, z).isSolid()) {
                //     PxVec3 localPos(x - 8 + 0.5f, y - 8 + 0.5f, z - 8 + 0.5f);
                //     PxShape* shape = physics->createShape(
                //         PxBoxGeometry(0.5f, 0.5f, 0.5f), *defaultMaterial);
                //     PxTransform localTransform(localPos);
                //     collider.actor->attachShape(*shape);
                //     collider.shapes.push_back(shape);
                // }
            }
        }
    }
    
    if (collider.shapes.empty()) {
        collider.actor->release();
        return;
    }
    
    scene->addActor(*collider.actor);
    chunkColliders[chunkPos] = std::move(collider);
}

void PhysicsSystem::removeChunkCollider(const glm::ivec3& chunkPos) {
    VF_TRACE("PhysicsSystem::removeChunkCollider not implemented");
    (void)chunkPos;
}

void PhysicsSystem::updateChunkCollider(Chunk* chunk) {
    VF_TRACE("PhysicsSystem::updateChunkCollider not implemented");
    (void)chunk;
}

RaycastHit PhysicsSystem::raycast(const glm::vec3& origin, const glm::vec3& direction,
                                   float maxDistance, uint16_t collisionMask) {
    VF_TRACE("PhysicsSystem::raycast not implemented");
    RaycastHit result;
    result.hit = false;
    (void)origin; (void)direction; (void)maxDistance; (void)collisionMask;
    return result;
}

std::vector<RaycastHit> PhysicsSystem::raycastAll(const glm::vec3& origin, const glm::vec3& direction,
                                                   float maxDistance, uint16_t collisionMask) {
    std::vector<RaycastHit> results;
    
    PxVec3 pxOrigin(origin.x, origin.y, origin.z);
    PxVec3 pxDir(direction.x, direction.y, direction.z);
    pxDir.normalize();
    
    PxRaycastBuffer hit;
    bool success = scene->raycast(pxOrigin, pxDir, maxDistance, hit, 
                                  PxHitFlag::eDEFAULT | PxHitFlag::eMESH_MULTIPLE);
    
    if (success) {
        for (PxU32 i = 0; i < hit.nbTouches; i++) {
            RaycastHit result;
            result.hit = true;
            result.point = glm::vec3(hit.touches[i].position.x, hit.touches[i].position.y, 
                                     hit.touches[i].position.z);
            result.normal = glm::vec3(hit.touches[i].normal.x, hit.touches[i].normal.y, 
                                      hit.touches[i].normal.z);
            result.distance = hit.touches[i].distance;
            results.push_back(result);
        }
    }
    
    return results;
}

bool PhysicsSystem::overlapSphere(const glm::vec3& center, float radius,
                                   std::vector<uint32_t>& outEntities, uint16_t collisionMask) {
    VF_TRACE("PhysicsSystem::overlapSphere not implemented");
    (void)center; (void)radius; (void)outEntities; (void)collisionMask;
    return false;
}

bool PhysicsSystem::overlapBox(const glm::vec3& center, const glm::vec3& halfExtents,
                                std::vector<uint32_t>& outEntities, uint16_t collisionMask) {
    VF_TRACE("PhysicsSystem::overlapBox not implemented");
    (void)center; (void)halfExtents; (void)outEntities; (void)collisionMask;
    return false;
}

CharacterController* PhysicsSystem::createCharacterController(
    const glm::vec3& pos, float height, float radius) {
    
    auto controller = new CharacterController();
    CharacterControllerSettings settings;
    settings.height = height;
    settings.radius = radius;
    controller->init(this, pos, settings);
    return controller;
}

void PhysicsSystem::destroyCharacterController(CharacterController* controller) {
    if (controller) {
        controller->cleanup();
        delete controller;
    }
}

PxMaterial* PhysicsSystem::createMaterial(const PhysicsMaterial& material) {
    return physics->createMaterial(material.staticFriction, 
                                   material.dynamicFriction,
                                   material.restitution);
}

void PhysicsSystem::setGravity(const glm::vec3& gravity) {
    settings.gravity = gravity;
    // TODO: PhysX scene->setGravity not available
    // if (scene) {
    //     scene->setGravity(PxVec3(gravity.x, gravity.y, gravity.z));
    // }
}

glm::vec3 PhysicsSystem::getGravity() const {
    return settings.gravity;
}

void PhysicsSystem::setCollisionCallback(
    std::function<void(uint32_t, uint32_t, const CollisionInfo&)> callback) {
    // TODO: PhysX collisionCallback not available
    // collisionCallback = std::move(callback);
    (void)callback;
}

void PhysicsSystem::setupFiltering(PxShape* shape, CollisionGroup group, uint16_t mask) {
    // TODO: PhysX setupFiltering not available
    // PxFilterData filterData;
    // filterData.word0 = static_cast<uint32_t>(group);
    // filterData.word1 = mask;
    // shape->setSimulationFilterData(filterData);
    (void)shape; (void)group; (void)mask;
}

// ============== DynamicActor ==============

DynamicActor::DynamicActor() = default;

DynamicActor::~DynamicActor() {
    cleanup();
}

void DynamicActor::init(PhysicsSystem* system, const glm::vec3& pos,
                        const glm::vec3& halfExtents, float mass) {
    auto* physics = system->getPhysics();
    auto* material = system->getDefaultMaterial();
    
    PxTransform transform(PxVec3(pos.x, pos.y, pos.z));
    rigidBody = physics->createRigidDynamic(transform);
    
    PxBoxGeometry geometry(halfExtents.x, halfExtents.y, halfExtents.z);
    shape = physics->createShape(geometry, *material);
    rigidBody->attachShape(*shape);
    
    PxRigidBodyExt::setMassAndUpdateInertia(*rigidBody, mass);
    
    system->getScene()->addActor(*rigidBody);
}

void DynamicActor::cleanup() {
    if (rigidBody) {
        if (rigidBody->getScene()) {
            rigidBody->getScene()->removeActor(*rigidBody);
        }
        rigidBody->release();
        rigidBody = nullptr;
    }
}

void DynamicActor::setPosition(const glm::vec3& pos) {
    if (rigidBody) {
        PxTransform transform = rigidBody->getGlobalPose();
        transform.p = PxVec3(pos.x, pos.y, pos.z);
        rigidBody->setGlobalPose(transform);
    }
}

glm::vec3 DynamicActor::getPosition() const {
    if (rigidBody) {
        PxVec3 pos = rigidBody->getGlobalPose().p;
        return glm::vec3(pos.x, pos.y, pos.z);
    }
    return glm::vec3(0.0f);
}

void DynamicActor::setRotation(const glm::quat& rot) {
    if (rigidBody) {
        PxTransform transform = rigidBody->getGlobalPose();
        transform.q = PxQuat(rot.x, rot.y, rot.z, rot.w);
        rigidBody->setGlobalPose(transform);
    }
}

glm::quat DynamicActor::getRotation() const {
    if (rigidBody) {
        PxQuat q = rigidBody->getGlobalPose().q;
        return glm::quat(q.w, q.x, q.y, q.z);
    }
    return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

void DynamicActor::setLinearVelocity(const glm::vec3& vel) {
    if (rigidBody) {
        rigidBody->setLinearVelocity(PxVec3(vel.x, vel.y, vel.z));
    }
}

glm::vec3 DynamicActor::getLinearVelocity() const {
    if (rigidBody) {
        PxVec3 vel = rigidBody->getLinearVelocity();
        return glm::vec3(vel.x, vel.y, vel.z);
    }
    return glm::vec3(0.0f);
}

void DynamicActor::setAngularVelocity(const glm::vec3& vel) {
    if (rigidBody) {
        rigidBody->setAngularVelocity(PxVec3(vel.x, vel.y, vel.z));
    }
}

glm::vec3 DynamicActor::getAngularVelocity() const {
    if (rigidBody) {
        PxVec3 vel = rigidBody->getAngularVelocity();
        return glm::vec3(vel.x, vel.y, vel.z);
    }
    return glm::vec3(0.0f);
}

void DynamicActor::addForce(const glm::vec3& force) {
    if (rigidBody) {
        rigidBody->addForce(PxVec3(force.x, force.y, force.z));
    }
}

void DynamicActor::addImpulse(const glm::vec3& impulse) {
    if (rigidBody) {
        rigidBody->addForce(PxVec3(impulse.x, impulse.y, impulse.z), 
                           PxForceMode::eIMPULSE);
    }
}

void DynamicActor::addTorque(const glm::vec3& torque) {
    if (rigidBody) {
        rigidBody->addTorque(PxVec3(torque.x, torque.y, torque.z));
    }
}

void DynamicActor::setGravityEnabled(bool enabled) {
    if (rigidBody) {
        rigidBody->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !enabled);
    }
}

bool DynamicActor::isGravityEnabled() const {
    if (rigidBody) {
        return !rigidBody->getActorFlags().isSet(PxActorFlag::eDISABLE_GRAVITY);
    }
    return false;
}

void DynamicActor::setCollisionGroup(CollisionGroup group) {
    collisionGroup = group;
}

CollisionGroup DynamicActor::getCollisionGroup() const {
    return collisionGroup;
}

void DynamicActor::setCollisionMask(uint16_t mask) {
    // Update filter data
}

void DynamicActor::setMass(float mass) {
    if (rigidBody) {
        PxRigidBodyExt::setMassAndUpdateInertia(*rigidBody, mass);
    }
}

float DynamicActor::getMass() const {
    if (rigidBody) {
        return rigidBody->getMass();
    }
    return 0.0f;
}

void DynamicActor::setKinematic(bool kinematic) {
    if (rigidBody) {
        rigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);
    }
}

bool DynamicActor::isKinematic() const {
    if (rigidBody) {
        return rigidBody->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC);
    }
    return false;
}

void DynamicActor::lockRotation(bool lockX, bool lockY, bool lockZ) {
    if (rigidBody) {
        rigidBody->setRigidDynamicLockFlags(
            (lockX ? PxRigidDynamicLockFlag::eLOCK_ANGULAR_X : PxRigidDynamicLockFlags{}) |
            (lockY ? PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y : PxRigidDynamicLockFlags{}) |
            (lockZ ? PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z : PxRigidDynamicLockFlags{})
        );
    }
}

PxRigidActor* DynamicActor::getActor() const {
    return rigidBody;
}

// ============== StaticActor ==============

StaticActor::StaticActor() = default;

StaticActor::~StaticActor() {
    cleanup();
}

void StaticActor::init(PhysicsSystem* system, const glm::vec3& pos,
                       const glm::vec3& halfExtents) {
    auto* physics = system->getPhysics();
    auto* material = system->getDefaultMaterial();
    
    PxTransform transform(PxVec3(pos.x, pos.y, pos.z));
    rigidBody = physics->createRigidStatic(transform);
    
    PxBoxGeometry geometry(halfExtents.x, halfExtents.y, halfExtents.z);
    shape = physics->createShape(geometry, *material);
    rigidBody->attachShape(*shape);
    
    system->getScene()->addActor(*rigidBody);
}

void StaticActor::cleanup() {
    if (rigidBody) {
        if (rigidBody->getScene()) {
            rigidBody->getScene()->removeActor(*rigidBody);
        }
        rigidBody->release();
        rigidBody = nullptr;
    }
}

void StaticActor::setPosition(const glm::vec3& pos) {
    if (rigidBody) {
        PxTransform transform = rigidBody->getGlobalPose();
        transform.p = PxVec3(pos.x, pos.y, pos.z);
        rigidBody->setGlobalPose(transform);
    }
}

glm::vec3 StaticActor::getPosition() const {
    if (rigidBody) {
        PxVec3 pos = rigidBody->getGlobalPose().p;
        return glm::vec3(pos.x, pos.y, pos.z);
    }
    return glm::vec3(0.0f);
}

void StaticActor::setRotation(const glm::quat& rot) {
    if (rigidBody) {
        PxTransform transform = rigidBody->getGlobalPose();
        transform.q = PxQuat(rot.x, rot.y, rot.z, rot.w);
        rigidBody->setGlobalPose(transform);
    }
}

glm::quat StaticActor::getRotation() const {
    if (rigidBody) {
        PxQuat q = rigidBody->getGlobalPose().q;
        return glm::quat(q.w, q.x, q.y, q.z);
    }
    return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

void StaticActor::setCollisionGroup(CollisionGroup group) {
    collisionGroup = group;
}

CollisionGroup StaticActor::getCollisionGroup() const {
    return collisionGroup;
}

void StaticActor::setCollisionMask(uint16_t mask) {
    // Update filter data
}

PxRigidActor* StaticActor::getActor() const {
    return rigidBody;
}

} // namespace VoxelForge
