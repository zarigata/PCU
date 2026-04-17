# Runtime Dependencies

**Date:** 2026-04-17

## Build-Time Dependencies (FetchContent — resolved at configure)

| Dependency | Version | Purpose | Notes |
|------------|---------|---------|-------|
| GLFW | 3.3.8 | Window management | Git fetch |
| GLM | 1.0.1 | Math library | Git fetch |
| nlohmann/json | v3.11.3 | JSON parsing | Git fetch |
| stb | master | Image loading | **Warning: uses `master` tag — non-deterministic** |
| glslang | 16.2.0 | Shader compilation | Git fetch |
| tinygltf | v2.8.21 | glTF model loading | Git fetch |
| spdlog | v1.12.0 | Logging | Git fetch |
| zstd | v1.5.7 | Compression | Git fetch, static build |
| ENet | v1.3.17 | Networking (UDP) | Git fetch |
| sol2 | v3.3.1 | Lua bindings | Git fetch |
| Lua | v5.4.6 | Scripting engine | Git fetch |
| GoogleTest | v1.14.0 | Unit testing | Tests only |

## System Dependencies (find_package)

| Dependency | Required | Platform Notes |
|------------|----------|----------------|
| Vulkan SDK | Yes | 1.3+; CI uses 1.3.296.0; must have compatible GPU driver |
| OpenGL | Yes | System package; linked as GL |
| Freetype | Yes | vcpkg on Windows; brew on macOS; apt/dnf on Linux |

## Claimed But NOT Present

| Claimed Dependency | Where Claimed | Reality |
|--------------------|---------------|---------|
| NVIDIA PhysX 5.x | README.md "Technical Stack" | **STUB**: `include/VoxelForge/extern/PhysXStub.hpp` |
| FMOD 2.02+ | README.md "Technical Stack" | **STUB**: `include/VoxelForge/extern/FMODStub.hpp` |

These stubs allow compilation but provide no functionality. The physics and audio subsystems are entirely stubbed.

## CI Dependency Installation

| Platform | Method |
|----------|--------|
| Windows | vcpkg (Freetype), Vulkan SDK installer action |
| macOS | brew: cmake, vulkan-loader, vulkan-headers, freetype |
| Ubuntu | apt: libvulkan-dev, libfreetype6-dev, libgl1-mesa-dev, X11 libs |
| Fedora | dnf: vulkan-loader-devel, freetype-devel, mesa-libGL-devel |
| Arch | pacman: vulkan-icd-loader, freetype2, mesa |

## Missing Runtime Assets

| Asset | Expected Location | Status |
|-------|-------------------|--------|
| Game textures, models, sounds, shaders | `assets/` | **DIRECTORY DOES NOT EXIST** |
| Mod files | `mods/` | **DIRECTORY DOES NOT EXIST** |

CMake `install(DIRECTORY assets DESTINATION share/VoxelForge)` will fail on fresh clone.
CI uses `cp -r assets ... 2>/dev/null || true` to silently skip missing assets.

## Key Risks

1. **Non-deterministic builds**: stb uses `master` tag — different clones may get different versions
2. **Network dependency**: First build requires internet for FetchContent; offline builds fail
3. **Missing assets**: Any build artifact is unrunnable without textures/shaders
4. **Fake dependencies**: PhysX and FMOD claimed but stubbed — misleading to contributors
5. **Vulkan requirement**: Not all machines have Vulkan-capable GPUs; no software fallback
6. **No vendoring**: All deps fetched at configure time; no fallback if GitHub is down

## Recommendations

1. Pin stb to a specific commit hash instead of `master`
2. Create `assets/` with minimal placeholder content (even a single texture)
3. Remove PhysX and FMOD from README or clearly mark as "planned, currently stubbed"
4. Consider vendoring critical deps for offline CI reliability
5. Add a CMake check that warns (not errors) when assets/ is missing
