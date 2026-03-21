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
# - eval      - QuickJS scripting test
# - draw      - 2D draw API test (shapes, images, text)
```

### Test Project Initialization

Test projects have two initialization options:

#### `_iron_init()` - Basic Initialization

Use when you only need GPU rendering without the 2D draw system:

```c
void _kickstart() {
    iron_window_options_t *ops = GC_ALLOC_INIT(iron_window_options_t, {...});
    _iron_init(ops);
    // draw_* functions will NOT work
    _iron_set_update_callback(render);
    iron_start();
}
```

Use for: `triangle`, `cube`, `poly` tests that render directly via GPU.

#### `sys_start()` - Full Initialization with Draw System

Use when you need the 2D draw API (`draw_filled_rect`, `draw_image`, `draw_string`, etc.):

```c
void _kickstart() {
    iron_window_options_t *ops = GC_ALLOC_INIT(iron_window_options_t, {...});
    sys_start(ops);  // Calls _iron_init() + draw_init()
    // draw_* functions will work
    _iron_set_update_callback(render);
    iron_start();
}
```

Use for: tests that use `iron_draw` API (images, shapes, text).

**Why**: `draw_init()` is only called inside `sys_start()` (see `iron_sys.c:158`). Using `_iron_init()` alone leaves the draw system uninitialized.

### 2D Draw API

Iron provides a 2D drawing API via `iron_draw.h`. All draw operations must be wrapped between `draw_begin()` and `draw_end()`.

#### Basic Usage

```c
void render() {
    draw_begin(NULL, true, 0xff1a1a2e);  // Clear with dark blue
    
    // Draw shapes
    draw_set_color(0xffe94560);
    draw_filled_rect(50, 50, 150, 100);
    draw_rect(220, 50, 150, 100, 3.0f);
    draw_line(50, 200, 200, 200, 2.0f);
    draw_filled_circle(120, 400, 50, 32);
    
    // Draw text (requires font initialization)
    draw_font_t *font = data_get_font("font.ttf");
    draw_font_init(font);
    draw_set_font(font, 32);
    draw_set_color(0xffffffff);
    draw_string("Hello!", 50, 50);
    
    draw_end();
}
```

#### Drawing Images

Images must be loaded using `data_get_image()` with the `.k` extension (build-time compressed format):

```c
gpu_texture_t *image;

void _kickstart() {
    sys_start(ops);
    image = data_get_image("color_wheel.k");  // Note: .k extension
    // ...
}

void render() {
    draw_begin(NULL, true, 0xff1a1a2e);
    
    // Various image drawing functions
    draw_image(image, x, y);                    // Original size
    draw_scaled_image(image, x, y, w, h);       // Scale to specific size
    draw_sub_image(image, sx, sy, sw, sh, x, y);  // Draw sub-rectangle
    draw_scaled_sub_image(image, sx, sy, sw, sh, dx, dy, dw, dh);  // Scaled sub-rect
    
    draw_end();
}
```

**Important**: Image files (`.png`, `.jpg`) are converted to `.k` format at build time. Use `data_get_image("filename.k")` - not the original extension.

#### Draw API Reference

| Function | Description |
|----------|-------------|
| `draw_begin(target, clear, color)` | Begin drawing (target=NULL for default framebuffer) |
| `draw_end()` | End drawing |
| `draw_set_color(color)` | Set RGBA color (0xRRGGBBAA) |
| `draw_filled_rect(x, y, w, h)` | Draw filled rectangle |
| `draw_rect(x, y, w, h, strength)` | Draw rectangle outline |
| `draw_line(x0, y0, x1, y1, strength)` | Draw line |
| `draw_line_aa(x0, y0, x1, y1, strength)` | Draw anti-aliased line |
| `draw_filled_circle(cx, cy, radius, segments)` | Draw filled circle |
| `draw_circle(cx, cy, radius, segments, strength)` | Draw circle outline |
| `draw_image(tex, x, y)` | Draw image at original size |
| `draw_scaled_image(tex, x, y, w, h)` | Draw scaled image |
| `draw_sub_image(tex, sx, sy, sw, sh, x, y)` | Draw sub-region |
| `draw_scaled_sub_image(tex, sx, sy, sw, sh, dx, dy, dw, dh)` | Draw scaled sub-region |
| `draw_string(text, x, y)` | Draw text (requires font) |
| `draw_set_font(font, size)` | Set font and size |

#### Test Project Example

See `tests/draw/` for a complete working example:

```bash
cd base/tests/draw
../../make --run
```

Required `project.js` configuration:
```js
let flags = globalThis.flags;
flags.with_eval = false;

let project = new Project("test");
project.add_project("../../");
project.add_cfiles("main.c");
project.add_assets("assets/*", {destination: "data/{name}"});
return project;
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

- **Scripting**: QuickJS via `iron_eval.h`
- **Audio**: stb_vorbis OGG decoding, 16-channel mixer
- **Physics**: ASIM (`sources/libs/asim.c`) - lightweight sphere-mesh collision with BVH
- **Raycasting**: `iron_raycast.h` - screen-to-world rays

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
