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

- **core/** — Minic-facing APIs. Each `*_api.c` registers functions callable from `.minic` scripts via `minic_register()`. `runtime_api.c` is the unified registration entry point. `engine_world.c` holds the global world state. `minic_system.c` manages system lifecycle (loading, init/step/draw callbacks, manifest parsing). Additional modules: `asset_loader.c` (scene/mesh loading), `scene_3d_api.c` (3D scene operations), `scene_api.c` (scene management), `camera2d.c` (2D camera), `prefab.c` (prefab load/save), `ui_ext_api.c` (extended UI), `input.c` (input handling), `game_loop.c` (frame loop).
- **ecs/** — Flecs ECS internals. `ecs_world.c` manages the Flecs world lifecycle. `ecs_dynamic.c` handles runtime component registration. `ecs_components.c` registers all built-in components. Bridge files sync ECS state to Iron: `ecs_bridge.c` (base), `sprite_bridge.c` (2D sprites), `render2d_bridge.c` (2D draw calls), `camera_bridge.c` (2D camera), `render3d_bridge.c` (3D draw calls), `camera_bridge_3d.c` (3D camera), `mesh_bridge_3d.c` (3D mesh sync).
- **components/** — Built-in component definitions: `transform.c` (2D position/rotation/scale), `mesh_renderer.c` (3D mesh renderer), `camera.c` (2D/3D camera), `material.c` (PBR material).
- `game_engine.c` — Top-level init/shutdown/start. Wires together ECS world, API registration, Iron callbacks, and script loading.

### Bridge Pattern

Bridge systems run as Flecs systems that detect ECS component changes and sync them to Iron engine objects:

- **2D bridges:** `sprite_bridge.c`, `render2d_bridge.c`, `camera_bridge.c` — handle 2D sprites, batch rendering, and 2D camera.
- **3D bridges:** `render3d_bridge.c`, `camera_bridge_3d.c`, `mesh_bridge_3d.c` — handle 3D mesh rendering, 3D camera sync, and mesh object lifecycle.

For example, when a sprite component is added, the sprite bridge creates the corresponding Iron object and adds it to the active scene.

### Minic Script System

Minic is a tree-walking interpreter bundled in `base/sources/libs/minic.c` with 500+ Iron API bindings. Game scripts live in `engine/assets/scripts/` and `engine/assets/systems/`. Scripts define components, entities, and systems at runtime.

Systems are loaded via a manifest file `engine/assets/systems.manifest`. At startup, `_kickstart()` in `game_engine.c` calls `minic_system_load_manifest("data/systems.manifest")`, which reads the manifest and loads each listed system. To add or toggle systems, edit the manifest — no C code changes needed, just re-export assets with `base/make`.

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

### 3D Components

3D ECS components use the `comp_3d_` naming prefix:

| Component | Struct | Fields |
|-----------|--------|--------|
| Position | `comp_3d_position` | `x, y, z` |
| Rotation | `comp_3d_rotation` | `x, y, z, w` (quaternion) |
| Scale | `comp_3d_scale` | `x, y, z` |
| Camera | `comp_3d_camera` | `fov, near_plane, far_plane, perspective, ortho_*` |
| Mesh Renderer | `comp_3d_mesh_renderer` | `mesh_path, material_path, cast_shadows, receive_shadows` |
| Material | `comp_3d_material` | `metallic, roughness, albedo_rgb, emissive_rgb, ao` |
| Directional Light | `comp_directional_light` | `dir_xyz, color_rgb, strength, enabled` |

The 3D render pipeline is driven by `render3d_bridge.c`. Mesh objects are synced to Iron via `mesh_bridge_3d.c`, and the 3D camera via `camera_bridge_3d.c`. The `asset_loader` module provides `asset_loader_load_scene()` and `asset_loader_load_mesh()` for loading 3D assets from Minic scripts.

## Context Files

Detailed documentation for subsystems lives in separate README files at the repo root:

- **KONG_README.md** — Kong shader compiler: language syntax, types, built-in functions, backend pipeline, advanced patterns from paint shaders.
- **MINIC_README.md** — Minic scripting language: type system, ECS API (entities, components, queries), input/drawing APIs, system lifecycle, complete examples.
- **AMAKE_README.md** — Build system (amake): shell wrapper → make.js → project.js architecture, platform commands, Project API, shader compilation, asset export.
- **ARM3D_README.md** — `.arm` binary format: Scene format (Blender export), Project format (ArmorPaint export), binary encoding, vertex packing, loading pipeline, ECS integration.
- **3D_PLAN.md** — 3D deferred rendering pipeline implementation plan: 5-pass architecture, component types, lighting system, post-processing roadmap.

Read these files when working on the corresponding subsystems.

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

## Known Issues

### Metal Viewport Depth Range Bug

`render_path_set_target()` calls `gpu_viewport()` which hardcodes `znear=0.1, zfar=100.0`, breaking Metal's depth range mapping. On Metal, the default viewport correctly maps NDC z [-1,1] to depth buffer [0,1], but these hardcoded values override it and cause all 3D fragments to fail depth testing.

**Workaround:** Use `_gpu_begin()` + `render_path_submit_draw()` + `gpu_end()` instead of `render_path_set_target()` + `render_path_draw_meshes()`:

```c
void render_commands() {
    _gpu_begin(NULL, NULL, NULL, GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH, 0xff6495ed, 1.0);
    render_path_submit_draw("mesh");
    gpu_end();
}
```

Also initialize `render_path_current_w`/`h` in `scene_ready()` before the first frame to prevent division-by-zero in `camera_object_proj_jitter()`.
