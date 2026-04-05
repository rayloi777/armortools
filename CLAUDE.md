# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ArmorTools is a 3D content creation suite built on the Iron engine. It contains three main components:

- **base/** — Core Iron 3D engine (C). Provides rendering, UI, input, and platform abstraction. **Do not modify this directory.**
- **paint/** — ArmorPaint, a 3D painting/texturing application built on the engine.
- **engine/** — Game engine layer adding ECS, scripting, and 2D support on top of base/. All new game engine code goes here.

The engine layer uses **Flecs ECS** for entity/component/system management and **Minic** (a built-in C-like scripting language) for runtime game logic. Scripts in `.minic` files are hot-loaded without recompilation.

## Build Commands

```bash
# Build the game engine (from engine/) — TWO STEPS on macOS:
cd engine && ../base/make macos metal   # Step 1: exports assets + shaders
cd engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build  # Step 2: compiles C code

# Run the built binary
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame

# Build ArmorPaint (from paint/)
cd paint && ../base/make

# Build and run immediately (Linux only)
../base/make --run

# Build for other platforms
../base/make linux vulkan
../base/make windows direct3d12
../base/make --target ios
../base/make --target android
../base/make --target wasm

# Format code
clang-format -i source.c
clang-format --dry-run -Werror source.c

# Run a test project (from base/tests/<test_name>/)
../../make macos metal
```

## Architecture

### Layered Design

```
Minic Scripts (.minic)          # Runtime game logic, hot-reloadable
    ↓ calls C API registered via minic_register()
Core API (engine/sources/core/) # entity_api, component_api, system_api, query_api, sprite_api
    ↓ operates on
Flecs ECS (engine/sources/ecs/) # Entity-component-system world
    ↓ synced via bridge systems
Iron Engine (base/)             # Rendering, platform, UI — never modified
```

### Key Modules in engine/sources/

- **core/** — Minic-facing APIs. Each `*_api.c` registers functions callable from `.minic` scripts via `minic_register()`. `runtime_api.c` is the unified registration entry point. `engine_world.c` holds the global world state.
- **ecs/** — Flecs ECS internals. `ecs_world.c` manages the Flecs world lifecycle. `ecs_dynamic.c` handles runtime component registration. `*_bridge.c` files sync ECS state to Iron (render bridge, sprite bridge, camera bridge).
- **components/** — Built-in component definitions (Transform, MeshRenderer, Camera).
- `game_engine.c` — Top-level init/shutdown/start. Wires together ECS world, API registration, Iron callbacks, and script loading.

### Bridge Pattern

Bridge systems (`ecs_bridge.c`, `sprite_bridge.c`, `render2d_bridge.c`, `camera_bridge.c`) run as Flecs systems that detect ECS component changes and sync them to Iron engine objects. For example, when a sprite component is added, the sprite bridge creates the corresponding Iron object and adds it to the active scene.

### Minic Script System

Minic is a tree-walking interpreter bundled in `base/sources/libs/minic.c` with 500+ Iron API bindings. Game scripts live in `engine/assets/scripts/` and `engine/assets/systems/`. Scripts define components, entities, and systems at runtime. The entry point is loaded via `load_and_run_script()` in `game_engine.c`.

#### Minic Types

| Keyword | Internal | C equivalent | Use |
|---------|----------|-------------|-----|
| `int` | `MINIC_T_INT` | `int32_t` | General integers |
| `float` | `MINIC_T_FLOAT` | `float` | Floating-point |
| `bool` | `MINIC_T_BOOL` | `bool` | Booleans |
| `void` | `MINIC_T_VOID` | `void` | Function returns |
| `id` | `MINIC_T_ID` | `uint64_t` | Entity/component IDs (Flecs `ecs_id_t`) |

The `id` type stores 64-bit Flecs entity/component IDs natively. It supports comparison operators (`==`, `!=`, `<`, `>`, `<=`, `>=`) via a fast path that compares `uint64_t` directly (no double precision loss). It does **not** support arithmetic. Use it for variables that hold entity or component IDs returned by `entity_create()`, `component_lookup()`, `query_foreach()` callbacks, etc.

```c
// Example usage in .minic scripts:
id g_player = entity_create();
id g_pos_comp = component_lookup("comp_2d_position");
entity_add(g_player, g_pos_comp);
```

### 2D Components

2D ECS components use the `comp_2d_` naming prefix (e.g., `comp_2d_position`, `comp_2d_sprite`). The 2D render pipeline is driven by `render2d_bridge.c` which batches and submits 2D draw calls.

## Code Style

- **Formatting**: 4 spaces, 160 column limit, K&R braces. Uses `.clang-format` at repo root.
- **Types**: `snake_case_t` (e.g., `game_world_t`). Use engine aliases (`f32`, `i32`, `u8`, etc.) not raw C types.
- **Functions**: `module_function()` prefix pattern (e.g., `entity_create()`, `component_register()`).
- **Headers**: `#pragma once` for include guards. System includes first, then project headers.
- **Platform guards**: `IRON_WINDOWS`, `IRON_LINUX`, `IRON_MACOS`, `IRON_IOS`, `IRON_ANDROID`, `IRON_WASM`.

## Upstream API Notes

### BC7 Texture Compression

The upstream Iron engine now includes BC7 texture compression support via `base/sources/libs/bc7enc.c`. The engine layer can optionally use these APIs:

- `GPU_TEXTURE_FORMAT_RGBA32_BC7` — texture format enum for BC7-compressed textures.
- `gpu_bc7_supported()` — returns whether the GPU supports BC7 (always true on desktop).
- `gpu_bc7_compress(image_data, width, height)` — multi-threaded BC7 compression.

### Removed APIs

- `gpu_destroy()` has been removed from the Iron GPU API. Resources are now cleaned up through their respective destroy functions or via the scene/object lifecycle.

### Paint Variable Naming

ArmorPaint globals were renamed for consistency. If engine code references paint internals:
- `context_raw` is now `g_context`
- `config_raw` is now `g_config`
- `project_raw` is now `g_project`

### Kong Shader Compiler

Kong was rewritten to a single-file architecture. The shader compiler is now `base/sources/kong/kong.c` + `kong.h` with individual backend files `kong_spirv.c`, `kong_metal.c`, `kong_hlsl.c`, `kong_cstyle.c`, `kong_wgsl.c`. The old `backends/` subdirectory no longer exists.

## Upstream Sync

When merging upstream Iron changes into `base/`, conflicts typically arise in:

- `base/sources/libs/minic.c` — our `MINIC_T_ID` type (enum value 6) and `id`-type operators. Resolve by re-applying our additions after accepting upstream changes.
- `base/project.js` — our engine `add_cfiles()` and `add_libs()` entries. Keep both upstream and engine entries.

After resolving conflicts, rebuild with both steps: `base/make` then `xcodebuild`.

## Critical Rule

**Never modify files in `base/`.** All new engine functionality goes in `engine/`. The base directory is the upstream Iron engine and must remain untouched.
