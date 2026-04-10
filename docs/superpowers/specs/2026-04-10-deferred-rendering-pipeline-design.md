# Engine Deferred Rendering Pipeline — Design

**Date:** 2026-04-10
**Status:** Approved
**Approach:** Incremental (Method B)

## Overview

Replace the engine layer's current forward rendering pipeline (simple `_gpu_begin` + `render_path_submit_draw("mesh")` + `gpu_end`) with a full deferred rendering pipeline. The design adapts the proven architecture from `paint/` but is purpose-built for the engine's ECS-driven entity/component system.

**Constraints:**
- All new code in `engine/` — never modify `base/`
- Kong shader format (same as paint/)
- Multi-platform: Metal, Vulkan, D3D12, WebGPU
- 2D sprites compose as overlay on top of deferred 3D output

---

## Pipeline Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│  Pass 1: GEOMETRY PASS                                          │
│  render_path_set_target(gbuffer0, gbuffer1, main, MRT)          │
│  → world_gbuffer.kong outputs to gbuffer0 + gbuffer1           │
│  → Depth written to main (D32)                                 │
│  [All mesh entities rendered in a single draw call batch]       │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│  Pass 2: LIGHTING PASS                                          │
│  render_path_set_target(buf)                                    │
│  → deferred_light.kong reads gbuffer0 + gbuffer1 + main        │
│  → Outputs lit color to buf (RGBA64)                           │
│  [Full-screen quad, single draw call]                          │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│  Pass 3: POST-PROCESSING (Phase 3 only)                        │
│  buf → SSAO → Bloom → TAA → last                               │
│  [Each pass: render_path_set_target + draw_shader]             │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│  Pass 4: 2D OVERLAY                                            │
│  render_path_set_target(screen)                                │
│  → render2d_bridge sprite batch (unchanged, forward pass)       │
└─────────────────────────────────────────────────────────────────┘
```

---

## G-Buffer Layout

| Target     | Format  | Size     | Content                                      |
|------------|---------|----------|----------------------------------------------|
| `main`     | D32     | W × H    | Depth buffer (shared across all passes)      |
| `gbuffer0` | RGBA64  | W × H    | Normal.xy (octahedron), Roughness, Metallic  |
| `gbuffer1` | RGBA64  | W × H    | Albedo.rgb, AO                              |
| `buf`      | RGBA64  | W × H    | Intermediate / lighting output               |
| `last`     | RGBA64  | W × H    | Final composited output (post-fx result)    |

---

## Phase 1: G-Buffer + Deferred Lighting

### New Files

| File | Purpose |
|------|---------|
| `engine/sources/ecs/deferred_gbuffer.c` | G-buffer lifecycle (init/shutdown/resize/render_geometry) |
| `engine/sources/ecs/deferred_gbuffer.h` | Header |
| `engine/sources/ecs/deferred_lighting.c` | Lighting pass lifecycle |
| `engine/sources/ecs/deferred_lighting.h` | Header |
| `engine/assets/shaders/world_gbuffer.kong` | Geometry pass shader (MRT output) |
| `engine/assets/shaders/deferred_light.kong` | Lighting pass shader (from paint/, adapted) |

### Modified Files

| File | Change |
|------|--------|
| `engine/sources/ecs/render3d_bridge.c` | Replace `sys_3d_render_commands` body: call gbuffer → lighting → postfx chain |
| `engine/sources/ecs/camera_bridge_3d.c` | Export view/proj matrices to deferred lighting |
| `engine/assets/systems.manifest` | Add `deferred3d` system |

### deferred_gbuffer.c

```c
void deferred_gbuffer_init(void);
void deferred_gbuffer_shutdown(void);
void deferred_gbuffer_resize(int width, int height);
void deferred_gbuffer_render_geometry(ecs_world_t *ecs);
```

Creates render targets in `init()`. On `resize()`, destroys and recreates at new dimensions (matches `render_path_base_get_super_sampling()` scale factor). `render_geometry()` queries all entities with `comp_3d_position` + `comp_3d_mesh_renderer` + `RenderObject3D` and calls `render_path_submit_draw("mesh")` with the MRT-bound geometry pass.

### deferred_lighting.c

```c
void deferred_lighting_init(void);
void deferred_lighting_shutdown(void);
void deferred_lighting_render(const camera_t *cam,
                               const comp_directional_light *light);
```

Renders a full-screen quad with `deferred_light.kong`. Binds gbuffer0/gbuffer1/main as input textures. Uses `invVP` (inverse view-projection) to reconstruct world positions from depth. Directional light direction/color/strength come from ECS. Ambient term uses a constant fallback (skybox/shirr support added in a future phase).

### world_gbuffer.kong

Adapted from `paint/shaders/world_pass.kong` for engine use. Outputs to MRT:
- Color attachment 0 → `gbuffer0`: `float4(normal_x, normal_y, roughness, metallic)`
- Color attachment 1 → `gbuffer1`: `float4(albedo_r, albedo_g, albedo_b, ao)`

Normal encoding: octahedron (same as paint). Defaults: roughness=0.5, metallic=0.0, ao=1.0, albedo=(0.8, 0.8, 0.8).

### deferred_light.kong

Adapted from `paint/shaders/deferred_light.kong`. Reads gbuffer0/gbuffer1/gbufferD. Performs:
- World position reconstruction from depth using `invVP`
- Diffuse: `dot(N, L) * light_color * light_strength` per directional light
- Specular: env map lookup with roughness-based mip level
- PBR: surface_albedo, surface_f0, env_brdf_approx

### Platform Compatibility

The Kong shader compiler (`base/sources/kong/kong.c`) generates code for all backends (Metal, Vulkan, D3D12, WebGPU) from the same `.kong` source. No backend-specific shader code needed.

---

## Phase 2: PBR Materials + Multi-Light Support

### New/Modified Components

Extend `comp_3d_mesh_renderer` or create new `comp_material` ECS component:

```c
typedef struct {
    char *albedo_texture_path;
    char *normal_texture_path;
    char *metallic_texture_path;  // or packed: roughness in R, metallic in G
    char *ao_texture_path;
    float albedo_r, albedo_g, albedo_b;  // fallback color
    float metallic;   // fallback (used when texture absent)
    float roughness;  // fallback
    float ao;         // fallback
} comp_material;
```

`world_gbuffer.kong` is extended: if a texture path is set, `texture_sample()` from that texture; otherwise use the fallback constants.

### Multi-Light

Query all entities with `comp_directional_light` where `enabled == true`. Sum contributions in the deferred lighting pass. For Phase 1, support only directional lights (no point/spot — those require shadow map infrastructure).

### Light Component Bridge

Create `light_bridge.c` to sync `comp_directional_light` ECS components to deferred lighting uniforms. No Iron scene graph integration needed — lights are purely data for the deferred shader.

---

## Phase 3: Post-Processing

Post-processing is purely screen-space passes reading from `buf` and writing to `last`.

### Pass Order

```
buf → SSAO → SSAO_blur → Bloom_down → Bloom_up → TAA → last
     ↓
compositor_pass (vignette + grain + tonemapping)
     ↓
last → screen (2D overlay on top)
```

### New Shaders

| Shader | Source | Changes |
|--------|--------|---------|
| `ssao_pass.kong` | paint/ | None or minimal |
| `ssao_blur_pass.kong` | paint/ | None |
| `bloom_downsample_pass.kong` | paint/ | None |
| `bloom_upsample_pass.kong` | paint/ | None |
| `taa_pass.kong` | paint/ | Needs history frame (frame N-1) |
| `compositor_pass.kong` | paint/ | None |

### TAA History

`deferred_lighting.c` maintains a `last` frame texture. On each frame:
1. `render_path_bind_target("last", "history_tex")` for TAA pass
2. After TAA renders to `last`, swap `buf` ↔ `last` for next frame

### Post-Processing Module

```c
// postfx.c
void postfx_init(void);
void postfx_shutdown(void);
void postfx_render(const char *input_target, const char *output_target);
```

`postfx_render()` executes the full post-fx chain. Each sub-pass uses `render_path_set_target(next_target)` + `render_path_bind_target(input, "tex")` + `render_path_draw_shader(...)`.

---

## 2D Sprite Overlay

The 2D sprite overlay (`render2d_bridge.c`) is unchanged. It renders after all 3D/deferred passes as a forward pass over the final framebuffer:

```
... → last → render2d_bridge (forward sprites) → screen
```

`render2d_bridge_draw()` uses `render_path_set_target(NULL)` to render to the default framebuffer.

---

## Backward Compatibility

The deferred pipeline is opt-in. A compile-time flag or `systems.manifest` toggle controls whether `render3d_bridge` uses forward or deferred rendering:

```
# In systems.manifest:
forward3d    # old forward path (retained as fallback)
# deferred3d  # new deferred path (Phase 1 complete → enable)
```

When `forward3d` is active, `render3d_bridge` uses the existing `_gpu_begin` + `render_path_submit_draw("mesh")` path. When `deferred3d` is active, it uses the deferred pipeline.

---

## File Summary

### Phase 1 (Priority)

**New files:**
- `engine/sources/ecs/deferred_gbuffer.c/h`
- `engine/sources/ecs/deferred_lighting.c/h`
- `engine/assets/shaders/world_gbuffer.kong`
- `engine/assets/shaders/deferred_light.kong`

**Modified files:**
- `engine/sources/ecs/render3d_bridge.c`
- `engine/sources/ecs/camera_bridge_3d.c`
- `engine/assets/systems.manifest`

### Phase 2

**New/modified files:**
- `engine/sources/components/material.c/h` (new component)
- `engine/sources/ecs/material_bridge.c/h` (new)
- `engine/sources/ecs/light_bridge.c/h` (new)
- `engine/sources/ecs/ecs_components.c/h` (register material component)
- `engine/assets/shaders/world_gbuffer.kong` (texture sampling)
- `engine/assets/shaders/deferred_light.kong` (multi-light loop)

### Phase 3

**New files:**
- `engine/sources/ecs/postfx.c/h`
- `engine/assets/shaders/ssao_pass.kong`
- `engine/assets/shaders/ssao_blur_pass.kong`
- `engine/assets/shaders/bloom_downsample_pass.kong`
- `engine/assets/shaders/bloom_upsample_pass.kong`
- `engine/assets/shaders/taa_pass.kong`
- `engine/assets/shaders/compositor_pass.kong`

**Modified files:**
- `engine/sources/ecs/render3d_bridge.c` (wire postfx into pipeline)
- `engine/sources/ecs/deferred_lighting.c` (TAA history management)
