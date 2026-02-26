/**
 * @file Camera.hpp
 * @brief Camera system for 3D rendering
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace VoxelForge {

class Camera {
public:
    enum class ProjectionType {
        Perspective,
        Orthographic
    };
    
    Camera();
    ~Camera() = default;
    
    // Position
    void setPosition(const glm::vec3& pos);
    void setPosition(float x, float y, float z);
    const glm::vec3& getPosition() const { return position; }
    
    // Rotation (Euler angles in degrees)
    void setRotation(const glm::vec3& rot);
    void setRotation(float pitch, float yaw, float roll = 0.0f);
    const glm::vec3& getRotation() const { return rotation; }
    
    // Direction vectors
    glm::vec3 getForward() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const;
    
    // Look at target
    void lookAt(const glm::vec3& target);
    void lookAt(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up = {0, 1, 0});
    
    // Movement
    void move(const glm::vec3& offset);
    void moveForward(float distance);
    void moveRight(float distance);
    void moveUp(float distance);
    
    // Rotation
    void rotate(float pitch, float yaw);
    void rotateYaw(float angle);
    void rotatePitch(float angle);
    
    // Projection
    void setProjectionType(ProjectionType type) { projectionType = type; dirty = true; }
    ProjectionType getProjectionType() const { return projectionType; }
    
    void setPerspective(float fov, float aspectRatio, float nearPlane, float farPlane);
    void setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
    
    float getFOV() const { return fov; }
    void setFOV(float f) { fov = f; dirty = true; }
    
    float getAspectRatio() const { return aspectRatio; }
    void setAspectRatio(float ar) { aspectRatio = ar; dirty = true; }
    
    float getNearPlane() const { return nearPlane; }
    float getFarPlane() const { return farPlane; }
    
    // Matrices
    const glm::mat4& getViewMatrix();
    const glm::mat4& getProjectionMatrix();
    const glm::mat4& getViewProjectionMatrix();
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    
    // Frustum
    void updateFrustum();
    bool isInFrustum(const glm::vec3& point) const;
    bool isInFrustum(const glm::vec3& min, const glm::vec3& max) const;
    
    // Clipping
    void setClipPlane(const glm::vec4& plane) { clipPlane = plane; useClipPlane = true; dirty = true; }
    void disableClipPlane() { useClipPlane = false; dirty = true; }
    
private:
    void updateViewMatrix();
    void updateProjectionMatrix();
    
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);  // pitch, yaw, roll in degrees
    
    ProjectionType projectionType = ProjectionType::Perspective;
    
    // Perspective
    float fov = 70.0f;
    float aspectRatio = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    
    // Orthographic
    float orthoLeft = -10.0f;
    float orthoRight = 10.0f;
    float orthoBottom = -10.0f;
    float orthoTop = 10.0f;
    
    // Matrices
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    glm::mat4 viewProjectionMatrix = glm::mat4(1.0f);
    
    // Frustum planes
    glm::vec4 frustumPlanes[6];
    
    // Clipping
    glm::vec4 clipPlane = glm::vec4(0.0f);
    bool useClipPlane = false;
    
    bool dirty = true;
};

// Player camera with first-person controls
class PlayerCamera : public Camera {
public:
    PlayerCamera();
    
    void setMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }
    float getMouseSensitivity() const { return mouseSensitivity; }
    
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void processKeyboard(float deltaTime, bool forward, bool backward, bool left, bool right, bool jump, bool sneak);
    
    void setSpeed(float speed) { movementSpeed = speed; }
    float getSpeed() const { return movementSpeed; }
    
private:
    float mouseSensitivity = 0.1f;
    float movementSpeed = 5.0f;
};

} // namespace VoxelForge
