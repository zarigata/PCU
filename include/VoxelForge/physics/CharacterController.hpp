/**
 * @file CharacterController.hpp
 * @brief Character controller for player and mob physics
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>

namespace physx {
    class PxController;
    class PxControllerManager;
    struct PxControllerCollisionFlags;
}

namespace VoxelForge {

class PhysicsSystem;

// Character controller settings
struct CharacterControllerSettings {
    float height = 1.8f;           // Standing height
    float radius = 0.3f;           // Capsule radius
    float crouchHeight = 1.5f;     // Crouching height
    float slopeLimit = 45.0f;      // Max walkable slope in degrees
    float stepOffset = 0.6f;       // Max step height
    float jumpForce = 9.0f;        // Jump velocity
    float moveSpeed = 4.3f;        // Walk speed (m/s)
    float sprintMultiplier = 1.5f;
    float airControl = 0.3f;       // Movement control in air (0-1)
    float groundFriction = 10.0f;
    float airFriction = 0.5f;
};

// Character state
struct CharacterState {
    bool isGrounded = false;
    bool isCrouching = false;
    bool isSprinting = false;
    bool isJumping = false;
    bool isOnSlope = false;
    bool isTouchingCeiling = false;
    bool isTouchingWall = false;
    float groundDistance = 0.0f;
    glm::vec3 groundNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
};

// Character collision flags
enum class CharacterCollisionFlags {
    None = 0,
    Sides = 1 << 0,
    Above = 1 << 1,
    Below = 1 << 2
};

// Character controller for player-like movement
class CharacterController {
public:
    CharacterController();
    ~CharacterController();
    
    // No copy
    CharacterController(const CharacterController&) = delete;
    CharacterController& operator=(const CharacterController&) = delete;
    
    void init(PhysicsSystem* system, const glm::vec3& pos,
              const CharacterControllerSettings& settings = {});
    void cleanup();
    
    // Movement
    void move(const glm::vec3& displacement, float deltaTime);
    void jump();
    void jump(float force);
    
    // Position
    void setPosition(const glm::vec3& pos);
    glm::vec3 getPosition() const;
    glm::vec3 getFootPosition() const;  // Bottom of capsule
    
    // Velocity
    void setVelocity(const glm::vec3& vel);
    glm::vec3 getVelocity() const;
    void addVelocity(const glm::vec3& vel);
    
    // State
    const CharacterState& getState() const { return state; }
    bool isGrounded() const { return state.isGrounded; }
    bool isCrouching() const { return state.isCrouching; }
    
    // Crouch
    void setCrouch(bool crouch);
    void toggleCrouch();
    
    // Sprint
    void setSprint(bool sprint);
    void toggleSprint();
    
    // Resize
    void resize(float height);
    float getHeight() const;
    float getRadius() const;
    
    // Settings
    void updateSettings(const CharacterControllerSettings& newSettings);
    const CharacterControllerSettings& getSettings() const { return settings; }
    
    // Collision filtering
    void setCollisionGroup(uint16_t group);
    void setCollisionMask(uint16_t mask);
    
    // Physics system access
    physx::PxController* getController() const { return controller; }
    
    // Update (call each frame)
    void update(float deltaTime);
    
    // External forces
    void applyForce(const glm::vec3& force);
    void applyImpulse(const glm::vec3& impulse);
    
    // Teleport (no collision check)
    void teleport(const glm::vec3& pos);
    
    // Ground check
    bool checkGround(float maxDistance = 0.1f);
    
    // Knockback
    void applyKnockback(const glm::vec3& direction, float strength);
    
private:
    void updateGroundState();
    void applyGravity(float deltaTime);
    void applyFriction(float deltaTime);
    void integrateVelocity(float deltaTime);
    
    PhysicsSystem* physicsSystem = nullptr;
    physx::PxController* controller = nullptr;
    
    CharacterControllerSettings settings;
    CharacterState state;
    
    glm::vec3 targetVelocity = glm::vec3(0.0f);
    glm::vec3 pendingDisplacement = glm::vec3(0.0f);
    bool jumpRequested = false;
    float jumpForce = 0.0f;
    
    float currentHeight;
};

// Character controller manager for multiple characters
class CharacterControllerManager {
public:
    CharacterControllerManager();
    ~CharacterControllerManager();
    
    void init(PhysicsSystem* system);
    void cleanup();
    
    CharacterController* createController(const glm::vec3& pos, float height, float radius);
    void destroyController(CharacterController* controller);
    
    void updateAll(float deltaTime);
    
    size_t getControllerCount() const { return controllers.size(); }
    
private:
    PhysicsSystem* physicsSystem = nullptr;
    std::vector<std::unique_ptr<CharacterController>> controllers;
};

} // namespace VoxelForge
