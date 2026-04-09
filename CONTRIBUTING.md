# Contributing to VoxelForge

Thank you for your interest in contributing to VoxelForge! This document provides guidelines and standards for contributing.

## Code Style

We use **clang-format** and **clang-tidy** for consistent code style and static analysis.

### Formatting

Run clang-format before committing:

```bash
# Format all files
find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Check formatting (CI)
find src -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror
```

### Static Analysis

Run clang-tidy to check for issues:

```bash
# Run analysis
find src -name "*.cpp" -o -name "*.h" | xargs clang-tidy
```

### Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Classes | `CamelCase` | `WorldManager` |
| Structs | `CamelCase` | `ChunkPosition` |
| Enums | `CamelCase` | `BlockType` |
| Functions | `camelBack` | `getBlockAt()` |
| Variables | `camelBack` | `blockPosition` |
| Constants | `UPPER_CASE` | `MAX_CHUNK_SIZE` |
| Member variables | `camelBack_` | `worldSize_` |
| Namespaces | `CamelCase` | `VoxelForge` |

### Code Guidelines

1. **Use C++20 features** when appropriate
2. **Prefer `std::unique_ptr` over raw pointers** for ownership
3. **Use `std::vector` and `std::string`** instead of raw arrays
4. **Follow RAII** - Resource Acquisition Is Initialization
5. **Keep functions small** - aim for cognitive complexity < 25
6. **Use const correctness** - mark methods that don't modify state
7. **Document public APIs** with Doxygen-style comments

## Build System

We use **CMake** for building.

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)

# Test
cd build && ctest
```

## Git Workflow

1. **Fork the repository**
2. **Create a feature branch** from `main`
3. **Make small, focused commits**
4. **Run tests locally** before pushing
5. **Open a Pull Request**

### Commit Messages

Use conventional commits:

```
feat: Add new block type
fix: Fix chunk loading issue
docs: Update API documentation
refactor: Improve rendering performance
test: Add unit tests for world generation
chore: Update dependencies
```

## Testing

All new features should include tests:

```bash
# Run tests
cd build && ctest --output-on-failure
```

## Documentation

- Use **Doxygen** comments for API documentation
- Update **README.md** for user-facing changes
- Add inline comments for complex algorithms

## Pull Request Process

1. Ensure all CI checks pass
2. Request review from maintainers
3. Address review feedback
4. Squash commits before merge

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

---

*Last updated: 2026-04-09*