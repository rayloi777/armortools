# Mesh Texture + Camera Orthographic Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add texture sampling to the mesh shader and add perspective/orthographic camera APIs for Minic scripts.

**Architecture:** Replace mesh.kong with the cube test's texture-sampling shader. Add ortho fields to `comp_3d_camera`, expose `camera_3d_perspective`/`camera_3d_orthographic` in Minic, and sync ortho data through the existing camera bridge.

**Tech Stack:** Kong shader language, C (engine layer), Flecs ECS, Minic scripting

---

### Task 1: Replace mesh.kong with texture-sampling shader

**Files:**
- Modify: `engine/assets/shaders/mesh.kong`

- [ ] **Step 1: Replace mesh.kong content**

Replace the entire file with the cube test's texture-sampling shader:

```
#[set(everything)]
const constants: {
	WVP: float4x4;
};

#[set(everything)]
const sampler_linear: sampler;

#[set(everything)]
const my_texture: tex2d;

struct vert_in {
	pos: float4;
	tex: float2;
}

struct vert_out {
	pos: float4;
	tex: float2;
}

fun mesh_vert(input: vert_in): vert_out {
	var output: vert_out;
	output.tex = input.tex;
	output.pos = constants.WVP * float4(input.pos.xyz, 1.0);
	return output;
}

fun mesh_frag(input: vert_out): float4 {
	return sample(my_texture, sampler_linear, input.tex);
}

#[pipe]
struct pipe {
	vertex = mesh_vert;
	fragment = mesh_frag;
}
```

- [ ] **Step 2: Commit**

```bash
git add engine/assets/shaders/mesh.kong
git commit -m "feat(shader): add texture sampling to mesh.kong

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

### Task 2: Add ortho fields to comp_3d_camera

**Files:**
- Modify: `engine/sources/components/camera.h`
- Modify: `engine/sources/ecs/ecs_components.c`

- [ ] **Step 1: Update comp_3d_camera struct**

In `engine/sources/components/camera.h`, replace the entire struct:

```c
#pragma once

#include <stdbool.h>

typedef struct {
    float fov;
    float near_plane;
    float far_plane;
    bool  perspective;
    bool  active;
    float ortho_left, ortho_right, ortho_bottom, ortho_top;
} comp_3d_camera;

void camera_init(void);
void camera_shutdown(void);
```

- [ ] **Step 2: Register ortho fields in ecs_components.c**

In `engine/sources/ecs/ecs_components.c`, find the block that registers `comp_3d_camera_entity` fields (currently registers `fov`, `near_plane`, `far_plane`, `perspective`, `active`). Add 4 new field registrations after the `active` line:

```c
    ecs_dynamic_component_add_field(id, "ortho_left", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_camera, ortho_left));
    ecs_dynamic_component_add_field(id, "ortho_right", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_camera, ortho_right));
    ecs_dynamic_component_add_field(id, "ortho_bottom", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_camera, ortho_bottom));
    ecs_dynamic_component_add_field(id, "ortho_top", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_camera, ortho_top));
```

- [ ] **Step 3: Commit**

```bash
git add engine/sources/components/camera.h engine/sources/ecs/ecs_components.c
git commit -m "feat(camera): add ortho fields to comp_3d_camera component

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

### Task 3: Add camera_3d_perspective and camera_3d_orthographic Minic APIs

**Files:**
- Modify: `engine/sources/core/scene_3d_api.c`

- [ ] **Step 1: Add camera_3d_perspective function**

Add this function before `scene_3d_api_register`:

```c
// camera_3d_perspective(entity, fov, near, far)
static minic_val_t minic_camera_3d_perspective(minic_val_t *args, int argc) {
    if (argc < 4 || !g_runtime_world) return minic_val_void();
    uint64_t entity = extract_entity_id3d(&args[0]);
    float fov = (float)minic_val_to_d(args[1]);
    float near_plane = (float)minic_val_to_d(args[2]);
    float far_plane = (float)minic_val_to_d(args[3]);

    uint64_t cam_id = ecs_component_comp_3d_camera();
    comp_3d_camera *cam = (comp_3d_camera *)entity_get_component_data(g_runtime_world, entity, cam_id);
    if (cam) {
        cam->fov = fov;
        cam->near_plane = near_plane;
        cam->far_plane = far_plane;
        cam->perspective = true;
    }
    return minic_val_void();
}
```

- [ ] **Step 2: Add camera_3d_orthographic function**

Add after `minic_camera_3d_perspective`:

```c
// camera_3d_orthographic(entity, left, right, bottom, top, near, far)
static minic_val_t minic_camera_3d_orthographic(minic_val_t *args, int argc) {
    if (argc < 7 || !g_runtime_world) return minic_val_void();
    uint64_t entity = extract_entity_id3d(&args[0]);
    float left = (float)minic_val_to_d(args[1]);
    float right = (float)minic_val_to_d(args[2]);
    float bottom = (float)minic_val_to_d(args[3]);
    float top = (float)minic_val_to_d(args[4]);
    float near_plane = (float)minic_val_to_d(args[5]);
    float far_plane = (float)minic_val_to_d(args[6]);

    uint64_t cam_id = ecs_component_comp_3d_camera();
    comp_3d_camera *cam = (comp_3d_camera *)entity_get_component_data(g_runtime_world, entity, cam_id);
    if (cam) {
        cam->ortho_left = left;
        cam->ortho_right = right;
        cam->ortho_bottom = bottom;
        cam->ortho_top = top;
        cam->near_plane = near_plane;
        cam->far_plane = far_plane;
        cam->perspective = false;
    }
    return minic_val_void();
}
```

- [ ] **Step 3: Register both functions**

In `scene_3d_api_register`, add these two lines after the existing `camera_3d_set_fov` registration:

```c
    minic_register_native("camera_3d_perspective", minic_camera_3d_perspective);
    minic_register_native("camera_3d_orthographic", minic_camera_3d_orthographic);
```

- [ ] **Step 4: Commit**

```bash
git add engine/sources/core/scene_3d_api.c
git commit -m "feat(scene_3d_api): add camera_3d_perspective and camera_3d_orthographic Minic APIs

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

### Task 4: Sync ortho data in camera_bridge_3d

**Files:**
- Modify: `engine/sources/ecs/camera_bridge_3d.c`

- [ ] **Step 1: Add ortho sync to camera_bridge_3d_update**

In `camera_bridge_3d_update()`, find the block that syncs camera data from component to Iron (lines that set `g_camera_3d_data->fov`, `g_camera_3d_data->near_plane`, `g_camera_3d_data->far_plane`). After setting `far_plane`, add:

```c
            // Sync perspective/ortho mode
            if (cam_fresh->perspective) {
                g_camera_3d_data->ortho = NULL;
            } else {
                // Create or reuse f32_array_t for ortho bounds
                if (g_camera_3d_data->ortho == NULL || g_camera_3d_data->ortho->length < 4) {
                    g_camera_3d_data->ortho = gc_alloc(sizeof(f32_array_t));
                    g_camera_3d_data->ortho->buffer = gc_alloc(sizeof(float) * 4);
                    g_camera_3d_data->ortho->length = 4;
                    g_camera_3d_data->ortho->capacity = 4;
                }
                g_camera_3d_data->ortho->buffer[0] = cam_fresh->ortho_left;
                g_camera_3d_data->ortho->buffer[1] = cam_fresh->ortho_right;
                g_camera_3d_data->ortho->buffer[2] = cam_fresh->ortho_bottom;
                g_camera_3d_data->ortho->buffer[3] = cam_fresh->ortho_top;
            }
```

This must go after the `far_plane` sync but before the `camera_object_build_proj` call, since `build_proj` reads `g_camera_3d_data->ortho`.

- [ ] **Step 2: Commit**

```bash
git add engine/sources/ecs/camera_bridge_3d.c
git commit -m "feat(camera_bridge): sync perspective/ortho mode from ECS to Iron

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

---

### Task 5: Build and verify

- [ ] **Step 1: Export assets and shaders**

```bash
cd engine && ../base/make macos metal
```

Expected: `mesh.kong` compiles successfully. Done in ~700ms.

- [ ] **Step 2: Compile C code**

```bash
cd engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build
```

Expected: BUILD SUCCEEDED.

- [ ] **Step 3: Run and verify**

```bash
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame
```

Expected:
- Window opens with a textured cube (not flat color)
- No crash
- Console shows scene loaded successfully

- [ ] **Step 4: Commit any build fixes if needed**

```bash
git add -u
git commit -m "fix: build fixes for texture and ortho camera"
```
