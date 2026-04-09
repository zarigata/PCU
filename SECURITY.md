# Security Policy

## Supported Versions

| Version | Supported          | Status |
| ------- | ------------------ | ------ |
| main    | ✅ Active development | Alpha |

## Reporting a Vulnerability

**DO NOT** open a public issue for security vulnerabilities.

Instead, please report security issues by:

1. **GitHub Security Advisory** (preferred)
   - Go to https://github.com/zarigata/PCU/security/advisories
   - Click "Report a vulnerability"

2. **Email** (alternative)
   - Contact the maintainer via GitHub

### What to Include

- Description of the vulnerability
- Steps to reproduce
- Affected versions
- Potential impact
- Suggested fix (if available)

### Response Timeline

- **Initial response**: Within 48 hours
- **Status update**: Within 7 days
- **Fix timeline**: Depends on severity

## Security Considerations for VoxelForge

### Multiplayer Security

- Server-client communication should use encryption (TLS)
- Packet validation for all network messages
- Anti-cheat measures for fair gameplay
- Rate limiting for server connections

### Code Safety

- Input validation for all user data
- Buffer overflow prevention (use std::string, std::vector)
- Bounds checking for chunk coordinates
- Safe deserialization for save files and network packets

### External Dependencies

| Dependency | Purpose | Security Notes |
|------------|---------|----------------|
| Vulkan | Graphics API | Maintained by Khronos |
| GLFW | Window/input | Well-maintained |
| GLM | Math library | Header-only, stable |
| spdlog | Logging | No known vulnerabilities |
| EnTT | Entity system | Header-only, safe |
| sol2 | Lua bindings | Validate Lua scripts |

## Known Issues

- Alpha stage - not production ready
- Multiplayer authentication not implemented
- No packet encryption yet

---

*Last updated: 2026-04-09*