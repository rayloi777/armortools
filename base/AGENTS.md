# AGENTS.md - Iron Engine (base) Developer Guide

This file provides coding guidelines for agents working on the Iron 3D engine (armortools/base).

## Build Commands

### Building

```bash
cd base

# On macOS/Linux:
./make [platform] [graphics]

# On Windows:
make.bat [platform] [graphics]

# Examples:
./make macos metal          # macOS with Metal
./make linux vulkan         # Linux with Vulkan
./make windows direct3d12   # Windows with Direct3D12
./make android vulkan       # Android
./make wasm webgpu          # WebAssembly/WebGPU

# Compile only (skip project generation, used in CI):
./make --compile
```

### Running Tests

Tests are standalone projects in `tests/` directory. Each has its own `project.js`.

```bash
# Run a specific test:
cd base/tests/[test_name]
../../make [platform] [graphics]

# Or run directly (recommended for debugging):
../../make --run

# Available tests:
# - triangle  - Simple triangle rendering (no MVP)
# - poly      - Triangle with MVP matrix and perspective projection
# - cube      - Basic 3D cube rendering
# - fall      - Physics/falling objects test (uses ASIM)
# - wren      - Wren scripting test
# - eval      - QuickJS scripting test
```

## Code Style Guidelines

### Formatting

- **Indentation**: 4 spaces (no tabs)
- **Column limit**: 160 characters
- **Braces**: K&R style - opening brace on same line
- **Else placement**: `BeforeElse: true` (else on new line)
- **Line endings**: LF (not CRLF)
- **Reference**: `.clang-format` in repository root

### Naming Conventions

- **Types**: `snake_case_t` (e.g., `iron_array_t`, `u32_array_t`)
- **Functions**: `module_function` prefix (e.g., `iron_log()`, `buffer_create()`)
- **Constants**: `IRON_CONSTANT` uppercase with underscores
- **Variables**: `snake_case` (e.g., `file_location`, `buffer_size`)
- **Globals**: Prefix with module name (e.g., `update_callback`)
- **Statics**: `snake_case` (e.g., `static char *fileslocation`)

### Type Aliases

Use these instead of raw C types (defined in `iron_global.h`):

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

### File Organization

- **Headers (.h)**: Declarations, type definitions, inline functions
- **Sources (.c)**: Implementation
- **Pairs**: `module_name.h` and `module_name.c`
- **Include guard**: `#pragma once` (preferred)

### Imports/Includes Order

1. System headers (`<stdio.h>`, `<stdlib.h>`, etc.)
2. Platform-specific with `#ifdef` guards
3. Project headers (e.g., `#include "iron_system.h"`)
4. Sort alphabetically within groups
5. Use quotes for local, brackets for system

```c
#include "iron_gc.h"
#include "iron_string.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef IRON_WINDOWS
#include <windows.h>
#endif
```

### Error Handling

- `iron_log()` - informational messages
- `iron_error()` - errors (shows message box on Windows)
- Return 0 for success, -1 for error
- Check return values of file operations

### Platform-Specific Code

Use these defines (wrap in `#ifdef`):
- `IRON_WINDOWS`, `IRON_LINUX`, `IRON_MACOS`
- `IRON_IOS`, `IRON_ANDROID`, `IRON_WASM`, `IRON_POSIX`

Keep platform code in `sources/backends/` directory.

```c
#ifdef IRON_WINDOWS
    // Windows-specific
#elif defined(IRON_LINUX)
    // Linux-specific
#endif
```

### Memory Management

- Use `malloc()`/`free()` for dynamic allocation
- Check for NULL after allocation
- Match allocation with appropriate free
- Use GC from `iron_gc.h` when appropriate for managed memory

### Functions

- Keep functions focused and single-purpose
- Use `static` for internal functions
- Return early for error conditions

## Engine Architecture

### Core Modules

| Module | Responsibility |
|--------|-----------------|
| `iron.h` | Entry point, kickstart(), global state |
| `iron_system` | Window, display, input callbacks |
| `iron_gpu` | Graphics abstraction (textures, buffers, shaders) |
| `iron_array` | Dynamic arrays (i8, u8, i16...f32, any, buffer) |
| `iron_map` | Key-value maps (i32, f32, any) |
| `iron_gc` | Mark-and-sweep garbage collector |
| `iron_ui` | Immediate mode GUI |
| `iron_draw` | 2D rendering (images, shapes, text) |
| `iron_input` | Input state (mouse, keyboard, pen, touch) |
| `engine` | Scene graph, objects, cameras, materials |

### Graphics Backends

Supported via platform-specific implementations:
- **Metal** (macOS/iOS) - `metal_gpu.c`
- **Vulkan** (Linux/Android) - `vulkan_gpu.c`
- **Direct3D12** (Windows) - `direct3d12_gpu.c`
- **WebGPU** (WASM) - `webgpu_gpu.c`

### Additional Systems

- **Scripting**: Wren or QuickJS via `iron_wren.h` or `iron_eval.h`
- **Audio**: stb_vorbis OGG decoding, 16-channel mixer
- **Physics**: ASIM (`sources/libs/asim.c`) - lightweight sphere-mesh collision with BVH
- **Raycasting**: `iron_raycast.h` - screen-to-world rays

### Wren Scripting

Iron supports Wren as an alternative scripting language. Wren is a small, fast, class-based concurrent scripting language.

#### Building with Wren

```bash
cd base
./make [platform] [graphics] --with-wren
```

Example:
```bash
./make macos metal --with-wren
./make linux vulkan --with-wren
```

#### Wren Script Location

Wren scripts should be placed in `assets/wren/`:
```
assets/wren/
├── main.wren          # Entry point (required)
├── math_test.wren
└── game_logic.wren
```

#### Wren API

In your Wren scripts, you can use:

```wren
// Basic output
Iron.print("Hello from Wren!")

// Math functions
Math.sin(0)
Math.cos(0)
Math.sqrt(16)
Math.PI

// Data structures
var list = List.new()
list.add(1)
var map = Map.new()
map.set("key", "value")
```

#### Adding More Wren Bindings

To add new Wren bindings:

1. Create `sources/wren_xxx.c` with foreign method definitions
2. Register methods in `iron_wren.c`'s `wren_bind_foreign_method_fn`
3. Add the C file to `project.js` under `--with-wren` section

### QuickJS Scripting

Iron supports QuickJS as an embedded JavaScript engine via `iron_eval.h`.

#### Building with QuickJS

QuickJS is enabled by default (via `WITH_EVAL` flag in project.js). No special flag needed.

#### QuickJS API

```c
#include <iron_eval.h>

// Initialize QuickJS runtime
js_init();

// Execute JavaScript code, returns float result
float result = js_eval("1 + 2");        // Returns 3.0
float result = js_eval("Math.sin(0.5)"); // Returns ~0.479

// Call JS function (requires getting function pointer first)
js_call(void *fn_handle);
js_call_ptr(void *fn, void *arg);
js_call_ptr_str(void *fn, void *arg0, char *arg1);
```

#### Reading JS Files from Assets

```c
#include <iron_eval.h>
#include <stdio.h>

char *js_code = NULL;

void load_js() {
    iron_file_reader_t reader;
    if (iron_file_reader_open(&reader, "data/a.js", IRON_FILE_TYPE_ASSET)) {
        size_t size = iron_file_reader_size(&reader);
        js_code = malloc(size + 1);
        iron_file_reader_read(&reader, js_code, size);
        js_code[size] = '\0';
        iron_file_reader_close(&reader);
    }
}

void render() {
    float result = js_eval(js_code);
    fprintf(stderr, "JS result: %f\n", result);
}
```

#### Required Stubs

When using QuickJS, you must provide `console_trace()`:

```c
void console_trace(char *s) {
    fprintf(stderr, "%s\n", s);
}
```

#### QuickJS Test

See `tests/eval/` for a complete working example:
```bash
cd base/tests/eval
../../make --run
```

### Matrix Storage

Iron uses **column-major** matrix storage:
- Translation stored in `m[12]`, `m[13]`, `m[14]` (not m03, m13, m23)
- Access elements via `mat.m[0]` through `mat.m[15]`

### Custom Shader Rendering (MVP)

For custom shaders with MVP matrix:

#### 1. MVP Matrix Order

Correct multiplication order is `model * view * proj`:
```c
mat4_t proj = mat4_persp(0.85, 1280.0 / 720.0, 0.1, 100.0);
mat4_t view = mat4_init_translate(0, 0, -5);
mat4_t model = mat4_rot_z(rotation);
mat4_t mvp = mat4_identity();
mvp = mat4_mult_mat(model, view);
mvp = mat4_mult_mat(mvp, proj);  // model * view * proj
```

#### 2. Shader with Perspective Divide

Metal's `[[position]]` does NOT auto-divide by w. Manual perspective divide is required:
```kong
#[set(root)]
const root: {
    mvp: float4x4;
};

fun test_vert(input: vert_in): vert_out {
    var output: vert_out;
    var pos: float4 = float4(input.pos, 1.0);
    pos = root.mvp * pos;
    // Perspective divide - REQUIRED for perspective projection!
    pos.x = pos.x / pos.w;
    pos.y = pos.y / pos.w;
    pos.z = pos.z / pos.w;
    output.pos = pos;
    return output;
}
```

#### 3. Setting Matrix in C

Use `gpu_set_matrix4(0, matrix)` to write to engine's internal constant buffer:
```c
_gpu_begin(NULL, NULL, NULL, GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH, 0xff000000, 1.0);
gpu_set_pipeline(pipeline);
gpu_set_vertex_buffer(vb);
gpu_set_index_buffer(ib);
gpu_set_matrix4(0, mvp);  // Write to offset 0 in internal constant buffer
gpu_draw();
gpu_end();
```

#### 4. Why `gpu_set_matrix4` Works

- Engine has internal `constant_buffer` (512 bytes, multiple slots)
- `gpu_set_matrix4(location, matrix)` writes directly to this buffer
- `gpu_draw()` automatically binds the internal buffer to shader
- Custom constant buffers get overwritten by `gpu_draw()` - use engine's buffer instead

### Known Issues

- First frame WVP matrix may be invalid (-inf) due to camera not being initialized
- The engine handles this by skipping NaN/inf matrices in `uniforms_set_obj_consts`

### Vertex Format: short4norm

- `short4norm` = int16 normalized to [-1, 1]
- `short2norm` = int16 normalized to [0, 1] (for UVs)
- GPU automatically unpacks these to float in Metal

## Project Structure

```
base/
├── sources/
│   ├── iron_*.c/h           # Core modules
│   ├── backends/           # Platform/graphics backends
│   │   ├── metal_gpu.c
│   │   ├── vulkan_gpu.c
│   │   ├── direct3d12_gpu.c
│   │   ├── webgpu_gpu.c
│   │   └── [platform]_system.c
│   ├── libs/               # Third-party (QuickJS, stb_vorbis, LZ4)
│   └── kong/               # Shader compiler (.kong -> MSL/SPIRV/DXIL)
├── shaders/                # .kong shader sources
├── assets/                 # Textures, fonts, themes
├── tests/                  # Standalone test projects
└── tools/                  # amake build system
```

## Common Tasks

### Adding New Source File

1. Add to `project.js`: `project.add_cfiles("sources/module_name.c")`
2. Create header in `sources/`
3. Add include path if needed: `project.add_include_dir("path")`

### Adding Platform Backend

1. Create `sources/backends/[platform]_[module].c`
2. Add conditional compilation in `project.js`
3. Use `IRON_*` define for platform detection

## Debugging Tips

### Running Tests with Output

```bash
cd base/tests/[test_name]
../../make --run 2>&1 | head -50
```

### Common Issues

#### WVP Matrix Not Working

If meshes don't render but rendering is called:
1. Check if `constants.WVP` is correctly set in shader
2. Ensure `_world_view_proj_matrix` link is set in material constants
3. The engine handles first-frame NaN/infinite values by skipping invalid matrices

Example shader usage:
```kong
#[set(root)]
const root: {
    WVP: float4x4;
};

fun mesh_vert(input: vert_in): vert_out {
    var output: vert_out;
    var pos: float4 = float4(input.pos.xyz, 1.0);
    pos = root.WVP * pos;
    // Add perspective divide for perspective projection
    pos.x = pos.x / pos.w;
    pos.y = pos.y / pos.w;
    pos.z = pos.z / pos.w;
    output.pos = pos;
    return output;
}
```

### IRON_LOG Output

- On macOS: `iron_log()` output goes to stdout (GUI apps may not show it)
- Use `fprintf(stderr, ...)` for immediate console output
- Check Xcode console for app output
