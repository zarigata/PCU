/**
 * @file test_command_summon.cpp
 * @brief Unit tests for the /summon command
 */

#include <gtest/gtest.h>
#include <VoxelForge/game/CommandManager.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/entity/Entity.hpp>
#include <VoxelForge/core/ECS.hpp>
#include <glm/glm.hpp>

using namespace VoxelForge;

class SummonCommandTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager.registerVanillaCommands();
    }

    void TearDown() override {}

    CommandManager manager;
};

TEST_F(SummonCommandTest, SummonCommandIsRegistered) {
    EXPECT_TRUE(manager.hasCommand("summon"));
}

TEST_F(SummonCommandTest, SummonCommandNoArgsFails) {
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    auto result = manager.execute("summon", ctx);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Usage"), std::string::npos);
}

TEST_F(SummonCommandTest, SummonCommandBasicEntity) {
    World world(12345);
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    ctx.world = &world;
    ctx.position = glm::vec3(0, 64, 0);

    auto result = manager.execute("summon zombie", ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.affectedCount, 1);
}

TEST_F(SummonCommandTest, SummonCommandWithAbsolutePosition) {
    World world(12345);
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    ctx.world = &world;
    ctx.position = glm::vec3(0, 0, 0);

    auto result = manager.execute("summon creeper 100 64 -50", ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.affectedCount, 1);
}

TEST_F(SummonCommandTest, SummonCommandWithRelativePosition) {
    World world(12345);
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    ctx.world = &world;
    ctx.position = glm::vec3(10, 64, 20);

    auto result = manager.execute("summon skeleton ~5 ~ ~-3", ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.affectedCount, 1);
}

TEST_F(SummonCommandTest, SummonCommandRelativePositionDefault) {
    World world(12345);
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    ctx.world = &world;
    ctx.position = glm::vec3(50, 70, 50);

    // Just ~ means use executor position exactly
    auto result = manager.execute("summon pig ~ ~ ~", ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.affectedCount, 1);
}

TEST_F(SummonCommandTest, SummonCommandPassiveMob) {
    World world(12345);
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    ctx.world = &world;

    auto result = manager.execute("summon cow", ctx);
    EXPECT_TRUE(result.success);
}

TEST_F(SummonCommandTest, SummonCommandAllHostileMobs) {
    World world(12345);
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    ctx.world = &world;

    const std::vector<std::string> mobs = {
        "zombie", "skeleton", "creeper", "spider",
        "enderman", "blaze", "slime"
    };

    for (const auto& mob : mobs) {
        auto result = manager.execute("summon " + mob, ctx);
        EXPECT_TRUE(result.success) << "Failed to summon: " << mob;
    }
}

TEST_F(SummonCommandTest, SummonCommandAllPassiveMobs) {
    World world(12345);
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    ctx.world = &world;

    const std::vector<std::string> mobs = {"cow", "pig", "sheep", "chicken"};

    for (const auto& mob : mobs) {
        auto result = manager.execute("summon " + mob, ctx);
        EXPECT_TRUE(result.success) << "Failed to summon: " << mob;
    }
}

TEST_F(SummonCommandTest, SummonCommandWithNamespace) {
    World world(12345);
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    ctx.world = &world;

    // poorcraftultra: prefix
    auto result = manager.execute("summon poorcraftultra:zombie", ctx);
    EXPECT_TRUE(result.success);

    // minecraft: prefix
    result = manager.execute("summon minecraft:creeper", ctx);
    EXPECT_TRUE(result.success);
}

TEST_F(SummonCommandTest, SummonCommandUnknownEntityCreatesGeneric) {
    World world(12345);
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    ctx.world = &world;

    // Unknown entity type should create a generic entity
    auto result = manager.execute("summon custom_entity", ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.affectedCount, 1);
}

TEST_F(SummonCommandTest, SummonCommandInvalidPositionFails) {
    World world(12345);
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    ctx.world = &world;

    auto result = manager.execute("summon zombie abc def ghi", ctx);
    EXPECT_FALSE(result.success);
}

TEST_F(SummonCommandTest, SummonCommandInsufficientPermission) {
    World world(12345);
    CommandContext ctx;
    ctx.source = CommandSource::Player;
    ctx.permissionLevel = 0; // Need level 2
    ctx.world = &world;

    auto result = manager.execute("summon zombie", ctx);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("permission"), std::string::npos);
}

TEST_F(SummonCommandTest, SummonCommandProjectile) {
    World world(12345);
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    ctx.world = &world;

    auto result = manager.execute("summon arrow 0 64 0", ctx);
    EXPECT_TRUE(result.success);

    result = manager.execute("summon snowball 0 64 0", ctx);
    EXPECT_TRUE(result.success);
}

TEST_F(SummonCommandTest, SummonCommandItemEntity) {
    World world(12345);
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    ctx.world = &world;

    auto result = manager.execute("summon item", ctx);
    EXPECT_TRUE(result.success);
}

TEST_F(SummonCommandTest, SummonCommandNoWorldFails) {
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    ctx.world = nullptr;

    auto result = manager.execute("summon zombie", ctx);
    EXPECT_FALSE(result.success);
}
