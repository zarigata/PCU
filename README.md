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

## Current Status: 🚧 In Development

| Component | Status | Progress |
|-----------|--------|----------|
| Core Systems | ✅ | 90% |
| Window/Input | ✅ | 100% |
| Event System | ✅ | 100% |
| Memory Management | ✅ | 100% |
| Camera System | ✅ | 100% |
| Block Registry | 🔄 | 30% |
| Chunk System | 🔄 | 40% |
| World Generation | ⏳ | 0% |
| Vulkan Renderer | ⏳ | 0% |
| Entity System | ⏳ | 10% |
| Physics | ⏳ | 0% |
| Audio | ⏳ | 0% |
| Networking | ⏳ | 0% |
| Modding API | ⏳ | 0% |

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

## Implemented Features

### Core Systems ✅
- **Logger** - spdlog-based logging with file rotation
- **Timer** - High-resolution timing and FPS counter
- **Memory** - Arena allocators for performance
- **ECS** - Custom Entity Component System

### Platform ✅
- **Window** - GLFW window management
- **Input** - Full keyboard/mouse input handling
- **Events** - Type-safe event bus

### Utilities ✅
- **Noise** - Perlin, Simplex, Voronoi noise
- **Random** - XorShift128+ PRNG
- **NBT** - Named Binary Tag serialization
- **Compression** - Zstandard streaming compression

### World 🔄
- **BlockRegistry** - 20+ vanilla blocks
- **Block States** - Property system for variants
- **Chunk System** - Paletted storage, light data

### Rendering 🔄
- **Camera** - Perspective/orthographic with frustum culling

## Statistics

| Metric | Current | Target |
|--------|---------|--------|
| Lines of Code | 7,382 | 200,000+ |
| Commits | 9 | - |
| Block Types | 20+ | 800+ |
| Item Types | 0 | 600+ |
| Entity Types | 0 | 70+ |

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
