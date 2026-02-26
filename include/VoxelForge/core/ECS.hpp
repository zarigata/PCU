/**
 * @file ECS.hpp
 * @brief Custom Entity Component System for VoxelForge
 * 
 * Designed specifically for voxel game needs with:
 * - Cache-friendly component storage
 * - Fast iteration over entities
 * - Component masking for efficient queries
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <bitset>
#include <typeindex>
#include <memory>
#include <functional>
#include <array>

namespace VoxelForge {

// ============================================
// Type Definitions
// ============================================

using Entity = uint64_t;
constexpr Entity INVALID_ENTITY = 0;

using ComponentID = uint32_t;
constexpr ComponentID MAX_COMPONENTS = 256;

using ComponentMask = std::bitset<MAX_COMPONENTS>;

// ============================================
// Component Mask Generation
// ============================================

namespace detail {
    inline ComponentID nextComponentID = 0;
    
    template<typename T>
    ComponentID getComponentID() {
        static ComponentID id = nextComponentID++;
        return id;
    }
}

// ============================================
// Component Pool Interface
// ============================================

class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void remove(Entity entity) = 0;
    virtual bool has(Entity entity) const = 0;
    virtual size_t size() const = 0;
};

// ============================================
// Component Pool Implementation
// ============================================

template<typename T>
class ComponentPool : public IComponentPool {
public:
    static_assert(std::is_trivially_copyable_v<T> || std::is_default_constructible_v<T>,
                  "Component must be trivially copyable or default constructible");
    
    void add(Entity entity, T component) {
        if (entityToIndex.find(entity) != entityToIndex.end()) {
            // Entity already has this component, update it
            components[entityToIndex[entity]] = component;
            return;
        }
        
        size_t index = components.size();
        components.push_back(component);
        entities.push_back(entity);
        entityToIndex[entity] = index;
    }
    
    void remove(Entity entity) override {
        auto it = entityToIndex.find(entity);
        if (it == entityToIndex.end()) return;
        
        size_t index = it->second;
        size_t lastIndex = components.size() - 1;
        
        // Swap with last element for O(1) removal
        if (index != lastIndex) {
            components[index] = std::move(components[lastIndex]);
            entities[index] = entities[lastIndex];
            entityToIndex[entities[index]] = index;
        }
        
        components.pop_back();
        entities.pop_back();
        entityToIndex.erase(entity);
    }
    
    T* get(Entity entity) {
        auto it = entityToIndex.find(entity);
        if (it == entityToIndex.end()) return nullptr;
        return &components[it->second];
    }
    
    const T* get(Entity entity) const {
        auto it = entityToIndex.find(entity);
        if (it == entityToIndex.end()) return nullptr;
        return &components[it->second];
    }
    
    bool has(Entity entity) const override {
        return entityToIndex.find(entity) != entityToIndex.end();
    }
    
    size_t size() const override {
        return components.size();
    }
    
    // Iterator support for iteration
    auto begin() { return components.begin(); }
    auto end() { return components.end(); }
    auto begin() const { return components.begin(); }
    auto end() const { return components.end(); }
    
    const std::vector<Entity>& getEntities() const { return entities; }
    const std::vector<T>& getComponents() const { return components; }

private:
    std::vector<T> components;
    std::vector<Entity> entities;
    std::unordered_map<Entity, size_t> entityToIndex;
};

// ============================================
// System Base Class
// ============================================

class System {
public:
    virtual ~System() = default;
    virtual void update(float deltaTime) = 0;
    
    const ComponentMask& getRequiredComponents() const { return requiredComponents; }
    
protected:
    ComponentMask requiredComponents;
    
    template<typename T>
    void requireComponent() {
        requiredComponents.set(detail::getComponentID<T>());
    }
};

// ============================================
// ECS World (Main Container)
// ============================================

class ECSWorld {
public:
    ECSWorld() {
        componentPools.resize(MAX_COMPONENTS, nullptr);
    }
    
    ~ECSWorld() {
        for (auto pool : componentPools) {
            delete pool;
        }
    }
    
    // ============================================
    // Entity Management
    // ============================================
    
    Entity createEntity() {
        Entity entity = nextEntity++;
        entityMasks[entity] = ComponentMask();
        return entity;
    }
    
    void destroyEntity(Entity entity) {
        // Remove all components
        for (auto pool : componentPools) {
            if (pool && pool->has(entity)) {
                pool->remove(entity);
            }
        }
        entityMasks.erase(entity);
    }
    
    bool isAlive(Entity entity) const {
        return entityMasks.find(entity) != entityMasks.end();
    }
    
    // ============================================
    // Component Management
    // ============================================
    
    template<typename T, typename... Args>
    T& addComponent(Entity entity, Args&&... args) {
        ComponentID id = detail::getComponentID<T>();
        
        if (!componentPools[id]) {
            componentPools[id] = new ComponentPool<T>();
        }
        
        auto pool = static_cast<ComponentPool<T>*>(componentPools[id]);
        pool->add(entity, T(std::forward<Args>(args)...));
        entityMasks[entity].set(id);
        
        return *pool->get(entity);
    }
    
    template<typename T>
    void removeComponent(Entity entity) {
        ComponentID id = detail::getComponentID<T>();
        
        if (componentPools[id]) {
            componentPools[id]->remove(entity);
            entityMasks[entity].reset(id);
        }
    }
    
    template<typename T>
    T* getComponent(Entity entity) {
        ComponentID id = detail::getComponentID<T>();
        
        if (!componentPools[id]) return nullptr;
        
        auto pool = static_cast<ComponentPool<T>*>(componentPools[id]);
        return pool->get(entity);
    }
    
    template<typename T>
    const T* getComponent(Entity entity) const {
        ComponentID id = detail::getComponentID<T>();
        
        if (!componentPools[id]) return nullptr;
        
        auto pool = static_cast<const ComponentPool<T>*>(componentPools[id]);
        return pool->get(entity);
    }
    
    template<typename T>
    bool hasComponent(Entity entity) const {
        ComponentID id = detail::getComponentID<T>();
        return entityMasks.at(entity).test(id);
    }
    
    template<typename... Components>
    bool hasAllComponents(Entity entity) const {
        return (hasComponent<Components>(entity) && ...);
    }
    
    // ============================================
    // System Management
    // ============================================
    
    template<typename T, typename... Args>
    T& registerSystem(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *system;
        systems.push_back(std::move(system));
        return ref;
    }
    
    void updateSystems(float deltaTime) {
        for (auto& system : systems) {
            system->update(deltaTime);
        }
    }
    
    // ============================================
    // Entity Views (Iterators)
    // ============================================
    
    template<typename... Components>
    class View {
    public:
        View(ECSWorld* world) : world(world) {
            requiredMask = ComponentMask();
            (requiredMask.set(detail::getComponentID<Components>()), ...);
        }
        
        class Iterator {
        public:
            Iterator(ECSWorld* world, ComponentMask mask, 
                     std::unordered_map<Entity, ComponentMask>::iterator it,
                     std::unordered_map<Entity, ComponentMask>::iterator end)
                : world(world), mask(mask), it(it), end(end) {
                // Find first matching entity
                while (this->it != this->end && (this->it->second & mask) != mask) {
                    ++this->it;
                }
            }
            
            Iterator& operator++() {
                ++it;
                while (it != end && (it->second & mask) != mask) {
                    ++it;
                }
                return *this;
            }
            
            bool operator!=(const Iterator& other) const {
                return it != other.it;
            }
            
            Entity operator*() const {
                return it->first;
            }
            
        private:
            ECSWorld* world;
            ComponentMask mask;
            std::unordered_map<Entity, ComponentMask>::iterator it;
            std::unordered_map<Entity, ComponentMask>::iterator end;
        };
        
        Iterator begin() {
            return Iterator(world, requiredMask, world->entityMasks.begin(), world->entityMasks.end());
        }
        
        Iterator end() {
            return Iterator(world, requiredMask, world->entityMasks.end(), world->entityMasks.end());
        }
        
        // For each helper
        template<typename Func>
        void each(Func&& func) {
            for (Entity entity : *this) {
                func(entity, *world->getComponent<Components>(entity)...);
            }
        }
        
        size_t size() {
            size_t count = 0;
            for (auto it = begin(); it != end(); ++it) {
                ++count;
            }
            return count;
        }
        
    private:
        ECSWorld* world;
        ComponentMask requiredMask;
    };
    
    template<typename... Components>
    View<Components...> view() {
        return View<Components...>(this);
    }
    
    // ============================================
    // Single Entity Query
    // ============================================
    
    template<typename... Components>
    std::vector<Entity> getEntitiesWith() {
        std::vector<Entity> result;
        for (auto [entity, mask] : entityMasks) {
            ComponentMask required;
            (required.set(detail::getComponentID<Components>()), ...);
            if ((mask & required) == required) {
                result.push_back(entity);
            }
        }
        return result;
    }

private:
    Entity nextEntity = 1;
    std::vector<IComponentPool*> componentPools;
    std::unordered_map<Entity, ComponentMask> entityMasks;
    std::vector<std::unique_ptr<System>> systems;
};

// ============================================
// Common Components
// ============================================

struct TransformComponent {
    Vec3 position = Vec3(0.0f);
    Vec3 rotation = Vec3(0.0f); // Euler angles in radians
    Vec3 scale = Vec3(1.0f);
    
    Mat4 toMatrix() const {
        Mat4 m = Mat4(1.0f);
        m = glm::translate(m, position);
        m = glm::rotate(m, rotation.x, Vec3(1, 0, 0));
        m = glm::rotate(m, rotation.y, Vec3(0, 1, 0));
        m = glm::rotate(m, rotation.z, Vec3(0, 0, 1));
        m = glm::scale(m, scale);
        return m;
    }
};

struct VelocityComponent {
    Vec3 velocity = Vec3(0.0f);
    Vec3 acceleration = Vec3(0.0f);
};

struct PhysicsComponent {
    bool isStatic = false;
    bool isTrigger = false;
    f32 mass = 1.0f;
    f32 friction = 0.5f;
    f32 restitution = 0.0f; // Bounciness
    AABB bounds = AABB(-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f);
    bool onGround = false;
};

struct RenderComponent {
    u32 meshID = 0;
    u32 materialID = 0;
    bool visible = true;
    bool castShadows = true;
    bool receiveShadows = true;
};

struct NameComponent {
    String name;
    String tag;
};

struct UUIDComponent {
    UUID uuid;
};

// ============================================
// Common Systems
// ============================================

class TransformSystem : public System {
public:
    TransformSystem() {
        requireComponent<TransformComponent>();
    }
    
    void update(float deltaTime) override {
        // Update transforms, handle parent-child relationships
        // This would be expanded for hierarchical transforms
    }
};

class PhysicsSystem : public System {
public:
    PhysicsSystem() {
        requireComponent<TransformComponent>();
        requireComponent<PhysicsComponent>();
        requireComponent<VelocityComponent>();
    }
    
    void update(float deltaTime) override {
        // Physics simulation would be handled by the main PhysicsSystem class
        // This is just for ECS integration
    }
};

} // namespace VoxelForge
