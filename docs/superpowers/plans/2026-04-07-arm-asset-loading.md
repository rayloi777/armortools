# .arm Asset Loading Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Load .arm scene files using Iron's scene_create() and create ECS entities with 3D components that sync to the Iron objects.

**Architecture:** Iron scene_create + ECS sync — call Iron's scene_create() to set up the scene graph, then iterate the created mesh/camera objects and create ECS entities with comp_3d_position/rotation/scale, comp_3d_mesh_renderer, and RenderObject3D pointing to the Iron objects.

**Tech Stack:** C, Flecs ECS v4.0, Iron engine, Minic scripting

---

### Task 1: Create asset_loader header

**Files:**
- Create: `engine/sources/core/asset_loader.h`

- [ ] **Step 1: Write asset_loader.h**

```c
#pragma once

#include <stdint.h>

struct game_world_t;

void asset_loader_set_world(struct game_world_t *world);
uint64_t asset_loader_load_scene(const char *path);
int asset_loader_load_mesh(uint64_t entity, const char *mesh_path, const char *material_path);
```

---

### Task 2: Create asset_loader implementation

**Files:**
- Create: `engine/sources/core/asset_loader.c`

- [ ] **Step 1: Write asset_loader.c**

```c
#include "asset_loader.h"
#include "ecs/ecs_world.h"
#include "ecs/ecs_components.h"
#include "ecs/mesh_bridge_3d.h"
#include <iron.h>
#include <stdio.h>
#include <string.h>

static game_world_t *g_asset_world = NULL;

void asset_loader_set_world(game_world_t *world) {
    g_asset_world = world;
}

uint64_t asset_loader_load_scene(const char *path) {
    if (!g_asset_world || !path) return 0;

    // Parse .arm file
    scene_t *scene_raw = data_get_scene_raw(path);
    if (!scene_raw) {
        fprintf(stderr, "Asset Loader: failed to load scene '%s'\n", path);
        return 0;
    }

    // Create Iron scene graph
    if (!_scene_root) {
        scene_create(scene_raw);
    }
    else {
        // Scene already initialized — add objects individually
        if (scene_raw->objects != NULL) {
            for (int i = 0; i < scene_raw->objects->length; i++) {
                obj_t *o = (obj_t *)scene_raw->objects->buffer[i];
                scene_create_object(o, scene_raw, _scene_root);
            }
        }
    }

    if (!_scene_root) {
        fprintf(stderr, "Asset Loader: scene_create failed for '%s'\n", path);
        return 0;
    }

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_asset_world);
    if (!ecs) return 0;

    uint64_t first_entity = 0;

    // Sync mesh objects from scene_meshes to ECS
    if (scene_meshes != NULL) {
        for (int i = 0; i < scene_meshes->length; i++) {
            mesh_object_t *mesh_obj = (mesh_object_t *)scene_meshes->buffer[i];
            if (!mesh_obj || !mesh_obj->base) continue;

            // Create ECS entity
            ecs_entity_t e = ecs_new(ecs);
            if (first_entity == 0) first_entity = (uint64_t)e;

            // Read transform from Iron object
            transform_t *t = mesh_obj->base->transform;

            // Set 3D position
            comp_3d_position pos = { t->loc.x, t->loc.y, t->loc.z };
            ecs_set_id(ecs, e, ecs_component_comp_3d_position(), sizeof(comp_3d_position), &pos);

            // Set 3D rotation (quaternion)
            comp_3d_rotation rot = { t->rot.x, t->rot.y, t->rot.z, t->rot.w };
            ecs_set_id(ecs, e, ecs_component_comp_3d_rotation(), sizeof(comp_3d_rotation), &rot);

            // Set 3D scale
            comp_3d_scale scl = { t->scale.x, t->scale.y, t->scale.z };
            ecs_set_id(ecs, e, ecs_component_comp_3d_scale(), sizeof(comp_3d_scale), &scl);

            // Set mesh renderer
            comp_3d_mesh_renderer mr = { .visible = true };
            ecs_set_id(ecs, e, ecs_component_comp_3d_mesh_renderer(), sizeof(comp_3d_mesh_renderer), &mr);

            // Set RenderObject3D pointing to Iron mesh object
            RenderObject3D robj = {
                .iron_mesh_object = mesh_obj->base,
                .iron_transform = mesh_obj->base->transform,
                .dirty = true
            };
            ecs_set_id(ecs, e, ecs_component_RenderObject3D(), sizeof(RenderObject3D), &robj);

            printf("Asset Loader: created mesh entity %u '%s'\n", (unsigned)e, mesh_obj->base->name ? mesh_obj->base->name : "");
        }
    }

    // Sync cameras from scene_cameras to ECS
    if (scene_cameras != NULL) {
        bool first_camera = true;
        for (int i = 0; i < scene_cameras->length; i++) {
            camera_object_t *cam_obj = (camera_object_t *)scene_cameras->buffer[i];
            if (!cam_obj || !cam_obj->base) continue;

            ecs_entity_t e = ecs_new(ecs);
            if (first_entity == 0) first_entity = (uint64_t)e;

            transform_t *t = cam_obj->base->transform;

            comp_3d_position pos = { t->loc.x, t->loc.y, t->loc.z };
            ecs_set_id(ecs, e, ecs_component_comp_3d_position(), sizeof(comp_3d_position), &pos);

            comp_3d_rotation rot = { t->rot.x, t->rot.y, t->rot.z, t->rot.w };
            ecs_set_id(ecs, e, ecs_component_comp_3d_rotation(), sizeof(comp_3d_rotation), &rot);

            comp_3d_scale scl = { t->scale.x, t->scale.y, t->scale.z };
            ecs_set_id(ecs, e, ecs_component_comp_3d_scale(), sizeof(comp_3d_scale), &scl);

            comp_3d_camera cam = {
                .fov = cam_obj->data->fov,
                .near_plane = cam_obj->data->near_plane,
                .far_plane = cam_obj->data->far_plane,
                .perspective = cam_obj->data->ortho == NULL,
                .active = first_camera
            };
            ecs_set_id(ecs, e, ecs_component_comp_3d_camera(), sizeof(comp_3d_camera), &cam);

            first_camera = false;
            printf("Asset Loader: created camera entity %u\n", (unsigned)e);
        }
    }

    printf("Asset Loader: scene '%s' loaded\n", path);
    return first_entity;
}

int asset_loader_load_mesh(uint64_t entity, const char *mesh_path, const char *material_path) {
    if (!g_asset_world || !entity || !mesh_path) return -1;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_asset_world);
    if (!ecs) return -1;

    ecs_entity_t e = (ecs_entity_t)entity;
    if (!ecs_is_alive(ecs, e)) return -1;

    // Ensure scene root exists
    if (!_scene_root) {
        scene_t *empty_scene = GC_ALLOC_INIT(scene_t, {0});
        scene_create(empty_scene);
    }

    mesh_data_t *mesh_data = data_get_mesh(mesh_path, mesh_path);
    if (!mesh_data) {
        fprintf(stderr, "Asset Loader: failed to load mesh '%s'\n", mesh_path);
        return -1;
    }

    material_data_t *mat_data = NULL;
    if (material_path != NULL) {
        mat_data = data_get_material(material_path, material_path);
    }

    mesh_object_t *mesh_obj = mesh_object_create(mesh_data, mat_data);
    if (!mesh_obj) {
        fprintf(stderr, "Asset Loader: failed to create mesh object for '%s'\n", mesh_path);
        return -1;
    }

    object_set_parent(mesh_obj->base, _scene_root);

    // Update RenderObject3D on the entity
    RenderObject3D *robj = (RenderObject3D *)ecs_get_id(ecs, e, ecs_component_RenderObject3D());
    if (robj) {
        robj->iron_mesh_object = mesh_obj->base;
        robj->iron_transform = mesh_obj->base->transform;
        robj->dirty = true;
    }
    else {
        RenderObject3D new_robj = {
            .iron_mesh_object = mesh_obj->base,
            .iron_transform = mesh_obj->base->transform,
            .dirty = true
        };
        ecs_set_id(ecs, e, ecs_component_RenderObject3D(), sizeof(RenderObject3D), &new_robj);
    }

    // Update mesh renderer
    comp_3d_mesh_renderer *mr = (comp_3d_mesh_renderer *)ecs_get_id(ecs, e, ecs_component_comp_3d_mesh_renderer());
    if (mr) {
        mr->visible = true;
    }
    else {
        comp_3d_mesh_renderer new_mr = { .visible = true };
        ecs_set_id(ecs, e, ecs_component_comp_3d_mesh_renderer(), sizeof(comp_3d_mesh_renderer), &new_mr);
    }

    return 0;
}
```

---

### Task 3: Create scene_api header

**Files:**
- Create: `engine/sources/core/scene_api.h`

- [ ] **Step 1: Write scene_api.h**

```c
#pragma once

void scene_api_register(void);
```

---

### Task 4: Create scene_api implementation

**Files:**
- Create: `engine/sources/core/scene_api.c`

This follows the exact pattern from `runtime_api.c` — static functions with `minic_val_t *args, int argc` signature, returning `minic_val_t`.

- [ ] **Step 1: Write scene_api.c**

```c
#include "scene_api.h"
#include "runtime_api.h"
#include "asset_loader.h"
#include "ecs/ecs_world.h"
#include <stdio.h>

static game_world_t *g_scene_api_world = NULL;

void scene_api_set_world(game_world_t *world) {
    g_scene_api_world = world;
}

static minic_val_t minic_scene_load(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_PTR) return minic_val_id(0);
    const char *path = (const char *)args[0].p;
    uint64_t root = asset_loader_load_scene(path);
    return minic_val_id(root);
}

static minic_val_t minic_mesh_load(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_int(-1);
    uint64_t entity = 0;
    if (args[0].type == MINIC_T_ID) entity = args[0].u64;
    else if (args[0].type == MINIC_T_PTR) entity = (uint64_t)(uintptr_t)args[0].p;
    else entity = (uint64_t)minic_val_to_d(args[0]);

    const char *mesh_path = (args[1].type == MINIC_T_PTR) ? (const char *)args[1].p : NULL;
    const char *mat_path = (argc > 2 && args[2].type == MINIC_T_PTR) ? (const char *)args[2].p : NULL;

    int result = asset_loader_load_mesh(entity, mesh_path, mat_path);
    return minic_val_int(result);
}

void scene_api_register(void) {
    minic_register_native("scene_load", minic_scene_load);
    minic_register_native("mesh_load", minic_mesh_load);
}
```

---

### Task 5: Wire into unity build (main.c)

**Files:**
- Modify: `engine/sources/main.c`

- [ ] **Step 1: Add includes after the 3D Systems block**

After the line `#include "ecs/mesh_bridge_3d.c"`, add:

```c
#include "core/asset_loader.c"
#include "core/scene_api.c"
```

---

### Task 6: Wire into engine init and runtime API

**Files:**
- Modify: `engine/sources/game_engine.c`

- [ ] **Step 1: Add include**

After the mesh_bridge_3d.h include, add:

```c
#include "core/asset_loader.h"
#include "core/scene_api.h"
```

- [ ] **Step 2: Add init calls**

After `mesh_bridge_3d_init();`, add:

```c
    asset_loader_set_world(g_world);
    scene_api_set_world(g_world);
```

**Files:**
- Modify: `engine/sources/core/runtime_api.c`

- [ ] **Step 3: Add include and registration call**

After the existing includes (line 6, after `#include "ui_ext_api.h"`), add:

```c
#include "scene_api.h"
```

Before `ui_ext_api_register();` (around line 1392), add:

```c
    // Scene/3D asset loading API
    scene_api_register();
```

---

### Task 7: Add .arm asset export to project.js

**Files:**
- Modify: `engine/project.js`

- [ ] **Step 1: Add .arm asset export line**

After the existing `project.add_assets("assets/images/*.png", ...)` line, add:

```javascript
project.add_assets("assets/3d/*.arm", {destination: "data/{name}"});
```

---

### Task 8: Build and verify

**Files:** None (verification only)

- [ ] **Step 1: Build the engine**

Run:
```bash
cd engine && ../base/make macos metal
cd build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | tail -20
```

Expected: Build succeeds with no errors.

- [ ] **Step 2: Commit**

```bash
git add engine/sources/core/asset_loader.h engine/sources/core/asset_loader.c engine/sources/core/scene_api.h engine/sources/core/scene_api.c engine/sources/main.c engine/sources/game_engine.c engine/sources/core/runtime_api.c engine/project.js
git commit -m "feat: add .arm asset loading with Minic scene_load/mesh_load API

asset_loader uses Iron's scene_create() to load .arm files, then
syncs mesh/camera objects to ECS entities with 3D components.
scene_api registers scene_load() and mesh_load() as Minic natives.

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>"
```

- [ ] **Step 3: Create test script and verify**

Create `engine/assets/scripts/test_3d.minic`:
```
// Test 3D scene loading
var root = scene_load("cube.arm");
trace("Loaded scene, root entity: " + int_to_string(root));
```

Add it to the systems manifest or run manually. Verify console output shows "Asset Loader: scene 'cube.arm' loaded" and mesh entities created.
