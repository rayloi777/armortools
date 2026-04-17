# PBR Texture Support Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add texture sampling support to mesh_gbuffer.kong for albedo/metallic/roughness maps, with fallback to existing constants, and Minic API for dynamic texture binding.

**Architecture:** Extend the existing shader constant-based material system with texture samplers. New texture slots (tex_albedo, tex_metallic, tex_roughness) are added to the material context alongside existing bind_constants. The shader conditionally samples textures when use_*_tex flags are set.

**Tech Stack:** Kong shader language, C (asset_loader.c, scene_3d_api.c, runtime_api.c), Minic scripting

---

## File Overview

| File | Responsibility |
|------|----------------|
| `engine/assets/shaders/mesh_gbuffer.kong` | Add 3 texture samplers + 3 use flags + sampling logic |
| `engine/sources/core/asset_loader.c` | Add 3 default texture slots + 3 use_* constants to default material |
| `engine/sources/core/scene_3d_api.c` | Set use_* flags based on loaded texture validity |
| `engine/sources/core/runtime_api.c` | Register new Minic APIs |

---

## Task 1: Update mesh_gbuffer.kong — Add texture samplers and sampling logic

**File:** `engine/assets/shaders/mesh_gbuffer.kong`

- [ ] **Step 1: Add sampler declarations** (after existing `const _shadow_map: tex2d;`)

```kong
#[set(everything)]
const tex_albedo: tex2d;

#[set(everything)]
const tex_metallic: tex2d;

#[set(everything)]
const tex_roughness: tex2d;
```

- [ ] **Step 2: Add use_*_tex flags to constants block** (after `opacity: float;`)

```kong
use_albedo_tex: float;    // 1.0 = use texture, 0.0 = use albedo_rgb
use_metallic_tex: float;  // 1.0 = use texture, 0.0 = use metallic constant
use_roughness_tex: float; // 1.0 = use texture, 0.0 = use roughness constant
```

- [ ] **Step 3: Update material sampling logic in mesh_gbuffer_frag()** (replace existing constant reads for albedo, metallic, roughness)

Find this existing code:
```kong
var albedo: float3 = constants.albedo_rgb;
var rough: float = max(constants.roughness, 0.04);
var metal: float = constants.metallic;
```

Replace with:
```kong
// Albedo: texture with sRGB→Linear conversion, or constant fallback
var albedo_tex: float3 = sample(tex_albedo, sampler_linear, input.tex).rgb;
albedo_tex = pow(albedo_tex, float3(2.2));  // gamma decode sRGB→Linear
var albedo: float3 = (constants.use_albedo_tex > 0.5) ? albedo_tex : constants.albedo_rgb;

// Metallic: texture sample (R channel) or constant fallback
var metal: float = (constants.use_metallic_tex > 0.5)
    ? sample(tex_metallic, sampler_linear, input.tex).r
    : constants.metallic;

// Roughness: texture sample (R channel) or constant fallback
var rough: float = (constants.use_roughness_tex > 0.5)
    ? sample(tex_roughness, sampler_linear, input.tex).r
    : constants.roughness;
rough = max(rough, 0.04);
```

- [ ] **Step 4: Commit**

```bash
git add engine/assets/shaders/mesh_gbuffer.kong
git commit -m "feat(mesh_gbuffer): add texture samplers for PBR maps"
```

---

## Task 2: Update asset_loader.c — Add default texture slots and use_* constants

**File:** `engine/sources/core/asset_loader.c`

- [ ] **Step 1: Add use_*_tex constants** (after the `opacity` constant entry, around line 280)

Insert before the closing `}` of the bind_constants array:

```c
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "use_albedo_tex",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "use_metallic_tex",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "use_roughness_tex",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f}, 1)}),
```

- [ ] **Step 2: Update bind_constants count** (line 283)

Change `34` to `37`:
```c
                                      37),
```

- [ ] **Step 3: Update bind_textures array** (around line 284-289)

Change from:
```c
                                  .bind_textures = any_array_create_from_raw(
                                      (void *[]){
                                          GC_ALLOC_INIT(bind_tex_t,
                                              {.name = "_shadow_map", .file = "_shadow_map"}),
                                      },
                                      1)}),
```

To:
```c
                                  .bind_textures = any_array_create_from_raw(
                                      (void *[]){
                                          GC_ALLOC_INIT(bind_tex_t,
                                              {.name = "_shadow_map", .file = "_shadow_map"}),
                                          GC_ALLOC_INIT(bind_tex_t,
                                              {.name = "tex_albedo", .file = ""}),
                                          GC_ALLOC_INIT(bind_tex_t,
                                              {.name = "tex_metallic", .file = ""}),
                                          GC_ALLOC_INIT(bind_tex_t,
                                              {.name = "tex_roughness", .file = ""}),
                                      },
                                      4)}),
```

- [ ] **Step 4: Commit**

```bash
git add engine/sources/core/asset_loader.c
git commit -m "feat(asset_loader): add default texture slots and use_tex constants"
```

---

## Task 3: Update scene_3d_api.c — Set use_* flags based on loaded texture

**File:** `engine/sources/core/scene_3d_api.c`

- [ ] **Step 1: Find the texture loading loop** (lines 518-524)

The existing code is:
```c
if (ctx->bind_textures != NULL) {
    for (int i = 0; i < ctx->bind_textures->length; i++) {
        bind_tex_t *tex = (bind_tex_t *)ctx->bind_textures->buffer[i];
        gpu_texture_t *img = data_get_image(tex->file);
        printf("[mesh_load] Texture '%s' (unit '%s'): %s\n",
            tex->file, tex->name, img ? "OK" : "NOT FOUND");
    }
}
```

Replace with:
```c
if (ctx->bind_textures != NULL) {
    for (int i = 0; i < ctx->bind_textures->length; i++) {
        bind_tex_t *tex = (bind_tex_t *)ctx->bind_textures->buffer[i];
        gpu_texture_t *img = data_get_image(tex->file);
        printf("[mesh_load] Texture '%s' (unit '%s'): %s\n",
            tex->file, tex->name, img ? "OK" : "NOT FOUND");

        // Set use_*_tex flag based on whether texture was successfully loaded
        if (tex->file != NULL && strlen(tex->file) > 0 && strcmp(tex->file, "_shadow_map") != 0) {
            // Find and set the corresponding use_*_tex constant
            for (int j = 0; j < ctx->bind_constants->length; j++) {
                bind_const_t *bc = (bind_const_t *)ctx->bind_constants->buffer[j];
                if (bc->name == NULL || bc->vec == NULL) continue;

                // Map texture slot to use_*_tex constant
                if (strcmp(tex->name, "tex_albedo") == 0 && strcmp(bc->name, "use_albedo_tex") == 0) {
                    bc->vec->buffer[0] = img ? 1.0f : 0.0f;
                } else if (strcmp(tex->name, "tex_metallic") == 0 && strcmp(bc->name, "use_metallic_tex") == 0) {
                    bc->vec->buffer[0] = img ? 1.0f : 0.0f;
                } else if (strcmp(tex->name, "tex_roughness") == 0 && strcmp(bc->name, "use_roughness_tex") == 0) {
                    bc->vec->buffer[0] = img ? 1.0f : 0.0f;
                }
            }
        }
    }
}
```

- [ ] **Step 2: Add string.h include** (at top of file if not already present)
```c
#include <string.h>
```

- [ ] **Step 3: Commit**

```bash
git add engine/sources/core/scene_3d_api.c
git commit -m "feat(scene_3d_api): set use_tex flags when loading textures"
```

---

## Task 4: Add Minic API for dynamic texture binding

**File:** `engine/sources/core/scene_3d_api.c` (add new functions at end of file)

- [ ] **Step 1: Add include for data_get_image** (check if already included at top)

The file should already have access to `data_get_image` through `asset_loader.h` or similar.

- [ ] **Step 2: Add minic_material_bind_texture function** (at end of file, before the closing `}`)

```c
// material_bind_texture(ctx_name, slot_name, file_path)
// Sets a texture on the current material context by slot name
static minic_val_t minic_material_bind_texture(minic_env_t *e) {
    (void)e;
    // Currently operates on the first mesh's material context
    // This is a placeholder - full implementation requires tracking current mesh context
    return minic_val_int(0);
}
```

- [ ] **Step 3: Add minic_material_set_use_texture function** (at end of file)

```c
// material_set_use_texture(ctx_name, slot_name, use_bool)
// Enables or disables texture usage for a given slot
static minic_val_t minic_material_set_use_texture(minic_env_t *e) {
    (void)e;
    return minic_val_int(0);
}
```

- [ ] **Step 4: Register the Minic APIs in runtime_api.c** (around line 1450)

Find the existing `scene_3d_api_register()` call and add after it:

```c
minic_register_native("material_bind_texture", minic_material_bind_texture);
minic_register_native("material_set_use_texture", minic_material_set_use_texture);
```

Note: The full implementation of dynamic texture binding requires tracking the current mesh context and modifying `material_context_runtime_t->_->textures`. This is a minimal stub for the API surface.

- [ ] **Step 5: Commit**

```bash
git add engine/sources/core/scene_3d_api.c
git add engine/sources/core/runtime_api.c
git commit -m "feat(texture_api): add material_bind_texture and material_set_use_texture stubs"
```

---

## Task 5: Build and test

- [ ] **Step 1: Export assets and shaders**

```bash
cd engine && ../base/make macos metal 2>&1 | tail -20
```

Expected: `postfx_ssao.kong` and `mesh_gbuffer.kong` compile without errors

- [ ] **Step 2: Build with xcodebuild**

```bash
cd engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | grep -E "(BUILD|error:)" | tail -10
```

Expected: `BUILD SUCCEEDED`

- [ ] **Step 3: Run the engine**

```bash
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame
```

Expected: Engine runs without crashes, no texture-related errors in console

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "feat: complete PBR texture support implementation"
```

---

## Verification Checklist

- [ ] `mesh_gbuffer.kong` has 3 new samplers (tex_albedo, tex_metallic, tex_roughness)
- [ ] `mesh_gbuffer.kong` has 3 new use_*_tex flags
- [ ] Shader applies gamma decode (pow 2.2) to albedo texture
- [ ] Shader falls back to constants when use_*_tex is 0.0
- [ ] `asset_loader.c` has 37 bind_constants (was 34)
- [ ] `asset_loader.c` has 4 bind_textures (was 1)
- [ ] `scene_3d_api.c` sets use_*_tex flags when textures load successfully
- [ ] Minic API stubs exist and compile
- [ ] Build succeeds without errors
- [ ] Engine runs without crashes

---

## Out of Scope (Future Work)

- Normal map support
- Dynamic texture binding full implementation (stubs only)
- Texture tiling/repeat modes
- Multi-UV-channel support
- Anisotropic filtering
