/**
 * @file test_mining.cpp
 * @brief Unit tests for MiningSystem
 */

#include <gtest/gtest.h>
#include <VoxelForge/game/MiningSystem.hpp>
#include <VoxelForge/world/Block.hpp>
#include <VoxelForge/world/BlockRegistry.hpp>

using namespace VoxelForge;

// Helper: register a test block with known properties
static BlockID registerTestBlock(const std::string& id, float hardness,
                                  bool requiresTool = false,
                                  ToolType requiredTool = ToolType::None,
                                  ToolTier minimumTier = ToolTier::None,
                                  Material material = Material::Stone) {
    auto& registry = BlockRegistry::get();
    BlockDefinition def;
    def.id = id;
    def.name = id;
    def.hardness = hardness;
    def.requiresTool = requiresTool;
    def.requiredTool = requiredTool;
    def.minimumTier = minimumTier;
    def.material = material;
    def.opaque = true;
    def.solid = true;
    return registry.registerBlock(id, std::move(def));
}

// Helper: register a test tool item
static ItemID registerTestTool(const std::string& id,
                                ToolProperties::Type type, int level,
                                float miningSpeed = 1.0f, int durability = 100) {
    auto& registry = ItemRegistry::get();
    ItemDefinition def;
    def.registryName = id;
    def.displayName = id;
    ToolProperties props;
    props.type = type;
    props.level = level;
    props.miningSpeed = miningSpeed;
    props.durability = durability;
    def.toolProperties = props;
    return registry.registerItem(id, std::move(def));
}

TEST(MiningSystemTest, InstantBreakZeroHardness) {
    BlockID blockId = registerTestBlock("test:instant_break", 0.0f);
    BlockState state(blockId);
    float breakTime = MiningSystem::getBreakTime(state, ItemStack{});
    EXPECT_FLOAT_EQ(breakTime, 0.0f);
}

TEST(MiningSystemTest, IndestructibleBlockNegativeBreakTime) {
    BlockID blockId = registerTestBlock("test:indestructible", -1.0f);
    BlockState state(blockId);
    float breakTime = MiningSystem::getBreakTime(state, ItemStack{});
    EXPECT_LT(breakTime, 0.0f);
}

TEST(MiningSystemTest, PositiveHardnessPositiveBreakTime) {
    BlockID blockId = registerTestBlock("test:dirt_block", 0.5f, false,
                                         ToolType::None, ToolTier::None,
                                         Material::Dirt);
    BlockState state(blockId);
    float breakTime = MiningSystem::getBreakTime(state, ItemStack{});
    EXPECT_GT(breakTime, 0.0f);
}

TEST(MiningSystemTest, ToolTierCheck) {
    ItemID woodPick = registerTestTool("test:wood_pickaxe",
                                        ToolProperties::Type::Pickaxe, 1);
    ItemID ironPick = registerTestTool("test:iron_pickaxe",
                                        ToolProperties::Type::Pickaxe, 3);

    BlockID diamondOre = registerTestBlock("test:diamond_ore_block", 3.0f,
                                            true, ToolType::Pickaxe,
                                            ToolTier::Iron, Material::Metal);
    BlockState oreState(diamondOre);

    EXPECT_FALSE(MiningSystem::canHarvest(ItemStack(woodPick), oreState));
    EXPECT_TRUE(MiningSystem::canHarvest(ItemStack(ironPick), oreState));
}

TEST(MiningSystemTest, ToolSpeedsUpMining) {
    BlockID stoneBlock = registerTestBlock("test:mining_stone", 1.5f, true,
                                            ToolType::Pickaxe, ToolTier::Wood,
                                            Material::Stone);
    BlockState state(stoneBlock);

    ItemStack emptyHand;
    ItemID pick = registerTestTool("test:pick_for_speed",
                                    ToolProperties::Type::Pickaxe, 2);
    ItemStack pickTool(pick);

    float handTime = MiningSystem::getBreakTime(state, emptyHand);
    float toolTime = MiningSystem::getBreakTime(state, pickTool);

    EXPECT_LT(toolTime, handTime);
}

TEST(MiningSystemTest, SilkTouchDropsBlock) {
    BlockID blockId = registerTestBlock("test:silk_block", 1.0f);
    BlockState state(blockId);

    MiningEnchantments enchants;
    enchants.silkTouch = 1;

    auto drops = MiningSystem::calculateDrops(state, ItemStack{}, enchants);
    ASSERT_EQ(drops.size(), 1u);
    EXPECT_EQ(drops[0].getItem(), blockId);
    EXPECT_EQ(drops[0].getCount(), 1);
}

TEST(MiningSystemTest, FortuneIncreasesDrops) {
    BlockID blockId = registerTestBlock("test:fortune_block", 1.0f, true,
                                         ToolType::Pickaxe, ToolTier::Wood,
                                         Material::Metal);
    BlockState state(blockId);

    ItemID pick = registerTestTool("test:fortune_pick",
                                    ToolProperties::Type::Pickaxe, 1);
    ItemStack tool(pick);

    MiningEnchantments noFortune;
    MiningEnchantments fortune3;
    fortune3.fortune = 3;

    auto normalDrops = MiningSystem::calculateDrops(state, tool, noFortune);
    auto fortuneDrops = MiningSystem::calculateDrops(state, tool, fortune3);

    ASSERT_EQ(normalDrops.size(), 1u);
    ASSERT_EQ(fortuneDrops.size(), 1u);
    EXPECT_GT(fortuneDrops[0].getCount(), normalDrops[0].getCount());
}

TEST(MiningSystemTest, NoDropsWithoutProperTool) {
    BlockID blockId = registerTestBlock("test:requires_pick", 1.5f, true,
                                         ToolType::Pickaxe, ToolTier::Wood,
                                         Material::Stone);
    BlockState state(blockId);

    auto drops = MiningSystem::calculateDrops(state, ItemStack{}, MiningEnchantments{});
    EXPECT_TRUE(drops.empty());
}

TEST(MiningSystemTest, ToolTierLevelMapping) {
    EXPECT_EQ(MiningSystem::getToolTierLevel(ItemStack{}), 0);

    ItemID woodTool = registerTestTool("test:tier_wood", ToolProperties::Type::Pickaxe, 1);
    EXPECT_EQ(MiningSystem::getToolTierLevel(ItemStack(woodTool)), 1);

    ItemID netheriteTool = registerTestTool("test:tier_netherite", ToolProperties::Type::Pickaxe, 5);
    EXPECT_EQ(MiningSystem::getToolTierLevel(ItemStack(netheriteTool)), 5);
}

TEST(MiningSystemTest, CancelMiningResetsState) {
    MiningSystem ms;
    ms.cancelMining();
    EXPECT_FALSE(ms.isMining());
    EXPECT_FLOAT_EQ(ms.getProgress(), 0.0f);
}
