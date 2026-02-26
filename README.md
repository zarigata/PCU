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

VoxelForge is a **complete, open-source Minecraft clone** built in modern C++20/23 featuring:
- Full Minecraft feature parity (blocks, items, entities, dimensions, etc.)
- Integrated dual-layer modding system (Native C++ + Lua scripting)
- High-performance Vulkan rendering
- Multiplayer support

## Quick Links

| Document | Description |
|----------|-------------|
| [Master Architecture Plan](docs/architecture/MASTER_PLAN.md) | Complete system architecture |
| [Feature Matrix](docs/architecture/FEATURE_MATRIX.md) | All features to implement |
| [Modding API](docs/modding/MODDING_API.md) | Complete modding documentation |

## Technical Stack

| Component | Technology |
|-----------|------------|
| Language | C++20/23 |
| Renderer | Vulkan 1.3 |
| Physics | NVIDIA PhysX 5.x |
| Audio | FMOD 2.02+ |
| Networking | ENet |
| Scripting | Lua 5.4 / LuaJIT |
| Build | CMake 3.26+ |

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
│   ├── VoxelForge/         # Public headers
│   └── thirdparty/         # Third-party headers
├── assets/                 # Game assets
│   ├── textures/
│   ├── models/
│   ├── audio/
│   └── shaders/
├── mods/                   # Mod directory
│   ├── core/               # Core mod
│   └── example_mod/        # Example mod
├── tools/                  # Development tools
├── tests/                  # Test suites
├── docs/                   # Documentation
├── scripts/                # Build/utility scripts
├── cmake/                  # CMake modules
└── thirdparty/             # Third-party libraries
```

## Building

### Prerequisites

- CMake 3.26+
- C++20 compatible compiler (GCC 12+, Clang 15+, MSVC 2022+)
- Vulkan SDK 1.3+
- Git

### Build Commands

```bash
# Clone repository
git clone https://github.com/voxelforge/voxelforge.git
cd voxelforge

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# Run
./bin/VoxelForge
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `VOXELFORGE_BUILD_TESTS` | ON | Build unit tests |
| `VOXELFORGE_BUILD_BENCHMARKS` | OFF | Build benchmarks |
| `VOXELFORGE_BUILD_TOOLS` | ON | Build dev tools |
| `VOXELFORGE_ENABLE_SANITIZERS` | OFF | Enable sanitizers |
| `VOXELFORGE_USE_LUAJIT` | ON | Use LuaJIT |

## Development Roadmap

### Phase 1: Foundation (Months 1-3)
- Core engine setup
- Vulkan rendering foundation
- Basic world generation

### Phase 2: Gameplay Core (Months 4-6)
- Player physics
- Inventory system
- Lighting system

### Phase 3: Entities & AI (Months 7-9)
- ECS implementation
- Mob AI system
- Combat system

### Phase 4: Advanced Features (Months 10-15)
- Networking/multiplayer
- Redstone system
- Dimensions & bosses

### Phase 5: Polish & Modding (Months 16-21)
- Modding system
- Audio system
- Final content

### Phase 6: Release & Support (Ongoing)
- 1.0.0 Release
- Community updates

## Statistics

| Metric | Value |
|--------|-------|
| Estimated Duration | 18-24 months |
| Block Types | 800+ |
| Item Types | 600+ |
| Entity Types | 70+ |
| Biome Types | 60+ |
| Commands | 50+ |
| Total Features | 1,600+ |

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
