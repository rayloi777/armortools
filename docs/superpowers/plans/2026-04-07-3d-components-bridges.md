# 3D Components & Bridges Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace 3D component stubs with real ECS data types and create bridge modules that sync ECS state to Iron engine objects.

**Architecture:** Direct ECS extension — six new Flecs components (`comp_3d_position`, `comp_3d_rotation`, `comp_3d_scale`, `comp_3d_camera`, `comp_3d_mesh_renderer`, `RenderObject3D`) registered identically to existing 2D components. Two new bridge modules (`camera_bridge_3d`, `mesh_bridge_3d`) follow the pattern established by `camera_bridge.c` and `ecs_bridge.c`.

**Tech Stack:** C, Flecs ECS v4.0, Iron engine (base/), Kong shaders

---

### Task 1: Replace transform.h stub with 3D component structs

**Files:**
- Modify: `engine/sources/components/transform.h`

- [ ] **Step 1: Write the new transform.h**

```c
#pragma once

typedef struct { float x, y, z; } comp_3d_position;
typedef struct { float x, y, z, w; } comp_3d_rotation;
typedef struct { float x, y, z; } comp_3d_scale;

void transform_init(void);
void transform_shutdown(void);
```

---

### Task 2: Replace camera.h stub with comp_3d_camera

**Files:**
- Modify: `engine/sources/components/camera.h`

- [ ] **Step 1: Write the new camera.h**

```c
#pragma once

#include <stdbool.h>

typedef struct {
    float fov;
    float near_plane;
    float far_plane;
    bool  perspective;
    bool  active;
} comp_3d_camera;

void camera_init(void);
void camera_shutdown(void);
```

---

### Task 3: Replace mesh_renderer.h stub with comp_3d_mesh_renderer

**Files:**
- Modify: `engine/sources/components/mesh_renderer.h`

- [ ] **Step 1: Write the new mesh_renderer.h**

```c
#pragma once

#include <stdbool.h>

typedef struct {
    char *mesh_path;
    char *material_path;
    bool  visible;
} comp_3d_mesh_renderer;

void mesh_renderer_init(void);
void mesh_renderer_shutdown(void);
```

---

### Task 4: Register 3D components in ecs_components.h

**Files:**
- Modify: `engine/sources/ecs/ecs_components.h`

- [ ] **Step 1: Add RenderObject3D struct and 3D component accessor declarations**

Add after the existing `comp_2d_camera` struct (line 55), before the function declarations (line 57):

```c
typedef struct {
    void *iron_mesh_object;
    void *iron_transform;
    bool  dirty;
} RenderObject3D;
```

Add after the existing `ecs_component_comp_2d_camera` declaration (line 72):

```c
uint64_t ecs_component_comp_3d_position(void);
uint64_t ecs_component_comp_3d_rotation(void);
uint64_t ecs_component_comp_3d_scale(void);
uint64_t ecs_component_comp_3d_camera(void);
uint64_t ecs_component_comp_3d_mesh_renderer(void);
uint64_t ecs_component_RenderObject3D(void);
```

---

### Task 5: Register 3D components in ecs_components.c

**Files:**
- Modify: `engine/sources/ecs/ecs_components.c`

- [ ] **Step 1: Add static entity IDs and accessor functions**

After line 16 (`static ecs_entity_t comp_2d_camera_entity = 0;`), add:

```c
static ecs_entity_t comp_3d_position_entity = 0;
static ecs_entity_t comp_3d_rotation_entity = 0;
static ecs_entity_t comp_3d_scale_entity = 0;
static ecs_entity_t comp_3d_camera_entity = 0;
static ecs_entity_t comp_3d_mesh_renderer_entity = 0;
static ecs_entity_t comp_RenderObject3D_entity = 0;
```

After line 27 (`ecs_component_comp_2d_camera`), add:

```c
ecs_entity_t ecs_component_comp_3d_position(void) { return comp_3d_position_entity; }
ecs_entity_t ecs_component_comp_3d_rotation(void) { return comp_3d_rotation_entity; }
ecs_entity_t ecs_component_comp_3d_scale(void) { return comp_3d_scale_entity; }
ecs_entity_t ecs_component_comp_3d_camera(void) { return comp_3d_camera_entity; }
ecs_entity_t ecs_component_comp_3d_mesh_renderer(void) { return comp_3d_mesh_renderer_entity; }
ecs_entity_t ecs_component_RenderObject3D(void) { return comp_RenderObject3D_entity; }
```

- [ ] **Step 2: Add register_component calls in ecs_register_components**

After line 63 (`comp_2d_camera_entity = ...`), add:

```c
    comp_3d_position_entity = register_component(ecs, "comp_3d_position", sizeof(comp_3d_position), _Alignof(comp_3d_position));
    comp_3d_rotation_entity = register_component(ecs, "comp_3d_rotation", sizeof(comp_3d_rotation), _Alignof(comp_3d_rotation));
    comp_3d_scale_entity = register_component(ecs, "comp_3d_scale", sizeof(comp_3d_scale), _Alignof(comp_3d_scale));
    comp_3d_camera_entity = register_component(ecs, "comp_3d_camera", sizeof(comp_3d_camera), _Alignof(comp_3d_camera));
    comp_3d_mesh_renderer_entity = register_component(ecs, "comp_3d_mesh_renderer", sizeof(comp_3d_mesh_renderer), _Alignof(comp_3d_mesh_renderer));
    comp_RenderObject3D_entity = register_component(ecs, "RenderObject3D", sizeof(RenderObject3D), _Alignof(RenderObject3D));
```

- [ ] **Step 3: Add field registrations in ecs_register_builtin_fields**

After the `comp_2d_camera` field registrations (after line 139), add:

```c
    id = comp_3d_position_entity;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_position, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_position, y));
    ecs_dynamic_component_add_field(id, "z", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_position, z));

    id = comp_3d_rotation_entity;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_rotation, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_rotation, y));
    ecs_dynamic_component_add_field(id, "z", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_rotation, z));
    ecs_dynamic_component_add_field(id, "w", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_rotation, w));

    id = comp_3d_scale_entity;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_scale, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_scale, y));
    ecs_dynamic_component_add_field(id, "z", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_scale, z));

    id = comp_3d_camera_entity;
    ecs_dynamic_component_add_field(id, "fov", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_camera, fov));
    ecs_dynamic_component_add_field(id, "near_plane", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_camera, near_plane));
    ecs_dynamic_component_add_field(id, "far_plane", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_camera, far_plane));
    ecs_dynamic_component_add_field(id, "perspective", DYNAMIC_TYPE_BOOL, offsetof(comp_3d_camera, perspective));
    ecs_dynamic_component_add_field(id, "active", DYNAMIC_TYPE_BOOL, offsetof(comp_3d_camera, active));

    id = comp_3d_mesh_renderer_entity;
    ecs_dynamic_component_add_field(id, "mesh_path", DYNAMIC_TYPE_PTR, offsetof(comp_3d_mesh_renderer, mesh_path));
    ecs_dynamic_component_add_field(id, "material_path", DYNAMIC_TYPE_PTR, offsetof(comp_3d_mesh_renderer, material_path));
    ecs_dynamic_component_add_field(id, "visible", DYNAMIC_TYPE_BOOL, offsetof(comp_3d_mesh_renderer, visible));

    id = comp_RenderObject3D_entity;
    ecs_dynamic_component_add_field(id, "iron_mesh_object", DYNAMIC_TYPE_PTR, offsetof(RenderObject3D, iron_mesh_object));
    ecs_dynamic_component_add_field(id, "iron_transform", DYNAMIC_TYPE_PTR, offsetof(RenderObject3D, iron_transform));
    ecs_dynamic_component_add_field(id, "dirty", DYNAMIC_TYPE_BOOL, offsetof(RenderObject3D, dirty));
```

- [ ] **Step 4: Add name lookups in ecs_get_builtin_component**

After line 153 (`comp_2d_camera` check), add:

```c
    if (strcmp(name, "comp_3d_position") == 0) return comp_3d_position_entity;
    if (strcmp(name, "comp_3d_rotation") == 0) return comp_3d_rotation_entity;
    if (strcmp(name, "comp_3d_scale") == 0) return comp_3d_scale_entity;
    if (strcmp(name, "comp_3d_camera") == 0) return comp_3d_camera_entity;
    if (strcmp(name, "comp_3d_mesh_renderer") == 0) return comp_3d_mesh_renderer_entity;
    if (strcmp(name, "RenderObject3D") == 0) return comp_RenderObject3D_entity;
```

- [ ] **Step 5: Add reverse name lookups in ecs_get_builtin_component_name**

After line 167 (`comp_2d_camera` check), add:

```c
    if (component_id == comp_3d_position_entity) return "comp_3d_position";
    if (component_id == comp_3d_rotation_entity) return "comp_3d_rotation";
    if (component_id == comp_3d_scale_entity) return "comp_3d_scale";
    if (component_id == comp_3d_camera_entity) return "comp_3d_camera";
    if (component_id == comp_3d_mesh_renderer_entity) return "comp_3d_mesh_renderer";
    if (component_id == comp_RenderObject3D_entity) return "RenderObject3D";
```

- [ ] **Step 6: Commit**

```bash
git add engine/sources/components/transform.h engine/sources/components/camera.h engine/sources/components/mesh_renderer.h engine/sources/ecs/ecs_components.h engine/sources/ecs/ecs_components.c
git commit -m "feat: add 3D component structs and ECS registration"
```

---

### Task 6: Build and verify components compile

**Files:** None (verification only)

- [ ] **Step 1: Build the engine**

Run:
```bash
cd engine && ../base/make macos metal
cd build && xcodebuild -project IronGame.xcodeproj -configuration Release build
```

Expected: Build succeeds with no errors. The new components are registered at startup but nothing references them yet.

---

### Task 7: Create camera_bridge_3d header

**Files:**
- Create: `engine/sources/ecs/camera_bridge_3d.h`

- [ ] **Step 1: Write camera_bridge_3d.h**

```c
#pragma once

#include <stdint.h>

struct game_world_t;
struct camera_object_t;

void camera_bridge_3d_set_world(struct game_world_t *world);
void camera_bridge_3d_init(void);
void camera_bridge_3d_shutdown(void);
void camera_bridge_3d_update(void);
struct camera_object_t *camera_bridge_3d_get_active(void);
```

---

### Task 8: Create camera_bridge_3d implementation

**Files:**
- Create: `engine/sources/ecs/camera_bridge_3d.c`

- [ ] **Step 1: Write camera_bridge_3d.c**

```c
#include "camera_bridge_3d.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "flecs.h"
#include <iron.h>
#include <stdio.h>
#include <string.h>

static game_world_t *g_world = NULL;
static camera_object_t *g_camera_3d = NULL;
static camera_data_t *g_camera_3d_data = NULL;
static ecs_query_t *g_camera_query = NULL;

void camera_bridge_3d_set_world(game_world_t *world) {
    g_world = world;
}

void camera_bridge_3d_init(void) {
    if (!g_world) {
        fprintf(stderr, "Camera Bridge 3D: game world not set\n");
        return;
    }

    // Create camera data with defaults
    g_camera_3d_data = GC_ALLOC_INIT(camera_data_t, {
        .name = "Camera3D",
        .near_plane = 0.1f,
        .far_plane = 100.0f,
        .fov = 60.0f,
        .frustum_culling = true
    });

    // Create camera object
    g_camera_3d = camera_object_create(g_camera_3d_data);
    if (!g_camera_3d) {
        fprintf(stderr, "Failed to create 3D camera object\n");
        return;
    }

    // Ensure scene root exists
    if (!_scene_root) {
        scene_t *empty_scene = GC_ALLOC_INIT(scene_t, {0});
        scene_create(empty_scene);
    }

    // Parent camera to scene root
    object_set_parent(g_camera_3d->base, _scene_root);

    // Build initial projection
    camera_object_build_proj(g_camera_3d, (f32)sys_width() / (f32)sys_height());
    camera_object_build_mat(g_camera_3d);

    // Cache query for per-frame camera lookup
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_world);
    g_camera_query = ecs_query_new(ecs, "[in] comp_3d_camera, [in] comp_3d_position, [in] comp_3d_rotation");

    printf("Camera Bridge 3D initialized\n");
}

void camera_bridge_3d_shutdown(void) {
    if (g_camera_query) {
        ecs_query_fini(g_camera_query);
        g_camera_query = NULL;
    }
    if (g_camera_3d) {
        camera_object_remove(g_camera_3d);
        g_camera_3d = NULL;
        g_camera_3d_data = NULL;
    }
    g_world = NULL;
    printf("Camera Bridge 3D shutdown\n");
}

void camera_bridge_3d_update(void) {
    if (!g_world || !g_camera_3d || !g_camera_query) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_world);
    if (!ecs) return;

    ecs_iter_t it = ecs_query_iter(ecs, g_camera_query);

    while (ecs_query_next(&it)) {
        comp_3d_camera *cam = ecs_field(&it, comp_3d_camera, 1);
        comp_3d_position *pos = ecs_field(&it, comp_3d_position, 2);
        comp_3d_rotation *rot = ecs_field(&it, comp_3d_rotation, 3);

        for (int i = 0; i < it.count; i++) {
            if (!cam[i].active) continue;

            // Sync position and rotation to Iron camera transform
            transform_t *t = g_camera_3d->base->transform;
            t->loc.x = pos[i].x;
            t->loc.y = pos[i].y;
            t->loc.z = pos[i].z;
            t->rot.x = rot[i].x;
            t->rot.y = rot[i].y;
            t->rot.z = rot[i].z;
            t->rot.w = rot[i].w;
            t->dirty = true;
            transform_build_matrix(t);

            // Update camera data from component
            g_camera_3d_data->fov = cam[i].fov;
            g_camera_3d_data->near_plane = cam[i].near_plane;
            g_camera_3d_data->far_plane = cam[i].far_plane;

            // Rebuild projection and view matrices
            camera_object_build_proj(g_camera_3d, (f32)sys_width() / (f32)sys_height());
            camera_object_build_mat(g_camera_3d);

            // Single active camera — stop after first match
            return;
        }
    }
}

camera_object_t *camera_bridge_3d_get_active(void) {
    return g_camera_3d;
}
```

- [ ] **Step 2: Commit**

```bash
git add engine/sources/ecs/camera_bridge_3d.h engine/sources/ecs/camera_bridge_3d.c
git commit -m "feat: add 3D camera bridge with ECS sync"
```

---

### Task 9: Create mesh_bridge_3d header

**Files:**
- Create: `engine/sources/ecs/mesh_bridge_3d.h`

- [ ] **Step 1: Write mesh_bridge_3d.h**

```c
#pragma once

#include <stdint.h>

struct game_world_t;

void mesh_bridge_3d_set_world(struct game_world_t *world);
void mesh_bridge_3d_init(void);
void mesh_bridge_3d_shutdown(void);
void mesh_bridge_3d_sync_transforms(void);
void mesh_bridge_3d_create_mesh(uint64_t entity);
void mesh_bridge_3d_destroy_mesh(uint64_t entity);
```

---

### Task 10: Create mesh_bridge_3d implementation

**Files:**
- Create: `engine/sources/ecs/mesh_bridge_3d.c`

- [ ] **Step 1: Write mesh_bridge_3d.c**

```c
#include "mesh_bridge_3d.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "flecs.h"
#include <iron.h>
#include <stdio.h>
#include <string.h>

static game_world_t *g_world = NULL;
static ecs_query_t *g_sync_query = NULL;
static ecs_query_t *g_cleanup_query = NULL;

void mesh_bridge_3d_set_world(game_world_t *world) {
    g_world = world;
}

void mesh_bridge_3d_init(void) {
    if (!g_world) {
        fprintf(stderr, "Mesh Bridge 3D: game world not set\n");
        return;
    }

    // Ensure scene root exists
    if (!_scene_root) {
        scene_t *empty_scene = GC_ALLOC_INIT(scene_t, {0});
        scene_create(empty_scene);
    }

    // Cache queries
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_world);
    g_sync_query = ecs_query_new(ecs,
        "[in] comp_3d_position, [in] comp_3d_rotation, [in] comp_3d_scale, [inout] RenderObject3D");
    g_cleanup_query = ecs_query_new(ecs, "[in] RenderObject3D");

    printf("Mesh Bridge 3D initialized\n");
}

void mesh_bridge_3d_shutdown(void) {
    if (g_world) {
        ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_world);
        if (ecs && g_cleanup_query) {
            ecs_iter_t it = ecs_query_iter(ecs, g_cleanup_query);
            while (ecs_query_next(&it)) {
                RenderObject3D *robj = ecs_field(&it, RenderObject3D, 1);
                for (int i = 0; i < it.count; i++) {
                    if (robj[i].iron_mesh_object) {
                        object_remove((object_t *)robj[i].iron_mesh_object);
                        robj[i].iron_mesh_object = NULL;
                        robj[i].iron_transform = NULL;
                    }
                }
            }
        }
    }
    if (g_sync_query) { ecs_query_fini(g_sync_query); g_sync_query = NULL; }
    if (g_cleanup_query) { ecs_query_fini(g_cleanup_query); g_cleanup_query = NULL; }
    g_world = NULL;
    printf("Mesh Bridge 3D shutdown\n");
}

void mesh_bridge_3d_sync_transforms(void) {
    if (!g_world || !g_sync_query) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_world);
    if (!ecs) return;

    ecs_iter_t it = ecs_query_iter(ecs, g_sync_query);

    while (ecs_query_next(&it)) {
        comp_3d_position *pos = ecs_field(&it, comp_3d_position, 1);
        comp_3d_rotation *rot = ecs_field(&it, comp_3d_rotation, 2);
        comp_3d_scale *scale = ecs_field(&it, comp_3d_scale, 3);
        RenderObject3D *robj = ecs_field(&it, RenderObject3D, 4);

        for (int i = 0; i < it.count; i++) {
            if (!robj[i].iron_transform) continue;

            transform_t *t = (transform_t *)robj[i].iron_transform;
            t->loc.x = pos[i].x;
            t->loc.y = pos[i].y;
            t->loc.z = pos[i].z;
            t->rot.x = rot[i].x;
            t->rot.y = rot[i].y;
            t->rot.z = rot[i].z;
            t->rot.w = rot[i].w;
            t->scale.x = scale[i].x;
            t->scale.y = scale[i].y;
            t->scale.z = scale[i].z;
            t->dirty = true;
            transform_build_matrix(t);
            robj[i].dirty = true;
        }
    }
}

void mesh_bridge_3d_create_mesh(uint64_t entity) {
    if (!g_world || !entity) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_world);
    if (!ecs) return;

    ecs_entity_t e = (ecs_entity_t)entity;

    RenderObject3D *robj = (RenderObject3D *)ecs_get_id(ecs, e, ecs_component_RenderObject3D());
    comp_3d_mesh_renderer *mr = (comp_3d_mesh_renderer *)ecs_get_id(ecs, e, ecs_component_comp_3d_mesh_renderer());

    if (!robj || !mr || robj->iron_mesh_object != NULL) return;
    if (mr->mesh_path == NULL) return;

    mesh_data_t *mesh_data = data_get_mesh(mr->mesh_path, mr->mesh_path);
    material_data_t *mat_data = NULL;
    if (mr->material_path != NULL) {
        mat_data = data_get_material(mr->material_path, mr->material_path);
    }

    if (mesh_data != NULL) {
        mesh_object_t *mesh_obj = mesh_object_create(mesh_data, mat_data);
        if (mesh_obj != NULL) {
            robj->iron_mesh_object = mesh_obj->base;
            robj->iron_transform = mesh_obj->base->transform;
            robj->dirty = true;
            object_set_parent(mesh_obj->base, _scene_root);
        }
    }
}

void mesh_bridge_3d_destroy_mesh(uint64_t entity) {
    if (!g_world || !entity) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_world);
    if (!ecs) return;

    ecs_entity_t e = (ecs_entity_t)entity;

    RenderObject3D *robj = (RenderObject3D *)ecs_get_id(ecs, e, ecs_component_RenderObject3D());
    if (robj && robj->iron_mesh_object) {
        object_remove((object_t *)robj->iron_mesh_object);
        robj->iron_mesh_object = NULL;
        robj->iron_transform = NULL;
        robj->dirty = false;
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add engine/sources/ecs/mesh_bridge_3d.h engine/sources/ecs/mesh_bridge_3d.c
git commit -m "feat: add 3D mesh bridge with transform sync and Iron object lifecycle"
```

---

### Task 11: Wire 3D bridges into game_engine.c

**Files:**
- Modify: `engine/sources/game_engine.c`

- [ ] **Step 1: Add includes**

After line 15 (`#include "ecs/render2d_bridge.h"`), add:

```c
#include "ecs/camera_bridge_3d.h"
#include "ecs/mesh_bridge_3d.h"
```

- [ ] **Step 2: Add init calls**

After line 126 (`camera_bridge_init();`), add:

```c
    camera_bridge_3d_set_world(g_world);
    camera_bridge_3d_init();

    mesh_bridge_3d_set_world(g_world);
    mesh_bridge_3d_init();
```

- [ ] **Step 3: Add shutdown calls**

Before line 166 (`game_world_destroy(g_world);`), add:

```c
    mesh_bridge_3d_shutdown();
    camera_bridge_3d_shutdown();
```

- [ ] **Step 4: Commit**

```bash
git add engine/sources/game_engine.c
git commit -m "feat: wire 3D camera and mesh bridges into engine init/shutdown"
```

---

### Task 12: Wire 3D updates into game_loop.c

**Files:**
- Modify: `engine/sources/core/game_loop.c`

- [ ] **Step 1: Add includes**

After line 4 (`#include "ecs/render2d_bridge.h"`), add:

```c
#include "ecs/camera_bridge_3d.h"
#include "ecs/mesh_bridge_3d.h"
```

- [ ] **Step 2: Add 3D update calls in game_loop_update**

After line 40 (`draw_begin(NULL, true, 0xff1a1a2e);`), before line 41 (`camera2d_update(...)`), add:

```c
    camera_bridge_3d_update();
    mesh_bridge_3d_sync_transforms();
```

- [ ] **Step 3: Commit**

```bash
git add engine/sources/core/game_loop.c
git commit -m "feat: add 3D camera and mesh sync to game loop"
```

---

### Task 13: Build and verify

**Files:** None (verification only)

- [ ] **Step 1: Build the engine**

Run:
```bash
cd engine && ../base/make macos metal
cd build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | tail -20
```

Expected: Build succeeds. The 3D bridges initialize at startup (check console output for "Camera Bridge 3D initialized" and "Mesh Bridge 3D initialized").

- [ ] **Step 2: Run the binary**

Run:
```bash
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame 2>&1 | head -30
```

Expected: Engine starts, shows "Camera Bridge 3D initialized" and "Mesh Bridge 3D initialized" in console output alongside existing 2D messages. No crashes.

- [ ] **Step 3: Final commit (if any build fixes needed)**

```bash
git add -A
git commit -m "fix: build fixes for 3D components and bridges"
```
