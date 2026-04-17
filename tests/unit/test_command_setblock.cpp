/**
 * @file test_command_setblock.cpp
 * @brief Unit tests for /setblock command
 */

#include <gtest/gtest.h>
#include <VoxelForge/game/CommandManager.hpp>
#include <VoxelForge/world/Block.hpp>

using namespace VoxelForge;

TEST(SetBlockCommand, IsRegistered) {
    CommandManager mgr;
    mgr.registerVanillaCommands();
    EXPECT_TRUE(mgr.hasCommand("setblock"));
}

TEST(SetBlockCommand, RejectsInsufficientArguments) {
    CommandManager mgr;
    mgr.registerVanillaCommands();
    
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    
    // No arguments
    auto result = mgr.execute("setblock", ctx);
    EXPECT_FALSE(result.success);
    
    // Only position, no block
    result = mgr.execute("setblock 0 64 0", ctx);
    EXPECT_FALSE(result.success);
    
    // Only x y, no z or block
    result = mgr.execute("setblock 0 64", ctx);
    EXPECT_FALSE(result.success);
}

TEST(SetBlockCommand, RejectsWithoutWorld) {
    CommandManager mgr;
    mgr.registerVanillaCommands();
    
    CommandContext ctx;
    ctx.source = CommandSource::Console;
    ctx.permissionLevel = 4;
    ctx.world = nullptr;
    
    auto result = mgr.execute("setblock 0 64 0 stone", ctx);
    EXPECT_FALSE(result.success);
}

TEST(SetBlockCommand, RequiresPermission) {
    CommandManager mgr;
    mgr.registerVanillaCommands();
    
    CommandContext ctx;
    ctx.source = CommandSource::Player;
    ctx.permissionLevel = 0;
    
    auto result = mgr.execute("setblock 0 64 0 stone", ctx);
    EXPECT_FALSE(result.success);
}

TEST(SetBlockCommand, HasCorrectMetadata) {
    CommandManager mgr;
    mgr.registerVanillaCommands();
    
    const Command* cmd = mgr.getCommand("setblock");
    ASSERT_NE(cmd, nullptr);
    EXPECT_EQ(cmd->name, "setblock");
    EXPECT_EQ(cmd->requiredPermission, 2);
    EXPECT_FALSE(cmd->usage.empty());
}
