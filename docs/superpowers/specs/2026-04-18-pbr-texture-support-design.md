# PBR Texture Support Design

**Date:** 2026-04-18
**Status:** Approved
**Approach:** Add texture samplers to mesh_gbuffer.kong, bind via existing bind_textures mechanism, add Minic API for dynamic texture binding

---

## Context

The current `mesh_gbuffer.kong` shader uses only shader constants (`albedo_rgb`, `metallic`, `roughness`) for PBR material properties. There is no texture sampling support. The goal is to add texture sampling for albedo, metallic, and roughness maps while maintaining backward compatibility with existing .arm files and constant-based materials.

---

## Design

### 1. Shader Changes (`mesh_gbuffer.kong`)

#### New sampler declarations
```kong
#[set(everything)]
const tex_albedo: tex2d;

#[set(everything)]
const tex_metallic: tex2d;

#[set(everything)]
const tex_roughness: tex2d;
```

#### New uniform flags (added to existing constants block)
```kong
use_albedo_tex: float;    // 1.0 = use texture, 0.0 = use albedo_rgb
use_metallic_tex: float;  // 1.0 = use texture, 0.0 = use metallic constant
use_roughness_tex: float; // 1.0 = use texture, 0.0 = use roughness constant
```

#### Material property sampling logic
In `mesh_gbuffer_frag()`, replace the existing constant reads:

```kong
// Albedo: texture with sRGB→Linear conversion, or constant fallback
var albedo_tex: float3 = sample(tex_albedo, sampler_linear, input.tex).rgb;
albedo_tex = pow(albedo_tex, float3(2.2));  // gamma decode sRGB→Linear
var albedo: float3 = (constants.use_albedo_tex > 0.5) ? albedo_tex : constants.albedo_rgb;

// Metallic: texture sample (R channel) or constant fallback
var metallic: float = (constants.use_metallic_tex > 0.5)
    ? sample(tex_metallic, sampler_linear, input.tex).r
    : constants.metallic;

// Roughness: texture sample (R channel) or constant fallback
var roughness: float = (constants.use_roughness_tex > 0.5)
    ? sample(tex_roughness, sampler_linear, input.tex).r
    : constants.roughness;
```

#### Constants block update
The existing `constants` block in `mesh_gbuffer.kong` is extended with three new float entries:
- `use_albedo_tex` (default 0.0)
- `use_metallic_tex` (default 0.0)
- `use_roughness_tex` (default 0.0)

UV coordinates are sourced from the vertex's `tex` attribute (existing `input.tex`), no changes needed.

---

### 2. C Side Changes

#### `asset_loader.c` — Default texture slots

In `scene_ensure_defaults()`, extend the `bind_textures` array to include the new texture slots with empty file paths as the "no texture" sentinel:

```c
.bind_textures = any_array_create_from_raw((void *[]){
    // Existing
    GC_ALLOC_INIT(bind_tex_t, {.name = "_shadow_map", .file = "_shadow_map"}),
    // New slots
    GC_ALLOC_INIT(bind_tex_t, {.name = "tex_albedo", .file = ""}),
    GC_ALLOC_INIT(bind_tex_t, {.name = "tex_metallic", .file = ""}),
    GC_ALLOC_INIT(bind_tex_t, {.name = "tex_roughness", .file = ""}),
}, 4),
```

Also extend `bind_constants` to include the three new `use_*_tex` float entries initialized to 0.0.

#### `scene_3d_api.c` — Texture loading

In the existing texture loading loop (lines 518-524 of `scene_3d_api.c`), when iterating `bind_textures`:
- If `tex->file != ""` and `tex->file != "_shadow_map"`: load via `data_get_image(tex->file)` and set the corresponding `use_*_tex` bind_constant to 1.0
- If `tex->file == ""`: leave `use_*_tex` at 0.0 (constant fallback)

#### `render3d_bridge.c` — Per-frame flags

In `set_material_const()` or a new helper, ensure the `use_*_tex` flags are set correctly each frame based on whether the texture was successfully loaded (exists on disk). The constant value should reflect the loaded state, not be recalculated each frame.

#### `material_api.c` or new `texture_api.c` — Minic API

New functions registered with `minic_register_native()`:

```c
// material_bind_texture(ctx_name, slot_name, file_path)
// Sets a texture on a material context by slot name, loads the texture.
// Example: material_bind_texture("mesh", "tex_albedo", "textures/brick_diffuse.png")
minic_val_t minic_material_bind_texture(minic_env_t *e);

// material_set_use_texture(ctx_name, slot_name, use_texture_bool)
// Enables or disables texture usage for a given slot.
// Example: material_set_use_texture("mesh", "tex_albedo", true)
minic_val_t minic_material_set_use_texture(minic_env_t *e);
```

These APIs operate on the current mesh's material context, similar to how `set_material_const` works. The texture is loaded via `data_get_image()` and stored in `material_context_runtime_t->_->textures`.

---

### 3. Texture Loading Flow

```
mesh_load_arm() → scene_3d_api.c
    ↓
material_context_load(ctx)
    ↓
for each bind_tex in ctx->bind_textures:
    if tex->file != "" and tex->file != "_shadow_map":
        texture = data_get_image(tex->file)
        set use_*_tex = 1.0 in bind_constants
    else:
        use_*_tex = 0.0 (fallback to constant)
    ↓
uniforms_set_material_consts() at draw time
    → binds texture to shader sampler slot
    → passes use_*_tex float to shader
```

---

### 4. Backward Compatibility

- **Existing .arm files without texture declarations**: `file = ""` → `use_*_tex = 0.0` → materials continue to use float constants unchanged
- **Existing .arm files with texture declarations**: textures are loaded and used automatically
- **No changes to existing shader behavior** for non-textured materials
- **Shadow map mechanism** is unaffected (separate `_shadow_map` slot)

---

### 5. Texture Parameters (User-Confirmed)

| Parameter | Value |
|-----------|-------|
| sRGB handling | Yes — shader applies `pow(color, 2.2)` gamma decode |
| Fallback when no texture | Uses existing `albedo_rgb` float constant |
| Filtering | Bilinear (default sampler_linear) |
| UV source | Vertex `tex` attribute (existing) |

---

## Files to Modify

| File | Change |
|------|--------|
| `engine/assets/shaders/mesh_gbuffer.kong` | Add 3 texture samplers + 3 use flags + sampling logic |
| `engine/sources/core/asset_loader.c` | Add 3 default texture slots + 3 use_* constants |
| `engine/sources/core/scene_3d_api.c` | Set use_* flags based on loaded texture validity |
| `engine/sources/core/material_api.c` | Add `material_bind_texture()` Minic API |
| `engine/sources/core/runtime_api.c` | Register new Minic APIs |
| `docs/superpowers/specs/2026-04-18-pbr-texture-support-design.md` | This document |

## Files to Create

| File | Purpose |
|------|---------|
| (none) | No new files needed |

---

## Out of Scope

- Normal map support (separate feature, future work)
- Texture tiling/repeat modes (use engine defaults)
- Texture animation (UV scrolling, etc.)
- Multi-UV-channel support (always uses first UV channel)
- Anisotropic filtering (use bilinear)
- Texture LOD (future optimization)
