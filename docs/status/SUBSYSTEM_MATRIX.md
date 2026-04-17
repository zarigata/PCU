Date: 2026-04-17

# Subsystem Status Matrix

A per-subsystem matrix showing file counts, implementation status, and evidence based on provided data.

| Subsystem | .cpp files | .h/.hpp files | Total | Status |
|-----------|-----------:|-------------:|------:|:---:|
| rendering | 22 | 20 | 42 | 🟡 Partial |
| world | 15 | 16 | 31 | 🟡 Partial |
| game | 9 | 8 | 17 | 🟡 Partial |
| entity | 8 | 4 | 12 | 🟡 Partial |
| core | 7 | 7 | 14 | 🟡 Partial |
| engine | 5 | 5 | 10 | 🟡 Partial |
| utils | 5 | 5 | 10 | 🟢 Implemented |
| networking | 5 | 2 | 7 | 🟡 Partial |
| physics | 3 | 3 | 6 | 🔴 Stubbed |
| modding | 4 | 2 | 6 | 🟡 Partial |
| scripting | 2 | 3 | 5 | 🔴 Stubbed |
| audio | 2 | 1 | 3 | 🔴 Stubbed |
| server (entry points) | 2 | 0 | 2 | N/A |
| externs (stubs) | 0 | 3 | 3 | 🔴 Stubbed |
| types | 0 | 1 | 1 | N/A |

## Subsystem Breakdown (Evidence)

- rendering
  - Notable issues: Vulkan*.cpp files are commented out due to missing headers; some Renderer TODOs remain.
- world
  - Large subsystem with many TODOs for world generation, chunk handling, and registries.
- game
  - World init and Vulkan rendering TODOs; several build directives comment out additional features.
- entity
  - Player.cpp excluded from CMake; scaffolding present in core entity files.
- core
  - Core utilities implemented: Logger.cpp, Timer.cpp, Memory.cpp; Window.cpp has event-dispatch TODOs; ECS.hpp had build issues.
- engine
  - EventSystem.cpp compiles; several components return false or placeholders; Config.cpp returns false on get(); server commands TODOs.
- utils
  - Implemented components; functions like Noise, Random, NBT, Profiler are functional.
- networking
  - Scaffold with NetworkManager, Server, Client; many return stubs and TODOs in server_main.
- physics
  - Stubbed physics stack; PhysX not present; uses PhysXStub.hpp.
- modding
  - Mod Loader and context scaffolding; JSON-to-Lua conversion and world access TODOs; API version returns 0.
- scripting
  - Lua engine is stubbed; bindings exist.
- audio
  - FMOD unavailable; audio system and sound manager are stubs.
- server (entry points)
  - Entry point files present; status not explicitly defined in the provided data.
- externs (stubs)
  - Explicitly stubs; evidence of stubs in codebase.
- types
  - Basic header presence; minimal typing support indicated.

## Critical Path

- Core
- Engine
- World
- Rendering
- Game

Reasoning: These subsystems contain the majority of TODOs and scaffolding that stall a full integration pass. They determine the viability of the main vertical slice from initialization to rendering and gameplay loop. The other subsystems provide supporting functionality (utils) or are stubbed scaffolds (audio, physics, scripting, networking, modding) that do not yet block a minimal run.

Date: 2026-04-17
