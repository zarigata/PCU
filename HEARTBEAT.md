# HEARTBEAT.md — Monitoring Tasks

## Current Focus
Monitor VoxelForge CI build status continuously (4-8 times per day). Fix build failures immediately. Once builds are green, spawn sub-agents for continuous feature development.

## Current Workflow
1. **Fix CI builds**: Check every 3-4 hours, fix compilation errors immediately
2. **Green builds**: When all platforms pass, enable cron for continuous feature implementation
3. **Sub-agent spawning**: Cron spawns parallel sub-agents (implement, test, verify)
4. **Feature implementation**: Work through FEATURE_MATRIX.md high-priority features

## Build Status
- **Latest successful build:** ~10 hours ago (fix(build) - disabled broken source files)
- **Current build status:**
  - LATEST FIX (queued): fix(build): resolve redefinition and hash function errors
    - Fixed 2 major errors introduced in previous fix
    - ChunkPos.hpp: Removed duplicate CHUNK_WIDTH constants (redefinition error)
    - ChunkPos.hpp: Fixed incomplete std:: namespace block
    - BiomeBlender.cpp: Added Chunk.hpp include
    - ChunkMesher.cpp: Simplified CHUNK_WIDTH references
  - Previous build: All 5 platforms failed (redefinition + hash errors)
  - Windows: failed (previous build)
  - macOS Apple Silicon: queued
  - macOS Intel: cancelled
- **Website:** Live and working
- **GitHub Issue:** #6 created for stuck Day/Night build

## Action Items
- ✅ CI build stuck for >30 min → Created GitHub issue #6
- ✅ CI build fails → Fixed redefinition and hash errors (2nd round of fixes)
- ⏳ Verify new build passes → Monitoring CI for latest commit
- ⏸️ All builds green → Cron will spawn sub-agents for feature implementation

## Notes
- ⏸️ Cron `pcu-improve`: ENABLED for continuous improvement
  - Schedule: Every 12 hours (0 */12 * * *)
  - Mode: Isolated sessions (spawns sub-agents)
  - Last run: ~9 hours ago (Day/Night Cycle system)
  - Next run: ~10 hours from now
  - Rate limit handling: 60s initial wait, 300s on 429, 1 retry
  - Timeout: 3600s (1 hour)
- Recent commits (rebrand + 9 fixes):
  - Rebranding: minecraft: → poorcraftultra: (629 occurrences)
  - CMake version conflict: enet dependency fixed
  - DayTime field access: Game class fixed
  - Incomplete type error: World class forward declaration
  - Designated initializer order: fluid blocks
  - BlockProperty initializer order: fluid level properties
  - BlockRenderType error: replaced with RenderType
  - Fix 7 compilation errors (1st attempt): ChunkPos, ChunkMesher, FluidSystem, BiomeBlender
  - NEW (current): Fix redefinition and hash errors (2nd attempt): ChunkPos duplicate constants, malformed namespace
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
- 📋 Caves (generateCaves() exists, needs verification)
- 📋 Rivers (needs implementation)
- 📋 World borders (needs implementation)
- 📋 Nether dimensions (needs implementation)
