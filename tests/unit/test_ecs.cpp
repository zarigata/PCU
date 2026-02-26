/**
 * @file test_ecs.cpp
 * @brief ECS unit tests
 */

#include <gtest/gtest.h>
#include <VoxelForge/core/ECS.hpp>

using namespace VoxelForge;

// Test components
struct PositionComponent {
    float x = 0, y = 0, z = 0;
};

struct VelocityComponent {
    float vx = 0, vy = 0, vz = 0;
};

struct NameComponent {
    std::string name;
};

class ECSTest : public ::testing::Test {
protected:
    ECSWorld world;
};

TEST_F(ECSTest, CreateEntity) {
    Entity e = world.createEntity();
    EXPECT_NE(e, INVALID_ENTITY);
    EXPECT_TRUE(world.isAlive(e));
}

TEST_F(ECSTest, CreateMultipleEntities) {
    Entity e1 = world.createEntity();
    Entity e2 = world.createEntity();
    Entity e3 = world.createEntity();
    
    EXPECT_NE(e1, e2);
    EXPECT_NE(e2, e3);
    EXPECT_NE(e1, e3);
}

TEST_F(ECSTest, DestroyEntity) {
    Entity e = world.createEntity();
    EXPECT_TRUE(world.isAlive(e));
    
    world.destroyEntity(e);
    EXPECT_FALSE(world.isAlive(e));
}

TEST_F(ECSTest, AddComponent) {
    Entity e = world.createEntity();
    
    auto& pos = world.addComponent<PositionComponent>(e, 10.0f, 20.0f, 30.0f);
    
    EXPECT_FLOAT_EQ(pos.x, 10.0f);
    EXPECT_FLOAT_EQ(pos.y, 20.0f);
    EXPECT_FLOAT_EQ(pos.z, 30.0f);
}

TEST_F(ECSTest, GetComponent) {
    Entity e = world.createEntity();
    world.addComponent<PositionComponent>(e, 1.0f, 2.0f, 3.0f);
    
    auto* pos = world.getComponent<PositionComponent>(e);
    ASSERT_NE(pos, nullptr);
    EXPECT_FLOAT_EQ(pos->x, 1.0f);
    EXPECT_FLOAT_EQ(pos->y, 2.0f);
    EXPECT_FLOAT_EQ(pos->z, 3.0f);
}

TEST_F(ECSTest, GetComponentNonExistent) {
    Entity e = world.createEntity();
    
    auto* pos = world.getComponent<PositionComponent>(e);
    EXPECT_EQ(pos, nullptr);
}

TEST_F(ECSTest, HasComponent) {
    Entity e = world.createEntity();
    
    EXPECT_FALSE(world.hasComponent<PositionComponent>(e));
    
    world.addComponent<PositionComponent>(e);
    
    EXPECT_TRUE(world.hasComponent<PositionComponent>(e));
}

TEST_F(ECSTest, RemoveComponent) {
    Entity e = world.createEntity();
    world.addComponent<PositionComponent>(e);
    
    EXPECT_TRUE(world.hasComponent<PositionComponent>(e));
    
    world.removeComponent<PositionComponent>(e);
    
    EXPECT_FALSE(world.hasComponent<PositionComponent>(e));
}

TEST_F(ECSTest, MultipleComponents) {
    Entity e = world.createEntity();
    
    world.addComponent<PositionComponent>(e, 1.0f, 2.0f, 3.0f);
    world.addComponent<VelocityComponent>(e, 4.0f, 5.0f, 6.0f);
    world.addComponent<NameComponent>(e, NameComponent{"TestEntity"});
    
    EXPECT_TRUE(world.hasComponent<PositionComponent>(e));
    EXPECT_TRUE(world.hasComponent<VelocityComponent>(e));
    EXPECT_TRUE(world.hasComponent<NameComponent>(e));
    
    auto* name = world.getComponent<NameComponent>(e);
    ASSERT_NE(name, nullptr);
    EXPECT_EQ(name->name, "TestEntity");
}

TEST_F(ECSTest, HasAllComponents) {
    Entity e = world.createEntity();
    
    world.addComponent<PositionComponent>(e);
    world.addComponent<VelocityComponent>(e);
    
    EXPECT_TRUE(world.hasAllComponents<PositionComponent, VelocityComponent>(e));
    EXPECT_FALSE(world.hasAllComponents<PositionComponent, NameComponent>(e));
}

TEST_F(ECSTest, ViewIteration) {
    // Create entities with different component combinations
    Entity e1 = world.createEntity();
    world.addComponent<PositionComponent>(e1);
    world.addComponent<VelocityComponent>(e1);
    
    Entity e2 = world.createEntity();
    world.addComponent<PositionComponent>(e2);
    world.addComponent<VelocityComponent>(e2);
    
    Entity e3 = world.createEntity();
    world.addComponent<PositionComponent>(e3); // No velocity
    
    int count = 0;
    for (auto entity : world.view<PositionComponent, VelocityComponent>()) {
        count++;
        EXPECT_TRUE(world.hasComponent<PositionComponent>(entity));
        EXPECT_TRUE(world.hasComponent<VelocityComponent>(entity));
    }
    
    EXPECT_EQ(count, 2); // Only e1 and e2 have both components
}

TEST_F(ECSTest, ViewEach) {
    Entity e1 = world.createEntity();
    world.addComponent<PositionComponent>(e1, 10.0f, 0.0f, 0.0f);
    
    Entity e2 = world.createEntity();
    world.addComponent<PositionComponent>(e2, 20.0f, 0.0f, 0.0f);
    
    float totalX = 0;
    world.view<PositionComponent>().each([&](Entity e, PositionComponent& pos) {
        totalX += pos.x;
    });
    
    EXPECT_FLOAT_EQ(totalX, 30.0f);
}

TEST_F(ECSTest, ViewSize) {
    world.addComponent<PositionComponent>(world.createEntity());
    world.addComponent<PositionComponent>(world.createEntity());
    world.addComponent<PositionComponent>(world.createEntity());
    world.addComponent<VelocityComponent>(world.createEntity()); // No position
    
    auto view = world.view<PositionComponent>();
    EXPECT_EQ(view.size(), 3);
}

TEST_F(ECSTest, GetEntitiesWith) {
    Entity e1 = world.createEntity();
    world.addComponent<PositionComponent>(e1);
    world.addComponent<VelocityComponent>(e1);
    
    Entity e2 = world.createEntity();
    world.addComponent<PositionComponent>(e2);
    
    Entity e3 = world.createEntity();
    world.addComponent<PositionComponent>(e3);
    world.addComponent<VelocityComponent>(e3);
    
    auto entities = world.getEntitiesWith<PositionComponent, VelocityComponent>();
    EXPECT_EQ(entities.size(), 2);
}

TEST_F(ECSTest, ComponentModification) {
    Entity e = world.createEntity();
    world.addComponent<PositionComponent>(e, 0.0f, 0.0f, 0.0f);
    
    // Modify component
    auto* pos = world.getComponent<PositionComponent>(e);
    pos->x = 100.0f;
    
    // Verify modification
    pos = world.getComponent<PositionComponent>(e);
    EXPECT_FLOAT_EQ(pos->x, 100.0f);
}

TEST_F(ECSTest, LargeEntityCount) {
    const int ENTITY_COUNT = 10000;
    
    for (int i = 0; i < ENTITY_COUNT; i++) {
        Entity e = world.createEntity();
        world.addComponent<PositionComponent>(e, float(i), 0.0f, 0.0f);
    }
    
    int count = 0;
    for (auto e : world.view<PositionComponent>()) {
        count++;
    }
    
    EXPECT_EQ(count, ENTITY_COUNT);
}
