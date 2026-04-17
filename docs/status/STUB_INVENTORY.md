Date: 2026-04-17

This document lists every known stub, TODO, and placeholder observed in the codebase, organized by subsystem and severity. It uses the exact data provided and does not introduce new items.

## Critical Path Blockers
- WORLD: World.cpp L130, L137, L159, L170, L226 — TODOs for chunk saving, unloading, features, and caves. Severity HIGH. Blocks vertical slice.
- WORLD: ChunkManager.cpp L71, L85, L94 — core chunk lifecycle and saving. Severity HIGH.
- WORLD: ChunkMesher.cpp L237, L427, L446, L278, L290, L295, L640, L649 — color, lighting, texture, and fallbacks. Severity HIGH.
- WORLD: LightEngine.cpp L104, L108 — incremental lighting and removal. Severity HIGH.
- WORLD: AnvilLoader.cpp L31, L51, L83, L96, L98 — loading/saving lifecycle. Severity HIGH.
- WORLD: Chunk.cpp L82, L126, L476, L236, L327, L334, L399, L357, L366, L431 — core data structure stubs. Severity HIGH.
- WORLD: ChunkSection.cpp L86, L362, L431 — section stubs. L474 MEDIUM.
- WORLD: FluidSystem.cpp L214, L378, L387, L397 — content stubs. Severity LOW.
- RENDERING: Game.cpp L31, L86, L105, L120 — game loop and rendering stubs. Severity HIGH.
- RENDERING: Window.cpp L102, L108, L114, L120, L126, L132, L138 — event dispatch stubs. Severity HIGH.
- RENDERING: Vulkan-related files in CMakeLists.txt (VulkanPipeline.cpp, VulkanImage.cpp, VulkanDescriptor.cpp, VulkanFramebuffer.cpp) — missing headers. Severity HIGH.
- ENGINE: ResourceManager.cpp — texture/shader/model loading stubs. Severity HIGH.
- PHYSICS — ENTIRELY STUBBED: PhysXStub.hpp, PhysicsSystem.cpp, CharacterController.cpp, CollisionManager.cpp — stub implementations. Severity HIGH.

##WORLD (Critical Path — Block vertical slice)
- World.cpp: L130: TODO; L137: TODO; L159: TODO; L170: TODO; L226: TODO
- ChunkManager.cpp: L71: return nullptr (getChunk); L85: TODO: Call world generator; L94: TODO: Save chunk before unloading
- ChunkMesher.cpp: L237: TODO: Get actual block color based on biome; L427: TODO: Implement proper light interpolation; L446: TODO: Get actual texture coordinates from texture atlas; L278/L290/L295: return false; L640: return false; L649: return nullptr
- LightEngine.cpp: L104: TODO: Implement incremental light updates; L108: TODO: Implement light removal
- AnvilLoader.cpp: L31/L51/L83: return nullptr; L96: TODO: Implement chunk saving; L98: return false
- Chunk.cpp: L82/L126/L476: return false; L236/L327/L334/L399: return nullptr; L357/L366/L431: return 0
- ChunkSection.cpp: L86: return false; L362: return nullptr; L431: return 0
- FluidSystem.cpp: L214: return false; L378/L387: return 0; L397: return false
- Game.cpp: L31: TODO: Initialize world; L86: TODO: Vulkan rendering; L105: TODO: Player movement; L120: TODO: Apply to camera
- Window.cpp: L102: TODO: Dispatch WindowResizeEvent; L108: Dispatch WindowCloseEvent; L114: Dispatch KeyEvent; L120: Dispatch MouseButtonEvent; L126: Dispatch MouseScrollEvent; L132: Dispatch MouseMovedEvent; L138: Dispatch KeyTypedEvent
- Vulkan rendering: VulkanPipeline.cpp, VulkanImage.cpp, VulkanDescriptor.cpp, VulkanFramebuffer.cpp — missing headers; build blockers

## RENDERING (Critical Path)
- Game.cpp: L31, L86, L105, L120
- Window.cpp: L102, L108, L114, L120, L126, L132, L138
- Vulkan-related files in CMakeLists.txt: VulkanPipeline.cpp, VulkanImage.cpp, VulkanDescriptor.cpp, VulkanFramebuffer.cpp

## ENGINE
- ResourceManager.cpp: TODOs for texture, shader, and model loading

## PHYSICS — ENTIRELY STUBBED
- PhysXStub.hpp: Physics system is a stub; not built
- PhysicsSystem.cpp: Stub implementation
- CharacterController.cpp: Stub implementation
- CollisionManager.cpp: Stub implementation

## AUDIO — ENTIRELY STUBBED
- FMODStub.hpp: Stub; FMOD not in build
- AudioSystem.cpp: "initialized (stub - FMOD not available)"
- SoundManager.cpp: Stub

## SCRIPTING — STUBBED
- LuaEngine.cpp: Header notes indicate stubbed behavior

## MODDING — SCAFFOLDING
- ModLoader.cpp: L288,511,519,536,544: return false/nullptr
- ModContext.cpp: L65: TODO JSON to Lua; L81: TODO Lua to JSON; L182-183: TODO get world; L188: return nullptr
- NativePlugin.cpp: getAPIMinorVersion() returns 0

## NETWORKING — SCAFFOLDING
- Packet.cpp: multiple return 0 stubs
- Server.cpp (server_main.cpp): L56: TODO initialize server systems; L68: TODO server tick

## GAME SYSTEMS
- CraftingSystem.cpp: L18, L23, L26, L50, L131: return false; L90, L349: return nullptr
- RecipeRegistry.cpp: L123: TODO handle item containers; L539: TODO load recipes from JSON
- AchievementSystem.cpp: L202, L271, L278: TODO load/save; L518: TODO grant experience; L523: TODO unlock recipes; L527: TODO execute function
- Inventory.cpp: L149: Lightweight stubs for modding/loader API compatibility (no-op)
- DayNightCycle.cpp: L242, L253, L267, L272: return false

## UTILS
- Compression.cpp: L199: "Fast Compression (LZ4 stub - would need LZ4 library)"

## SUMMARY STATISTICS
- Total files with stubs/TODOs: 30+
- HIGH severity stubs: ~25
- MEDIUM severity: ~10
- LOW severity: ~15
- Entire subsystems stubbed: Physics (PhysX), Audio (FMOD)
- Entire subsystems commented out of build: 4 Vulkan core components, plus other large subsystems

Date: 2026-04-17
