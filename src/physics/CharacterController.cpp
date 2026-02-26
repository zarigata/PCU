/**
 * @file CharacterController.cpp
 * @brief Character controller implementation
 */

#include "CharacterController.hpp"
#include "PhysicsSystem.hpp"
#include <VoxelForge/core/Logger.hpp>

#include <PxPhysicsAPI.h>
#include <characterkinematic/PxControllerManager.h>
#include <characterkinematic/PxCapsuleController.h>

using namespace physx;

namespace VoxelForge {

CharacterController::CharacterController() = default;

CharacterController::~CharacterController() {
    cleanup();
}

void CharacterController::init(PhysicsSystem* system, const glm::vec3& pos,
                                const CharacterControllerSettings& settings) {
    this->physicsSystem = system;
    this->settings = settings;
    this->currentHeight = settings.height;
    
    auto* manager = system->getPhysics()->getControllerManager();
    
    PxCapsuleControllerDesc desc;
    desc.position = PxExtendedVec3(pos.x, pos.y, pos.z);
    desc.height = settings.height - 2.0f * settings.radius;  // Cylinder part
    desc.radius = settings.radius;
    desc.climbingMode = PxCapsuleClimbingMode::eEASY;
    desc.material = system->getDefaultMaterial();
    desc.stepOffset = settings.stepOffset;
    desc.slopeLimit = glm::radians(settings.slopeLimit);
    desc.contactOffset = 0.01f;
    
    controller = static_cast<PxCapsuleController*>(manager->createController(desc));
    
    if (!controller) {
        Logger::error("Failed to create character controller");
        return;
    }
    
    Logger::debug("CharacterController created at ({}, {}, {})", pos.x, pos.y, pos.z);
}

void CharacterController::cleanup() {
    if (controller) {
        controller->release();
        controller = nullptr;
    }
    physicsSystem = nullptr;
}

void CharacterController::move(const glm::vec3& displacement, float deltaTime) {
    if (!controller) return;
    
    // Accumulate displacement
    pendingDisplacement += displacement;
    
    // Set target velocity based on displacement
    if (deltaTime > 0.0f) {
        targetVelocity = displacement / deltaTime;
    }
}

void CharacterController::jump() {
    jump(settings.jumpForce);
}

void CharacterController::jump(float force) {
    if (!controller || !state.isGrounded) return;
    
    jumpRequested = true;
    jumpForce = force;
}

void CharacterController::setPosition(const glm::vec3& pos) {
    if (controller) {
        controller->setPosition(PxExtendedVec3(pos.x, pos.y, pos.z));
    }
}

glm::vec3 CharacterController::getPosition() const {
    if (controller) {
        auto pos = controller->getPosition();
        return glm::vec3(static_cast<float>(pos.x), 
                        static_cast<float>(pos.y), 
                        static_cast<float>(pos.z));
    }
    return glm::vec3(0.0f);
}

glm::vec3 CharacterController::getFootPosition() const {
    glm::vec3 pos = getPosition();
    pos.y -= currentHeight / 2.0f;
    return pos;
}

void CharacterController::setVelocity(const glm::vec3& vel) {
    state.velocity = vel;
    targetVelocity = vel;
}

glm::vec3 CharacterController::getVelocity() const {
    return state.velocity;
}

void CharacterController::addVelocity(const glm::vec3& vel) {
    state.velocity += vel;
}

void CharacterController::setCrouch(bool crouch) {
    if (state.isCrouching == crouch) return;
    
    state.isCrouching = crouch;
    
    if (crouch) {
        resize(settings.crouchHeight);
    } else {
        // Check if we can stand up
        float oldHeight = currentHeight;
        resize(settings.height);
        // If collision, go back to crouching
        // (PhysX handles this with overlap check)
    }
}

void CharacterController::toggleCrouch() {
    setCrouch(!state.isCrouching);
}

void CharacterController::setSprint(bool sprint) {
    state.isSprinting = sprint;
}

void CharacterController::toggleSprint() {
    setSprint(!state.isSprinting);
}

void CharacterController::resize(float height) {
    if (controller) {
        currentHeight = height;
        float cylinderHeight = height - 2.0f * settings.radius;
        controller->resize(cylinderHeight);
    }
}

float CharacterController::getHeight() const {
    return currentHeight;
}

float CharacterController::getRadius() const {
    return settings.radius;
}

void CharacterController::updateSettings(const CharacterControllerSettings& newSettings) {
    settings = newSettings;
    
    if (controller) {
        controller->setStepOffset(newSettings.stepOffset);
        controller->setSlopeLimit(glm::radians(newSettings.slopeLimit));
        resize(newSettings.height);
    }
}

void CharacterController::setCollisionGroup(uint16_t group) {
    // Update collision filter
}

void CharacterController::setCollisionMask(uint16_t mask) {
    // Update collision filter
}

void CharacterController::update(float deltaTime) {
    if (!controller || !physicsSystem) return;
    
    // Apply gravity
    applyGravity(deltaTime);
    
    // Apply friction
    applyFriction(deltaTime);
    
    // Integrate velocity
    integrateVelocity(deltaTime);
    
    // Handle jump
    if (jumpRequested && state.isGrounded) {
        state.velocity.y = jumpForce;
        state.isJumping = true;
        jumpRequested = false;
    }
    
    // Move controller
    PxVec3 disp(state.velocity.x * deltaTime, 
                state.velocity.y * deltaTime, 
                state.velocity.z * deltaTime);
    
    PxControllerCollisionFlags flags = controller->move(disp, 0.001f, deltaTime, nullptr);
    
    // Update ground state
    state.isGrounded = flags & PxControllerCollisionFlag::eCOLLISION_DOWN;
    state.isTouchingCeiling = flags & PxControllerCollisionFlag::eCOLLISION_UP;
    state.isTouchingWall = flags & PxControllerCollisionFlag::eCOLLISION_SIDES;
    
    // Reset jump state when grounded
    if (state.isGrounded) {
        state.isJumping = false;
    }
    
    // Reset pending displacement
    pendingDisplacement = glm::vec3(0.0f);
}

void CharacterController::applyForce(const glm::vec3& force) {
    // Convert force to velocity change (F = ma, a = F/m, v = a*dt)
    // Assuming mass of 1 for simplicity
    state.velocity += force * 0.01f;  // Scale factor
}

void CharacterController::applyImpulse(const glm::vec3& impulse) {
    state.velocity += impulse;
}

void CharacterController::teleport(const glm::vec3& pos) {
    if (controller) {
        controller->setPosition(PxExtendedVec3(pos.x, pos.y, pos.z));
        state.velocity = glm::vec3(0.0f);
    }
}

bool CharacterController::checkGround(float maxDistance) {
    if (!controller) return false;
    
    // Cast ray downward
    PxExtendedVec3 pos = controller->getPosition();
    PxVec3 origin(static_cast<float>(pos.x), 
                  static_cast<float>(pos.y) - currentHeight / 2.0f, 
                  static_cast<float>(pos.z));
    PxVec3 direction(0.0f, -1.0f, 0.0f);
    
    PxRaycastBuffer hit;
    bool result = physicsSystem->getScene()->raycast(origin, direction, maxDistance, hit);
    
    if (result) {
        state.groundDistance = hit.block.distance;
        state.groundNormal = glm::vec3(hit.block.normal.x, hit.block.normal.y, hit.block.normal.z);
        return true;
    }
    
    return false;
}

void CharacterController::applyKnockback(const glm::vec3& direction, float strength) {
    glm::vec3 knockback = glm::normalize(direction) * strength;
    state.velocity += knockback;
}

void CharacterController::updateGroundState() {
    state.isGrounded = checkGround(0.1f);
}

void CharacterController::applyGravity(float deltaTime) {
    if (!state.isGrounded) {
        glm::vec3 gravity = physicsSystem->getGravity();
        state.velocity += gravity * deltaTime;
    }
}

void CharacterController::applyFriction(float deltaTime) {
    float friction = state.isGrounded ? settings.groundFriction : settings.airFriction;
    
    // Apply friction to horizontal velocity
    glm::vec3 horizontalVel(state.velocity.x, 0.0f, state.velocity.z);
    float speed = glm::length(horizontalVel);
    
    if (speed > 0.001f) {
        float drop = friction * deltaTime * speed;
        float newSpeed = std::max(0.0f, speed - drop);
        horizontalVel = horizontalVel * (newSpeed / speed);
        state.velocity.x = horizontalVel.x;
        state.velocity.z = horizontalVel.z;
    }
}

void CharacterController::integrateVelocity(float deltaTime) {
    // Blend target velocity with current velocity
    float control = state.isGrounded ? 1.0f : settings.airControl;
    
    // Apply movement speed
    float moveSpeed = settings.moveSpeed;
    if (state.isSprinting) moveSpeed *= settings.sprintMultiplier;
    if (state.isCrouching) moveSpeed *= 0.3f;
    
    glm::vec3 target = targetVelocity * moveSpeed;
    
    // Smooth interpolation
    float t = control * 10.0f * deltaTime;
    t = std::min(t, 1.0f);
    
    state.velocity.x = glm::mix(state.velocity.x, target.x, t);
    state.velocity.z = glm::mix(state.velocity.z, target.z, t);
}

// ============== CharacterControllerManager ==============

CharacterControllerManager::CharacterControllerManager() = default;

CharacterControllerManager::~CharacterControllerManager() {
    cleanup();
}

void CharacterControllerManager::init(PhysicsSystem* system) {
    physicsSystem = system;
}

void CharacterControllerManager::cleanup() {
    controllers.clear();
    physicsSystem = nullptr;
}

CharacterController* CharacterControllerManager::createController(
    const glm::vec3& pos, float height, float radius) {
    
    auto controller = std::make_unique<CharacterController>();
    CharacterControllerSettings settings;
    settings.height = height;
    settings.radius = radius;
    controller->init(physicsSystem, pos, settings);
    
    auto* ptr = controller.get();
    controllers.push_back(std::move(controller));
    return ptr;
}

void CharacterControllerManager::destroyController(CharacterController* controller) {
    controllers.erase(
        std::remove_if(controllers.begin(), controllers.end(),
            [controller](const std::unique_ptr<CharacterController>& c) {
                return c.get() == controller;
            }),
        controllers.end()
    );
}

void CharacterControllerManager::updateAll(float deltaTime) {
    for (auto& controller : controllers) {
        controller->update(deltaTime);
    }
}

} // namespace VoxelForge
