/**
 * @file test_ecs.cpp
 * @brief ECS unit tests
 */

#include <gtest/gtest.h>
#include <VoxelForge/core/ECS.hpp>

using namespace VoxelForge;

// Test components (use distinct names to avoid clashing with VoxelForge::VelocityComponent etc.)
struct PositionComponent {
    float x = 0, y = 0, z = 0;
};

struct TestVelocity {
    float vx = 0, vy = 0, vz = 0;
};

struct TestName {
    std::string name;
};

class ECSTest : public ::testing::Test {
protected:
    ECSWorld world;
};

TEST_F(ECSTest, CreateEntity) {
    EntityID e = world.createEntity();
    EXPECT_NE(e, INVALID_ENTITY);
    EXPECT_TRUE(world.isAlive(e));
}

TEST_F(ECSTest, CreateMultipleEntities) {
    EntityID e1 = world.createEntity();
    EntityID e2 = world.createEntity();
    EntityID e3 = world.createEntity();
    
    EXPECT_NE(e1, e2);
    EXPECT_NE(e2, e3);
    EXPECT_NE(e1, e3);
}

TEST_F(ECSTest, DestroyEntity) {
    EntityID e = world.createEntity();
    EXPECT_TRUE(world.isAlive(e));
    
    world.destroyEntity(e);
    EXPECT_FALSE(world.isAlive(e));
}

TEST_F(ECSTest, AddComponent) {
    EntityID e = world.createEntity();
    
    auto& pos = world.addComponent<PositionComponent>(e, 10.0f, 20.0f, 30.0f);
    
    EXPECT_FLOAT_EQ(pos.x, 10.0f);
    EXPECT_FLOAT_EQ(pos.y, 20.0f);
    EXPECT_FLOAT_EQ(pos.z, 30.0f);
}

TEST_F(ECSTest, GetComponent) {
    EntityID e = world.createEntity();
    world.addComponent<PositionComponent>(e, 1.0f, 2.0f, 3.0f);
    
    auto* pos = world.getComponent<PositionComponent>(e);
    ASSERT_NE(pos, nullptr);
    EXPECT_FLOAT_EQ(pos->x, 1.0f);
    EXPECT_FLOAT_EQ(pos->y, 2.0f);
    EXPECT_FLOAT_EQ(pos->z, 3.0f);
}

TEST_F(ECSTest, GetComponentNonExistent) {
    EntityID e = world.createEntity();
    
    auto* pos = world.getComponent<PositionComponent>(e);
    EXPECT_EQ(pos, nullptr);
}

TEST_F(ECSTest, HasComponent) {
    EntityID e = world.createEntity();
    
    EXPECT_FALSE(world.hasComponent<PositionComponent>(e));
    
    world.addComponent<PositionComponent>(e);
    
    EXPECT_TRUE(world.hasComponent<PositionComponent>(e));
}

TEST_F(ECSTest, RemoveComponent) {
    EntityID e = world.createEntity();
    world.addComponent<PositionComponent>(e);
    
    EXPECT_TRUE(world.hasComponent<PositionComponent>(e));
    
    world.removeComponent<PositionComponent>(e);
    
    EXPECT_FALSE(world.hasComponent<PositionComponent>(e));
}

TEST_F(ECSTest, MultipleComponents) {
    EntityID e = world.createEntity();
    
    world.addComponent<PositionComponent>(e, 1.0f, 2.0f, 3.0f);
    world.addComponent<TestVelocity>(e, 4.0f, 5.0f, 6.0f);
    world.addComponent<TestName>(e, TestName{"TestEntity"});
    
    EXPECT_TRUE(world.hasComponent<PositionComponent>(e));
    EXPECT_TRUE(world.hasComponent<TestVelocity>(e));
    EXPECT_TRUE(world.hasComponent<TestName>(e));
    
    auto* name = world.getComponent<TestName>(e);
    ASSERT_NE(name, nullptr);
    EXPECT_EQ(name->name, "TestEntity");
}

TEST_F(ECSTest, HasAllComponents) {
    EntityID e = world.createEntity();
    
    world.addComponent<PositionComponent>(e);
    world.addComponent<TestVelocity>(e);
    
    EXPECT_TRUE((world.hasAllComponents<PositionComponent, TestVelocity>(e)));
    EXPECT_FALSE((world.hasAllComponents<PositionComponent, TestName>(e)));
}

TEST_F(ECSTest, ViewIteration) {
    EntityID e1 = world.createEntity();
    world.addComponent<PositionComponent>(e1);
    world.addComponent<TestVelocity>(e1);
    
    EntityID e2 = world.createEntity();
    world.addComponent<PositionComponent>(e2);
    world.addComponent<TestVelocity>(e2);
    
    EntityID e3 = world.createEntity();
    world.addComponent<PositionComponent>(e3); // No velocity
    
    int count = 0;
    for (auto entity : world.view<PositionComponent, TestVelocity>()) {
        count++;
        EXPECT_TRUE(world.hasComponent<PositionComponent>(entity));
        EXPECT_TRUE(world.hasComponent<TestVelocity>(entity));
    }
    
    EXPECT_EQ(count, 2); // Only e1 and e2 have both components
}

TEST_F(ECSTest, ViewEach) {
    EntityID e1 = world.createEntity();
    world.addComponent<PositionComponent>(e1, 10.0f, 0.0f, 0.0f);
    
    EntityID e2 = world.createEntity();
    world.addComponent<PositionComponent>(e2, 20.0f, 0.0f, 0.0f);
    
    float totalX = 0;
    world.view<PositionComponent>().each([&](EntityID e, PositionComponent& pos) {
        totalX += pos.x;
    });
    
    EXPECT_FLOAT_EQ(totalX, 30.0f);
}

TEST_F(ECSTest, ViewSize) {
    world.addComponent<PositionComponent>(world.createEntity());
    world.addComponent<PositionComponent>(world.createEntity());
    world.addComponent<PositionComponent>(world.createEntity());
    world.addComponent<TestVelocity>(world.createEntity()); // No position
    
    auto view = world.view<PositionComponent>();
    EXPECT_EQ(view.size(), 3);
}

TEST_F(ECSTest, GetEntitiesWith) {
    EntityID e1 = world.createEntity();
    world.addComponent<PositionComponent>(e1);
    world.addComponent<TestVelocity>(e1);
    
    EntityID e2 = world.createEntity();
    world.addComponent<PositionComponent>(e2);
    
    EntityID e3 = world.createEntity();
    world.addComponent<PositionComponent>(e3);
    world.addComponent<TestVelocity>(e3);
    
    auto entities = world.getEntitiesWith<PositionComponent, TestVelocity>();
    EXPECT_EQ(entities.size(), 2);
}

TEST_F(ECSTest, ComponentModification) {
    EntityID e = world.createEntity();
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
        EntityID e = world.createEntity();
        world.addComponent<PositionComponent>(e, float(i), 0.0f, 0.0f);
    }
    
    int count = 0;
    for (auto e : world.view<PositionComponent>()) {
        count++;
    }
    
    EXPECT_EQ(count, ENTITY_COUNT);
}
