# Spec 2: .arm Asset Loading

## Context

Spec 1 established 3D components (comp_3d_position, comp_3d_rotation, comp_3d_scale, comp_3d_camera, comp_3d_mesh_renderer, RenderObject3D) and bridge modules (camera_bridge_3d, mesh_bridge_3d). This spec adds the ability to load .arm scene files and create ECS entities with those components, and exposes asset loading to Minic scripts.

Test asset: `engine/assets/3d/cube.arm` (1.1 KB binary armpack file).

## Architecture: Iron scene_create + ECS Sync

The approach uses Iron's `scene_create()` to set up the scene graph (creates `_scene_root`, `scene_camera`, mesh objects, etc.), then iterates the created Iron objects and creates corresponding ECS entities with 3D components that point to the Iron objects.

This reuses Iron's full scene loading pipeline rather than reimplementing it, while keeping the ECS as the authoritative data source for game logic.

## asset_loader Module

**Files:** `engine/sources/core/asset_loader.h`, `engine/sources/core/asset_loader.c`

### asset_loader_load_scene(path)

Main entry point. Returns root entity ID (0 on failure).

1. Call `data_get_scene_raw(path)` to parse the .arm file into a `scene_t*`
2. If `_scene_root` is NULL, call `scene_create(scene_raw)` — this initializes the Iron scene graph globally (sets `_scene_root`, `scene_camera`, `scene_meshes`, `scene_cameras`, uploads mesh data to GPU)
3. If `_scene_root` already exists, iterate `scene_raw->objects` and call `scene_create_object()` for each, parenting to `_scene_root`
4. After Iron objects exist, iterate `scene_meshes` (global array set by `scene_create`). For each mesh object:
   - Create an ECS entity
   - Read the mesh object's transform: set `comp_3d_position` from `transform->loc`, `comp_3d_rotation` from `transform->rot`, `comp_3d_scale` from `transform->scale`
   - Set `RenderObject3D.iron_mesh_object` to the mesh object's base, `RenderObject3D.iron_transform` to its transform
   - Set `comp_3d_mesh_renderer.visible = true`
5. If cameras exist in `scene_cameras`, create a camera entity with `comp_3d_camera` (fov, near, far from `camera_data_t`) and position/rotation from its transform. Set `active = true` on the first one.
6. Return the first entity ID created (or a designated root entity)

### asset_loader_load_mesh(entity, mesh_path, material_path)

Load a single mesh onto an existing entity. Returns 0 on success, -1 on failure.

1. Call `data_get_mesh(mesh_path, mesh_name)` to load mesh data
2. If `material_path` is not NULL, call `data_get_material(material_path, material_name)`
3. Call `mesh_data_build(mesh_data)` to upload to GPU
4. Create `mesh_object_create(mesh_data, material_data)` and parent to `_scene_root`
5. Set `RenderObject3D` on the entity pointing to the new Iron mesh object

## Minic Scene API

**Files:** `engine/sources/core/scene_api.h`, `engine/sources/core/scene_api.c`

Register two native functions:

- `scene_load(path)` → `id` (entity ID of root entity, or 0 on failure)
- `mesh_load(entity_id, mesh_path, material_path)` → `int` (0 success, -1 failure)

Both use the `minic_register_native()` pattern established in `runtime_api.c`.

**Wiring:** `scene_api_register()` is called from `runtime_api_register()` in `engine/sources/core/runtime_api.c`.

## Key Iron API Functions Used

| Function | Purpose |
|----------|---------|
| `data_get_scene_raw(file)` | Parse .arm file → `scene_t*` |
| `scene_create(format)` | Initialize Iron scene graph from parsed data |
| `scene_create_object(obj, format, parent)` | Create individual Iron objects |
| `data_get_mesh(file, name)` | Load mesh data |
| `data_get_material(file, name)` | Load material data |
| `mesh_data_build(raw)` | Upload mesh to GPU |
| `mesh_object_create(data, material)` | Create runtime mesh object |

## Key Global Variables

| Variable | Type | Set by |
|----------|------|--------|
| `_scene_root` | `object_t*` | `scene_create()` |
| `scene_meshes` | `any_array_t*` | `scene_create()` |
| `scene_cameras` | `any_array_t*` | `scene_create()` |
| `scene_camera` | `camera_object_t*` | `scene_create()` |
| `_scene_raw` | `scene_t*` | `scene_create()` |

## Error Handling

- If `data_get_scene_raw()` returns NULL, print error and return 0
- If `scene_create()` returns NULL, print error and return 0
- If individual mesh creation fails, skip that mesh and continue (don't abort the whole scene)
- If `data_get_mesh()` returns NULL in `load_mesh`, return -1

## File Inventory

### New files (4):
- `engine/sources/core/asset_loader.h`
- `engine/sources/core/asset_loader.c`
- `engine/sources/core/scene_api.h`
- `engine/sources/core/scene_api.c`

### Modified files (2):
- `engine/sources/main.c` — add `#include "core/asset_loader.c"` and `#include "core/scene_api.c"` to unity build
- `engine/sources/core/runtime_api.c` — add `#include "scene_api.h"` and call `scene_api_register()`

### Build changes:
- `engine/project.js` — add `project.add_assets("assets/3d/*.arm", {destination: "data/{name}"});` to export .arm files

## Verification

1. Build: `cd engine && ../base/make macos metal && cd build && xcodebuild -project IronGame.xcodeproj -configuration Release build`
2. Create a test `.minic` script:
   ```
   var root = scene_load("cube.arm");
   trace("Loaded scene, root entity: " + int_to_string(root));
   ```
3. Run the binary — verify "cube.arm" loads, mesh objects appear in `scene_meshes`, ECS entities created with correct components
