# Multi-Mesh Material Binding Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Enable per-mesh independent material contexts when loading multi-mesh `.arm` files (e.g., `Camera_01_4k.arm` with `Camera_01_body` and `Camera_01_strap`), and provide a `material_bind_texture_by_name(mesh, slot, path)` API.

**Architecture:** Clone `material_data_t` for each mesh after `scene_create()` so they have independent `material_context_t` arrays. Add a new Minic API to bind textures by mesh name. Fix `render3d_bridge.c` helpers to iterate all meshes.

**Tech Stack:** C (engine/sources/core/scene_3d_api.c, engine/sources/ecs/render3d_bridge.c), Minic scripting

---

## File Map

| File | Changes |
|------|---------|
| `engine/sources/core/scene_3d_api.c` | Clone material per mesh in `minic_mesh_load_arm`; Add `minic_material_bind_texture_by_name` |
| `engine/sources/ecs/render3d_bridge.c` | Create `mesh_apply_material_to_all()` helper; Update helper functions to iterate all meshes |
| `engine/assets/systems/scene3d_test.minic` | Update to use `material_bind_texture_by_name` for per-mesh textures |

---

## Task 1: Clone Material for Each Mesh in `minic_mesh_load_arm`

**Files:**
- Modify: `engine/sources/core/scene_3d_api.c:486-580`

- [ ] **Step 1: Add `clone_material_data` helper function before `minic_mesh_load_arm`**

```c
// Deep clone a material_data_t so each mesh gets independent material context.
// Lives at file scope before minic_mesh_load_arm.
static material_data_t* clone_material_data(material_data_t* original) {
    if (!original) return NULL;
    material_data_t* clone = (material_data_t*)gc_alloc(sizeof(material_data_t));
    *clone = *original;  // shallow copy

    // Deep copy bind_constants
    clone->bind_constants = any_array_create_from_raw(NULL, 0);
    if (original->bind_constants) {
        for (int i = 0; i < original->bind_constants->length; i++) {
            bind_const_t* orig_bc = (bind_const_t*)original->bind_constants->buffer[i];
            bind_const_t* new_bc = (bind_const_t*)gc_alloc(sizeof(bind_const_t));
            *new_bc = *orig_bc;
            new_bc->name = orig_bc->name ? strdup(orig_bc->name) : NULL;
            if (orig_bc->vec) {
                new_bc->vec = f32_array_create_from_raw(orig_bc->vec->buffer, orig_bc->vec->length);
            }
            any_array_push(clone->bind_constants, new_bc);
        }
    }

    // Deep copy bind_textures
    clone->bind_textures = any_array_create_from_raw(NULL, 0);
    if (original->bind_textures) {
        for (int i = 0; i < original->bind_textures->length; i++) {
            bind_tex_t* orig_bt = (bind_tex_t*)original->bind_textures->buffer[i];
            bind_tex_t* new_bt = (bind_tex_t*)gc_alloc(sizeof(bind_tex_t));
            *new_bt = *orig_bt;
            new_bt->name = orig_bt->name ? strdup(orig_bt->name) : NULL;
            new_bt->file = (orig_bt->file && orig_bt->file[0] != '\0' && strcmp(orig_bt->file, "_shadow_map") != 0)
                ? strdup(orig_bt->file) : orig_bt->file;
            any_array_push(clone->bind_textures, new_bt);
        }
    }

    // Deep copy contexts — each mesh gets its own material_context_t array
    clone->contexts = any_array_create_from_raw(NULL, 0);
    if (original->contexts) {
        for (int i = 0; i < original->contexts->length; i++) {
            material_context_t* orig_ctx = (material_context_t*)original->contexts->buffer[i];
            material_context_t* new_ctx = (material_context_t*)gc_alloc(sizeof(material_context_t));
            *new_ctx = *orig_ctx;
            // bind_textures and bind_constants in context also need deep copy
            if (orig_ctx->bind_textures) {
                new_ctx->bind_textures = any_array_create_from_raw(NULL, 0);
                for (int j = 0; j < orig_ctx->bind_textures->length; j++) {
                    bind_tex_t* orig_bt = (bind_tex_t*)orig_ctx->bind_textures->buffer[j];
                    bind_tex_t* new_bt = (bind_tex_t*)gc_alloc(sizeof(bind_tex_t));
                    *new_bt = *orig_bt;
                    new_bt->name = orig_bt->name ? strdup(orig_bt->name) : NULL;
                    new_bt->file = (orig_bt->file && orig_bt->file[0] != '\0' && strcmp(orig_bt->file, "_shadow_map") != 0)
                        ? strdup(orig_bt->file) : orig_bt->file;
                    any_array_push(new_ctx->bind_textures, new_bt);
                }
            }
            if (orig_ctx->bind_constants) {
                new_ctx->bind_constants = any_array_create_from_raw(NULL, 0);
                for (int j = 0; j < orig_ctx->bind_constants->length; j++) {
                    bind_const_t* orig_bc = (bind_const_t*)orig_ctx->bind_constants->buffer[j];
                    bind_const_t* new_bc = (bind_const_t*)gc_alloc(sizeof(bind_const_t));
                    *new_bc = *orig_bc;
                    new_bc->name = orig_bc->name ? strdup(orig_bc->name) : NULL;
                    if (orig_bc->vec) {
                        new_bc->vec = f32_array_create_from_raw(orig_bc->vec->buffer, orig_bc->vec->length);
                    }
                    any_array_push(new_ctx->bind_constants, new_bc);
                }
            }
            any_array_push(clone->contexts, new_ctx);
        }
    }

    return clone;
}
```

- [ ] **Step 2: After `scene_create(scene_raw)` in `minic_mesh_load_arm`, add material cloning loop**

Insert after line 511 (`scene_create(scene_raw);`) and before the texture logging block (line 514):

```c
    // Clone material for each mesh so they have independent material contexts
    if (scene_meshes != NULL && scene_meshes->length > 0) {
        // First mesh keeps the original material (already initialized with shader/pipeline)
        mesh_object_t *first_mesh = (mesh_object_t *)scene_meshes->buffer[0];
        material_data_t *first_mat = first_mesh ? first_mesh->material : NULL;

        // All additional meshes get a clone of the first mesh's material
        for (int i = 1; i < scene_meshes->length; i++) {
            mesh_object_t *m = (mesh_object_t *)scene_meshes->buffer[i];
            if (m && m->material == first_mat && first_mat != NULL) {
                m->material = clone_material_data(first_mat);
                printf("[mesh_load] Cloned material for mesh[%d]\n", i);
            }
        }
    }
```

- [ ] **Step 3: Build to verify**

```bash
cd /Users/rayloi/Documents/GitHub/armortools/engine
../base/make macos metal 2>&1 | tail -20
cd build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | tail -30
```

Expected: clean build, no errors

---

## Task 2: Add `minic_material_bind_texture_by_name` API

**Files:**
- Modify: `engine/sources/core/scene_3d_api.c:646-672` (register in `scene_3d_api_register`)
- Modify: `engine/sources/core/scene_3d_api.c` (add implementation before register)

- [ ] **Step 1: Implement `minic_material_bind_texture_by_name` before `scene_3d_api_register`**

```c
// material_bind_texture_by_name(mesh_name, slot_name, file_path) -> int
// Binds a texture to a specific mesh by name.
// Example: material_bind_texture_by_name("Camera_01_body", "tex_albedo", "3d/textures/body_diff.k");
static minic_val_t minic_material_bind_texture_by_name(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_int(0);
    if (args[0].type != MINIC_T_PTR || args[1].type != MINIC_T_PTR || args[2].type != MINIC_T_PTR) {
        return minic_val_int(0);
    }

    const char *mesh_name = (const char *)args[0].p;
    const char *slot_name = (const char *)args[1].p;
    const char *file_path = (const char *)args[2].p;
    if (!mesh_name || !slot_name || !file_path) return minic_val_int(0);

    if (!scene_meshes || scene_meshes->length == 0) return minic_val_int(0);

    // Find mesh by name
    mesh_object_t *target_mesh = NULL;
    int target_idx = -1;
    for (int i = 0; i < scene_meshes->length; i++) {
        mesh_object_t *m = (mesh_object_t *)scene_meshes->buffer[i];
        if (m && m->base && m->base->name && strcmp(m->base->name, mesh_name) == 0) {
            target_mesh = m;
            target_idx = i;
            break;
        }
    }
    if (!target_mesh || !target_mesh->material) {
        printf("[material_bind_texture_by_name] Mesh '%s' not found\n", mesh_name);
        return minic_val_int(0);
    }

    material_context_t *ctx = material_data_get_context(target_mesh->material, "mesh");
    if (!ctx || !ctx->bind_textures) return minic_val_int(0);

    // Find texture slot index
    int tex_idx = -1;
    for (int i = 0; i < ctx->bind_textures->length; i++) {
        bind_tex_t *tex = (bind_tex_t *)ctx->bind_textures->buffer[i];
        if (tex->name && strcmp(tex->name, slot_name) == 0) {
            tex_idx = i;
            break;
        }
    }
    if (tex_idx < 0) {
        printf("[material_bind_texture_by_name] Slot '%s' not found\n", slot_name);
        return minic_val_int(0);
    }

    // Load texture
    gpu_texture_t *tex = data_get_image(file_path);
    if (!tex) {
        printf("[material_bind_texture_by_name] Failed to load '%s'\n", file_path);
        return minic_val_int(0);
    }

    // Update bind_tex file path
    bind_tex_t *bind_tex = (bind_tex_t *)ctx->bind_textures->buffer[tex_idx];
    if (bind_tex->file && bind_tex->file[0] != '\0' && strcmp(bind_tex->file, "_shadow_map") != 0) {
        // Overwrite (may be static string from material template, don't free)
    }
    bind_tex->file = strdup(file_path);

    // Set use_*_tex flag to 1.0
    const char *use_flag_name = NULL;
    if (strcmp(slot_name, "tex_albedo") == 0) use_flag_name = "use_albedo_tex";
    else if (strcmp(slot_name, "tex_metallic") == 0) use_flag_name = "use_metallic_tex";
    else if (strcmp(slot_name, "tex_roughness") == 0) use_flag_name = "use_roughness_tex";

    if (use_flag_name && ctx->bind_constants) {
        for (int i = 0; i < ctx->bind_constants->length; i++) {
            bind_const_t *bc = (bind_const_t *)ctx->bind_constants->buffer[i];
            if (bc->name && strcmp(bc->name, use_flag_name) == 0 && bc->vec && bc->vec->length > 0) {
                bc->vec->buffer[0] = 1.0f;
                break;
            }
        }
    }

    printf("[material_bind_texture_by_name] Bound '%s' to '%s' slot '%s'\n", file_path, mesh_name, slot_name);
    return minic_val_int(1);
}
```

- [ ] **Step 2: Register the new API in `scene_3d_api_register`**

Add after line 668 (`material_bind_texture` registration):

```c
    minic_register_native("material_bind_texture_by_name", minic_material_bind_texture_by_name);
```

- [ ] **Step 3: Build to verify**

```bash
cd /Users/rayloi/Documents/GitHub/armortools/engine
../base/make macos metal 2>&1 | tail -5
cd build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | grep -E "(error|warning.*scene_3d)" | head -20
```

Expected: clean build

---

## Task 3: Fix `render3d_bridge.c` Helper Functions to Iterate All Meshes

**Files:**
- Modify: `engine/sources/ecs/render3d_bridge.c:27-157`

- [ ] **Step 1: Create `mesh_apply_material_to_all` helper**

Add after line 26 (`g_stats` declaration), before `compute_light_vp`:

```c
// Helper: apply a material bind constant by name to ALL meshes.
// This ensures material constants (light_dir, cam_pos, shadow_vp, etc.) are
// propagated to every mesh's material context, not just the first.
static void mesh_apply_material_to_all(const char *name, float value) {
    if (!scene_meshes || scene_meshes->length == 0) return;
    for (int i = 0; i < scene_meshes->length; i++) {
        mesh_object_t *m = (mesh_object_t *)scene_meshes->buffer[i];
        if (!m || !m->material) continue;
        material_context_t *mc = material_data_get_context(m->material, "mesh");
        if (!mc || !mc->bind_constants) continue;
        for (int j = 0; j < mc->bind_constants->length; j++) {
            bind_const_t *bc = (bind_const_t *)mc->bind_constants->buffer[j];
            if (bc->name && bc->vec && strcmp(bc->name, name) == 0) {
                bc->vec->buffer[0] = value;
                break;
            }
        }
    }
}
```

- [ ] **Step 2: Update `compute_light_vp` to read from first mesh, write to all meshes**

Replace the content of `compute_light_vp` (lines 27-78) with:

```c
static void compute_light_vp(void) {
    // Read light direction from material constants of the first mesh
    float ldx = -0.5f, ldy = -0.7f, ldz = -0.5f;
    if (scene_meshes != NULL && scene_meshes->length > 0) {
        mesh_object_t *m = (mesh_object_t *)scene_meshes->buffer[0];
        if (m && m->material) {
            material_context_t *mc = material_data_get_context(m->material, "mesh");
            if (mc && mc->bind_constants) {
                for (int i = 0; i < mc->bind_constants->length; i++) {
                    bind_const_t *bc = (bind_const_t *)mc->bind_constants->buffer[i];
                    if (bc->name && bc->vec) {
                        if (strcmp(bc->name, "light_dir") == 0 && bc->vec->length >= 3) {
                            ldx = bc->vec->buffer[0];
                            ldy = bc->vec->buffer[1];
                            ldz = bc->vec->buffer[2];
                        }
                    }
                }
            }
        }
    }

    // Normalize light direction
    float len = sqrtf(ldx * ldx + ldy * ldy + ldz * ldz);
    if (len > 0.0001f) { ldx /= len; ldy /= len; ldz /= len; }

    // Build light view matrix using Iron's convention: view = inv(world)
    vec4_t eye = vec4_create(-ldx * 5.0f, -ldy * 5.0f, -ldz * 5.0f, 1.0f);
    vec4_t target = vec4_create(0.0f, 0.0f, 0.0f, 1.0f);
    vec4_t up = vec4_create(0.0f, 1.0f, 0.0f, 0.0f);

    vec4_t fwd = vec4_norm(vec4_sub(target, eye));
    vec4_t right = vec4_norm(vec4_cross(fwd, up));
    vec4_t real_up = vec4_cross(right, fwd);

    mat4_t light_world = mat4_identity();
    light_world.m00 = right.x; light_world.m01 = right.y; light_world.m02 = right.z;
    light_world.m10 = real_up.x; light_world.m11 = real_up.y; light_world.m12 = real_up.z;
    light_world.m20 = -fwd.x; light_world.m21 = -fwd.y; light_world.m22 = -fwd.z;
    light_world.m30 = eye.x; light_world.m31 = eye.y; light_world.m32 = eye.z;

    mat4_t view = mat4_inv(light_world);
    float ortho_size = 3.0f;
    mat4_t proj = mat4_ortho(-ortho_size, ortho_size, -ortho_size, ortho_size, 0.1f, 15.0f);
    g_light_vp = mat4_mult_mat(view, proj);

    // Write light_vp matrix rows to ALL meshes' material constants
    if (!scene_meshes || scene_meshes->length == 0) return;
    for (int i = 0; i < scene_meshes->length; i++) {
        mesh_object_t *m = (mesh_object_t *)scene_meshes->buffer[i];
        if (!m || !m->material) continue;
        material_context_t *mc = material_data_get_context(m->material, "mesh");
        if (!mc || !mc->bind_constants) continue;
        for (int j = 0; j < mc->bind_constants->length; j++) {
            bind_const_t *bc = (bind_const_t *)mc->bind_constants->buffer[j];
            if (!bc->name || !bc->vec) continue;
            if (strcmp(bc->name, "light_vp_r0") == 0 && bc->vec->length >= 4) {
                bc->vec->buffer[0] = g_light_vp.m00; bc->vec->buffer[1] = g_light_vp.m10;
                bc->vec->buffer[2] = g_light_vp.m20; bc->vec->buffer[3] = g_light_vp.m30;
            }
            else if (strcmp(bc->name, "light_vp_r1") == 0 && bc->vec->length >= 4) {
                bc->vec->buffer[0] = g_light_vp.m01; bc->vec->buffer[1] = g_light_vp.m11;
                bc->vec->buffer[2] = g_light_vp.m21; bc->vec->buffer[3] = g_light_vp.m31;
            }
            else if (strcmp(bc->name, "light_vp_r2") == 0 && bc->vec->length >= 4) {
                bc->vec->buffer[0] = g_light_vp.m02; bc->vec->buffer[1] = g_light_vp.m12;
                bc->vec->buffer[2] = g_light_vp.m22; bc->vec->buffer[3] = g_light_vp.m32;
            }
            else if (strcmp(bc->name, "light_vp_r3") == 0 && bc->vec->length >= 4) {
                bc->vec->buffer[0] = g_light_vp.m03; bc->vec->buffer[1] = g_light_vp.m13;
                bc->vec->buffer[2] = g_light_vp.m23; bc->vec->buffer[3] = g_light_vp.m33;
            }
        }
    }
}
```

- [ ] **Step 3: Update `update_cam_pos_material` to iterate all meshes**

Replace lines 81-114 (`update_cam_pos_material`):

```c
static void update_cam_pos_material(void) {
    if (!scene_camera || !scene_camera->base || !scene_camera->base->transform) return;
    if (!scene_meshes || scene_meshes->length == 0) return;

    vec4_t cam_loc = scene_camera->base->transform->loc;

    // Iterate all meshes and update their material constants
    for (int mesh_idx = 0; mesh_idx < scene_meshes->length; mesh_idx++) {
        mesh_object_t *mesh_obj = (mesh_object_t *)scene_meshes->buffer[mesh_idx];
        if (!mesh_obj || !mesh_obj->material) continue;

        material_context_t *mctx = material_data_get_context(mesh_obj->material, "mesh");
        if (!mctx || !mctx->bind_constants) continue;

        for (int i = 0; i < mctx->bind_constants->length; i++) {
            bind_const_t *bc = (bind_const_t *)mctx->bind_constants->buffer[i];
            if (!bc->name || !bc->vec) continue;
            if (strcmp(bc->name, "cam_pos") == 0 && bc->vec->length >= 3) {
                bc->vec->buffer[0] = cam_loc.x;
                bc->vec->buffer[1] = cam_loc.y;
                bc->vec->buffer[2] = cam_loc.z;
            }
            else if (strcmp(bc->name, "point_light_pos0") == 0 && bc->vec->length >= 3) {
                bc->vec->buffer[0] = 2.0f;
                bc->vec->buffer[1] = 2.0f;
                bc->vec->buffer[2] = 0.0f;
            }
            else if (strcmp(bc->name, "point_light_strength0") == 0) {
                bc->vec->buffer[0] = 5.0f;
            }
            else if (strcmp(bc->name, "num_point_lights") == 0) {
                bc->vec->buffer[0] = 1.0f;
            }
        }
    }
}
```

- [ ] **Step 4: Update `set_material_const` to iterate all meshes**

Replace lines 144-157 (`set_material_const`):

```c
static void set_material_const(const char *name, float value) {
    if (!scene_meshes || scene_meshes->length == 0) return;
    for (int i = 0; i < scene_meshes->length; i++) {
        mesh_object_t *m = (mesh_object_t *)scene_meshes->buffer[i];
        if (!m || !m->material) continue;
        material_context_t *mc = material_data_get_context(m->material, "mesh");
        if (!mc || !mc->bind_constants) continue;
        for (int j = 0; j < mc->bind_constants->length; j++) {
            bind_const_t *bc = (bind_const_t *)mc->bind_constants->buffer[j];
            if (bc->name && bc->vec && strcmp(bc->name, name) == 0) {
                bc->vec->buffer[0] = value;
                break;
            }
        }
    }
}
```

- [ ] **Step 5: Update shadow map binding in G-buffer pass (line 227-235)**

The current code binds shadow map to only `buffer[0]`. Replace lines 227-235 with:

```c
    // Bind shadow map texture to ALL meshes' material
    for (int i = 0; i < scene_meshes->length; i++) {
        mesh_object_t *m = (mesh_object_t *)scene_meshes->buffer[i];
        if (m && m->material && m->material->_) {
            material_context_t *mc = material_data_get_context(m->material, "mesh");
            if (mc && mc->_ && mc->_->textures && mc->_->textures->length > 0) {
                mc->_->textures->buffer[0] = sd->shadow_map;
            }
        }
    }
```

- [ ] **Step 6: Build to verify**

```bash
cd /Users/rayloi/Documents/GitHub/armortools/engine
../base/make macos metal 2>&1 | tail -5
cd build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | grep -E "(error|warning.*render3d)" | head -20
```

Expected: clean build

---

## Task 4: Update Test Script and Verify

**Files:**
- Modify: `engine/assets/systems/scene3d_test.minic:29-36`

- [ ] **Step 1: Update `scene3d_test.minic` to use per-mesh texture binding**

Replace lines 29-36 with:

```minic
    // --- Load Camera_01_4k.arm scene data (may have multiple meshes) ---
    void *mesh_obj = mesh_load_arm("Camera_01_4k.arm");
    printf("Camera_01_4k.arm loaded, mesh_obj=%p\n", mesh_obj);

    // Bind PBR textures for Camera body mesh
    material_bind_texture_by_name("Camera_01_body", "tex_albedo", "3d/textures/Camera_01_body_diff_4k.k");
    material_bind_texture_by_name("Camera_01_body", "tex_metallic", "3d/textures/Camera_01_body_metallic_4k.k");
    material_bind_texture_by_name("Camera_01_body", "tex_roughness", "3d/textures/Camera_01_body_roughness_4k.k");

    // Bind PBR textures for Camera strap mesh (if present in the .arm file)
    material_bind_texture_by_name("Camera_01_strap", "tex_albedo", "3d/textures/Camera_01_strap_diff_4k.k");
    material_bind_texture_by_name("Camera_01_strap", "tex_metallic", "3d/textures/Camera_01_strap_metallic_4k.k");
    material_bind_texture_by_name("Camera_01_strap", "tex_roughness", "3d/textures/Camera_01_strap_roughness_4k.k");
```

Note: If strap textures don't exist yet, the binding will fail gracefully (returns 0 but continues). Use 1x1 placeholder textures if actual textures are not available.

- [ ] **Step 2: Export assets**

```bash
cd /Users/rayloi/Documents/GitHub/armortools/engine && ../base/make macos metal 2>&1 | tail -10
```

- [ ] **Step 3: Build**

```bash
cd /Users/rayloi/Documents/GitHub/armortools/engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | tail -5
```

Expected: clean build

---

## Verification Checklist

- [ ] Build succeeds without errors
- [ ] `Camera_01_4k.arm` loads with multiple meshes (check log for "Cloned material for mesh[1]")
- [ ] Each mesh has independent material context
- [ ] `material_bind_texture_by_name` finds meshes by name
- [ ] Material constants (cam_pos, light_dir, shadow_vp) propagate to all meshes
- [ ] Both meshes render with correct individual textures (visual verification)
