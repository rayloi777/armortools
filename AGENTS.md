# AGENTS.md - Iron Engine Developer Guide

This file provides coding guidelines for agents working on the iron 3D engine (part of armortools).

## Build Commands

### Building the Engine (base)

```bash
cd base

# Using the bundled amake tool
# On macOS/Linux:
./make [platform] [graphics]

# On Windows:
make.bat [platform] [graphics]

# Examples:
./make macos metal      # macOS with Metal backend
./make linux vulkan     # Linux with Vulkan backend
./make windows direct3d12  # Windows with Direct3D12
```

### Building Paint (ArmorPaint Application)

```bash
cd paint

# Build for current platform (automatic detection)
../base/make

# Build and run immediately (Linux)
../base/make --run

# Build for specific target platform
../base/make --target android
../base/make --target ios
../base/make --target wasm

# Embed data files (requires C23 #embed support, clang 19+)
../base/make --embed

# Generate locale file
cd ../base
./make --js base/tools/extract_locales.js <locale_code>
# Output: paint/assets/locale/<locale_code>.json
```

### Building Tests

Tests are located in `base/tests/`. Each test has its own `project.js`.

```bash
# Run a specific test (e.g., cube)
cd base/tests/cube
../../make [platform] [graphics]

# Available test projects:
# - cube      - Basic 3D cube rendering
# - fall      - Physics/falling objects test
# - triangle  - Simple triangle rendering
```

### Make Tool Flags

The amake build system (based on kmake) supports these common flags:

| Flag | Description |
|------|-------------|
| `[platform]` | Target platform (macos, linux, windows, android, ios, wasm) |
| `[graphics]` | Graphics backend (metal, vulkan, direct3d12, opengl, glsl) |
| `--run` | Build and execute after compilation (Linux) |
| `--target` | Specify target platform for cross-compilation |
| `--compile` | Compile without building (WASM) |
| `--embed` | Embed data files using C23 #embed |
| `--nowindow` | Run without creating a window (Linux) |
| `--js` | Run JavaScript tool from tools/ |

## Code Style Guidelines

### Formatting

- **Indentation**: 4 spaces (no tabs). The project uses `.clang-format` at repository root.
- **Column limit**: 160 characters
- **Braces**: K&R style - opening brace on same line, `else` on new line with brace before `else`
- **Line endings**: LF (not CRLF)

### Naming Conventions

- **Types**: `snake_case_t` (e.g., `iron_system_t`, `u32`)
- **Functions**: `module_function` prefix (e.g., `iron_log()`, `iron_file_read()`)
- **Constants**: `IRON_CONSTANT` uppercase with underscores
- **Variables**: `snake_case` (e.g., `file_location`, `buffer_size`)
- **Globals**: Prefix with module name (e.g., `update_callback`)
- **Statics**: `snake_case` (e.g., `static char *fileslocation`)

### Type Aliases (from iron_global.h)

```c
typedef float    f32;
typedef double   f64;
typedef int8_t   i8;
typedef uint8_t  u8;
typedef int16_t  i16;
typedef uint16_t u16;
typedef int32_t  i32;
typedef uint32_t u32;
typedef int64_t  i64;
typedef uint64_t u64;
```

Use these instead of raw C types for consistency.

### File Organization

- **Headers (.h)**: Declarations, type definitions, inline functions
- **Sources (.c)**: Implementation
- **Naming**: `module_name.h` and `module_name.c` pairs
- **Include guard**: `#pragma once` in headers (preferred)

### Imports/Includes

- System includes first (`<stdio.h>`, `<stdlib.h>`)
- Platform-specific includes with `#ifdef` guards
- Project headers after (e.g., `#include "iron_system.h"`)
- Sort includes alphabetically within groups
- Use quotes for local headers, brackets for system headers

```c
#include "iron_system.h"
#include "iron_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef IRON_WINDOWS
#include <windows.h>
#endif
```

### Error Handling

- Use `iron_log()` for informational messages
- Use `iron_error()` for errors (shows message box on Windows)
- Return error codes where appropriate (0 for success, -1 for error)
- Check return values of file operations

### Platform-Specific Code

Wrap platform-specific code with defines:
- `IRON_WINDOWS`, `IRON_LINUX`, `IRON_MACOS`, `IRON_IOS`, `IRON_ANDROID`, `IRON_WASM`, `IRON_POSIX`
- Keep platform code in `sources/backends/` directory

```c
#ifdef IRON_WINDOWS
    // Windows-specific code
#elif defined(IRON_LINUX)
    // Linux-specific code
#endif
```

### Memory Management

- Use `malloc()`/`free()` for dynamic allocation
- Check for NULL after allocation
- Match allocation with appropriate free
- Consider using the GC (garbage collector) in `iron_gc.h` when appropriate

### Functions

- Keep functions focused and single-purpose
- Use static for internal functions
- Document complex functions with comments
- Return early for error conditions

### Testing

- Tests are in `base/tests/` directory
- Each test is a standalone project with its own `project.js`
- Test projects reference the main engine via `project.add_project("../../")`

### Linting and Formatting

- The project uses `.clang-format` at repository root for code formatting
- Run clang-format to format your code before committing
- Format C files: `clang-format -i source.c`
- Check formatting: `clang-format --dry-run -Werror source.c`

## Project Structure

```
armortools/
├── base/                    # Core 3D engine
│   ├── sources/             # C source files
│   │   ├── iron_*.c/h      # Core engine modules
│   │   ├── kong/           # Kong shader compiler (kong.c + kong_spirv.c, kong_metal.c, kong_hlsl.c, kong_cstyle.c, kong_wgsl.c)
│   │   ├── backends/       # Platform-specific code
│   │   └── libs/           # Third-party libraries (includes bc7enc.c/h for BC7 compression)
│   ├── shaders/            # Shader sources (.kong)
│   ├── assets/             # Textures, fonts, themes
│   ├── tests/              # Test projects
│   └── tools/              # Build tools (amake)
├── paint/                  # ArmorPaint application
├── engine/                 # Game engine layer (ECS, scripting, 2D)
└── .clang-format           # Code formatting rules
```

## Common Tasks

### Adding a New Source File

1. Add to `project.js` using `project.add_cfiles("sources/module_name.c")`
2. Create corresponding header in `sources/`
3. Add include path if needed: `project.add_include_dir("path")`

### Adding a Platform-Specific Backend

1. Create file in `sources/backends/[platform]_[module].c`
2. Add conditional compilation in `project.js`
3. Use appropriate `IRON_*` define for platform detection