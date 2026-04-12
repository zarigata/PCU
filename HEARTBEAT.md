# HEARTBEAT.md — Monitoring Tasks

## Current Focus
Monitor VoxelForge CI build status continuously (4-8 times per day). Fix build failures immediately. Once builds are green, spawn sub-agents for continuous feature development.

## Current Workflow
1. **Fix CI builds**: Check every 3-4 hours, fix compilation errors immediately
2. **Green builds**: When all platforms pass, enable cron for continuous feature implementation
3. **Sub-agent spawning**: Cron spawns parallel sub-agents (implement, test, verify)
4. **Feature implementation**: Work through FEATURE_MATRIX.md high-priority features

## Build Status
- **Latest build:** ⏳ FIX PUSHED (2026-04-12 13:33 UTC)
  - Combat commit: `82e744a` feat(combat): implement 1.9+ attack cooldown system - FAILED
  - Fix 1: `d75a290` fix(build): correct Entity type and includes in EntityBase.cpp - FAILED
  - Fix 2: `30551d2` fix(build): correct ECS API usage and types in entity systems - FAILED
  - Fix 3: `71ce40d` fix(build): add missing system class declarations and fix ProjectileEntity - FAILED
  - Fix 4: `03f3c68` fix(build): remove duplicate code in ProjectileEntity update - FAILED
  - Fix 5: `3e451a4` fix(build): add ItemEntitySystem declaration and fix ProjectileEntity - FAILED
  - Fix 6: `9cf26d2` fix(build): fix ItemEntity and ProjectileEntity API usage - FAILED
  - Fix 7: `8c037f7` fix(build): fix PlayerInventory API access in ItemEntity - FAILED
  - Fix 8: `6f0948f` fix(build): fix std::min type mismatch in ItemEntity - FAILED
  - Fix 9: `5c77e64` fix(build): fix Vulkan logging and include paths - FAILED
  - Fix 10: `39d5cf4` fix(build): fix VulkanSwapchain include and Vulkan logging/callback - FAILED
  - Fix 11: `6aa5a47` fix(build): fix Vulkan logging macros and include paths
  - Fixed 15 files total: Entity.hpp, Entity.cpp, EntityBase.cpp, LivingEntity.cpp, MobBase.cpp, ProjectileEntity.cpp, ItemEntity.cpp, VulkanDevice.cpp, VulkanContext.cpp, VulkanSwapchain.cpp, VulkanCommandBuffer.cpp
  - Monitoring CI for latest fix
- **Website:** Live and working
- **GitHub Issue:** #6 created for stuck Day/Night build

## Action Items
- ✅ CI build stuck for >30 min → Created GitHub issue #6
- ✅ CI build fails (10 rounds total) → Fixed GLM, UUID, and Entity type errors
- ⏳ Combat system + rendering build failed → 11 rounds of fixes pushed
  - Round 1: Entity type and includes (EntityBase.cpp)
  - Round 2: ECS API usage and types (4 files)
  - Round 3: System class declarations and ProjectileEntity fixes (3 files)
  - Round 4: Removed duplicate code in ProjectileEntity update
  - Round 5: Added ItemEntitySystem declaration and fixed ProjectileEntity logging
  - Round 6: Complete ItemEntity rewrite and ProjectileEntity pointer fix
  - Round 7: Fixed PlayerInventory API access (mainInventory doesn't exist)
  - Round 8: Fixed std::min type mismatch (ItemCount → int)
  - Round 9: Fixed Vulkan logging (LOG_TRACE/VF_DEBUG) and include path
  - Round 10: Fixed VulkanSwapchain include, Logger usage, lambda callback
  - Round 11: Fixed VulkanCommandBuffer include, VF_DEBUG → VF_TRACE, broken ternary
- ⏳ Verify new build passes → Monitoring CI for latest fix

## Notes
- ⏸️ Cron `pcu-improve`: ENABLED for continuous improvement
  - Schedule: Every 12 hours (0 */12 * * *)
  - Mode: Isolated sessions (spawns sub-agents)
  - Last run: 2026-04-12 12:01 UTC — Implemented World Border system
  - Previous run: 2026-04-12 00:01 UTC — Implemented 1.9+ attack cooldown combat system
  - Next run: ~12 hours from now (00:01 UTC)
  - Rate limit handling: 60s initial wait, 300s on 429, 1 retry
  - Timeout: 3600s (1 hour)
- Recent commits (rebrand + 17 fixes):
  - Rebranding: minecraft: → poorcraftultra: (629 occurrences)
  - CMake version conflict: enet dependency fixed
  - DayTime field access: Game class fixed
  - Incomplete type error: World class forward declaration
  - Designated initializer order: fluid blocks
  - BlockProperty initializer order: fluid level properties
  - BlockRenderType error: replaced with RenderType
  - Fix 7 compilation errors (1st attempt): ChunkPos, ChunkMesher, FluidSystem, BiomeBlender
  - Fix redefinition and hash errors (2nd attempt): ChunkPos duplicate constants, malformed namespace
  - Fix hash, AIR_BLOCK, logging, and API errors (3rd attempt): 5 files fixed
  - Fix BlockDefinition isSeeThrough and EntityManager errors (4th attempt): 2 files fixed
  - Fix duplicate class definitions and missing includes (5th attempt): 4 files fixed
  - Fix missing types and logging macros (6th attempt): 2 files fixed
  - Fix circular dependency and Entity type errors (7th attempt): 2 files fixed
  - Fix UUID and AABB redefinition errors (8th attempt): 1 file fixed
  - Sync include guards between Engine.hpp and ECS.hpp (9th attempt): 1 file fixed
  - Fix GLM, UUID, and Entity type errors (10th attempt): 2 files fixed
  - ✅ Builds GREEN after 10th round of fixes
  - feat(combat): 1.9+ attack cooldown system (Player.hpp, Player.cpp, Inventory.hpp, LivingEntity.cpp)
  - fix(build): correct Entity type and includes in EntityBase.cpp (combat system fix)
  - fix(build): add missing system class declarations and fix ProjectileEntity (3rd round)
  - fix(build): add ItemEntitySystem declaration and fix ProjectileEntity (5th round)
  - fix(build): fix ItemEntity and ProjectileEntity API usage (6th round)
  - fix(build): fix PlayerInventory API access in ItemEntity (7th round)
  - fix(build): fix std::min type mismatch in ItemEntity (8th round)
  - fix(build): fix Vulkan logging and include paths (9th round)
  - fix(build): fix VulkanSwapchain include and Vulkan logging/callback (10th round)
  - fix(build): fix Vulkan logging macros and include paths (11th round)
  - feat(world): implement World Border system
- Recent features implemented:
  - Fluid Physics System (water/lava flow simulation, scheduled updates, fluid mixing)
  - ChunkManager/LightEngine/AnvilLoader headers (world management infrastructure)
  - Biome expansion: 13 → 50 biomes (forest, mountain, ocean, badlands, swamp, etc.)
  - BiomeBlender: Smooth biome transitions with radius-based sampling and interpolation
  - Chunk loading/unloading: View-distance based, load queue, automatic unloading
  - Lighting engine: Sky light (from height map), block light (from emissive blocks)
  - Anvil loader: Minecraft Anvil format support (region files, NBT parsing)

## Next Priority Features (when builds are green)
From FEATURE_MATRIX.md - High priority:
- ✅ Chunk loading/unloading (ChunkManager added)
- ✅ Infinite world generation (WorldGenerator complete)
- ✅ World saving/loading (AnvilLoader added)
- ✅ Biomes 50+ types (expanded to 50 biomes)
- ✅ Biome blending (BiomeBlender implemented)
- ✅ World borders (WorldBorder class implemented)
- 📋 Caves (generateCaves() exists, needs verification)
- 📋 Rivers (needs implementation)
- 📋 Nether dimensions (needs implementation)
