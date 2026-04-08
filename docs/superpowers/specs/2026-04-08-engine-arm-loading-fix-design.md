# Engine .arm Loading Fix — ECS-First Design

## Problem

`mesh_create("cube.arm")` crashes because it calls `data_get_mesh()` on a scene file.
`render3d_bridge.c` uses `render_path_set_target()` which triggers the Metal viewport depth bug
(hardcoded znear/zfar breaks Metal depth range mapping).

## Approach

Smart `mesh_create` — detect `.arm` extension, route to scene-based loading.
ECS-first: all transform data flows through Flecs components, bridges sync to Iron.

## Data Flow

```
.arm file
  → data_get_scene_raw() → scene_t*
  → auto-generate missing data (camera, material, shader, world)
  → scene_create(scene_raw) → Iron scene graph (mesh objects, camera objects)
  → for each Iron object, create ECS entity with:
      - comp_3d_position / comp_3d_rotation / comp_3d_scale (from obj_t->transform)
      - comp_3d_mesh_renderer
      - RenderObject3D → pointer to existing Iron mesh object
  → each frame: mesh_bridge_3d_sync_transforms() reads ECS → updates Iron transforms
```

## Changes

### 1. `engine/sources/core/scene_3d_api.c` — Smart mesh_create

`minic_mesh_create()` detects `.arm` extension in `mesh_path`:
- If `.arm`: call `asset_loader_load_scene(mesh_path)`, return entity ID.
- Otherwise: existing behavior (`asset_loader_load_mesh()`).

### 2. `engine/sources/core/asset_loader.c` — Enhanced asset_loader_load_scene

**Auto-generate missing scene data** when fields are NULL/empty:

| Field | Generated value |
|---|---|
| `camera_datas` | Single camera_data_t: fov=0.85, near=0.01, far=100.0 |
| `camera_ref` | "Camera" |
| Camera object | Added to `scene->objects` with type="camera_object", data_ref="Camera" |
| `material_ref` on mesh_object | Set to "DefaultMaterial" if NULL |
| `material_datas` | Single material_data_t with "mesh" context |
| `shader_datas` | Single shader_data_t with mesh.vert/mesh.frag, WVP constant, D32 depth |
| `world_ref` / `world_datas` | Single world_data_t with black background |

Shader vertex_elements: `pos` (short4norm) + `tex` (short2norm).
Shader texture_units: `my_texture` (if texture assets exist).

**ECS entity creation reads from obj_t->transform** (raw .arm data):
- `obj_t->transform` is `f32_array_t*` with 10 floats: `[loc_x, loc_y, loc_z, rot_x, rot_y, rot_z, rot_w, scale_x, scale_y, scale_z]`
- If transform is NULL, defaults: position=(0,0,0), rotation=(0,0,0,1), scale=(1,1,1)
- Set these directly on ECS components instead of reading back from Iron transform_t

### 3. `engine/sources/ecs/render3d_bridge.c` — Metal depth bug fix

Replace `render_path_set_target()` + `render_path_draw_meshes()` with:
```c
_gpu_begin(NULL, NULL, NULL, GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH, 0xff1a1a2e, 1.0f);
render_path_submit_draw("mesh");
gpu_end();
```

Initialize `render_path_current_w` and `render_path_current_h` in `sys_3d_init()`
to prevent division-by-zero in `camera_object_proj_jitter()` on first frame.

### 4. Test assets

Copy `base/tests/cube/assets/cube.arm` and `base/tests/cube/assets/texture.png`
to `engine/assets/3d/` so the engine can find them.

### 5. Minic test script

Update `engine/assets/systems/scene3d_test.minic` to use:
```
id mesh_ent = mesh_create("cube.arm");
```
The `mesh_create` smart-routing will handle the rest.

## What stays the same

- `mesh_bridge_3d.c` — already syncs ECS transforms to Iron correctly
- Non-`.arm` `mesh_create` path — still uses `data_get_mesh()`
- Bridge pattern — ECS → Iron sync unchanged
- `camera_bridge_3d.c` — no changes needed
