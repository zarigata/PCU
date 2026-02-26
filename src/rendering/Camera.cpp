/**
 * @file Camera.cpp
 * @brief Camera implementation
 */

#include <VoxelForge/rendering/Camera.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace VoxelForge {

Camera::Camera() {
    updateViewMatrix();
    updateProjectionMatrix();
}

void Camera::setPosition(const glm::vec3& pos) {
    position = pos;
    dirty = true;
}

void Camera::setPosition(float x, float y, float z) {
    setPosition(glm::vec3(x, y, z));
}

void Camera::setRotation(const glm::vec3& rot) {
    rotation = rot;
    dirty = true;
}

void Camera::setRotation(float pitch, float yaw, float roll) {
    setRotation(glm::vec3(pitch, yaw, roll));
}

glm::vec3 Camera::getForward() const {
    float yaw = glm::radians(rotation.y);
    float pitch = glm::radians(rotation.x);
    
    return glm::normalize(glm::vec3(
        std::cos(yaw) * std::cos(pitch),
        std::sin(pitch),
        std::sin(yaw) * std::cos(pitch)
    ));
}

glm::vec3 Camera::getRight() const {
    return glm::normalize(glm::cross(getForward(), glm::vec3(0, 1, 0)));
}

glm::vec3 Camera::getUp() const {
    return glm::normalize(glm::cross(getRight(), getForward()));
}

void Camera::lookAt(const glm::vec3& target) {
    lookAt(position, target);
}

void Camera::lookAt(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up) {
    position = eye;
    
    glm::vec3 forward = glm::normalize(target - eye);
    rotation.x = glm::degrees(std::asin(forward.y));
    rotation.y = glm::degrees(std::atan2(forward.z, forward.x));
    
    dirty = true;
}

void Camera::move(const glm::vec3& offset) {
    position += offset;
    dirty = true;
}

void Camera::moveForward(float distance) {
    move(getForward() * distance);
}

void Camera::moveRight(float distance) {
    move(getRight() * distance);
}

void Camera::moveUp(float distance) {
    move(glm::vec3(0, 1, 0) * distance);
}

void Camera::rotate(float pitch, float yaw) {
    rotation.x += pitch;
    rotation.y += yaw;
    dirty = true;
}

void Camera::rotateYaw(float angle) {
    rotation.y += angle;
    dirty = true;
}

void Camera::rotatePitch(float angle) {
    rotation.x += angle;
    dirty = true;
}

void Camera::setPerspective(float fov, float aspectRatio, float nearPlane, float farPlane) {
    this->fov = fov;
    this->aspectRatio = aspectRatio;
    this->nearPlane = nearPlane;
    this->farPlane = farPlane;
    projectionType = ProjectionType::Perspective;
    dirty = true;
}

void Camera::setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
    orthoLeft = left;
    orthoRight = right;
    orthoBottom = bottom;
    orthoTop = top;
    this->nearPlane = nearPlane;
    this->farPlane = farPlane;
    projectionType = ProjectionType::Orthographic;
    dirty = true;
}

const glm::mat4& Camera::getViewMatrix() {
    if (dirty) {
        updateViewMatrix();
    }
    return viewMatrix;
}

const glm::mat4& Camera::getProjectionMatrix() {
    if (dirty) {
        updateProjectionMatrix();
    }
    return projectionMatrix;
}

const glm::mat4& Camera::getViewProjectionMatrix() {
    if (dirty) {
        updateViewMatrix();
        updateProjectionMatrix();
        viewProjectionMatrix = projectionMatrix * viewMatrix;
    }
    return viewProjectionMatrix;
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + getForward(), glm::vec3(0, 1, 0));
}

glm::mat4 Camera::getProjectionMatrix() const {
    if (projectionType == ProjectionType::Perspective) {
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    } else {
        return glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, nearPlane, farPlane);
    }
}

void Camera::updateViewMatrix() {
    viewMatrix = glm::lookAt(position, position + getForward(), glm::vec3(0, 1, 0));
}

void Camera::updateProjectionMatrix() {
    if (projectionType == ProjectionType::Perspective) {
        projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    } else {
        projectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, nearPlane, farPlane);
    }
    
    updateFrustum();
}

void Camera::updateFrustum() {
    glm::mat4 vp = projectionMatrix * viewMatrix;
    
    // Extract frustum planes
    // Left
    frustumPlanes[0] = glm::vec4(
        vp[0][3] + vp[0][0],
        vp[1][3] + vp[1][0],
        vp[2][3] + vp[2][0],
        vp[3][3] + vp[3][0]
    );
    
    // Right
    frustumPlanes[1] = glm::vec4(
        vp[0][3] - vp[0][0],
        vp[1][3] - vp[1][0],
        vp[2][3] - vp[2][0],
        vp[3][3] - vp[3][0]
    );
    
    // Bottom
    frustumPlanes[2] = glm::vec4(
        vp[0][3] + vp[0][1],
        vp[1][3] + vp[1][1],
        vp[2][3] + vp[2][1],
        vp[3][3] + vp[3][1]
    );
    
    // Top
    frustumPlanes[3] = glm::vec4(
        vp[0][3] - vp[0][1],
        vp[1][3] - vp[1][1],
        vp[2][3] - vp[2][1],
        vp[3][3] - vp[3][1]
    );
    
    // Near
    frustumPlanes[4] = glm::vec4(
        vp[0][3] + vp[0][2],
        vp[1][3] + vp[1][2],
        vp[2][3] + vp[2][2],
        vp[3][3] + vp[3][2]
    );
    
    // Far
    frustumPlanes[5] = glm::vec4(
        vp[0][3] - vp[0][2],
        vp[1][3] - vp[1][2],
        vp[2][3] - vp[2][2],
        vp[3][3] - vp[3][2]
    );
    
    // Normalize planes
    for (int i = 0; i < 6; i++) {
        float length = std::sqrt(
            frustumPlanes[i].x * frustumPlanes[i].x +
            frustumPlanes[i].y * frustumPlanes[i].y +
            frustumPlanes[i].z * frustumPlanes[i].z
        );
        if (length > 0) {
            frustumPlanes[i] /= length;
        }
    }
}

bool Camera::isInFrustum(const glm::vec3& point) const {
    for (int i = 0; i < 6; i++) {
        float distance = frustumPlanes[i].x * point.x +
                        frustumPlanes[i].y * point.y +
                        frustumPlanes[i].z * point.z +
                        frustumPlanes[i].w;
        if (distance < 0) return false;
    }
    return true;
}

bool Camera::isInFrustum(const glm::vec3& min, const glm::vec3& max) const {
    // Check all 8 corners
    glm::vec3 corners[8] = {
        {min.x, min.y, min.z},
        {max.x, min.y, min.z},
        {min.x, max.y, min.z},
        {max.x, max.y, min.z},
        {min.x, min.y, max.z},
        {max.x, min.y, max.z},
        {min.x, max.y, max.z},
        {max.x, max.y, max.z}
    };
    
    for (int i = 0; i < 6; i++) {
        int out = 0;
        for (int j = 0; j < 8; j++) {
            float distance = frustumPlanes[i].x * corners[j].x +
                            frustumPlanes[i].y * corners[j].y +
                            frustumPlanes[i].z * corners[j].z +
                            frustumPlanes[i].w;
            if (distance < 0) out++;
        }
        if (out == 8) return false;
    }
    
    return true;
}

// ============================================
// Player Camera
// ============================================

PlayerCamera::PlayerCamera() : Camera() {
}

void PlayerCamera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;
    
    rotation.y += xoffset;
    rotation.x -= yoffset;
    
    // Constrain pitch
    if (constrainPitch) {
        if (rotation.x > 89.0f) rotation.x = 89.0f;
        if (rotation.x < -89.0f) rotation.x = -89.0f;
    }
    
    dirty = true;
}

void PlayerCamera::processKeyboard(float deltaTime, bool forward, bool backward, bool left, bool right, bool jump, bool sneak) {
    float velocity = movementSpeed * deltaTime;
    
    if (forward)  move(getForward() * velocity);
    if (backward) move(-getForward() * velocity);
    if (left)     move(-getRight() * velocity);
    if (right)    move(getRight() * velocity);
    if (jump)     moveUp(velocity);
    if (sneak)    moveUp(-velocity);
}

} // namespace VoxelForge
