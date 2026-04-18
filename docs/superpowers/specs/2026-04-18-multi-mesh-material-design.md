# Multi-Mesh Material Binding Design

## Overview

When an `.arm` file contains multiple mesh objects (e.g., `Camera_01_4k.arm` with `Camera_01_body` and `Camera_01_strap`), each mesh needs its own independent material context so textures can be bound individually. Previously, only the first mesh's material was used, causing incorrect rendering.

## Problem Statement

**Current Issues:**
1. `render3d_bridge.c` helper functions (`set_material_const`, `update_cam_pos_material`, `compute_light_vp`) only update `scene_meshes->buffer[0]`
2. `material_bind_texture` only affects the first mesh
3. When `mesh_load_arm` creates multiple meshes, they all share the same `material_data_t`
4. Changes to one mesh's material affect all meshes sharing that material

**Example: `Camera_01_4k.arm`**
- `Camera_01_body` → needs `body_diff.k`, `body_metallic.k`, `body_roughness.k`
- `Camera_01_strap` → needs `strap_diff.k`, `strap_metallic.k`, `strap_roughness.k`

## Solution

### 1. Per-Mesh Independent Material Context

When `mesh_load_arm` creates meshes, clone the `material_data_t` for each mesh so they have independent `material_context_t` arrays.

**Implementation in `scene_3d_api.c`:**
- After `scene_create` populates `scene_meshes`
- For each mesh object, clone its `material_data_t`
- Update `mesh_object->material` to point to the cloned data

### 2. New Minic API

```minic
// Bind texture to a specific mesh by name
// material_bind_texture_by_name(mesh_name, slot_name, file_path)
material_bind_texture_by_name("Camera_01_body", "tex_albedo", "3d/textures/body_diff.k");
material_bind_texture_by_name("Camera_01_strap", "tex_albedo", "3d/textures/strap_diff.k");
```

**Implementation:**
- Find mesh by name in `scene_meshes` array
- Load texture via `data_get_image()`
- Update the mesh's material `bind_tex->file` path
- Set corresponding `use_*_tex` flag to `1.0`

### 3. Fix `render3d_bridge` Helpers

**Current problematic functions only handle `buffer[0]`:**
- `set_material_const()` — sets shader constants only on first mesh
- `update_cam_pos_material()` — updates cam_pos only on first mesh
- `compute_light_vp()` — reads/writes light VP only on first mesh

**Fix approach:**
- Create helper that iterates all meshes: `for_each_mesh_do(mesh_action)`
- Or make functions accept mesh index parameter

**Note:** This is a deferred renderer — the G-buffer pass renders ALL meshes via `render_path_submit_draw("mesh")`. The issue is that material constants (light direction, camera position, shadow matrices) are only set on the first mesh, not propagated to the shader system correctly.

## File Changes

| File | Changes |
|------|---------|
| `scene_3d_api.c` | Clone material for each mesh in `minic_mesh_load_arm`; Add `minic_material_bind_texture_by_name` |
| `render3d_bridge.c` | Create `mesh_apply_material_to_all()` helper; Update helper functions to iterate all meshes |

## API

### New Functions

```c
// scene_3d_api.c
static minic_val_t minic_material_bind_texture_by_name(minic_val_t *args, int argc) {
    // args[0] = mesh name (const char*)
    // args[1] = slot name (const char*) e.g., "tex_albedo"
    // args[2] = file path (const char*) e.g., "3d/textures/body_diff.k"
    // Returns: int (1 = success, 0 = failure)
}
```

```minic
// Usage in .minic scripts
material_bind_texture_by_name("Camera_01_body", "tex_albedo", "3d/textures/body_diff.k");
material_bind_texture_by_name("Camera_01_body", "tex_metallic", "3d/textures/body_metallic.k");
material_bind_texture_by_name("Camera_01_body", "tex_roughness", "3d/textures/body_roughness.k");

material_bind_texture_by_name("Camera_01_strap", "tex_albedo", "3d/textures/strap_diff.k");
material_bind_texture_by_name("Camera_01_strap", "tex_metallic", "3d/textures/strap_metallic.k");
material_bind_texture_by_name("Camera_01_strap", "tex_roughness", "3d/textures/strap_roughness.k");
```

### Material Cloning

```c
// Clone material_data_t for independent mesh materials
material_data_t* clone_material_data(material_data_t* original) {
    material_data_t* clone = gc_alloc(sizeof(material_data_t));
    *clone = *original;  // shallow copy
    clone->contexts = any_array_create_from_raw(NULL, 0);  // empty contexts
    // Deep copy bind_constants
    clone->bind_constants = any_array_create_from_raw(NULL, 0);
    for (int i = 0; i < original->bind_constants->length; i++) {
        bind_const_t* orig_bc = original->bind_constants->buffer[i];
        bind_const_t* new_bc = gc_alloc(sizeof(bind_const_t));
        *new_bc = *orig_bc;
        new_bc->name = strdup(orig_bc->name);
        if (orig_bc->vec) {
            new_bc->vec = f32_array_create_from_raw(orig_bc->vec->buffer, orig_bc->vec->length);
        }
        any_array_push(clone->bind_constants, new_bc);
    }
    // Deep copy bind_textures
    clone->bind_textures = any_array_create_from_raw(NULL, 0);
    for (int i = 0; i < original->bind_textures->length; i++) {
        bind_tex_t* orig_bt = original->bind_textures->buffer[i];
        bind_tex_t* new_bt = gc_alloc(sizeof(bind_tex_t));
        *new_bt = *orig_bt;
        new_bt->name = strdup(orig_bt->name);
        new_bt->file = orig_bt->file ? strdup(orig_bt->file) : NULL;
        any_array_push(clone->bind_textures, new_bt);
    }
    return clone;
}
```

## Testing

1. Load `Camera_01_4k.arm` (2 meshes)
2. Bind different textures to each mesh by name
3. Verify both meshes render with correct textures
4. Verify material changes don't cross-contaminate between meshes
