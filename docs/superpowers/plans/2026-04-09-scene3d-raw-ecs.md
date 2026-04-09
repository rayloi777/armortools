# scene3d_test Raw ECS Rewrite Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rewrite scene3d_test.minic to create camera, mesh, and light entities using the raw Flecs ECS pattern instead of high-level wrappers.

**Architecture:** Add a new `mesh_load()` C API that loads .arm scene data into Iron objects without creating ECS entities. Update the mesh bridge sync to read Iron objects from entity components instead of by-index. Rewrite scene3d_test.minic to use `entity_create()` + `component_lookup()` + `entity_add()` + `comp_set_*()` for all three entity types.

**Tech Stack:** C (Flecs ECS, Iron engine), Minic scripting language.

---

## File Structure

| File | Action | Responsibility |
|------|--------|----------------|
| `engine/sources/core/scene_3d_api.c` | Modify | Add `mesh_load()` native function + registration |
| `engine/sources/ecs/mesh_bridge_3d.c` | Modify | Update sync query to include RenderObject3D, read mesh object from entity |
| `engine/assets/systems/scene3d_test.minic` | Rewrite | Full rewrite to raw ECS pattern |

---

### Task 1: Add `mesh_load()` C API

**Files:**
- Modify: `engine/sources/core/scene_3d_api.c`

- [ ] **Step 1: Add the mesh_load native function**

Add the following function before the `scene_3d_api_register` function in `engine/sources/core/scene_3d_api.c` (after the `minic_light_directional` function, around line 335):

```c
// mesh_load(path) -> ptr — loads .arm scene into Iron objects without creating ECS entities
static minic_val_t minic_mesh_load(minic_val_t *args, int argc) {
    if (!g_runtime_world) return minic_val_ptr(NULL);

    const char *mesh_path = (argc > 0 && args[0].type == MINIC_T_PTR) ? (const char *)args[0].p : NULL;
    if (!mesh_path) return minic_val_ptr(NULL);

    // Parse .arm file
    scene_t *scene_raw = data_get_scene_raw(mesh_path);
    if (!scene_raw) {
        fprintf(stderr, "[mesh_load] Failed to load scene '%s'\n", mesh_path);
        return minic_val_ptr(NULL);
    }

    // Auto-generate missing camera/material/shader/world data
    // Access scene_ensure_defaults via asset_loader header
    scene_ensure_defaults(scene_raw);

    // Cache patched scene under its own name so scene_create can find it
    if (scene_raw->name != NULL) {
        any_map_set(data_cached_scene_raws, scene_raw->name, scene_raw);
    }

    // Remove existing scene if present, then create fresh
    if (_scene_root != NULL) {
        scene_remove();
    }
    scene_create(scene_raw);

    if (!_scene_root) {
        fprintf(stderr, "[mesh_load] scene_create failed for '%s'\n", mesh_path);
        return minic_val_ptr(NULL);
    }

    // Initialize viewport dimensions to prevent division-by-zero
    render_path_current_w = sys_w();
    render_path_current_h = sys_h();

    // Position the auto-created camera behind origin
    if (scene_camera != NULL && scene_camera->base != NULL) {
        transform_t *t = scene_camera->base->transform;
        t->loc = vec4_create(0, 2, 5, 1);
        t->rot = quat_create(0, 0, 0, 1);
        transform_build_matrix(t);
        camera_object_build_proj(scene_camera, (f32)sys_w() / (f32)sys_h());
        camera_object_build_mat(scene_camera);
    }

    // Return the first mesh object pointer (or NULL)
    if (scene_meshes != NULL && scene_meshes->length > 0) {
        mesh_object_t *mesh_obj = (mesh_object_t *)scene_meshes->buffer[0];
        if (mesh_obj && mesh_obj->base) {
            printf("[mesh_load] Loaded '%s', returning mesh object\n", mesh_path);
            return minic_val_ptr(mesh_obj->base);
        }
    }

    printf("[mesh_load] Loaded '%s', no mesh objects found\n", mesh_path);
    return minic_val_ptr(NULL);
}
```

Note: `scene_ensure_defaults()` is declared `static` in `asset_loader.c`. It needs to be made non-static and declared in `asset_loader.h`. Add to `engine/sources/core/asset_loader.h`:

```c
void scene_ensure_defaults(scene_t *scene);
```

And in `engine/sources/core/asset_loader.c`, change line 16 from:

```c
static void scene_ensure_defaults(scene_t *scene) {
```

to:

```c
void scene_ensure_defaults(scene_t *scene) {
```

- [ ] **Step 2: Register mesh_load in scene_3d_api_register**

Add this line inside `scene_3d_api_register()` (in `engine/sources/core/scene_3d_api.c`), after the existing `minic_register_native("light_directional", ...)` line:

```c
    minic_register_native("mesh_load", minic_mesh_load);
```

- [ ] **Step 3: Commit**

```bash
git add engine/sources/core/scene_3d_api.c engine/sources/core/asset_loader.c engine/sources/core/asset_loader.h
git commit -m "feat(scene_3d_api): add mesh_load() — loads .arm without creating ECS entities"
```

---

### Task 2: Update mesh bridge sync to read from entity component

**Files:**
- Modify: `engine/sources/ecs/mesh_bridge_3d.c`

The current `mesh_bridge_3d_sync_transforms()` iterates a query for entities with pos/rot/scale/mesh_renderer, then maps by index to `scene_meshes[]`. This breaks when entities are created via raw ECS (not in `scene_meshes[]`).

The fix: add `RenderObject3D` to the sync query, then read the Iron mesh object from the entity's `RenderObject3D` component instead of `scene_meshes[]` by index.

- [ ] **Step 1: Update the sync query to include RenderObject3D**

In `mesh_bridge_3d_init()` (around line 32), add `RenderObject3D` as a 5th query term:

Replace:

```c
    ecs_query_desc_t sync_desc = {0};
    sync_desc.terms[0].id = ecs_component_comp_3d_position();
    sync_desc.terms[1].id = ecs_component_comp_3d_rotation();
    sync_desc.terms[2].id = ecs_component_comp_3d_scale();
    sync_desc.terms[3].id = ecs_component_comp_3d_mesh_renderer();
    g_sync_query = ecs_query_init(ecs, &sync_desc);
```

With:

```c
    ecs_query_desc_t sync_desc = {0};
    sync_desc.terms[0].id = ecs_component_comp_3d_position();
    sync_desc.terms[1].id = ecs_component_comp_3d_rotation();
    sync_desc.terms[2].id = ecs_component_comp_3d_scale();
    sync_desc.terms[3].id = ecs_component_comp_3d_mesh_renderer();
    sync_desc.terms[4].id = ecs_component_RenderObject3D();
    g_sync_query = ecs_query_init(ecs, &sync_desc);
```

- [ ] **Step 2: Rewrite sync loop to read from RenderObject3D**

Replace the entire `mesh_bridge_3d_sync_transforms()` function with:

```c
void mesh_bridge_3d_sync_transforms(void) {
    if (!g_mesh_3d_world || !g_sync_query) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_mesh_3d_world);
    if (!ecs) return;

    ecs_iter_t it = ecs_query_iter(ecs, g_sync_query);

    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            // Read RenderObject3D to get the Iron mesh object pointer
            const RenderObject3D *robj = ecs_get_id(ecs, it.entities[i], ecs_component_RenderObject3D());
            if (!robj || !robj->iron_mesh_object) continue;

            object_t *base = (object_t *)robj->iron_mesh_object;
            if (!base->transform) continue;

            // Read fresh component data via ecs_get_id (bypasses stale iterator
            // after Flecs archetype moves triggered by ecs_set_id)
            const comp_3d_position *pos = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_position());
            const comp_3d_rotation *rot = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_rotation());
            const comp_3d_scale *scale = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_scale());
            if (!pos || !rot || !scale) continue;

            transform_t *t = base->transform;
            t->loc.x = pos->x;
            t->loc.y = pos->y;
            t->loc.z = pos->z;
            t->rot.x = rot->x;
            t->rot.y = rot->y;
            t->rot.z = rot->z;
            t->rot.w = rot->w;
            t->scale.x = scale->x;
            t->scale.y = scale->y;
            t->scale.z = scale->z;
            t->dirty = true;
            transform_build_matrix(t);
        }
    }
}
```

- [ ] **Step 3: Commit**

```bash
git add engine/sources/ecs/mesh_bridge_3d.c
git commit -m "fix(mesh_bridge_3d): read Iron mesh object from entity RenderObject3D component"
```

---

### Task 3: Rewrite scene3d_test.minic

**Files:**
- Modify: `engine/assets/systems/scene3d_test.minic`

- [ ] **Step 1: Write the complete rewritten script**

Replace the entire contents of `engine/assets/systems/scene3d_test.minic` with:

```minic
// 3D Test — raw ECS pattern: entity_create + component_lookup + entity_add + comp_set_*

// Component IDs (resolved in init)
id g_pos_comp = 0;
id g_rot_comp = 0;
id g_scale_comp = 0;
id g_cam_comp = 0;
id g_light_comp = 0;
id g_mesh_comp = 0;
id g_render_comp = 0;

// Entity IDs
id g_cam = 0;
id g_mesh = 0;
float g_angle = 0.0;

int init(void) {
    printf("\n=== 3D Test System Init (Raw ECS) ===\n");

    // Look up component IDs
    g_pos_comp = component_lookup("comp_3d_position");
    g_rot_comp = component_lookup("comp_3d_rotation");
    g_scale_comp = component_lookup("comp_3d_scale");
    g_cam_comp = component_lookup("comp_3d_camera");
    g_light_comp = component_lookup("comp_directional_light");
    g_mesh_comp = component_lookup("comp_3d_mesh_renderer");
    g_render_comp = component_lookup("RenderObject3D");

    // --- Load cube.arm scene data (creates Iron objects, not ECS entities) ---
    void *mesh_obj = mesh_load("cube.arm");
    printf("Cube.arm loaded, mesh_obj=%p\n", mesh_obj);

    // --- Create mesh entity ---
    g_mesh = entity_create();
    entity_add(g_mesh, g_pos_comp);
    entity_add(g_mesh, g_rot_comp);
    entity_add(g_mesh, g_scale_comp);
    entity_add(g_mesh, g_mesh_comp);
    entity_add(g_mesh, g_render_comp);
    // Set position
    comp_set_floats(g_pos_comp, entity_get(g_mesh, g_pos_comp), "x,y,z", 0.0, 0.0, 0.0);
    // Identity rotation
    comp_set_floats(g_rot_comp, entity_get(g_mesh, g_rot_comp), "x,y,z,w", 0.0, 0.0, 0.0, 1.0);
    // Unit scale
    comp_set_floats(g_scale_comp, entity_get(g_mesh, g_scale_comp), "x,y,z", 1.0, 1.0, 1.0);
    // Set Iron mesh object pointer in RenderObject3D
    comp_set_ptr(g_render_comp, entity_get(g_mesh, g_render_comp), "iron_mesh_object", mesh_obj);
    comp_set_bool(g_render_comp, entity_get(g_mesh, g_render_comp), "dirty", 1);
    // Mark visible
    comp_set_bool(g_mesh_comp, entity_get(g_mesh, g_mesh_comp), "visible", 1);
    printf("Mesh entity created: %llu\n", g_mesh);

    // --- Create camera entity ---
    g_cam = entity_create();
    entity_add(g_cam, g_pos_comp);
    entity_add(g_cam, g_rot_comp);
    entity_add(g_cam, g_cam_comp);
    // Position at (0, 2, 5)
    comp_set_floats(g_pos_comp, entity_get(g_cam, g_pos_comp), "x,y,z", 0.0, 2.0, 5.0);
    // Identity rotation
    comp_set_floats(g_rot_comp, entity_get(g_cam, g_rot_comp), "x,y,z,w", 0.0, 0.0, 0.0, 1.0);
    // Camera params (fov in radians, ~60 degrees)
    comp_set_floats(g_cam_comp, entity_get(g_cam, g_cam_comp), "fov,near_plane,far_plane", 1.047, 0.1, 100.0);
    comp_set_bool(g_cam_comp, entity_get(g_cam, g_cam_comp), "perspective", 1);
    comp_set_bool(g_cam_comp, entity_get(g_cam, g_cam_comp), "active", 1);
    entity_set_name(g_cam, "main_camera");
    printf("Camera entity created: %llu\n", g_cam);

    // --- Create directional light entity ---
    id light = entity_create();
    entity_add(light, g_light_comp);
    comp_set_floats(g_light_comp, entity_get(light, g_light_comp), "dir_x,dir_y,dir_z", 0.5, -1.0, 0.3);
    comp_set_floats(g_light_comp, entity_get(light, g_light_comp), "color_r,color_g,color_b", 1.0, 0.9, 0.8);
    comp_set_float(g_light_comp, entity_get(light, g_light_comp), "strength", 1.0);
    comp_set_bool(g_light_comp, entity_get(light, g_light_comp), "enabled", 1);
    printf("Light entity created: %llu\n", light);

    printf("=== 3D Test Init Complete ===\n\n");
    return 0;
}

int step(void) {
    if (g_mesh == 0) return 0;

    float dt = sys_delta_time();
    g_angle = g_angle + dt * 0.5;

    // Rotate mesh around Y axis via raw ECS component access
    float half = g_angle * 0.5;
    float rx = 0.0;
    float ry = sin(half);
    float rz = 0.0;
    float rw = cos(half);
    comp_set_floats(g_rot_comp, entity_get(g_mesh, g_rot_comp), "x,y,z,w", rx, ry, rz, rw);

    return 0;
}

int draw(void) {
    draw_set_font("font.ttf", 16);
    draw_set_color(0xffffffff);
    draw_string("3D Scene Test (Raw ECS)", 10.0, 10.0);
    draw_string("WASD=Move  Q/E=Up/Down  RMB=Look  Scroll=Speed", 10.0, 30.0);
    draw_fps(10.0, 50.0);
    return 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add engine/assets/systems/scene3d_test.minic
git commit -m "refactor(scene3d_test): rewrite to raw ECS pattern for camera/mesh/light"
```

---

### Task 4: Build and verify

**Files:** None (build + manual test)

- [ ] **Step 1: Export assets and compile**

```bash
cd engine && ../base/make macos metal
cd build && xcodebuild -project IronGame.xcodeproj -configuration Release build
```

Expected: Build succeeds with no errors. Console output should show:
```
[mesh_load] Loaded 'cube.arm', returning mesh object
Mesh entity created: <id>
Camera entity created: <id>
Light entity created: <id>
```

- [ ] **Step 2: Run and test**

```bash
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame
```

Verify:
1. Cube renders and auto-rotates (mesh bridge sync works from RenderObject3D)
2. Camera shows the scene from (0, 2, 5)
3. Camera controls work (WASD + mouse look via camera3d_control_system)
4. Console shows entity IDs for all three entities
5. No crashes or assertion errors

- [ ] **Step 3: Commit any fixes**

```bash
git add -u
git commit -m "fix: adjustments from build/test verification"
```
