# Mesh Texture + Camera Orthographic Design

## Problem

1. Engine's `mesh.kong` shader outputs a flat color — no texture sampling. Cube renders without its texture.
2. `camera_3d_create` only supports perspective. No way to create orthographic cameras from Minic scripts.
3. `camera_bridge_3d` ignores the `perspective` boolean in `comp_3d_camera` — never syncs ortho data to Iron.

## Approach

Two independent fixes:
1. Replace `mesh.kong` with texture-sampling version (match cube test reference)
2. Add `camera_3d_perspective` / `camera_3d_orthographic` Minic APIs + bridge sync

## Change 1: mesh.kong Texture Shader

**File:** `engine/assets/shaders/mesh.kong`

Replace with cube test's shader that:
- Adds `tex: float2` to `vert_in` and `vert_out`
- Declares `sampler_linear: sampler` and `my_texture: tex2d` constants
- Passes UV through vertex shader
- Samples texture in fragment: `sample(my_texture, sampler_linear, input.tex)`
- Removes incorrect perspective division (`pos.x / pos.w` etc.)

No C code changes needed — `scene_ensure_defaults` already binds `texture.k` to `my_texture` and the shader declares the `my_texture` texture unit.

## Change 2: Perspective/Orthographic Camera APIs

### comp_3d_camera (camera.h)

Add ortho fields:

```c
typedef struct {
    float fov;
    float near_plane;
    float far_plane;
    bool  perspective;
    bool  active;
    float ortho_left, ortho_right, ortho_bottom, ortho_top;
} comp_3d_camera;
```

### Minic API (scene_3d_api.c)

- `camera_3d_perspective(entity, fov, near, far)` — sets perspective mode on existing entity
- `camera_3d_orthographic(entity, left, right, bottom, top, near, far)` — sets orthographic mode on existing entity

Both modify the `comp_3d_camera` component on the given entity.

### camera_bridge_3d sync (camera_bridge_3d.c)

In `camera_bridge_3d_update()`, after syncing fov/near/far:
- If `cam_fresh->perspective == false`: set `g_camera_3d_data->ortho` to a `f32_array_t` with `[left, right, bottom, top]`
- If `cam_fresh->perspective == true`: set `g_camera_3d_data->ortho = NULL`

Iron's `camera_object_build_proj()` already checks `data->ortho != NULL` to choose ortho vs perspective projection.

### ecs_components.c field registration

Register the 4 new ortho fields (`ortho_left`, `ortho_right`, `ortho_bottom`, `ortho_top`) and the existing `perspective` field for dynamic component access.
