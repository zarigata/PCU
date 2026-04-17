# VoxelForge

```
██╗   ██╗ ██████╗ ██╗██████╗ ███████╗██████╗ 
██║   ██║██╔═══██╗██║██╔══██╗██╔════╝██╔══██╗
██║   ██║██║   ██║██║██║  ██║█████╗  ██████╔╝
╚██╗ ██╔╝██║   ██║██║██║  ██║██╔══╝  ██╔══██╗
 ╚████╔╝ ╚██████╔╝██║██████╔╝███████╗██║  ██║
  ╚═══╝   ╚═════╝ ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝
        C++ Minecraft Clone with Integrated Modding
```

## Overview

VoxelForge is a C++20 voxel sandbox game aiming to become a Minecraft-like experience. Currently in **Phase 0: Build Rescue & Truth Establishment** — the project compiles but is not yet playable.

## Current Phase: Phase 0 — Build Rescue

The project was previously documented as "100% complete" across all subsystems. An honest audit (2026-04-17) revealed this was inaccurate. We are now establishing a truthful baseline before adding features.

**Audit documents:** [`docs/status/BUILD_TRUTH.md`](docs/status/BUILD_TRUTH.md) | [`docs/status/SUBSYSTEM_MATRIX.md`](docs/status/SUBSYSTEM_MATRIX.md) | [`docs/status/STUB_INVENTORY.md`](docs/status/STUB_INVENTORY.md) | [`docs/status/RUNTIME_DEPENDENCIES.md`](docs/status/RUNTIME_DEPENDENCIES.md)

## Subsystem Reality

| Subsystem | Status | Notes |
|-----------|--------|-------|
| Core (Logger, Timer, Memory) | 🟢 Implemented | Tested, functional |
| ECS | 🟢 Implemented | 16 tests passing |
| Utils (Noise, Random, NBT, Compression) | 🟢 Implemented | Core utilities work |
| Window/Input | 🔴 Stubbed | All event dispatch is TODO |
| World Generation | 🟡 Partial | Scaffolding exists; features/caves/save TODO |
| Chunk System | 🟡 Partial | Data structures exist; save/load TODO |
| Vulkan Renderer | 🔴 Broken | 4 core components excluded from build |
| Physics | 🔴 Stubbed | PhysX not present; compile-time stub |
| Audio | 🔴 Stubbed | FMOD not present; compile-time stub |
| Networking | 🟡 Scaffolding | Files exist; internal stubs |
| Scripting (Lua) | 🔴 Stubbed | LuaEngine is stub |
| Modding | 🟡 Scaffolding | Loader/registry files exist; internal stubs |
| Game Loop | 🔴 Stubbed | World init, rendering, movement all TODO |
| Save/Load | 🔴 TODO | AnvilLoader chunk save is TODO |

## Quick Links

| Document | Description |
|----------|-------------|
| [Master Architecture Plan](docs/architecture/MASTER_PLAN.md) | Complete system architecture |
| [Feature Matrix](docs/architecture/FEATURE_MATRIX.md) | All features to implement |
| [Modding API](docs/modding/MODDING_API.md) | Complete modding documentation |

## Technical Stack

| Component | Technology | Status |
|-----------|------------|--------|
| Language | C++20 | Active |
| Renderer | Vulkan 1.3 | Partial (4 components excluded) |
| Window | GLFW 3.3.8 | Active |
| Math | GLM 1.0.1 | Active |
| Networking | ENet 1.3.17 | Scaffolding |
| Scripting | Lua 5.4 / sol2 | Stubbed |
| Physics | Custom (PhysX planned) | Stubbed |
| Audio | Custom (FMOD planned) | Stubbed |
| Build | CMake 3.26+ | Active |

## Building

### Prerequisites

- CMake 3.26+
- C++20 compatible compiler (GCC 12+, Clang 15+, MSVC 2022+)
- Vulkan SDK 1.3+
- Git

### Build Commands

```bash
# Clone repository
git clone https://github.com/zarigata/PCU.git
cd PCU

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# Run
./bin/VoxelForge
```

## Project Structure

```
VoxelForge/
├── src/                    # Source files
│   ├── core/               # Core engine systems
│   ├── engine/             # Engine management
│   ├── world/              # World, chunks, blocks
│   ├── entity/             # Entity system
│   ├── rendering/          # Vulkan rendering
│   ├── physics/            # PhysX integration
│   ├── audio/              # FMOD audio
│   ├── networking/         # ENet multiplayer
│   ├── scripting/          # Lua engine
│   ├── modding/            # Mod loader
│   ├── game/               # Game logic
│   └── utils/              # Utilities
├── include/                # Header files
├── assets/                 # Game assets
├── mods/                   # Mod directory
├── tests/                  # Test suites
└── docs/                   # Documentation
```

## Vertical Slice Status

NOT YET ACHIEVED. The game cannot currently:
- Launch to a menu
- Create or load a world
- Accept player input (event dispatch is TODO)
- Render anything (4 Vulkan components excluded from build)
- Save or load progress
- Play for 20 minutes without crashing

See [`docs/status/STUB_INVENTORY.md`](docs/status/STUB_INVENTORY.md) for the full list of blocking stubs.

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

MIT License - see [LICENSE](LICENSE) for details.

## Acknowledgments

- Mojang Studios for Minecraft inspiration
- Vulkan, PhysX, FMOD, ENet communities
- All contributors and testers

---

*"I'll be back... with working code."*

**- T-800**
