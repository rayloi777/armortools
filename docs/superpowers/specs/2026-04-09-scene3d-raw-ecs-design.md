# scene3d_test Raw ECS Rewrite Design

**Date:** 2026-04-09
**Status:** Draft

## Summary

Rewrite `scene3d_test.minic` to use the raw Flecs ECS pattern (`entity_create` + `component_lookup` + `entity_add` + `comp_set_*`) for creating camera, mesh, and light entities, instead of the high-level convenience wrappers (`camera_3d_create`, `mesh_create`, `light_directional`).

## Requirements

- Camera entity created via raw ECS: `entity_create()` + `entity_add(pos, rot, cam)` + `comp_set_floats/bool`
- Light entity created via raw ECS: `entity_create()` + `entity_add(light)` + `comp_set_floats/bool`
- Mesh entity created via raw ECS: `mesh_load()` for asset loading, then `entity_create()` + `entity_add(pos, rot, mesh_renderer, RenderObject3D)` + `comp_set_ptr`
- Camera bridge auto-detects entities (already works via query)
- Mesh bridge auto-detects entities with `RenderObject3D` that have valid `iron_mesh_object`

## Component Names

| Component | Lookup Name | Fields |
|-----------|-------------|--------|
| `comp_3d_position` | `"comp_3d_position"` | `x, y, z` (float) |
| `comp_3d_rotation` | `"comp_3d_rotation"` | `x, y, z, w` (float) |
| `comp_3d_camera` | `"comp_3d_camera"` | `fov, near_plane, far_plane` (float), `perspective, active` (bool) |
| `comp_3d_mesh_renderer` | `"comp_3d_mesh_renderer"` | `mesh_path, material_path` (ptr), `visible` (bool) |
| `comp_directional_light` | `"comp_directional_light"` | `dir_x, dir_y, dir_z, color_r, color_g, color_b, strength` (float), `enabled` (bool) |
| `RenderObject3D` | `"RenderObject3D"` | `iron_mesh_object, iron_transform` (ptr), `dirty` (bool) |

## Camera Entity (No C Changes)

The `camera_bridge_3d` runs a query every frame for entities with `comp_3d_position` + `comp_3d_rotation` + `comp_3d_camera`. Creating an entity via raw ECS with these components will be auto-detected.

Minic flow:
1. `component_lookup("comp_3d_position")` / `"comp_3d_rotation"` / `"comp_3d_camera"`
2. `entity_create()` → `entity_add(cam, pos/rot/cam_comp)`
3. `comp_set_floats(pos_comp, entity_get(cam, pos_comp), "x,y,z", 0.0, 2.0, 5.0)`
4. `comp_set_floats(cam_comp, entity_get(cam, cam_comp), "fov,near_plane,far_plane", 1.047, 0.1, 100.0)`
5. `comp_set_bool(cam_comp, entity_get(cam, cam_comp), "perspective", 1)`
6. `comp_set_bool(cam_comp, entity_get(cam, cam_comp), "active", 1)`
7. `entity_set_name(cam, "main_camera")`

The `camera_3d_set_position` / `camera_3d_look_at` calls in the step loop are replaced with direct `comp_set_floats` on the position and rotation components.

## Light Entity (No C Changes)

Entity created via raw ECS with `comp_directional_light`. The component data is set via `comp_set_floats` and `comp_set_bool`. No bridge exists yet to sync to Iron rendering — this is a separate task.

Minic flow:
1. `component_lookup("comp_directional_light")`
2. `entity_create()` → `entity_add(light, light_comp)`
3. `comp_set_floats(light_comp, entity_get(light, light_comp), "dir_x,dir_y,dir_z", 0.5, -1.0, 0.3)`
4. `comp_set_floats(light_comp, entity_get(light, light_comp), "color_r,color_g,color_b", 1.0, 0.9, 0.8)`
5. `comp_set_float(light_comp, entity_get(light, light_comp), "strength", 1.0)`
6. `comp_set_bool(light_comp, entity_get(light, light_comp), "enabled", 1)`

## Mesh Entity (New C API Required)

### New C API: `mesh_load(path)`

Registered in `scene_3d_api.c`. Loads a `.arm` scene file, creates Iron mesh/material objects via the existing asset pipeline, but does NOT create Flecs entities. Returns a pointer to the Iron mesh object that can be stored in `RenderObject3D.iron_mesh_object`.

Implementation:
- Calls `data_get_scene_raw(path)` to parse .arm
- Calls `scene_ensure_defaults()` to add defaults
- Calls `scene_create()` to create Iron objects
- Returns the first mesh object's `mesh_object_t` pointer
- Does NOT call the ECS entity creation code

### Mesh Bridge Auto-Detection

Update `mesh_bridge_3d_sync_transforms()` to also handle entities that have `RenderObject3D` with a valid `iron_mesh_object` but were created via raw ECS (not via the asset_loader's entity creation). The sync already iterates entities with `RenderObject3D` — it should work if the pointer is set correctly.

### Minic Flow

1. `component_lookup("comp_3d_position")` / `"comp_3d_rotation"` / `"comp_3d_mesh_renderer"` / `"RenderObject3D"`
2. `void *mesh_obj = mesh_load("cube.arm")` — loads Iron mesh data
3. `entity_create()` → `entity_add(mesh, pos/rot/mesh_comp/render_comp)`
4. `comp_set_ptr(render_comp, entity_get(mesh, render_comp), "iron_mesh_object", mesh_obj)`
5. `comp_set_bool(mesh_comp, entity_get(mesh, mesh_comp), "visible", 1)`

## Step Loop Changes

The step loop currently uses `entity_set_rotation(g_mesh, ...)` — a high-level wrapper. This is replaced with direct component access:
```minic
void *rot_data = entity_get(g_mesh, rot_comp);
comp_set_floats(rot_comp, rot_data, "x,y,z,w", rx, ry, rz, rw);
```

The `camera3d_control_system` continues to find the camera by name ("main_camera") and control it — no changes needed there.

## Files Changed

### C Code
- `engine/sources/core/scene_3d_api.c` — add `mesh_load()` native function + registration
- `engine/sources/ecs/mesh_bridge_3d.c` — verify sync handles raw-ECS-created entities

### Minic Scripts
- `engine/assets/systems/scene3d_test.minic` — full rewrite to raw ECS pattern
