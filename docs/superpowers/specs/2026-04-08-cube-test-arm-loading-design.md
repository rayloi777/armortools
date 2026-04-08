# Cube Test: Load cube.arm + Texture Support

## Goal

Replace the hardcoded mesh data in `base/tests/cube` with actual `.arm` file loading via `data_get_scene_raw()`, and add texture support using the existing `texture.png` asset.

## Current State

- `base/tests/cube/sources/main.c` builds the entire `scene_t` in C with hardcoded vertex/index arrays (~35 lines of mesh data).
- `base/tests/cube/assets/cube.arm` contains the same cube mesh but is never loaded.
- `base/tests/cube/assets/texture.png` exists but is unused.
- `mesh.kong` outputs solid red — no texture sampling.

## Changes

### 1. `base/tests/cube/sources/main.c`

**Remove:** The full `scene_t` construction with `GC_ALLOC_INIT` including hardcoded `mesh_datas`, `vertex_arrays`, and `index_array`.

**Add:**

1. Call `data_get_scene_raw("cube")` to load `cube.arm`. This returns a `scene_t` with `mesh_datas`, `objects`, and transforms already populated.
2. Supplement the loaded scene with pipeline data that `cube.arm` lacks:
   - `camera_datas` — single camera (near=0.01, far=100, fov=0.85)
   - `camera_ref` — "Camera"
   - `material_datas` — "MyMaterial" with "MyShader" and a material context containing `bind_textures` (bind_tex_t: name="my_texture", file="texture.k")
   - `shader_datas` — "MyShader" with mesh context, vertex elements for `pos` (short4norm) and `tex` (short2norm), WVP constant, texture_units (tex_unit_t: name="my_texture"), depth attachment "D32"
   - `world_datas` — "MyWorld" with black background
   - `world_ref` — "MyWorld"
3. Cache with `any_map_set(data_cached_scene_raws, scene->name, scene)` and call `scene_create(scene)`.

The `render_commands`, `scene_ready`, `_kickstart`, and stub functions remain unchanged.

### 2. `base/tests/cube/shaders/mesh.kong`

**Add:**

```kong
#[set(everything)]
const sampler_linear: sampler;

#[set(everything)]
const my_texture: tex2d;
```

**Modify `vert_in`** to include `tex: float2`.

**Add `vert_out` field** `tex: float2`.

**Modify `mesh_vert`** to pass through `input.tex` to `output.tex`.

**Modify `mesh_frag`** to sample the texture: `sample(my_texture, sampler_linear, input.tex)` instead of hardcoded red.

### 3. No new assets needed

`cube.arm` and `texture.png` already exist in `assets/`. The `project.js` already exports them via `add_assets("assets/*")`.

## Files Changed

| File | Change |
|------|--------|
| `base/tests/cube/sources/main.c` | Replace hardcoded scene with `data_get_scene_raw("cube")` + pipeline data |
| `base/tests/cube/shaders/mesh.kong` | Add texture sampler, tex coords, sample() call |

## Build & Test

```bash
cd base/tests/cube && ../../make macos metal
cd build && xcodebuild -project test.xcodeproj -configuration Release build
build/Release/test.app/Contents/MacOS/test
```

Expected result: a textured cube rendered at z=5 with the texture.png applied.
