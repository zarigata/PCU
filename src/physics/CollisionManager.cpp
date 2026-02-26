/**
 * @file CollisionManager.cpp
 * @brief Collision detection and management implementation
 */

#include "CollisionManager.hpp"
#include "PhysicsSystem.hpp"
#include <VoxelForge/core/Logger.hpp>
#include <algorithm>
#include <cmath>

namespace VoxelForge {

// ============== CollisionManager ==============

CollisionManager::CollisionManager() = default;

CollisionManager::~CollisionManager() {
    shutdown();
}

void CollisionManager::init(PhysicsSystem* physics) {
    this->physics = physics;
    
    // Create default layers
    createLayer("Default", 0xFFFF);
    createLayer("Player", static_cast<uint16_t>(CollisionGroup::Terrain) | 
                          static_cast<uint16_t>(CollisionGroup::Mob) |
                          static_cast<uint16_t>(CollisionGroup::Item));
    createLayer("Mob", 0xFFFF);
    createLayer("Item", 0xFFFF);
    createLayer("Projectile", 0xFFFF);
    createLayer("Trigger", 0xFFFF);
    
    Logger::info("CollisionManager initialized with {} layers", layers.size());
}

void CollisionManager::shutdown() {
    entityCollisionData.clear();
    activeCollisions.clear();
    layers.clear();
    layerNameMap.clear();
    physics = nullptr;
}

uint16_t CollisionManager::createLayer(const std::string& name, uint16_t collideWith) {
    uint16_t id = static_cast<uint16_t>(layers.size());
    
    CollisionLayer layer;
    layer.id = id;
    layer.name = name;
    layer.collideWith = collideWith;
    
    layers.push_back(layer);
    layerNameMap[name] = id;
    
    return id;
}

CollisionLayer* CollisionManager::getLayer(uint16_t id) {
    if (id < layers.size()) {
        return &layers[id];
    }
    return nullptr;
}

CollisionLayer* CollisionManager::getLayer(const std::string& name) {
    auto it = layerNameMap.find(name);
    if (it != layerNameMap.end()) {
        return getLayer(it->second);
    }
    return nullptr;
}

void CollisionManager::setLayerCollision(uint16_t layerA, uint16_t layerB, bool canCollide) {
    if (layerA >= layers.size() || layerB >= layers.size()) return;
    
    if (canCollide) {
        layers[layerA].collideWith |= (1 << layerB);
        layers[layerB].collideWith |= (1 << layerA);
    } else {
        layers[layerA].collideWith &= ~(1 << layerB);
        layers[layerB].collideWith &= ~(1 << layerA);
    }
}

void CollisionManager::setEntityLayer(uint32_t entityId, uint16_t layerId) {
    auto it = entityCollisionData.find(entityId);
    if (it != entityCollisionData.end()) {
        it->second.layer = layerId;
    }
}

uint16_t CollisionManager::getEntityLayer(uint32_t entityId) const {
    auto it = entityCollisionData.find(entityId);
    if (it != entityCollisionData.end()) {
        return it->second.layer;
    }
    return 0;
}

void CollisionManager::setEntityShape(uint32_t entityId, const CollisionShape& shape) {
    auto it = entityCollisionData.find(entityId);
    if (it != entityCollisionData.end()) {
        it->second.shape = shape;
    }
}

CollisionShape CollisionManager::getEntityShape(uint32_t entityId) const {
    auto it = entityCollisionData.find(entityId);
    if (it != entityCollisionData.end()) {
        return it->second.shape;
    }
    return {};
}

void CollisionManager::setEntityTrigger(uint32_t entityId, bool isTrigger) {
    auto it = entityCollisionData.find(entityId);
    if (it != entityCollisionData.end()) {
        it->second.isTrigger = isTrigger;
    }
}

bool CollisionManager::isEntityTrigger(uint32_t entityId) const {
    auto it = entityCollisionData.find(entityId);
    if (it != entityCollisionData.end()) {
        return it->second.isTrigger;
    }
    return false;
}

bool CollisionManager::checkCollision(uint32_t entityA, uint32_t entityB) {
    // Check if these two entities are currently colliding
    auto key = entityA < entityB ? 
        std::make_pair(entityA, entityB) : std::make_pair(entityB, entityA);
    return activeCollisions.find(key) != activeCollisions.end();
}

std::vector<uint32_t> CollisionManager::getCollidingEntities(uint32_t entityId) {
    std::vector<uint32_t> result;
    
    for (const auto& [pair, _] : activeCollisions) {
        if (pair.first == entityId) {
            result.push_back(pair.second);
        } else if (pair.second == entityId) {
            result.push_back(pair.first);
        }
    }
    
    return result;
}

std::vector<uint32_t> CollisionManager::getEntitiesInBox(const glm::vec3& min, const glm::vec3& max) {
    std::vector<uint32_t> result;
    
    glm::vec3 center = (min + max) * 0.5f;
    glm::vec3 halfExtents = (max - min) * 0.5f;
    
    if (physics) {
        physics->overlapBox(center, halfExtents, result);
    }
    
    return result;
}

std::vector<uint32_t> CollisionManager::getEntitiesInSphere(const glm::vec3& center, float radius) {
    std::vector<uint32_t> result;
    
    if (physics) {
        physics->overlapSphere(center, radius, result);
    }
    
    return result;
}

std::vector<uint32_t> CollisionManager::getEntitiesInCapsule(
    const glm::vec3& point1, const glm::vec3& point2, float radius) {
    std::vector<uint32_t> result;
    
    // Use sphere checks along the capsule axis
    glm::vec3 dir = point2 - point1;
    float length = glm::length(dir);
    
    if (length < 0.001f) {
        return getEntitiesInSphere(point1, radius);
    }
    
    dir /= length;
    
    // Sample points along capsule
    int samples = static_cast<int>(length / radius) + 1;
    for (int i = 0; i <= samples; i++) {
        float t = static_cast<float>(i) / samples;
        glm::vec3 samplePoint = point1 + dir * (t * length);
        
        auto entities = getEntitiesInSphere(samplePoint, radius);
        for (uint32_t id : entities) {
            if (std::find(result.begin(), result.end(), id) == result.end()) {
                result.push_back(id);
            }
        }
    }
    
    return result;
}

bool CollisionManager::raycast(const glm::vec3& origin, const glm::vec3& direction,
                               float maxDistance, RaycastHit& hit, uint16_t layerMask) {
    if (!physics) return false;
    
    auto result = physics->raycast(origin, direction, maxDistance, layerMask);
    
    if (result.hit) {
        hit.entityId = result.entityId;
        hit.point = result.point;
        hit.normal = result.normal;
        hit.distance = result.distance;
        return true;
    }
    
    return false;
}

std::vector<CollisionManager::RaycastHit> CollisionManager::raycastAll(
    const glm::vec3& origin, const glm::vec3& direction,
    float maxDistance, uint16_t layerMask) {
    
    std::vector<RaycastHit> results;
    
    if (!physics) return results;
    
    auto hits = physics->raycastAll(origin, direction, maxDistance, layerMask);
    
    for (const auto& h : hits) {
        RaycastHit hit;
        hit.entityId = h.entityId;
        hit.point = h.point;
        hit.normal = h.normal;
        hit.distance = h.distance;
        results.push_back(hit);
    }
    
    return results;
}

void CollisionManager::setCollisionCallback(CollisionCallback callback) {
    collisionCallback = std::move(callback);
}

void CollisionManager::onCollision(const CollisionEvent& event) {
    if (collisionCallback) {
        collisionCallback(event);
    }
}

void CollisionManager::update() {
    // Process PhysX collision events
    // This would be called after physics simulation step
}

void CollisionManager::processEvents() {
    // Process accumulated collision events
    // Track enter/exit for active collisions
}

void CollisionManager::setDebugDrawEnabled(bool enabled) {
    debugDrawEnabled = enabled;
}

void CollisionManager::getDebugGeometry(std::vector<glm::vec3>& outLines) {
    if (!debugDrawEnabled) return;
    
    // Generate wireframe boxes for collision shapes
    for (const auto& [id, data] : entityCollisionData) {
        if (data.shape.type == CollisionShapeType::Box) {
            glm::vec3 min = data.position - data.shape.halfExtents;
            glm::vec3 max = data.position + data.shape.halfExtents;
            
            // 12 edges of a box
            outLines.push_back({min.x, min.y, min.z});
            outLines.push_back({max.x, min.y, min.z});
            
            outLines.push_back({max.x, min.y, min.z});
            outLines.push_back({max.x, max.y, min.z});
            
            outLines.push_back({max.x, max.y, min.z});
            outLines.push_back({min.x, max.y, min.z});
            
            outLines.push_back({min.x, max.y, min.z});
            outLines.push_back({min.x, min.y, min.z});
            
            // Top face
            outLines.push_back({min.x, min.y, max.z});
            outLines.push_back({max.x, min.y, max.z});
            
            outLines.push_back({max.x, min.y, max.z});
            outLines.push_back({max.x, max.y, max.z});
            
            outLines.push_back({max.x, max.y, max.z});
            outLines.push_back({min.x, max.y, max.z});
            
            outLines.push_back({min.x, max.y, max.z});
            outLines.push_back({min.x, min.y, max.z});
            
            // Vertical edges
            outLines.push_back({min.x, min.y, min.z});
            outLines.push_back({min.x, min.y, max.z});
            
            outLines.push_back({max.x, min.y, min.z});
            outLines.push_back({max.x, min.y, max.z});
            
            outLines.push_back({max.x, max.y, min.z});
            outLines.push_back({max.x, max.y, max.z});
            
            outLines.push_back({min.x, max.y, min.z});
            outLines.push_back({min.x, max.y, max.z});
        }
    }
}

// ============== CollisionUtils ==============

namespace CollisionUtils {

bool pointInBox(const glm::vec3& point, const glm::vec3& boxMin, const glm::vec3& boxMax) {
    return point.x >= boxMin.x && point.x <= boxMax.x &&
           point.y >= boxMin.y && point.y <= boxMax.y &&
           point.z >= boxMin.z && point.z <= boxMax.z;
}

bool pointInSphere(const glm::vec3& point, const glm::vec3& center, float radius) {
    return glm::length(point - center) <= radius;
}

bool boxBoxIntersect(const glm::vec3& minA, const glm::vec3& maxA,
                     const glm::vec3& minB, const glm::vec3& maxB) {
    return minA.x <= maxB.x && maxA.x >= minB.x &&
           minA.y <= maxB.y && maxA.y >= minB.y &&
           minA.z <= maxB.z && maxA.z >= minB.z;
}

bool sphereSphereIntersect(const glm::vec3& centerA, float radiusA,
                           const glm::vec3& centerB, float radiusB) {
    float distance = glm::length(centerA - centerB);
    return distance <= (radiusA + radiusB);
}

bool rayBoxIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                     const glm::vec3& boxMin, const glm::vec3& boxMax,
                     float& tMin, float& tMax) {
    tMin = 0.0f;
    tMax = std::numeric_limits<float>::max();
    
    for (int i = 0; i < 3; i++) {
        float invD = 1.0f / (&rayDir.x)[i];
        float t0 = ((&boxMin.x)[i] - (&rayOrigin.x)[i]) * invD;
        float t1 = ((&boxMax.x)[i] - (&rayOrigin.x)[i]) * invD;
        
        if (invD < 0.0f) {
            std::swap(t0, t1);
        }
        
        tMin = std::max(tMin, t0);
        tMax = std::min(tMax, t1);
        
        if (tMax < tMin) {
            return false;
        }
    }
    
    return true;
}

bool raySphereIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                        const glm::vec3& center, float radius, float& t) {
    glm::vec3 oc = rayOrigin - center;
    float a = glm::dot(rayDir, rayDir);
    float b = 2.0f * glm::dot(oc, rayDir);
    float c = glm::dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0.0f) {
        return false;
    }
    
    t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    return t >= 0.0f;
}

glm::vec3 closestPointOnSegment(const glm::vec3& point,
                                const glm::vec3& segStart,
                                const glm::vec3& segEnd) {
    glm::vec3 seg = segEnd - segStart;
    float length2 = glm::dot(seg, seg);
    
    if (length2 < 0.0001f) {
        return segStart;
    }
    
    float t = glm::dot(point - segStart, seg) / length2;
    t = glm::clamp(t, 0.0f, 1.0f);
    
    return segStart + t * seg;
}

bool sphereCapsuleIntersect(const glm::vec3& sphereCenter, float sphereRadius,
                            const glm::vec3& capsuleStart, const glm::vec3& capsuleEnd,
                            float capsuleRadius) {
    glm::vec3 closest = closestPointOnSegment(sphereCenter, capsuleStart, capsuleEnd);
    float distance = glm::length(sphereCenter - closest);
    return distance <= (sphereRadius + capsuleRadius);
}

void getAABB(const CollisionShape& shape, const glm::vec3& pos, const glm::quat& rot,
             glm::vec3& outMin, glm::vec3& outMax) {
    switch (shape.type) {
        case CollisionShapeType::Box: {
            // Get all 8 corners and transform them
            glm::vec3 corners[8] = {
                {-shape.halfExtents.x, -shape.halfExtents.y, -shape.halfExtents.z},
                { shape.halfExtents.x, -shape.halfExtents.y, -shape.halfExtents.z},
                {-shape.halfExtents.x,  shape.halfExtents.y, -shape.halfExtents.z},
                { shape.halfExtents.x,  shape.halfExtents.y, -shape.halfExtents.z},
                {-shape.halfExtents.x, -shape.halfExtents.y,  shape.halfExtents.z},
                { shape.halfExtents.x, -shape.halfExtents.y,  shape.halfExtents.z},
                {-shape.halfExtents.x,  shape.halfExtents.y,  shape.halfExtents.z},
                { shape.halfExtents.x,  shape.halfExtents.y,  shape.halfExtents.z}
            };
            
            outMin = glm::vec3(std::numeric_limits<float>::max());
            outMax = glm::vec3(std::numeric_limits<float>::lowest());
            
            for (int i = 0; i < 8; i++) {
                glm::vec3 transformed = rot * corners[i] + pos;
                outMin = glm::min(outMin, transformed);
                outMax = glm::max(outMax, transformed);
            }
            break;
        }
        
        case CollisionShapeType::Sphere: {
            outMin = pos - glm::vec3(shape.radius);
            outMax = pos + glm::vec3(shape.radius);
            break;
        }
        
        case CollisionShapeType::Capsule: {
            float halfHeight = shape.height / 2.0f + shape.radius;
            outMin = pos - glm::vec3(shape.radius + halfHeight);
            outMax = pos + glm::vec3(shape.radius + halfHeight);
            break;
        }
        
        default:
            outMin = pos - glm::vec3(1.0f);
            outMax = pos + glm::vec3(1.0f);
            break;
    }
}

} // namespace CollisionUtils

} // namespace VoxelForge
