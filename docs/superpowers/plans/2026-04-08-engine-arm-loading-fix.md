# Engine .arm Loading Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix `mesh_create("cube.arm")` crash and Metal depth bug by routing `.arm` files through scene-based loading with ECS-first data flow.

**Architecture:** Smart `mesh_create` detects `.arm` extension → calls `asset_loader_load_scene` which auto-generates missing camera/material/shader/world data → `scene_create` builds Iron scene graph → ECS entities created pointing to Iron objects → bridges sync ECS→Iron each frame. Render3D bridge switches from `render_path_set_target` to `_gpu_begin` to avoid Metal depth bug.

**Tech Stack:** C (engine layer), Iron engine APIs (base/), Flecs ECS, Minic scripting

---

### Task 1: Fix render3d_bridge Metal depth bug

**Files:**
- Modify: `engine/sources/ecs/render3d_bridge.c`

This is the simplest, most isolated change — fixes rendering regardless of .arm loading.

- [ ] **Step 1: Replace render_path_set_target with _gpu_begin + render_path_submit_draw**

In `engine/sources/ecs/render3d_bridge.c`, replace the entire `sys_3d_render_commands` function:

```c
static void sys_3d_render_commands(void) {
    // Use _gpu_begin directly — render_path_set_target calls gpu_viewport
    // with znear=0.1/zfar=100.0 which breaks Metal's depth range
    _gpu_begin(NULL, NULL, NULL, GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH, 0xff1a1a2e, 1.0f);
    render_path_submit_draw("mesh");
    gpu_end();

    g_3d_rendered = true;
}
```

- [ ] **Step 2: Initialize render_path_current_w/h in sys_3d_init**

In `sys_3d_init()`, after setting `render_path_commands`, add viewport size initialization to prevent division-by-zero in `camera_object_proj_jitter()` on the first frame:

```c
void sys_3d_init(void) {
    render_path_commands = sys_3d_render_commands;
    render_path_current_w = sys_w();
    render_path_current_h = sys_h();
    printf("3D Render Bridge: initialized (forward rendering)\n");
}
```

- [ ] **Step 3: Commit**

```bash
git add engine/sources/ecs/render3d_bridge.c
git commit -m "fix(render3d): use _gpu_begin instead of render_path_set_target to fix Metal depth bug"
```

---

### Task 2: Add smart .arm routing to mesh_create

**Files:**
- Modify: `engine/sources/core/scene_3d_api.c`

- [ ] **Step 1: Add ends_with helper and .arm detection to minic_mesh_create**

In `engine/sources/core/scene_3d_api.c`, add a helper function before `minic_mesh_create`:

```c
// Check if string ends with suffix
static bool path_ends_with(const char *s, const char *suffix) {
    if (!s || !suffix) return false;
    size_t slen = strlen(s);
    size_t suflen = strlen(suffix);
    if (suflen > slen) return false;
    return strcmp(s + slen - suflen, suffix) == 0;
}
```

Then replace the `minic_mesh_create` function body. When `.arm` is detected, route to `asset_loader_load_scene`:

```c
// mesh_create(mesh_path, material_path) -> entity
static minic_val_t minic_mesh_create(minic_val_t *args, int argc) {
    if (!g_runtime_world) return minic_val_id(0);

    const char *mesh_path = (argc > 0 && args[0].type == MINIC_T_PTR) ? (const char *)args[0].p : NULL;
    const char *mat_path = (argc > 1 && args[1].type == MINIC_T_PTR) ? (const char *)args[1].p : NULL;

    // .arm files are scene files — route to scene-based loader
    if (mesh_path && path_ends_with(mesh_path, ".arm")) {
        uint64_t entity = asset_loader_load_scene(mesh_path);
        return minic_val_id(entity);
    }

    uint64_t entity = entity_create(g_runtime_world);
    if (entity == 0) return minic_val_id(0);

    // Add transform components
    uint64_t pos_id = ecs_component_comp_3d_position();
    uint64_t rot_id = ecs_component_comp_3d_rotation();
    uint64_t scale_id = ecs_component_comp_3d_scale();
    uint64_t mr_id = ecs_component_comp_3d_mesh_renderer();

    entity_add_component(g_runtime_world, entity, pos_id);
    entity_add_component(g_runtime_world, entity, rot_id);
    entity_add_component(g_runtime_world, entity, scale_id);
    entity_add_component(g_runtime_world, entity, mr_id);

    // Default transform
    comp_3d_position pos = {0.0f, 0.0f, 0.0f};
    comp_3d_rotation rot = {0.0f, 0.0f, 0.0f, 1.0f};
    comp_3d_scale scale = {1.0f, 1.0f, 1.0f};

    entity_set_component_data(g_runtime_world, entity, pos_id, &pos);
    entity_set_component_data(g_runtime_world, entity, rot_id, &rot);
    entity_set_component_data(g_runtime_world, entity, scale_id, &scale);

    // Load mesh if path provided
    if (mesh_path) {
        asset_loader_load_mesh(entity, mesh_path, mat_path);
    }

    return minic_val_id(entity);
}
```

Note: The `.arm` path returns the first entity from `asset_loader_load_scene`. The non-`.arm` path is unchanged.

- [ ] **Step 2: Commit**

```bash
git add engine/sources/core/scene_3d_api.c
git commit -m "feat(scene_3d_api): route .arm files to scene-based loader in mesh_create"
```

---

### Task 3: Enhance asset_loader_load_scene with auto-generated scene data

**Files:**
- Modify: `engine/sources/core/asset_loader.c`

This is the core change — when `.arm` files are missing camera/material/shader/world data, auto-generate defaults before calling `scene_create`.

- [ ] **Step 1: Add auto-generation helper function**

Add this static helper before `asset_loader_load_scene` in `engine/sources/core/asset_loader.c`:

```c
// Ensure scene_t has required data for scene_create().
// .arm files from ArmorPaint often lack camera, material, shader, and world data.
static void scene_ensure_defaults(scene_t *scene) {
    // Add camera object if scene has no camera_ref
    bool has_camera = false;
    if (scene->objects != NULL) {
        for (int i = 0; i < scene->objects->length; i++) {
            obj_t *o = (obj_t *)scene->objects->buffer[i];
            if (o->type && strcmp(o->type, "camera_object") == 0) {
                has_camera = true;
                break;
            }
        }
    }
    if (!has_camera && scene->objects != NULL) {
        any_array_push(scene->objects,
            GC_ALLOC_INIT(obj_t, {
                .name = "Camera",
                .type = "camera_object",
                .data_ref = "DefaultCamera",
                .visible = true,
                .spawn = true
            }));
    }

    // Set material_ref on mesh objects that lack it
    if (scene->objects != NULL) {
        for (int i = 0; i < scene->objects->length; i++) {
            obj_t *o = (obj_t *)scene->objects->buffer[i];
            if (o->type && strcmp(o->type, "mesh_object") == 0 && o->material_ref == NULL) {
                o->material_ref = "DefaultMaterial";
            }
        }
    }

    // Camera data
    if (scene->camera_datas == NULL || scene->camera_datas->length == 0) {
        scene->camera_datas = any_array_create_from_raw(
            (void *[]){
                GC_ALLOC_INIT(camera_data_t, {
                    .name = "DefaultCamera",
                    .near_plane = 0.01,
                    .far_plane = 100.0,
                    .fov = 0.85
                }),
            },
            1);
    }
    if (scene->camera_ref == NULL) {
        scene->camera_ref = "Camera";
    }

    // World data
    if (scene->world_datas == NULL || scene->world_datas->length == 0) {
        scene->world_datas = any_array_create_from_raw(
            (void *[]){
                GC_ALLOC_INIT(world_data_t, {.name = "DefaultWorld", .color = 0xff1a1a2e}),
            },
            1);
    }
    if (scene->world_ref == NULL) {
        scene->world_ref = "DefaultWorld";
    }

    // Material data — single material with "mesh" context
    if (scene->material_datas == NULL || scene->material_datas->length == 0) {
        scene->material_datas = any_array_create_from_raw(
            (void *[]){
                GC_ALLOC_INIT(material_data_t,
                    {.name     = "DefaultMaterial",
                     .shader   = "DefaultShader",
                     .contexts = any_array_create_from_raw(
                         (void *[]){
                             GC_ALLOC_INIT(material_context_t,
                                 {.name          = "mesh",
                                  .bind_textures = any_array_create_from_raw(
                                      (void *[]){
                                          GC_ALLOC_INIT(bind_tex_t, {.name = "my_texture", .file = "texture.k"}),
                                      },
                                      1)}),
                         },
                         1)}),
            },
            1);
    }

    // Shader data — forward rendering pipeline
    if (scene->shader_datas == NULL || scene->shader_datas->length == 0) {
        scene->shader_datas = any_array_create_from_raw(
            (void *[]){
                GC_ALLOC_INIT(shader_data_t,
                    {.name     = "DefaultShader",
                     .contexts = any_array_create_from_raw(
                         (void *[]){
                             GC_ALLOC_INIT(shader_context_t,
                                 {.name            = "mesh",
                                  .vertex_shader   = "mesh.vert",
                                  .fragment_shader = "mesh.frag",
                                  .compare_mode    = "less",
                                  .cull_mode       = "none",
                                  .depth_write     = true,
                                  .vertex_elements = any_array_create_from_raw(
                                      (void *[]){
                                          GC_ALLOC_INIT(vertex_element_t, {.name = "pos", .data = "short4norm"}),
                                          GC_ALLOC_INIT(vertex_element_t, {.name = "tex", .data = "short2norm"}),
                                      },
                                      2),
                                  .constants = any_array_create_from_raw(
                                      (void *[]){
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "WVP", .type = "mat4", .link = "_world_view_proj_matrix"}),
                                      },
                                      1),
                                  .texture_units = any_array_create_from_raw(
                                      (void *[]){
                                          GC_ALLOC_INIT(tex_unit_t, {.name = "my_texture"}),
                                      },
                                      1),
                                  .depth_attachment = "D32"}),
                         },
                         1)}),
            },
            1);
    }
}
```

- [ ] **Step 2: Call scene_ensure_defaults in asset_loader_load_scene**

Replace the existing `asset_loader_load_scene` function. The key changes are:
1. Call `scene_ensure_defaults(scene_raw)` before `scene_create`
2. Always call `scene_create` (remove the `_scene_root` already-exists branch — if scene is already created, call `scene_remove` first then `scene_create`)
3. Read ECS transforms from Iron `transform_t` (after `scene_create` has set them up from .arm mat4 data)

```c
uint64_t asset_loader_load_scene(const char *path) {
    if (!g_asset_loader_world || !path) return 0;

    // Parse .arm file
    scene_t *scene_raw = data_get_scene_raw(path);
    if (!scene_raw) {
        fprintf(stderr, "Asset Loader: failed to load scene '%s'\n", path);
        return 0;
    }

    // Auto-generate missing camera/material/shader/world data
    scene_ensure_defaults(scene_raw);

    // Remove existing scene if present, then create fresh
    if (_scene_root != NULL) {
        scene_remove();
    }
    scene_create(scene_raw);

    if (!_scene_root) {
        fprintf(stderr, "Asset Loader: scene_create failed for '%s'\n", path);
        return 0;
    }

    // Initialize viewport dimensions to prevent division-by-zero
    render_path_current_w = sys_w();
    render_path_current_h = sys_h();

    // Position the camera behind origin
    if (scene_camera != NULL && scene_camera->base != NULL) {
        transform_t *t = scene_camera->base->transform;
        t->loc = vec4_create(0, 2, 5, 1);
        t->rot = quat_create(0, 0, 0, 1);
        transform_build_matrix(t);
        camera_object_build_proj(scene_camera, (f32)sys_w() / (f32)sys_h());
        camera_object_build_mat(scene_camera);
    }

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_asset_loader_world);
    if (!ecs) return 0;

    uint64_t first_entity = 0;

    // Sync mesh objects to ECS entities — read from Iron transform_t
    // (which was initialized from .arm mat4 data by scene_create)
    if (scene_meshes != NULL) {
        for (int i = 0; i < scene_meshes->length; i++) {
            mesh_object_t *mesh_obj = (mesh_object_t *)scene_meshes->buffer[i];
            if (!mesh_obj || !mesh_obj->base) continue;

            ecs_entity_t e = ecs_new(ecs);
            if (first_entity == 0) first_entity = (uint64_t)e;

            transform_t *t = mesh_obj->base->transform;

            comp_3d_position pos = { t->loc.x, t->loc.y, t->loc.z };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_position(), sizeof(comp_3d_position), &pos);

            comp_3d_rotation rot = { t->rot.x, t->rot.y, t->rot.z, t->rot.w };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_rotation(), sizeof(comp_3d_rotation), &rot);

            comp_3d_scale scl = { t->scale.x, t->scale.y, t->scale.z };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_scale(), sizeof(comp_3d_scale), &scl);

            comp_3d_mesh_renderer mr = { .visible = true };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_mesh_renderer(), sizeof(comp_3d_mesh_renderer), &mr);

            RenderObject3D robj = {
                .iron_mesh_object = mesh_obj->base,
                .iron_transform = mesh_obj->base->transform,
                .dirty = true
            };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_RenderObject3D(), sizeof(RenderObject3D), &robj);
        }
    }

    // Sync cameras to ECS entities
    if (scene_cameras != NULL) {
        bool first_cam = true;
        for (int i = 0; i < scene_cameras->length; i++) {
            camera_object_t *cam_obj = (camera_object_t *)scene_cameras->buffer[i];
            if (!cam_obj || !cam_obj->base) continue;

            ecs_entity_t e = ecs_new(ecs);
            if (first_entity == 0) first_entity = (uint64_t)e;

            transform_t *t = cam_obj->base->transform;

            comp_3d_position pos = { t->loc.x, t->loc.y, t->loc.z };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_position(), sizeof(comp_3d_position), &pos);

            comp_3d_rotation rot = { t->rot.x, t->rot.y, t->rot.z, t->rot.w };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_rotation(), sizeof(comp_3d_rotation), &rot);

            comp_3d_scale scl = { t->scale.x, t->scale.y, t->scale.z };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_scale(), sizeof(comp_3d_scale), &scl);

            comp_3d_camera cam = {
                .fov = cam_obj->data->fov,
                .near_plane = cam_obj->data->near_plane,
                .far_plane = cam_obj->data->far_plane,
                .perspective = true,
                .active = first_cam
            };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_camera(), sizeof(comp_3d_camera), &cam);

            first_cam = false;
        }
    }

    printf("Asset Loader: scene '%s' loaded (%d meshes, %d cameras)\n",
        path,
        scene_meshes ? scene_meshes->length : 0,
        scene_cameras ? scene_cameras->length : 0);

    return first_entity;
}
```

- [ ] **Step 3: Commit**

```bash
git add engine/sources/core/asset_loader.c
git commit -m "feat(asset_loader): auto-generate missing scene data for .arm files"
```

---

### Task 4: Update minic test script

**Files:**
- Modify: `engine/assets/systems/scene3d_test.minic`

The test script already calls `mesh_create("cube.arm")` — it will now work with the smart routing. But we should remove the camera setup since the scene auto-generates a camera, and add a rotation to verify the ECS→Iron bridge sync works.

- [ ] **Step 1: Update scene3d_test.minic**

Replace the file content:

```minic
// 3D Test — exercises .arm scene loading via mesh_create
// mesh_create("cube.arm") triggers scene-based loading with auto-generated
// camera, material, shader, and world data.

id g_cam = 0;
float g_angle = 0.0;

int init(void) {
    printf("\n=== 3D Test System Init ===\n");

    // Load cube.arm scene — auto-creates camera, materials, shaders
    id mesh_ent = mesh_create("cube.arm");
    printf("Mesh entity created from cube.arm\n");

    // Create a separate 3D camera for orbit control
    g_cam = camera_3d_create(60.0, 0.1, 100.0);
    camera_3d_set_position(g_cam, 0.0, 2.0, 5.0);
    camera_3d_look_at(g_cam, 0.0, 0.0, 0.0);
    printf("Orbit camera created\n");

    // Add a directional light
    id light = light_directional(0.5, -1.0, 0.3, 1.0, 0.9, 0.8, 1.0);
    printf("Directional light created\n");

    printf("=== 3D Test Init Complete ===\n\n");
    return 0;
}

int step(void) {
    if (g_cam == 0) return 0;

    // Orbit camera around origin
    g_angle = g_angle + sys_delta_time() * 0.5;
    float dist = 5.0;
    float cx = sin(g_angle) * dist;
    float cz = cos(g_angle) * dist;
    camera_3d_set_position(g_cam, cx, 2.0, cz);
    camera_3d_look_at(g_cam, 0.0, 0.0, 0.0);

    return 0;
}

int draw(void) {
    draw_set_font("font.ttf", 16);
    draw_set_color(0xffffffff);
    draw_string("3D Scene Test", 10.0, 10.0);
    draw_string("Camera orbiting", 10.0, 30.0);
    draw_fps(10.0, 50.0);
    return 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add engine/assets/systems/scene3d_test.minic
git commit -m "chore(scene3d_test): update test to use .arm scene loading"
```

---

### Task 5: Build and verify

**Files:** None (build + run)

- [ ] **Step 1: Export assets and shaders**

```bash
cd engine && ../base/make macos metal
```

Expected: Completes without errors. `cube.arm` and `texture.png` get copied to `build/out/data/`.

- [ ] **Step 2: Compile C code**

```bash
cd engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build
```

Expected: Build succeeds with no errors. May have warnings from GC_ALLOC_INIT macros.

- [ ] **Step 3: Run and verify**

```bash
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame
```

Expected:
- Window opens with a textured cube visible
- Console prints "Asset Loader: scene 'cube.arm' loaded (1 meshes, 1 cameras)"
- No crash
- 3D scene renders with correct depth (no Metal depth bug)

- [ ] **Step 4: Final commit if any build fixes needed**

```bash
git add -u
git commit -m "fix: build fixes for engine .arm loading"
```

---

## Self-Review Checklist

**Spec coverage:**
- [x] Smart mesh_create .arm routing → Task 2
- [x] Auto-generate missing scene data → Task 3
- [x] ECS-first transform flow → Task 3 (reads from Iron transform_t after scene_create)
- [x] Metal depth bug fix → Task 1
- [x] Test assets → already in engine/assets/3d/
- [x] Minic test script update → Task 4

**Placeholder scan:** No TBDs, TODOs, or "implement later". All code is complete.

**Type consistency:**
- `asset_loader_load_scene` returns `uint64_t` → matches `minic_val_id()` in mesh_create
- `comp_3d_position`, `comp_3d_rotation`, `comp_3d_scale` types match ecs_components.h
- `RenderObject3D` fields match ecs_components.h (void* for iron_mesh_object/iron_transform)
- All `ecs_set_id` calls use correct component ID functions from ecs_components.h
