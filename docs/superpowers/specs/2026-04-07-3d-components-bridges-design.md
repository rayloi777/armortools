# Spec 1: 3D Components & Bridges

## Context

The engine layer currently only supports 2D sprite rendering. 3D component files (transform.c, camera.c, mesh_renderer.c) are stubs. The Iron engine backend has full 3D capabilities (scene, camera, mesh, material, transform) that need to be exposed through the ECS bridge pattern.

This is the first of four specs. It establishes the 3D component data model and bridge modules. Subsequent specs add .arm asset loading, deferred rendering, and Minic scripting APIs.

## Components

Six new Flecs components registered in `engine/sources/ecs/ecs_components.h/.c`, following the `comp_2d_*` pattern:

| Component | Fields | Purpose |
|-----------|--------|---------|
| `comp_3d_position` | `x, y, z` (f32) | World position |
| `comp_3d_rotation` | `x, y, z, w` (f32) | Quaternion rotation |
| `comp_3d_scale` | `x, y, z` (f32) | Non-uniform scale |
| `comp_3d_camera` | `fov, near_plane, far_plane` (f32), `perspective, active` (bool) | Camera params; `active` flag selects the rendering camera |
| `comp_3d_mesh_renderer` | `mesh_path, material_path` (char*), `visible` (bool) | References to .arm assets |
| `RenderObject3D` | `iron_mesh_object, iron_transform` (void*), `dirty` (bool) | Holds Iron engine object pointers; dirty triggers transform resync |

Registration uses `register_component()` and field registration uses `ecs_dynamic_component_add_field()` — identical to existing 2D components. String fields (`mesh_path`, `material_path`) use `DYNAMIC_TYPE_PTR`; the ECS dynamic system manages memory. Name lookups via `ecs_get_builtin_component()` / `ecs_get_builtin_component_name()` map to all six.

The component stubs in `engine/sources/components/transform.h`, `camera.h`, `mesh_renderer.h` are replaced with these actual struct definitions.

## Camera Bridge 3D

**Files:** `engine/sources/ecs/camera_bridge_3d.h`, `camera_bridge_3d.c`

Manages a single `camera_object_t` (Iron's camera type).

**`camera_bridge_3d_init()`**: Creates `camera_data_t` with defaults (fov=60, near=0.1, far=100, perspective=true). Creates `camera_object_t` via Iron API. Attaches to `_scene_root`.

**`camera_bridge_3d_update()`** (per frame):
1. Query entities with `comp_3d_camera` + `comp_3d_position` + `comp_3d_rotation`
2. Find first entity where `active == true`
3. Sync position from `comp_3d_position`, rotation from `comp_3d_rotation` to Iron `transform_t`
4. Call `transform_build_matrix()`, `camera_object_build_proj()` (with window aspect), `camera_object_build_mat()`

**`camera_bridge_3d_get_active()`**: Returns the `camera_object_t*`. Used by the deferred render pipeline for VP matrices.

## Mesh Bridge 3D

**Files:** `engine/sources/ecs/mesh_bridge_3d.h`, `mesh_bridge_3d.c`

**`mesh_bridge_3d_create_mesh(entity)`**:
1. Read `comp_3d_mesh_renderer` for `mesh_path` and `material_path`
2. Call `data_get_mesh(mesh_path)` and `data_get_material(material_path)` (Iron API)
3. Create `mesh_object_create()`, attach to `_scene_root`
4. Store Iron object pointers in `RenderObject3D` on the entity

**`mesh_bridge_3d_sync_transforms()`**: Flecs system callback querying `comp_3d_position` + `comp_3d_rotation` + `comp_3d_scale` + `RenderObject3D`. For each entity:
1. Set Iron `transform_t` `loc`/`rot`/`scale` from ECS components
2. Set `dirty = true`, call `transform_build_matrix()`

**`mesh_bridge_3d_destroy_mesh(entity)`**: Remove Iron object, zero out `RenderObject3D`.

## Engine Wiring

**`game_engine.c`**:
- Init: `camera_bridge_3d_set_world()`, `camera_bridge_3d_init()`, `mesh_bridge_3d_set_world()`, `mesh_bridge_3d_init()` — added after existing `camera_bridge_init()`, before `sys_2d_init()`
- Shutdown: `mesh_bridge_3d_shutdown()`, `camera_bridge_3d_shutdown()` — added before `game_world_destroy()`

**`game_loop.c`**:
- After `draw_begin()`, before `sys_2d_draw()`, add: `camera_bridge_3d_update()` and `mesh_bridge_3d_sync_transforms()`
- Actual 3D draw calls will be added in the deferred pipeline spec

## File Inventory

### New files (4):
- `engine/sources/ecs/camera_bridge_3d.h`
- `engine/sources/ecs/camera_bridge_3d.c`
- `engine/sources/ecs/mesh_bridge_3d.h`
- `engine/sources/ecs/mesh_bridge_3d.c`

### Modified files (7):
- `engine/sources/components/transform.h` — replace stub with `comp_3d_position`, `comp_3d_rotation`, `comp_3d_scale`
- `engine/sources/components/camera.h` — replace stub with `comp_3d_camera`
- `engine/sources/components/mesh_renderer.h` — replace stub with `comp_3d_mesh_renderer`
- `engine/sources/ecs/ecs_components.h` — add 3D component structs, `RenderObject3D`, accessor declarations
- `engine/sources/ecs/ecs_components.c` — register 3D components, fields, name lookups
- `engine/sources/game_engine.c` — add init/shutdown for 3D bridges
- `engine/sources/core/game_loop.c` — add 3D update calls in frame loop

## Verification

1. Build: `cd engine && ../base/make macos metal && cd build && xcodebuild -project IronGame.xcodeproj -configuration Release build`
2. Run the binary — verify no crashes on init, engine starts normally
3. Create a test `.minic` script that calls `entity_create()`, adds `comp_3d_position` and `comp_3d_camera`, verify entity has components via ECS query
