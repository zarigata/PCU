/**
 * @file CollisionManager.hpp
 * @brief Collision detection and management
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <functional>
#include <unordered_set>

namespace VoxelForge {

class PhysicsSystem;
class Entity;
class World;

// Collision shape types
enum class CollisionShapeType {
    Box,
    Sphere,
    Capsule,
    Cylinder,
    ConvexMesh,
    TriangleMesh,
    Heightfield
};

// Collision shape description
struct CollisionShape {
    CollisionShapeType type = CollisionShapeType::Box;
    glm::vec3 halfExtents = glm::vec3(0.5f);
    float radius = 0.5f;
    float height = 1.0f;
    
    // For mesh shapes
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
    
    static CollisionShape Box(const glm::vec3& halfExtents) {
        CollisionShape shape;
        shape.type = CollisionShapeType::Box;
        shape.halfExtents = halfExtents;
        return shape;
    }
    
    static CollisionShape Sphere(float radius) {
        CollisionShape shape;
        shape.type = CollisionShapeType::Sphere;
        shape.radius = radius;
        return shape;
    }
    
    static CollisionShape Capsule(float radius, float height) {
        CollisionShape shape;
        shape.type = CollisionShapeType::Capsule;
        shape.radius = radius;
        shape.height = height;
        return shape;
    }
};

// Collision layer for filtering
struct CollisionLayer {
    uint16_t id = 0;
    std::string name;
    uint16_t collideWith = 0xFFFF;  // Collides with all by default
    
    bool canCollideWith(const CollisionLayer& other) const {
        return (collideWith & other.id) != 0;
    }
};

// Collision event
struct CollisionEvent {
    enum class Type {
        Enter,
        Stay,
        Exit,
        TriggerEnter,
        TriggerExit
    };
    
    Type type;
    uint32_t entityA;
    uint32_t entityB;
    glm::vec3 contactPoint;
    glm::vec3 normal;
    float penetration;
};

// Collision callback info
struct CollisionCallbackInfo {
    uint32_t entityId;
    CollisionShape shape;
    glm::vec3 position;
    glm::quat rotation;
    uint16_t layer;
    bool isTrigger = false;
};

// Collision manager
class CollisionManager {
public:
    CollisionManager();
    ~CollisionManager();
    
    void init(PhysicsSystem* physics);
    void shutdown();
    
    // Layers
    uint16_t createLayer(const std::string& name, uint16_t collideWith = 0xFFFF);
    CollisionLayer* getLayer(uint16_t id);
    CollisionLayer* getLayer(const std::string& name);
    void setLayerCollision(uint16_t layerA, uint16_t layerB, bool canCollide);
    
    // Collision groups (for entities)
    void setEntityLayer(uint32_t entityId, uint16_t layerId);
    uint16_t getEntityLayer(uint32_t entityId) const;
    
    // Collision shapes for entities
    void setEntityShape(uint32_t entityId, const CollisionShape& shape);
    CollisionShape getEntityShape(uint32_t entityId) const;
    
    // Triggers
    void setEntityTrigger(uint32_t entityId, bool isTrigger);
    bool isEntityTrigger(uint32_t entityId) const;
    
    // Collision queries
    bool checkCollision(uint32_t entityA, uint32_t entityB);
    std::vector<uint32_t> getCollidingEntities(uint32_t entityId);
    
    // Spatial queries
    std::vector<uint32_t> getEntitiesInBox(const glm::vec3& min, const glm::vec3& max);
    std::vector<uint32_t> getEntitiesInSphere(const glm::vec3& center, float radius);
    std::vector<uint32_t> getEntitiesInCapsule(const glm::vec3& point1, const glm::vec3& point2, float radius);
    
    // Raycast
    struct RaycastHit {
        uint32_t entityId;
        glm::vec3 point;
        glm::vec3 normal;
        float distance;
    };
    
    bool raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, 
                 RaycastHit& hit, uint16_t layerMask = 0xFFFF);
    std::vector<RaycastHit> raycastAll(const glm::vec3& origin, const glm::vec3& direction,
                                        float maxDistance, uint16_t layerMask = 0xFFFF);
    
    // Callbacks
    using CollisionCallback = std::function<void(const CollisionEvent&)>;
    void setCollisionCallback(CollisionCallback callback);
    void onCollision(const CollisionEvent& event);
    
    // Update
    void update();
    void processEvents();
    
    // Debug visualization
    void setDebugDrawEnabled(bool enabled);
    bool isDebugDrawEnabled() const { return debugDrawEnabled; }
    void getDebugGeometry(std::vector<glm::vec3>& outLines);
    
private:
    PhysicsSystem* physics = nullptr;
    
    // Layers
    std::vector<CollisionLayer> layers;
    std::unordered_map<std::string, uint16_t> layerNameMap;
    
    // Entity collision data
    std::unordered_map<uint32_t, CollisionCallbackInfo> entityCollisionData;
    
    // Active collisions for tracking enter/exit
    std::unordered_set<std::pair<uint32_t, uint32_t>, PairHash> activeCollisions;
    
    // Callbacks
    CollisionCallback collisionCallback;
    
    // Debug
    bool debugDrawEnabled = false;
};

// Pair hash for unordered_set
struct PairHash {
    template<typename T1, typename T2>
    size_t operator()(const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

// Collision utility functions
namespace CollisionUtils {

// Point in box
bool pointInBox(const glm::vec3& point, const glm::vec3& boxMin, const glm::vec3& boxMax);

// Point in sphere
bool pointInSphere(const glm::vec3& point, const glm::vec3& center, float radius);

// Box-box intersection
bool boxBoxIntersect(const glm::vec3& minA, const glm::vec3& maxA,
                     const glm::vec3& minB, const glm::vec3& maxB);

// Sphere-sphere intersection
bool sphereSphereIntersect(const glm::vec3& centerA, float radiusA,
                           const glm::vec3& centerB, float radiusB);

// Ray-box intersection
bool rayBoxIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                     const glm::vec3& boxMin, const glm::vec3& boxMax,
                     float& tMin, float& tMax);

// Ray-sphere intersection
bool raySphereIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                        const glm::vec3& center, float radius,
                        float& t);

// Closest point on line segment
glm::vec3 closestPointOnSegment(const glm::vec3& point, 
                                const glm::vec3& segStart, 
                                const glm::vec3& segEnd);

// Sphere-capsule intersection
bool sphereCapsuleIntersect(const glm::vec3& sphereCenter, float sphereRadius,
                            const glm::vec3& capsuleStart, const glm::vec3& capsuleEnd,
                            float capsuleRadius);

// Get AABB from collision shape
void getAABB(const CollisionShape& shape, const glm::vec3& pos, const glm::quat& rot,
             glm::vec3& outMin, glm::vec3& outMax);

} // namespace CollisionUtils

} // namespace VoxelForge
