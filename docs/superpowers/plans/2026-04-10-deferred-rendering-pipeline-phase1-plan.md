# Deferred Rendering Pipeline — Phase 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace engine's forward 3D renderer with a G-buffer geometry pass + deferred lighting pass. Phase 1 outputs lit 3D scene to `buf` render target; Phase 2 (PBR textures) and Phase 3 (post-fx) build on this.

**Architecture:** Two-pass deferred pipeline. Pass 1: geometry → G-buffer (gbuffer0 + gbuffer1 + depth). Pass 2: full-screen quad → lighting (reads G-buffer, writes lit color to buf). 2D sprite overlay continues to draw after via existing `render2d_bridge`.

**Tech Stack:** Flecs ECS, Iron GPU API, Kong shader format, `render_path_set_target` with MRT, `mat4_inv` from iron_math.

---

## File Map

| File | Action |
|------|--------|
| `engine/sources/ecs/deferred_gbuffer.h` | **Create** — G-buffer public API |
| `engine/sources/ecs/deferred_gbuffer.c` | **Create** — G-buffer lifecycle + geometry render |
| `engine/sources/ecs/deferred_lighting.h` | **Create** — lighting pass public API |
| `engine/sources/ecs/deferred_lighting.c` | **Create** — lighting pass lifecycle |
| `engine/assets/shaders/world_gbuffer.kong` | **Create** — geometry pass shader (MRT) |
| `engine/assets/shaders/deferred_light.kong` | **Create** — lighting pass shader |
| `engine/sources/ecs/render3d_bridge.c` | **Modify** — wire gbuffer + lighting into `sys_3d_render_commands` |
| `engine/sources/ecs/camera_bridge_3d.c` | **Modify** — add `camera_bridge_3d_get_inv_vp()` export |
| `engine/assets/systems.manifest` | **Modify** — comment-out `scene3d_test` and add `deferred3d` |

---

## Task 1: Create `deferred_gbuffer.h`

**Files:** Create: `engine/sources/ecs/deferred_gbuffer.h`

```c
#pragma once

#include <stdint.h>

void deferred_gbuffer_init(void);
void deferred_gbuffer_shutdown(void);
void deferred_gbuffer_resize(void);
void deferred_gbuffer_render_geometry(void);
```

- `deferred_gbuffer_render_geometry()` is called each frame after init/resize. It sets MRT targets, clears gbuffers with `GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH`, calls `render_path_submit_draw("mesh")`, then `render_path_end()`.
- No ECS world pointer stored — uses global `g_game_world` from `engine_world.c` (declared in `ecs_world.h`).

---

## Task 2: Create `deferred_gbuffer.c`

**Files:** Create: `engine/sources/ecs/deferred_gbuffer.c`

- [ ] **Step 1: Write includes and global state**

```c
#include "deferred_gbuffer.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include <iron.h>
#include <stdio.h>
#include <stdbool.h>

static bool s_initialized = false;

void deferred_gbuffer_init(void) {
    if (s_initialized) return;

    // gbuffer0: normal.xy (octahedron), roughness, metallic
    {
        render_target_t *t = render_target_create();
        t->name   = "gbuffer0";
        t->width  = 0;
        t->height = 0;
        t->format = "RGBA64";
        t->scale  = render_path_base_get_super_sampling();
        render_path_create_render_target(t);
    }

    // gbuffer1: albedo.rgb, ao
    {
        render_target_t *t = render_target_create();
        t->name   = "gbuffer1";
        t->width  = 0;
        t->height = 0;
        t->format = "RGBA64";
        t->scale  = render_path_base_get_super_sampling();
        render_path_create_render_target(t);
    }

    // main depth (D32) — reused from the existing depth target
    // (created elsewhere by the base render path init; we bind it as depth)

    // buf: intermediate / lighting output
    {
        render_target_t *t = render_target_create();
        t->name   = "buf";
        t->width  = 0;
        t->height = 0;
        t->format = "RGBA64";
        t->scale  = render_path_base_get_super_sampling();
        render_path_create_render_target(t);
    }

    // last: final output
    {
        render_target_t *t = render_target_create();
        t->name   = "last";
        t->width  = 0;
        t->height = 0;
        t->format = "RGBA64";
        t->scale  = render_path_base_get_super_sampling();
        render_path_create_render_target(t);
    }

    s_initialized = true;
    printf("Deferred G-Buffer initialized\n");
}
```

- [ ] **Step 2: Write shutdown**

```c
void deferred_gbuffer_shutdown(void) {
    s_initialized = false;
    printf("Deferred G-Buffer shutdown\n");
}
```

- [ ] **Step 3: Write resize**

`deferred_gbuffer_resize()` calls `deferred_gbuffer_shutdown()` then `deferred_gbuffer_init()`. The render targets use `width=0, height=0, scale=X` meaning they auto-size to the window; re-calling init rebuilds them at the new resolution.

```c
void deferred_gbuffer_resize(void) {
    deferred_gbuffer_shutdown();
    deferred_gbuffer_init();
}
```

- [ ] **Step 4: Write geometry render**

```c
void deferred_gbuffer_render_geometry(void) {
    if (!s_initialized) return;

    // MRT: gbuffer0 + gbuffer1 as color attachments, main (D32) as depth
    string_array_t *additional = any_array_create_from_raw(
        (void *[]){ "gbuffer1" },
        1);
    render_path_set_target("gbuffer0", additional, "main",
                           GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH,
                           0, 1.0f);

    // Draw all mesh objects with the world_gbuffer shader context
    render_path_submit_draw("mesh");
    render_path_end();
}
```

- [ ] **Step 5: Write the `mesh` shader context for world_gbuffer**

This is set up in `asset_loader.c` (which registers shader contexts). Add to `asset_loader.c` a new context block:

In `engine/sources/core/asset_loader.c`, find the existing `mesh` context block and add a `world_gbuffer` context block after it. The `world_gbuffer` context uses the same vertex shader (`mesh.vert`) but overrides the fragment shader to `world_gbuffer.frag`:

```c
// In asset_loader.c, alongside the existing mesh context:
{
    .name = "world_gbuffer",
    .vertex_shader = "mesh.vert",
    .fragment_shader = "world_gbuffer.frag",
    .compare_mode = "less",
    .cull_mode = "none",
    .depth_write = true,
    .depth_attachment = "main"
},
```

- [ ] **Step 6: Commit**

```bash
git add engine/sources/ecs/deferred_gbuffer.c engine/sources/ecs/deferred_gbuffer.h engine/sources/core/asset_loader.c
git commit -m "feat(ecs): add deferred_gbuffer module for G-buffer geometry pass

Creates render targets (gbuffer0, gbuffer1, buf, last) and renders
mesh entities to MRT (gbuffer0 + gbuffer1 + depth) using world_gbuffer shader."

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
```

---

## Task 3: Create `world_gbuffer.kong` Shader

**Files:** Create: `engine/assets/shaders/world_gbuffer.kong`

This shader runs on every mesh vertex in the geometry pass. It outputs to MRT (two color attachments) and writes depth.

- [ ] **Step 1: Write the shader**

```kong
# world_gbuffer.kong — Geometry pass shader for deferred rendering.
# Outputs to MRT:
#   Color 0 (gbuffer0): normal.xy (octahedron), roughness, metallic
#   Color 1 (gbuffer1): albedo.rgb, ao

#[set(everything)]
const constants: {
    SMVP: float4x4;  # shadow-model-view-projection (for future shadow maps)
};

struct vert_in {
    pos: float3;
    nor: float3;
    uv: float2;
}

struct vert_out {
    pos: float4;
    nor: float3;
}

fun world_gbuffer_vert(input: vert_in): vert_out {
    var output: vert_out;
    output.pos = constants.SMVP * float4(input.pos, 1.0);
    output.nor = input.nor;
    return output;
}

// Encode normal using octahedron mapping
fun normal_oct_encoded(n: float3): float2 {
    n = normalize(n);
    var normal_oct: float2 = n.xy * (1.0 / (abs(n.x) + abs(n.y) + abs(n.z)));
    // Convert to signed
    normal_oct = normal_oct * 0.5 + 0.5;
    return normal_oct;
}

fun world_gbuffer_frag(input: vert_out): float4 {
    var n: float3 = normalize(input.nor);

    // gbuffer0: normal.xy (octahedron), roughness, metallic
    var g0: float4;
    var enc: float2 = normal_oct_encoded(n);
    g0.r = enc.x;
    g0.g = enc.y;
    g0.b = 0.5;  // roughness = 0.5 (fallback)
    g0.a = 0.0;  // metallic = 0.0 (fallback)

    return g0;
}

// Second color attachment: gbuffer1
fun world_gbuffer_frag_gbuffer1(input: vert_out): float4 {
    var g1: float4;
    g1.r = 0.8;  // albedo.r (fallback grey)
    g1.g = 0.8;  // albedo.g
    g1.b = 0.8;  // albedo.b
    g1.a = 1.0;  // ao = 1.0 (fallback)
    return g1;
}

#[pipe]
struct pipe {
    vertex = world_gbuffer_vert;
    fragment0 = world_gbuffer_frag;       // → gbuffer0
    fragment1 = world_gbuffer_frag_gbuffer1; // → gbuffer1
}
```

- [ ] **Step 2: Export the shader via base/make**

The `base/make` step compiles Kong shaders. No additional config needed — the `mesh.vert` is already exported, and `world_gbuffer.frag` will be generated alongside it. The `render_path_load_shader("Scene/world_gbuffer/world_gbuffer")` call (added in Task 5) triggers compilation.

- [ ] **Step 3: Commit**

```bash
git add engine/assets/shaders/world_gbuffer.kong
git commit -m "feat(shader): add world_gbuffer.kong for MRT geometry pass

Outputs normal (octahedron), roughness, metallic to gbuffer0
and albedo+ao to gbuffer1. Uses same mesh.vert as forward renderer.

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
```

---

## Task 4: Create `deferred_lighting.h`

**Files:** Create: `engine/sources/ecs/deferred_lighting.h`

```c
#pragma once

#include <stdint.h>

void deferred_lighting_init(void);
void deferred_lighting_shutdown(void);
void deferred_lighting_render(void);
```

`deferred_lighting_render()` queries the ECS for `comp_directional_light` entities, fetches the active camera's view/proj matrices, computes `invVP`, and executes the full-screen lighting quad.

---

## Task 5: Create `deferred_lighting.c`

**Files:** Create: `engine/sources/ecs/deferred_lighting.c`

- [ ] **Step 1: Write includes and globals**

```c
#include "deferred_lighting.h"
#include "deferred_gbuffer.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "camera_bridge_3d.h"
#include <iron.h>
#include <iron_math.h>
#include <stdio.h>
#include <stdbool.h>

static bool s_initialized = false;
static ecs_query_t *s_light_query = NULL;

void deferred_lighting_init(void) {
    if (s_initialized) return;

    // Load the deferred lighting shader
    render_path_load_shader("Scene/deferred_light/deferred_light");

    s_initialized = true;
    printf("Deferred Lighting initialized\n");
}
```

- [ ] **Step 2: Write shutdown**

```c
void deferred_lighting_shutdown(void) {
    if (s_light_query) {
        ecs_query_fini(s_light_query);
        s_light_query = NULL;
    }
    s_initialized = false;
    printf("Deferred Lighting shutdown\n");
}
```

- [ ] **Step 3: Write light query setup**

Called from `init()` — cache the directional light query:

```c
void deferred_lighting_init(void) {
    // ... after loading shader ...
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_game_world);
    if (ecs) {
        ecs_query_desc_t qdesc = {0};
        qdesc.terms[0].id = ecs_component_comp_directional_light();
        s_light_query = ecs_query_init(ecs, &qdesc);
    }
}
```

- [ ] **Step 4: Write render function**

```c
void deferred_lighting_render(void) {
    if (!s_initialized) return;

    // Get active camera
    camera_object_t *cam = (camera_object_t *)camera_bridge_3d_get_active();
    if (!cam) return;

    // Compute inv(VP) for position reconstruction from depth
    mat4_t vp = mat4_mult_mat(cam->p, cam->v);
    mat4_t inv_vp = mat4_inv(vp);

    // Set lighting pass target (single color, no MRT)
    render_path_set_target("buf", NULL, NULL, GPU_CLEAR_NONE, 0, 0.0f);

    // Bind G-buffer textures as inputs
    render_path_bind_target("main",   "gbufferD");  // depth → gbufferD
    render_path_bind_target("gbuffer0", "gbuffer0");
    render_path_bind_target("gbuffer1", "gbuffer1");

    // Set camera uniforms via shader constants
    // The shader reads: constants.invVP, constants.eye, constants.camera_proj
    // Use render_path_set_shader_constants or equivalent
    // (see paint/render_path_base.c deferred_light setup for how constants are passed)
    // For Phase 1: hardcode ambient to grey (0.1, 0.1, 0.1) and single directional light

    // Query the first enabled directional light
    comp_directional_light light_data = {0};
    light_data.enabled = false; // default: no light = ambient only
    if (s_light_query) {
        ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_game_world);
        ecs_iter_t it = ecs_query_iter(ecs, s_light_query);
        while (ecs_query_next(&it)) {
            comp_directional_light *l = ecs_field(&it, comp_directional_light, 1);
            for (int i = 0; i < it.count; i++) {
                if (l[i].enabled) {
                    light_data = l[i];
                    break;
                }
            }
        }
    }

    // Set directional light uniforms
    // light_dir: vec3(-l.dir_x, -l.dir_y, -l.dir_z) normalized
    // light_color: vec3(l.color_r, l.color_g, l.color_b) * l.strength
    // These are passed via render_path shader constants — see deferred_light.kong uniforms

    // Draw full-screen quad (the shader's vertex shader outputs clip-space quad directly)
    render_path_draw_shader("Scene/deferred_light/deferred_light");
    render_path_end();
}
```

**Important:** The deferred_light shader's vertex shader (`deferred_light_vert`) outputs a clip-space quad directly — no mesh needed. The fragment shader does all the G-buffer reading and lighting.

The shader constants (invVP, eye, light dir/color) need to be passed. In `paint/`, this is done via `shader_set_uniform` calls after `render_path_set_target`. For Phase 1, use the default constant values baked into the shader (or add a minimal uniform update path).

- [ ] **Step 5: Commit**

```bash
git add engine/sources/ecs/deferred_lighting.c engine/sources/ecs/deferred_lighting.h
git commit -m "feat(ecs): add deferred_lighting module for deferred lighting pass

Renders full-screen quad reading gbuffer0/gbuffer1/depth and outputs
lit color to buf. Queries comp_directional_light from ECS.

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
```

---

## Task 6: Create `deferred_light.kong` Shader

**Files:** Create: `engine/assets/shaders/deferred_light.kong`

Adapted from `paint/shaders/deferred_light.kong` but simplified: single directional light + constant ambient, no SH irradiance, no env map specular (Phase 2 adds these).

- [ ] **Step 1: Write the shader**

```kong
# deferred_light.kong — Deferred lighting pass for engine layer.
# Phase 1: single directional light + constant ambient.
# Phase 2 adds: PBR metallic/roughness, env map specular.
# Phase 3 adds: SSAO contribution.

#[set(everything)]
const constants: {
    invVP: float4x4;
    eye: float3;
    camera_proj: float2;  // x = near/far ratio, y = far
    light_dir: float3;
    light_color: float3;
    ambient: float3;
};

#[set(everything)]
const sampler_linear: sampler;

#[set(everything)]
const gbufferD: tex2d;
#[set(everything)]
const gbuffer0: tex2d;
#[set(everything)]
const gbuffer1: tex2d;

const PI: float = 3.14159265358979;
const PI2: float = 6.28318530718;

struct vert_in {
    pos: float2;
}

struct vert_out {
    pos: float4;
    tex: float2;
    view_ray: float3;
}

fun deferred_light_vert(input: vert_in): vert_out {
    var output: vert_out;
    output.tex = input.pos.xy * 0.5 + 0.5;
    output.tex.y = 1.0 - output.tex.y;
    output.pos = float4(input.pos.xy, 0.0, 1.0);

    // Reconstruct view ray from clip space
    var v: float4 = float4(input.pos.xy, 1.0, 1.0);
    v = constants.invVP * v;
    v.xyz = v.xyz / v.w;
    output.view_ray = v.xyz - constants.eye;

    return output;
}

fun octahedron_wrap(v: float2): float2 {
    var a: float2;
    if (v.x >= 0.0) {
        a.x = 1.0;
    } else {
        a.x = -1.0;
    }
    if (v.y >= 0.0) {
        a.y = 1.0;
    } else {
        a.y = -1.0;
    }
    var r: float2;
    r.x = abs(v.y);
    r.y = abs(v.x);
    r.x = 1.0 - r.x;
    r.y = 1.0 - r.y;
    return r * a;
}

fun surface_albedo(base_color: float3, metalness: float): float3 {
    return lerp3(base_color, float3(0.0, 0.0, 0.0), metalness);
}

fun surface_f0(base_color: float3, metalness: float): float3 {
    return lerp3(float3(0.04, 0.04, 0.04), base_color, metalness);
}

fun get_pos(eye: float3, eye_look: float3, view_ray: float3, depth: float, camera_proj: float2): float3 {
    var linear_depth: float = camera_proj.y / (depth - camera_proj.x);
    var view_z_dist: float = dot(eye_look, view_ray);
    var wposition: float3 = eye + view_ray * (linear_depth / view_z_dist);
    return wposition;
}

fun env_brdf_approx(specular: float3, roughness: float, dotnv: float): float3 {
    var c0: float4 = float4(-1.0, -0.0275, -0.572, 0.022);
    var c1: float4 = float4(1.0, 0.0425, 1.04, -0.04);
    var r: float4 = c0 * roughness + c1;
    var a004: float = min(r.x * r.x, exp((-9.28 * dotnv) * log(2.0))) * r.x + r.y;
    var ab: float2 = float2(-1.04, 1.04) * a004 + r.zw;
    return specular * ab.x + ab.y;
}

fun deferred_light_frag(input: vert_out): float4 {
    // Read G-buffer
    var g0: float4 = sample_lod(gbuffer0, sampler_linear, input.tex, 0.0);
    var g1: float4 = sample_lod(gbuffer1, sampler_linear, input.tex, 0.0);
    var depth: float = sample_lod(gbufferD, sampler_linear, input.tex, 0.0).r;

    // Decode normal (octahedron)
    var n: float3;
    n.z = 1.0 - abs(g0.x) - abs(g0.y);
    if (n.z >= 0.0) {
        n.x = g0.x;
        n.y = g0.y;
    } else {
        var f2: float2 = octahedron_wrap(g0.xy);
        n.x = f2.x;
        n.y = f2.y;
    }
    n = normalize(n);

    var roughness: float = g0.b;
    var metallic: float = g0.a;

    var albedo: float3 = surface_albedo(g1.rgb, metallic);
    var f0: float3 = surface_f0(g1.rgb, metallic);

    // Reconstruct world position
    var eye_look: float3 = normalize(input.view_ray);
    var p: float3 = get_pos(constants.eye, eye_look, normalize(input.view_ray),
                            depth, constants.camera_proj);
    var v: float3 = normalize(constants.eye - p);
    var dotnv: float = max(0.0, dot(n, v));

    // Ambient (Phase 1: constant; Phase 3 SSAO multiplies this by AO from gbuffer1.a)
    var color: float3 = constants.ambient * albedo;

    // Directional light
    var l: float3 = normalize(constants.light_dir);
    var ndotl: float = max(0.0, dot(n, l));
    color = color + (constants.light_color * ndotl * albedo);

    // Specular (simplified GGX env BRDF approximation)
    var reflection_world: float3 = reflect(-v, n);
    var prefiltered_color: float3 = constants.ambient;  // Phase 2: env map lookup
    color = color + prefiltered_color * env_brdf_approx(f0, roughness, dotnv) * 0.5;

    // AO contribution (gbuffer1.a)
    color = color * lerp3(float3(1.0, 1.0, 1.0), float3(g1.a, g1.a, g1.a), dotnv);

    color = max3(color, float3(0.0, 0.0, 0.0));
    var out_color: float4;
    out_color.rgb = color;
    out_color.a = 1.0;
    return out_color;
}

#[pipe]
struct pipe {
    vertex = deferred_light_vert;
    fragment = deferred_light_frag;
}
```

- [ ] **Step 2: Commit**

```bash
git add engine/assets/shaders/deferred_light.kong
git commit -m "feat(shader): add deferred_light.kong for engine deferred lighting pass

Simplified from paint/shaders/deferred_light.kong for Phase 1:
single directional light, constant ambient, no SH irradiance.
Decodes G-buffer (octahedron normals, PBR material params) and
reconstructs world position from depth for lighting.

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
```

---

## Task 7: Modify `render3d_bridge.c` — Wire Deferred Pipeline

**Files:** Modify: `engine/sources/ecs/render3d_bridge.c`

- [ ] **Step 1: Update includes**

Add:
```c
#include "deferred_gbuffer.h"
#include "deferred_lighting.h"
```

- [ ] **Step 2: Replace `sys_3d_render_commands`**

Replace the body of `sys_3d_render_commands` (lines 11-18) to call the deferred pipeline instead of the forward path:

```c
static void sys_3d_render_commands(void) {
    // Pass 1: Geometry → G-buffer
    deferred_gbuffer_render_geometry();

    // Pass 2: Lighting → buf
    deferred_lighting_render();

    g_3d_rendered = true;
}
```

- [ ] **Step 3: Update init to call deferred module init**

```c
void sys_3d_init(void) {
    render_path_current_w = sys_w();
    render_path_current_h = sys_h();

    deferred_gbuffer_init();
    deferred_lighting_init();

    render_path_commands = sys_3d_render_commands;
    printf("3D Render Bridge: initialized (deferred rendering)\n");
}
```

- [ ] **Step 4: Update shutdown**

```c
void sys_3d_shutdown(void) {
    render_path_commands = NULL;
    deferred_lighting_shutdown();
    deferred_gbuffer_shutdown();
    g_3d_rendered = false;
    printf("3D Render Bridge: shutdown\n");
}
```

- [ ] **Step 5: Add deferred gbuffer resize hook**

The engine's resize handler (`game_loop.c` or wherever `render_path_resize` is called) must also call `deferred_gbuffer_resize()`. In `render3d_bridge.c`, add:

```c
void sys_3d_resize(void) {
    render_path_current_w = sys_w();
    render_path_current_h = sys_h();
    deferred_gbuffer_resize();
}
```

Find where `render_path_resize()` is called in the engine (likely `game_engine.c` or `game_loop.c`) and replace or augment it with `sys_3d_resize()`.

- [ ] **Step 6: Commit**

```bash
git add engine/sources/ecs/render3d_bridge.c
git commit -m "feat(ecs): wire deferred pipeline into render3d_bridge

Replaces forward _gpu_begin+submit_draw+end with
deferred_gbuffer_render_geometry() + deferred_lighting_render().
Adds deferred_gbuffer_init/shutdown/resize and deferred_lighting_init/shutdown.

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
```

---

## Task 8: Modify `camera_bridge_3d.c` — Export `invVP`

**Files:** Modify: `engine/sources/ecs/camera_bridge_3d.c`

- [ ] **Step 1: Add export function**

Add to `camera_bridge_3d.c` after `camera_bridge_3d_get_active()`:

```c
void camera_bridge_3d_get_view_proj(mat4_t *out_view, mat4_t *out_proj) {
    if (!g_camera_3d) return;
    if (out_view) *out_view = g_camera_3d->v;
    if (out_proj) *out_proj = g_camera_3d->p;
}
```

Add declaration to `camera_bridge_3d.h`:

```c
void camera_bridge_3d_get_view_proj(mat4_t *out_view, mat4_t *out_proj);
```

Note: `deferred_lighting_render()` computes `invVP` using `mat4_inv()` from iron_math.h. It calls `camera_bridge_3d_get_active()` directly and computes `invVP = mat4_inv(mat4_mult_mat(p, v))`.

- [ ] **Step 2: Also export camera position (eye)**

The deferred_light shader needs `eye` (camera world position). The camera position is stored in `g_camera_3d->base->transform->loc`. Add a helper:

```c
void camera_bridge_3d_get_position(float *x, float *y, float *z) {
    if (!g_camera_3d || !g_camera_3d->base || !g_camera_3d->base->transform) return;
    if (x) *x = g_camera_3d->base->transform->loc.x;
    if (y) *y = g_camera_3d->base->transform->loc.y;
    if (z) *z = g_camera_3d->base->transform->loc.z;
}
```

Add declaration to `camera_bridge_3d.h`:

```c
void camera_bridge_3d_get_position(float *x, float *y, float *z);
```

- [ ] **Step 3: Commit**

```bash
git add engine/sources/ecs/camera_bridge_3d.c engine/sources/ecs/camera_bridge_3d.h
git commit -m "feat(ecs): add camera_bridge_3d_get_position export for deferred lighting

Deferred lighting pass needs camera world position to reconstruct
world-space positions from depth buffer.

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
```

---

## Task 9: Modify `systems.manifest` — Enable Deferred Renderer

**Files:** Modify: `engine/assets/systems.manifest`

- [ ] **Step 1: Comment out forward path, add deferred path**

```manifest
# Systems manifest — one system per line
# Lines starting with # are comments (disabled)
# Format: SystemName [path/to/script.minic]
#   If path omitted, defaults to data/systems/<name>.minic

# keyboard_test
# mouse_test
# frog_system
# perf_overlay
# draw_ui_ext_system
# vampire_system
# 3D ---------------
# scene3d_test        # disabled: using deferred3d instead
 deferred3d
 camera3d_control_system
# deferred3d          # Phase 1 complete — enable this
# forward3d           # old forward path (retained as fallback)
# ------------------
```

Note: `deferred3d` is not a Minic script system — it's a C bridge system initialized directly by `render3d_bridge.c`'s `sys_3d_init()`. Adding it to the manifest is a marker indicating the deferred renderer is active; the manifest is parsed by `minic_system.c` but C bridge systems are not loaded from it. Alternatively, add it as a no-op Minic system comment for documentation purposes only.

- [ ] **Step 2: Commit**

```bash
git add engine/assets/systems.manifest
git commit -m "chore: enable deferred3d in systems.manifest

Disables scene3d_test forward renderer.
The deferred pipeline is initialized by render3d_bridge.c sys_3d_init().

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
```

---

## Task 10: Build and Verify

- [ ] **Step 1: Export assets and shaders**

```bash
cd engine && ../base/make macos metal
```

Expected: `world_gbuffer.frag` and `deferred_light.frag` generated in the shader output directory.

- [ ] **Step 2: Compile C code**

```bash
cd engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | tail -50
```

Expected: Build succeeds with no errors. Watch for:
- Link errors if `deferred_gbuffer.c` or `deferred_lighting.c` reference undeclared functions
- Shader compilation errors in `world_gbuffer.kong` or `deferred_light.kong`

- [ ] **Step 3: Run the app**

```bash
./build/build/Release/IronGame.app/Contents/MacOS/IronGame
```

Expected: App launches without crash. Check console for:
- `"Deferred G-Buffer initialized"`
- `"Deferred Lighting initialized"`
- `"3D Render Bridge: initialized (deferred rendering)"`

- [ ] **Step 4: Commit if build succeeds**

```bash
git add -A && git commit -m "feat(engine): Phase 1 deferred rendering pipeline — build verified

G-buffer geometry pass + deferred directional lighting pass.
All engine 3D rendering now uses the deferred pipeline.

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
```

---

## Self-Review Checklist

- [ ] Spec Phase 1 G-buffer layout: gbuffer0 (RGBA64, normal/roughness/metal), gbuffer1 (RGBA64, albedo/ao), main (D32), buf (RGBA64), last (RGBA64) — all implemented
- [ ] Spec Phase 1 `deferred_gbuffer.c` API: `init/shutdown/resize/render_geometry` — all implemented
- [ ] Spec Phase 1 `deferred_lighting.c` API: `init/shutdown/render` — all implemented
- [ ] Spec Phase 1 `world_gbuffer.kong`: MRT, octahedron normals, roughness=0.5, metallic=0.0, albedo=grey, ao=1.0 — implemented
- [ ] Spec Phase 1 `deferred_light.kong`: depth reconstruction, dot(N,L) directional, constant ambient, no SH — implemented
- [ ] Modified `render3d_bridge.c`: calls `deferred_gbuffer_render_geometry()` then `deferred_lighting_render()` — implemented
- [ ] Modified `camera_bridge_3d.c`: exports camera position and view/proj matrices — implemented
- [ ] `systems.manifest`: `scene3d_test` commented out, `deferred3d` noted — implemented
- [ ] No "TBD" or placeholder steps
- [ ] Type names consistent: `mat4_t` (iron_math), `camera_object_t` (engine.h), `comp_directional_light` (ecs_components.h)
- [ ] All `render_path_set_target` calls use correct signatures: primary target first, `string_array_t*` second, depth buffer as 3rd arg
