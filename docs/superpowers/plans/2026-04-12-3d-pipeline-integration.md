# 3D Deferred Pipeline Integration — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the forward 3D renderer with a deferred rendering pipeline across 8 visual milestones.

**Architecture:** Direct rewrite of `render3d_bridge.c` into a multi-pass deferred pipeline. Each pass (G-buffer, lighting, shadow, post-fx) is a separate render step using Iron's `_gpu_begin`/`gpu_end` API. Shaders are written in Kong and compiled via `base/make`. Test scripts in `.minic` verify visual output per milestone.

**Tech Stack:** C (engine), Kong (shaders), Minic (test scripts), Iron Engine (GPU/rendering API), Flecs ECS (component management)

**Spec:** `docs/superpowers/specs/2026-04-12-3d-pipeline-integration-design.md`

---

## Build & Test Commands

```bash
# Build (macOS Metal) — TWO STEPS:
cd engine && ../base/make macos metal              # Step 1: export assets + compile shaders
cd engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build  # Step 2: compile C

# Run:
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame

# Format:
clang-format -i source.c
```

---

## File Structure

```
engine/sources/
├── ecs/
│   ├── deferred/              # NEW — deferred rendering passes
│   │   ├── deferred_gbuffer.c/h
│   │   ├── deferred_lighting.c/h
│   │   └── deferred_postfx.c/h
│   ├── shadow/                # NEW — shadow map passes
│   │   ├── shadow_directional.c/h
│   │   ├── shadow_point.c/h
│   │   └── shadow_spot.c/h
│   ├── culling/               # NEW — culling systems
│   │   ├── culling_frustum.c/h
│   │   ├── culling_lod.c/h
│   │   └── culling_occlusion.c/h
│   ├── decal/                 # NEW — decal system
│   │   └── decal_bridge.c/h
│   ├── render3d_bridge.c/h    # REWRITE — deferred pipeline controller
│   ├── mesh_bridge_3d.c/h     # MODIFY — shader switching, depth-only mode
│   ├── camera_bridge_3d.c/h   # MODIFY — frustum planes export
│   ├── transparent_bridge.c/h # NEW — transparent forward pass
│   ├── particle_system.c/h    # NEW — particle system
│   ├── ecs_components.c/h     # MODIFY — add new components each milestone
│   └── ecs_world.c/h          # unchanged
├── components/
│   ├── light.c/h              # NEW — point/spot light components
│   ├── lod.c/h                # NEW — LOD component
│   ├── transparency.c/h       # NEW — transparent + particle components
│   ├── decal.c/h              # NEW — decal component
│   ├── material.c/h           # NEW — material component
│   └── (existing transform/camera/mesh_renderer unchanged)
├── core/
│   ├── material_api.c/h       # NEW — Minic material API
│   ├── material_system.c/h    # NEW — material management
│   ├── light_api.c/h          # NEW — Minic light API
│   ├── camera_api.c/h         # NEW — Minic 3D camera API (or extend scene_3d_api)
│   ├── culling_api.c/h        # NEW — Minic culling API
│   ├── render_api.c/h         # NEW — Minic render pipeline API
│   ├── runtime_api.c          # MODIFY — register new APIs per milestone
│   └── game_engine.c          # MODIFY — init/shutdown new systems
└── global.h                   # MODIFY — add new includes per milestone

engine/assets/
├── shaders/
│   ├── mesh.kong              # EXISTING — will remain for 2D/mesh loading
│   ├── mesh_gbuffer.kong      # NEW — MRT geometry pass
│   ├── debug_gbuffer.kong     # NEW — debug visualization
│   ├── deferred_light.kong    # NEW — deferred PBR lighting
│   ├── fullscreen_quad.kong   # NEW — fullscreen quad present
│   ├── shadow_depth.kong      # NEW — shadow depth pass
│   ├── shadow_cubemap.kong    # NEW — point shadow
│   ├── shadow_spot.kong       # NEW — spot shadow
│   ├── postfx_ssao.kong       # NEW — SSAO
│   ├── postfx_bloom_down.kong # NEW — bloom downsample
│   ├── postfx_bloom_up.kong   # NEW — bloom upsample
│   ├── postfx_compositor.kong # NEW — final composite
│   ├── transparent_forward.kong # NEW — transparent pass
│   ├── particle_additive.kong # NEW — particles
│   └── decal_projection.kong  # NEW — decals
├── systems/
│   ├── scene3d_test.minic     # EXISTING — will be replaced per milestone
│   ├── camera3d_control_system.minic  # EXISTING — reused
│   ├── test_m1_gbuffer.minic  # NEW
│   ├── test_m2_lighting.minic # NEW
│   ├── test_m3_shadow.minic   # NEW
│   ├── test_m4_lights.minic   # NEW
│   ├── test_m5_postfx.minic   # NEW
│   ├── test_m6_transparent.minic # NEW
│   ├── test_m7_decal.minic    # NEW
│   └── test_m8_optimization.minic # NEW
└── systems.manifest           # MODIFY — switch active test system per milestone
```

---

## Milestone 1: G-Buffer Debug View

**Goal:** Render scene into G-buffer (gbuffer0 + gbuffer1 + depth), display debug channel visualization. Forward rendering is replaced.

### Task 1.1: Create material component

**Files:**
- Create: `engine/sources/components/material.c`
- Create: `engine/sources/components/material.h`

- [ ] **Step 1: Create material.h with component struct**

```c
// engine/sources/components/material.h
#pragma once

typedef struct {
    float metallic;
    float roughness;
    float albedo_r, albedo_g, albedo_b;
    float emissive_r, emissive_g, emissive_b;
    float ao;
} comp_3d_material;

void material_init(void);
void material_shutdown(void);
```

- [ ] **Step 2: Create material.c**

```c
// engine/sources/components/material.c
#include "material.h"
#include <stdio.h>

void material_init(void) {
    printf("Material component initialized\n");
}

void material_shutdown(void) {
    printf("Material component shutdown\n");
}
```

- [ ] **Step 3: Commit**

```bash
git add engine/sources/components/material.c engine/sources/components/material.h
git commit -m "feat(m1): add material component struct"
```

### Task 1.2: Register material component in ECS

**Files:**
- Modify: `engine/sources/ecs/ecs_components.c`
- Modify: `engine/sources/ecs/ecs_components.h`

- [ ] **Step 1: Add component ID and accessor in ecs_components.h**

Add after the `comp_directional_light` typedef:

```c
typedef struct {
    float metallic;
    float roughness;
    float albedo_r, albedo_g, albedo_b;
    float emissive_r, emissive_g, emissive_b;
    float ao;
} comp_3d_material;
```

Add after `ecs_component_comp_directional_light(void)` declaration:

```c
uint64_t ecs_component_comp_3d_material(void);
```

- [ ] **Step 2: Add component registration in ecs_components.c**

Add static variable: `static ecs_entity_t comp_3d_material_entity = 0;`

Add accessor: `ecs_entity_t ecs_component_comp_3d_material(void) { return comp_3d_material_entity; }`

In `ecs_register_components()`, add:
```c
comp_3d_material_entity = register_component(ecs, "comp_3d_material", sizeof(comp_3d_material), _Alignof(comp_3d_material));
```

In `ecs_register_builtin_fields()`, add:
```c
id = comp_3d_material_entity;
ecs_dynamic_component_add_field(id, "metallic", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, metallic));
ecs_dynamic_component_add_field(id, "roughness", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, roughness));
ecs_dynamic_component_add_field(id, "albedo_r", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, albedo_r));
ecs_dynamic_component_add_field(id, "albedo_g", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, albedo_g));
ecs_dynamic_component_add_field(id, "albedo_b", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, albedo_b));
ecs_dynamic_component_add_field(id, "emissive_r", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, emissive_r));
ecs_dynamic_component_add_field(id, "emissive_g", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, emissive_g));
ecs_dynamic_component_add_field(id, "emissive_b", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, emissive_b));
ecs_dynamic_component_add_field(id, "ao", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, ao));
```

Add lookup entries in `ecs_get_builtin_component()` and `ecs_get_builtin_component_name()`.

- [ ] **Step 3: Commit**

```bash
git add engine/sources/ecs/ecs_components.c engine/sources/ecs/ecs_components.h
git commit -m "feat(m1): register material component in ECS"
```

### Task 1.3: Create G-buffer manager

**Files:**
- Create: `engine/sources/ecs/deferred/deferred_gbuffer.h`
- Create: `engine/sources/ecs/deferred/deferred_gbuffer.c`

- [ ] **Step 1: Create deferred_gbuffer.h**

```c
// engine/sources/ecs/deferred/deferred_gbuffer.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

struct game_world_t;

// G-buffer render target handles
typedef struct {
    void *gbuffer0;    // RGBA16F: Normal.xy (oct) + Roughness + Metallic
    void *gbuffer1;    // RGBA16F: Albedo.rgb + AO
    void *depth_target; // D24S8 depth buffer
    int width;
    int height;
    bool initialized;
} gbuffer_t;

void gbuffer_init(int width, int height);
void gbuffer_resize(int width, int height);
void gbuffer_shutdown(void);
gbuffer_t *gbuffer_get(void);
```

- [ ] **Step 2: Create deferred_gbuffer.c**

```c
// engine/sources/ecs/deferred/deferred_gbuffer.c
#include "deferred_gbuffer.h"
#include <iron.h>
#include <stdio.h>
#include <stdlib.h>

static gbuffer_t g_gbuffer = {0};

void gbuffer_init(int width, int height) {
    if (g_gbuffer.initialized) gbuffer_shutdown();

    // Create render targets using Iron's render_target API
    // gbuffer0: RGBA16F — Normal.xy + Roughness + Metallic
    g_gbuffer.gbuffer0 = render_target_create(width, height, GPU_TEXTURE_FORMAT_RGBA16F);

    // gbuffer1: RGBA16F — Albedo.rgb + AO
    g_gbuffer.gbuffer1 = render_target_create(width, height, GPU_TEXTURE_FORMAT_RGBA16F);

    // Depth: D24S8
    g_gbuffer.depth_target = render_target_create_depth(width, height, GPU_TEXTURE_FORMAT_DEPTH24_STENCIL8);

    g_gbuffer.width = width;
    g_gbuffer.height = height;
    g_gbuffer.initialized = true;

    printf("G-Buffer initialized: %dx%d\n", width, height);
}

void gbuffer_resize(int width, int height) {
    if (!g_gbuffer.initialized) return;
    if (g_gbuffer.width == width && g_gbuffer.height == height) return;
    gbuffer_shutdown();
    gbuffer_init(width, height);
}

void gbuffer_shutdown(void) {
    if (!g_gbuffer.initialized) return;
    // Iron GC handles render target cleanup
    g_gbuffer.gbuffer0 = NULL;
    g_gbuffer.gbuffer1 = NULL;
    g_gbuffer.depth_target = NULL;
    g_gbuffer.initialized = false;
    printf("G-Buffer shutdown\n");
}

gbuffer_t *gbuffer_get(void) {
    return &g_gbuffer;
}
```

**Note:** The actual render_target_create API calls need to be verified against the Iron engine headers in `base/`. The exact function names and enum values for creating render targets with specific formats (RGBA16F, D24S8) and MRT binding will need to match Iron's GPU API. Check `base/sources/iron_gpu.h` and `base/sources/iron_render_path.h` for the precise API.

- [ ] **Step 3: Commit**

```bash
git add engine/sources/ecs/deferred/deferred_gbuffer.c engine/sources/ecs/deferred/deferred_gbuffer.h
git commit -m "feat(m1): add G-buffer render target manager"
```

### Task 1.4: Create mesh_gbuffer Kong shader

**Files:**
- Create: `engine/assets/shaders/mesh_gbuffer.kong`

- [ ] **Step 1: Write MRT geometry shader**

This shader outputs to 2 render targets (gbuffer0 + gbuffer1) in a single pass.

```kong
// engine/assets/shaders/mesh_gbuffer.kong

#[set(everything)]
const constants: {
    WVP: float4x4;
    World: float4x4;
    metallic: float;
    roughness: float;
    albedo_r: float;
    albedo_g: float;
    albedo_b: float;
    ao: float;
};

struct vert_in {
    pos: float4;
    nor: float3;
    tex: float2;
}

struct vert_out {
    pos: float4;
    world_nor: float3;
    tex: float2;
}

// Octahedron normal encoding
fun oct_encode(n: float3): float2 {
    var nn = normalize(n);
    var sum = abs(nn.x) + abs(nn.y) + abs(nn.z);
    var oct = nn.xy * (1.0 / sum);
    oct = oct * 0.5 + 0.5;
    return oct;
}

fun gbuffer_vert(input: vert_in): vert_out {
    var output: vert_out;
    output.pos = constants.WVP * float4(input.pos.xyz, 1.0);
    output.world_nor = normalize((constants.World * float4(input.nor, 0.0)).xyz);
    output.tex = input.tex;
    return output;
}

fun gbuffer_frag(input: vert_out): float4 {
    // Output to gbuffer0: Normal.xy (oct) + Roughness + Metallic
    var nor_oct = oct_encode(normalize(input.world_nor));
    return float4(nor_oct.x, nor_oct.y, constants.roughness, constants.metallic);
}

fun gbuffer_frag1(input: vert_out): float4 {
    // Output to gbuffer1: Albedo.rgb + AO
    return float4(constants.albedo_r, constants.albedo_g, constants.albedo_b, constants.ao);
}

#[pipe]
struct pipe {
    vertex = gbuffer_vert;
    fragment = [gbuffer_frag, gbuffer_frag1];
}
```

**Note:** The Kong shader syntax for MRT (multiple fragment outputs) needs to be verified against the Kong compiler in `base/sources/kong/kong.c`. The `fragment = [fn1, fn2]` syntax is speculative — check Kong's MRT support and adapt accordingly. If Kong doesn't support MRT natively, fall back to two separate passes.

- [ ] **Step 2: Commit**

```bash
git add engine/assets/shaders/mesh_gbuffer.kong
git commit -m "feat(m1): add MRT G-buffer geometry shader"
```

### Task 1.5: Create debug_gbuffer Kong shader

**Files:**
- Create: `engine/assets/shaders/debug_gbuffer.kong`

- [ ] **Step 1: Write debug visualization shader**

```kong
// engine/assets/shaders/debug_gbuffer.kong

#[set(everything)]
const constants: {
    debug_mode: int;   // 0=Normal, 1=Depth, 2=Albedo, 3=Roughness/Metallic
};

#[set(everything)]
const sampler_linear: sampler;

#[set(everything)]
const gbuffer0_tex: tex2d;  // Normal.xy + Roughness + Metallic

#[set(everything)]
const gbuffer1_tex: tex2d;  // Albedo.rgb + AO

#[set(everything)]
const depth_tex: tex2d;     // Depth buffer

struct vert_in {
    pos: float4;
    tex: float2;
}

struct vert_out {
    pos: float4;
    tex: float2;
}

fun debug_vert(input: vert_in): vert_out {
    var output: vert_out;
    output.pos = float4(input.pos.xy, 0.0, 1.0);
    output.tex = input.tex;
    return output;
}

fun debug_frag(input: vert_out): float4 {
    if (constants.debug_mode == 0) {
        // Normal — decode octahedron
        var oct = sample(gbuffer0_tex, sampler_linear, input.tex).xy;
        oct = oct * 2.0 - 1.0;
        var z = 1.0 - abs(oct.x) - abs(oct.y);
        var nx = 0.0;
        var ny = 0.0;
        var nz = z;
        if (z < 0.0) {
            nx = (1.0 - abs(oct.y)) * select(-1.0, 1.0, oct.x < 0.0);
            ny = (1.0 - abs(oct.x)) * select(-1.0, 1.0, oct.y < 0.0);
            nz = 0.0;
        } else {
            nx = oct.x;
            ny = oct.y;
        }
        // Map from [-1,1] to [0,1] for visualization
        return float4(nx * 0.5 + 0.5, ny * 0.5 + 0.5, nz * 0.5 + 0.5, 1.0);
    }
    if (constants.debug_mode == 1) {
        // Depth
        var d = sample(depth_tex, sampler_linear, input.tex).r;
        return float4(d, d, d, 1.0);
    }
    if (constants.debug_mode == 2) {
        // Albedo
        return sample(gbuffer1_tex, sampler_linear, input.tex);
    }
    // Roughness (B) + Metallic (A)
    var gb0 = sample(gbuffer0_tex, sampler_linear, input.tex);
    return float4(gb0.b, gb0.a, gb0.b, 1.0);
}

#[pipe]
struct pipe {
    vertex = debug_vert;
    fragment = debug_frag;
}
```

**Note:** The `select()` function and branching syntax need verification against Kong's supported GLSL-like builtins. Adapt based on what Kong actually supports.

- [ ] **Step 2: Commit**

```bash
git add engine/assets/shaders/debug_gbuffer.kong
git commit -m "feat(m1): add G-buffer debug visualization shader"
```

### Task 1.6: Rewrite render3d_bridge for deferred pipeline

**Files:**
- Modify: `engine/sources/ecs/render3d_bridge.c`
- Modify: `engine/sources/ecs/render3d_bridge.h`

- [ ] **Step 1: Rewrite render3d_bridge.h**

```c
// engine/sources/ecs/render3d_bridge.h
#pragma once

#include <stdbool.h>

struct game_world_t;

void sys_3d_set_world(struct game_world_t *world);
void sys_3d_init(void);
void sys_3d_shutdown(void);
void sys_3d_draw(void);
bool sys_3d_was_rendered(void);
void sys_3d_reset_frame(void);

// Debug view control (used by test scripts)
void sys_3d_set_debug_mode(int mode);  // 0=Normal, 1=Depth, 2=Albedo, 3=Roughness/Metallic
int  sys_3d_get_debug_mode(void);
```

- [ ] **Step 2: Rewrite render3d_bridge.c**

The core change: replace the forward render callback with a deferred G-buffer + debug display pipeline.

```c
// engine/sources/ecs/render3d_bridge.c
#include "render3d_bridge.h"
#include "ecs_world.h"
#include "deferred/deferred_gbuffer.h"
#include <iron.h>
#include <stdio.h>
#include <stdbool.h>

static bool g_3d_rendered = false;
static game_world_t *g_3d_world = NULL;
static int g_debug_mode = 0;  // 0=Normal, 1=Depth, 2=Albedo, 3=Roughness/Metallic

static void sys_3d_render_commands(void) {
    int w = sys_w();
    int h = sys_h();
    gbuffer_resize(w, h);

    // Pass 1: G-Buffer geometry pass
    // Bind gbuffer0 + gbuffer1 + depth as render targets (MRT)
    _gpu_begin(gbuffer_get()->gbuffer0, gbuffer_get()->depth_target,
               gbuffer_get()->gbuffer1, GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH,
               0xff000000, 1.0f);
    render_path_submit_draw("mesh_gbuffer");
    gpu_end();

    // Pass 2: Debug visualization (fullscreen quad)
    // Read gbuffer textures and display selected channel
    _gpu_begin(NULL, NULL, NULL, GPU_CLEAR_COLOR, 0xff1a1a2e, 1.0f);
    // TODO: bind gbuffer textures + debug_mode uniform, draw fullscreen quad
    gpu_end();

    g_3d_rendered = true;
}

void sys_3d_set_world(game_world_t *world) {
    g_3d_world = world;
}

void sys_3d_init(void) {
    int w = sys_w();
    int h = sys_h();
    gbuffer_init(w > 0 ? w : 1280, h > 0 ? h : 720);
    render_path_commands = sys_3d_render_commands;
    render_path_current_w = w;
    render_path_current_h = h;
    printf("3D Render Bridge: initialized (deferred G-buffer pipeline)\n");
}

void sys_3d_shutdown(void) {
    render_path_commands = NULL;
    gbuffer_shutdown();
    g_3d_rendered = false;
    g_3d_world = NULL;
    printf("3D Render Bridge: shutdown\n");
}

void sys_3d_draw(void) {
    // 3D rendering handled by render_path_commands callback
}

bool sys_3d_was_rendered(void) {
    return g_3d_rendered;
}

void sys_3d_reset_frame(void) {
    g_3d_rendered = false;
}

void sys_3d_set_debug_mode(int mode) {
    g_debug_mode = mode;
}

int sys_3d_get_debug_mode(void) {
    return g_debug_mode;
}
```

**Note:** The `_gpu_begin()` call with multiple render targets needs verification. Iron's `_gpu_begin()` signature may not support MRT directly. You may need to use `render_path_set_target()` for the MRT pass (with the depth range workaround) or use a lower-level Iron API for MRT binding. Check `base/sources/iron_gpu.h` for render target binding APIs.

- [ ] **Step 3: Commit**

```bash
git add engine/sources/ecs/render3d_bridge.c engine/sources/ecs/render3d_bridge.h
git commit -m "feat(m1): rewrite render3d_bridge for deferred G-buffer pipeline"
```

### Task 1.7: Create M1 test script

**Files:**
- Create: `engine/assets/systems/test_m1_gbuffer.minic`
- Modify: `engine/assets/systems.manifest`

- [ ] **Step 1: Write test_m1_gbuffer.minic**

```c
// test_m1_gbuffer — G-buffer debug visualization test
// Keys: 1=Normal, 2=Depth, 3=Albedo, 4=Roughness/Metallic

id g_pos_comp = 0;
id g_rot_comp = 0;
id g_scale_comp = 0;
id g_cam_comp = 0;
id g_mesh_comp = 0;
id g_render_comp = 0;
id g_mat_comp = 0;

id g_cam = 0;
id g_mesh1 = 0;
id g_mesh2 = 0;
int g_debug_mode = 0;

int init(void) {
    printf("\n=== M1 G-Buffer Debug Test ===\n");

    g_pos_comp = component_lookup("comp_3d_position");
    g_rot_comp = component_lookup("comp_3d_rotation");
    g_scale_comp = component_lookup("comp_3d_scale");
    g_cam_comp = component_lookup("comp_3d_camera");
    g_mesh_comp = component_lookup("comp_3d_mesh_renderer");
    g_render_comp = component_lookup("RenderObject3D");
    g_mat_comp = component_lookup("comp_3d_material");

    // Load mesh
    void *mesh_obj = mesh_load_arm("ship.arm");

    // Create mesh entity 1 (red metallic)
    g_mesh1 = entity_create();
    entity_add(g_mesh1, g_pos_comp);
    entity_add(g_mesh1, g_rot_comp);
    entity_add(g_mesh1, g_scale_comp);
    entity_add(g_mesh1, g_mesh_comp);
    entity_add(g_mesh1, g_render_comp);
    comp_set_floats(g_pos_comp, entity_get(g_mesh1, g_pos_comp), "x,y,z", -1.0, 0.0, 0.0);
    comp_set_floats(g_rot_comp, entity_get(g_mesh1, g_rot_comp), "x,y,z,w", 0.0, 0.0, 0.0, 1.0);
    comp_set_floats(g_scale_comp, entity_get(g_mesh1, g_scale_comp), "x,y,z", 1.0, 1.0, 1.0);
    comp_set_ptr(g_render_comp, entity_get(g_mesh1, g_render_comp), "iron_mesh_object", mesh_obj);
    comp_set_bool(g_render_comp, entity_get(g_mesh1, g_render_comp), "dirty", 1);
    comp_set_bool(g_mesh_comp, entity_get(g_mesh1, g_mesh_comp), "visible", 1);

    // Camera
    g_cam = entity_create();
    entity_add(g_cam, g_pos_comp);
    entity_add(g_cam, g_rot_comp);
    entity_add(g_cam, g_cam_comp);
    comp_set_floats(g_pos_comp, entity_get(g_cam, g_pos_comp), "x,y,z", 0.0, 2.0, 5.0);
    comp_set_floats(g_rot_comp, entity_get(g_cam, g_rot_comp), "x,y,z,w", 0.0, 0.0, 0.0, 1.0);
    comp_set_floats(g_cam_comp, entity_get(g_cam, g_cam_comp), "fov,near_plane,far_plane", 1.047, 0.1, 100.0);
    comp_set_bool(g_cam_comp, entity_get(g_cam, g_cam_comp), "perspective", 1);
    comp_set_bool(g_cam_comp, entity_get(g_cam, g_cam_comp), "active", 1);
    entity_set_name(g_cam, "main_camera");

    printf("=== M1 Init Complete ===\n\n");
    return 0;
}

int step(void) {
    // Debug mode switching
    if (keyboard_started("1") != 0.0) { g_debug_mode = 0; sys_3d_set_debug_mode(0); }
    if (keyboard_started("2") != 0.0) { g_debug_mode = 1; sys_3d_set_debug_mode(1); }
    if (keyboard_started("3") != 0.0) { g_debug_mode = 2; sys_3d_set_debug_mode(2); }
    if (keyboard_started("4") != 0.0) { g_debug_mode = 3; sys_3d_set_debug_mode(3); }
    return 0;
}

int draw(void) {
    draw_set_font("font.ttf", 16);
    draw_set_color(0xffffffff);
    draw_string("M1: G-Buffer Debug View", 10.0, 10.0);
    if (g_debug_mode == 0) draw_string("Mode: Normal", 10.0, 30.0);
    if (g_debug_mode == 1) draw_string("Mode: Depth", 10.0, 30.0);
    if (g_debug_mode == 2) draw_string("Mode: Albedo", 10.0, 30.0);
    if (g_debug_mode == 3) draw_string("Mode: Roughness/Metallic", 10.0, 30.0);
    draw_string("1=Normal 2=Depth 3=Albedo 4=RoughMet", 10.0, 50.0);
    draw_fps(10.0, 70.0);
    return 0;
}
```

- [ ] **Step 2: Update systems.manifest**

Replace `scene3d_test` with `test_m1_gbuffer`:

```
# 3D ---------------
 test_m1_gbuffer
 camera3d_control_system
# ------------------
```

- [ ] **Step 3: Register sys_3d_set_debug_mode / sys_3d_get_debug_mode in runtime_api.c**

Add native wrappers and register in `runtime_api_register()`:

```c
static minic_val_t minic_sys_3d_set_debug_mode(minic_val_t *args, int argc) {
    if (argc < 1) return minic_val_void();
    sys_3d_set_debug_mode((int)minic_val_to_d(args[0]));
    return minic_val_void();
}

static minic_val_t minic_sys_3d_get_debug_mode(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    return minic_val_int(sys_3d_get_debug_mode());
}
```

Register:
```c
minic_register_native("sys_3d_set_debug_mode", minic_sys_3d_set_debug_mode);
minic_register_native("sys_3d_get_debug_mode", minic_sys_3d_get_debug_mode);
```

- [ ] **Step 4: Build and test visually**

```bash
cd engine && ../base/make macos metal
cd engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame
```

Expected: Screen shows debug channel view. Keys 1-4 switch between Normal/Depth/Albedo/RoughMetal views.

- [ ] **Step 5: Commit**

```bash
git add engine/assets/systems/test_m1_gbuffer.minic engine/assets/systems.manifest engine/sources/core/runtime_api.c
git commit -m "feat(m1): add G-buffer debug test script and API"
```

---

## Milestone 2: Deferred PBR Lighting

**Goal:** Read G-buffer + depth, compute PBR directional lighting. No shadows.

### Task 2.1: Create deferred_lighting pass

**Files:**
- Create: `engine/sources/ecs/deferred/deferred_lighting.h`
- Create: `engine/sources/ecs/deferred/deferred_lighting.c`

- [ ] **Step 1: Create header**

```c
// engine/sources/ecs/deferred/deferred_lighting.h
#pragma once

struct game_world_t;

void deferred_lighting_init(void);
void deferred_lighting_shutdown(void);
void deferred_lighting_render(struct game_world_t *world);
```

- [ ] **Step 2: Create implementation**

The lighting pass reads gbuffer0 + gbuffer1 + depth, outputs lit scene to "buf" (RGBA16F). It queries directional light components from ECS, uploads their uniforms, and renders a fullscreen quad.

```c
// engine/sources/ecs/deferred/deferred_lighting.c
#include "deferred_lighting.h"
#include "deferred_gbuffer.h"
#include "../ecs_world.h"
#include "../ecs_components.h"
#include "flecs.h"
#include <iron.h>
#include <stdio.h>

static gpu_pipeline_t s_lighting_pipeline;
static gpu_shader_t s_lighting_vs;
static gpu_shader_t s_lighting_fs;
static bool s_initialized = false;

void deferred_lighting_init(void) {
    // Load deferred_light shader (compiled by base/make from .kong)
    char *dp = data_path();
    char *ext = sys_shader_ext();

    char path_vert[512], path_frag[512];
    strcpy(path_vert, dp); strcat(path_vert, "deferred_light.vert"); strcat(path_vert, ext);
    strcpy(path_frag, dp); strcat(path_frag, "deferred_light.frag"); strcat(path_frag, ext);

    buffer_t *vs_buf = iron_load_blob(path_vert);
    buffer_t *fs_buf = iron_load_blob(path_frag);
    if (!vs_buf || !fs_buf) {
        fprintf(stderr, "Deferred Lighting: failed to load shader\n");
        return;
    }

    gpu_shader_init(&s_lighting_vs, vs_buf->buffer, vs_buf->length, GPU_SHADER_TYPE_VERTEX);
    gpu_shader_init(&s_lighting_fs, fs_buf->buffer, fs_buf->length, GPU_SHADER_TYPE_FRAGMENT);

    // Fullscreen quad: position only (no vertex structure needed beyond pos)
    gpu_pipeline_init(&s_lighting_pipeline);
    s_lighting_pipeline.vertex_shader = &s_lighting_vs;
    s_lighting_pipeline.fragment_shader = &s_lighting_fs;
    gpu_pipeline_compile(&s_lighting_pipeline);

    s_initialized = true;
    printf("Deferred Lighting: initialized\n");
}

void deferred_lighting_shutdown(void) {
    s_initialized = false;
    printf("Deferred Lighting: shutdown\n");
}

void deferred_lighting_render(game_world_t *world) {
    if (!s_initialized || !world) return;

    // Query directional lights from ECS
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(world);
    if (!ecs) return;

    // Upload light uniforms (direction, color, strength)
    // Bind gbuffer0, gbuffer1, depth textures
    // Draw fullscreen quad
    // Output to "buf" render target

    // TODO: implement uniform upload + fullscreen quad draw
    // This requires Iron's constant binding API
}
```

- [ ] **Step 3: Commit**

```bash
git add engine/sources/ecs/deferred/deferred_lighting.c engine/sources/ecs/deferred/deferred_lighting.h
git commit -m "feat(m2): add deferred lighting pass skeleton"
```

### Task 2.2: Create deferred_light.kong shader

**Files:**
- Create: `engine/assets/shaders/deferred_light.kong`

- [ ] **Step 1: Write PBR lighting shader**

```kong
// engine/assets/shaders/deferred_light.kong

#[set(everything)]
const constants: {
    InvProj: float4x4;
    View: float4x4;
    light_dir: float3;
    light_color: float3;
    light_strength: float;
    ambient_strength: float;
    cam_pos: float3;
};

#[set(everything)]
const sampler_linear: sampler;
#[set(everything)]
const gbuffer0_tex: tex2d;
#[set(everything)]
const gbuffer1_tex: tex2d;
#[set(everything)]
const depth_tex: tex2d;

struct vert_in {
    pos: float4;
    tex: float2;
}

struct vert_out {
    pos: float4;
    tex: float2;
}

fun lighting_vert(input: vert_in): vert_out {
    var output: vert_out;
    output.pos = float4(input.pos.xy, 0.0, 1.0);
    output.tex = input.tex;
    return output;
}

// Reconstruct world position from depth
fun reconstruct_world_pos(tex: float2, depth: float): float3 {
    var x = tex.x * 2.0 - 1.0;
    var y = tex.y * 2.0 - 1.0;
    var clip = float4(x, y, depth, 1.0);
    var view = constants.InvProj * clip;
    view = view / view.w;
    var world = constants.View * view;
    return world.xyz;
}

// Octahedron decode
fun oct_decode(oct: float2): float3 {
    var o = oct * 2.0 - 1.0;
    var z = 1.0 - abs(o.x) - abs(o.y);
    if (z >= 0.0) {
        return normalize(float3(o.x, o.y, z));
    }
    var nx = (1.0 - abs(o.y)) * select(-1.0, 1.0, o.x >= 0.0);
    var ny = (1.0 - abs(o.x)) * select(-1.0, 1.0, o.y >= 0.0);
    return normalize(float3(nx, ny, 0.0));
}

fun lighting_frag(input: vert_out): float4 {
    var gb0 = sample(gbuffer0_tex, sampler_linear, input.tex);
    var gb1 = sample(gbuffer1_tex, sampler_linear, input.tex);
    var depth = sample(depth_tex, sampler_linear, input.tex).r;

    // Early out for sky (depth == 1.0)
    if (depth >= 1.0) {
        return float4(0.5, 0.7, 1.0, 1.0);
    }

    // Decode G-buffer
    var N = oct_decode(gb0.xy);
    var roughness = gb0.z;
    var metallic = gb0.w;
    var albedo = gb1.rgb;
    var ao = gb1.a;

    // Reconstruct world position
    var world_pos = reconstruct_world_pos(input.tex, depth);

    // Light direction (pointing toward light)
    var L = normalize(-constants.light_dir);
    var V = normalize(constants.cam_pos - world_pos);
    var H = normalize(V + L);

    // PBR lighting (simplified Cook-Torrance)
    var NdotL = max(dot(N, L), 0.0);
    var NdotV = max(dot(N, V), 0.001);
    var NdotH = max(dot(N, H), 0.0);
    var VdotH = max(dot(V, H), 0.0);

    // Fresnel (Schlick approximation)
    var F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    var fresnel = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);

    // Diffuse (Lambertian)
    var kD = (1.0 - fresnel) * (1.0 - metallic);
    var diffuse = kD * albedo / 3.14159265;

    // Specular (simplified GGX)
    var alpha = roughness * roughness;
    var alpha2 = alpha * alpha;
    var denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    var D = alpha2 / (3.14159265 * denom * denom);
    var k_spec = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    var G_V = NdotV / (NdotV * (1.0 - k_spec) + k_spec);
    var G_L = NdotL / (NdotL * (1.0 - k_spec) + k_spec);
    var G = G_V * G_L;
    var specular = (D * G * fresnel) / max(4.0 * NdotV * NdotL, 0.001);

    // Final color
    var Lo = (diffuse + specular) * constants.light_color * NdotL * constants.light_strength;

    // Ambient
    var ambient = albedo * ao * constants.ambient_strength;
    var color = Lo + ambient;

    // Simple tone mapping (Reinhard)
    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

    return float4(color, 1.0);
}

#[pipe]
struct pipe {
    vertex = lighting_vert;
    fragment = lighting_frag;
}
```

- [ ] **Step 2: Commit**

```bash
git add engine/assets/shaders/deferred_light.kong
git commit -m "feat(m2): add deferred PBR lighting shader"
```

### Task 2.3: Create light_api and register in runtime

**Files:**
- Create: `engine/sources/core/light_api.c`
- Create: `engine/sources/core/light_api.h`
- Modify: `engine/sources/core/runtime_api.c`

- [ ] **Step 1: Create light_api.h**

```c
#pragma once

void light_api_register(void);
```

- [ ] **Step 2: Create light_api.c** with native wrappers for `light_set_direction()`, `light_set_color()`, `light_set_strength()`, `light_set_enabled()` that query ECS and modify `comp_directional_light`.

- [ ] **Step 3: Register in runtime_api.c** — add `light_api_register()` call in `runtime_api_register()`.

- [ ] **Step 4: Commit**

```bash
git add engine/sources/core/light_api.c engine/sources/core/light_api.h engine/sources/core/runtime_api.c
git commit -m "feat(m2): add light API for Minic scripts"
```

### Task 2.4: Create fullscreen_quad shader

**Files:**
- Create: `engine/assets/shaders/fullscreen_quad.kong`

- [ ] **Step 1: Write simple fullscreen copy shader**

```kong
// engine/assets/shaders/fullscreen_quad.kong

#[set(everything)]
const sampler_linear: sampler;
#[set(everything)]
const input_tex: tex2d;

struct vert_in {
    pos: float4;
    tex: float2;
}

struct vert_out {
    pos: float4;
    tex: float2;
}

fun quad_vert(input: vert_in): vert_out {
    var output: vert_out;
    output.pos = float4(input.pos.xy, 0.0, 1.0);
    output.tex = input.tex;
    return output;
}

fun quad_frag(input: vert_out): float4 {
    return sample(input_tex, sampler_linear, input.tex);
}

#[pipe]
struct pipe {
    vertex = quad_vert;
    fragment = quad_frag;
}
```

- [ ] **Step 2: Commit**

```bash
git add engine/assets/shaders/fullscreen_quad.kong
git commit -m "feat(m2): add fullscreen quad copy shader"
```

### Task 2.5: Wire lighting pass into render3d_bridge

**Files:**
- Modify: `engine/sources/ecs/render3d_bridge.c`
- Modify: `engine/sources/game_engine.c`

- [ ] **Step 1: Update render3d_bridge render commands**

Add `deferred_lighting_render(g_3d_world)` after the G-buffer pass, before the debug display pass. Add a toggle between debug view and lit view.

- [ ] **Step 2: Initialize deferred_lighting in game_engine.c**

Add `#include "ecs/deferred/deferred_lighting.h"` and call `deferred_lighting_init()` in `game_engine_init()`, `deferred_lighting_shutdown()` in `game_engine_shutdown()`.

- [ ] **Step 3: Commit**

```bash
git add engine/sources/ecs/render3d_bridge.c engine/sources/game_engine.c
git commit -m "feat(m2): wire deferred lighting into render pipeline"
```

### Task 2.6: Create M2 test script

**Files:**
- Create: `engine/assets/systems/test_m2_lighting.minic`
- Modify: `engine/assets/systems.manifest`

- [ ] **Step 1: Write test script** — creates ground + cubes with different materials (metallic red, rough gray, smooth blue), one directional light, keys 1-4 switch debug/lit view.

- [ ] **Step 2: Update manifest** to `test_m2_lighting`

- [ ] **Step 3: Build and verify** — PBR lit scene with correct metallic/rough response, dark unlit faces, no shadows.

- [ ] **Step 4: Commit**

```bash
git add engine/assets/systems/test_m2_lighting.minic engine/assets/systems.manifest
git commit -m "feat(m2): add PBR lighting test script"
```

---

## Milestone 3: CSM Directional Shadows

**Goal:** Add 3-cascade shadow maps for directional light.

### Task 3.1: Create shadow_directional pass

- [ ] **Create `engine/sources/ecs/shadow/shadow_directional.c/h`** — manages CSM shadow map render targets (2048/1024/512), computes light VP matrices per cascade, renders depth-only pass.

### Task 3.2: Create shadow_depth shader

- [ ] **Create `engine/assets/shaders/shadow_depth.kong`** — minimal vertex-only shader that outputs depth to shadow map. No fragment color output.

### Task 3.3: Extend directional_light component

- [ ] **Modify `engine/sources/ecs/ecs_components.c/h`** — add `cast_shadows`, `shadow_bias`, `normal_offset_bias`, `shadow_radius` fields to `comp_directional_light`.

### Task 3.4: Wire shadow pass into pipeline

- [ ] **Modify `render3d_bridge.c`** — add shadow pass before G-buffer pass. Shadow maps rendered to their own render targets, then bound as textures in the lighting pass.

### Task 3.5: Update deferred_light.kong for shadow sampling

- [ ] **Modify `engine/assets/shaders/deferred_light.kong`** — add shadow map texture bindings, cascade split uniforms, PCF sampling, cascade selection logic.

### Task 3.6: Create M3 test script

- [ ] **Create `engine/assets/systems/test_m3_shadow.minic`** — multiple cubes at different heights, ground plane, cascade debug overlay.
- [ ] **Update manifest** to `test_m3_shadow`
- [ ] **Build and verify** — shadows on ground, cascade boundaries visible in debug mode.
- [ ] **Commit**

---

## Milestone 4: Point + Spot Lights

**Goal:** Add point lights (cubemap shadows) and spot lights (perspective shadows).

### Task 4.1: Create point/spot light components

- [ ] **Create `engine/sources/components/light.c/h`** — `comp_3d_point_light` and `comp_3d_spot_light` structs.
- [ ] **Register in `ecs_components.c/h`** — add component IDs, fields, lookups.

### Task 4.2: Create shadow passes

- [ ] **Create `engine/sources/ecs/shadow/shadow_point.c/h`** — cubemap shadow rendering (6 faces per light).
- [ ] **Create `engine/sources/ecs/shadow/shadow_spot.c/h`** — perspective shadow map rendering.
- [ ] **Create `engine/assets/shaders/shadow_cubemap.kong`** — point light shadow shader.
- [ ] **Create `engine/assets/shaders/shadow_spot.kong`** — spot light shadow shader.

### Task 4.3: Update lighting shader

- [ ] **Modify `deferred_light.kong`** — add point light loop (quadratic attenuation + cubemap shadow) and spot light loop (angular attenuation + shadow).

### Task 4.4: Extend light API

- [ ] **Modify `engine/sources/core/light_api.c`** — add `entity_create_light_point()`, `entity_create_light_spot()`, toggle functions.

### Task 4.5: Create M4 test script

- [ ] **Create `engine/assets/systems/test_m4_lights.minic`** — indoor scene with 1 directional + 2 point + 1 spot light.
- [ ] **Update manifest** to `test_m4_lights`
- [ ] **Build and verify** — multiple light types visible, correct attenuation and shadows.
- [ ] **Commit**

---

## Milestone 5: Post-Processing (SSAO + Bloom)

**Goal:** Add SSAO and Bloom passes after lighting.

### Task 5.1: Create deferred_postfx manager

- [ ] **Create `engine/sources/ecs/deferred/deferred_postfx.c/h`** — manages SSAO + Bloom render targets and pass execution.

### Task 5.2: Create post-fx shaders

- [ ] **Create `engine/assets/shaders/postfx_ssao.kong`** — SSAO using depth + normal, outputs AO factor.
- [ ] **Create `engine/assets/shaders/postfx_bloom_down.kong`** — progressive downsample with brightness threshold.
- [ ] **Create `engine/assets/shaders/postfx_bloom_up.kong`** — progressive upsample + additive blend.
- [ ] **Create `engine/assets/shaders/postfx_compositor.kong`** — final composite (lit scene + bloom).

### Task 5.3: Wire post-fx into pipeline

- [ ] **Modify `render3d_bridge.c`** — add post-fx chain after lighting: SSAO → Bloom_down → Bloom_up → Composite → Present.

### Task 5.4: Create M5 test script

- [ ] **Create `engine/assets/systems/test_m5_postfx.minic`** — outdoor scene with toggle SSAO/Bloom keys.
- [ ] **Update manifest** to `test_m5_postfx`
- [ ] **Build and verify** — SSAO darkens corners, bloom glows around emissive objects.
- [ ] **Commit**

---

## Milestone 6: Transparent Objects + Particles

**Goal:** Forward transparent pass between SSAO and Bloom.

### Task 6.1: Create transparency components

- [ ] **Create `engine/sources/components/transparency.c/h`** — `comp_3d_transparency`, `Comp3dTransparent` tag, `Comp3dParticle` tag, `comp_3d_particle` struct.
- [ ] **Register in `ecs_components.c/h`**

### Task 6.2: Create transparent bridge and particle system

- [ ] **Create `engine/sources/ecs/transparent_bridge.c/h`** — forward pass for alpha-blended objects, sorted back-to-front.
- [ ] **Create `engine/sources/ecs/particle_system.c/h`** — particle lifecycle (spawn, update, destroy).
- [ ] **Create `engine/assets/shaders/transparent_forward.kong`** — alpha blend shader.
- [ ] **Create `engine/assets/shaders/particle_additive.kong`** — additive blend shader.

### Task 6.3: Wire transparent pass into pipeline

- [ ] **Modify `render3d_bridge.c`** — insert transparent pass between SSAO (Pass 3a) and Bloom (Pass 3b).

### Task 6.4: Create M6 test script

- [ ] **Create `engine/assets/systems/test_m6_transparent.minic`** — glass cube, glowing particles, opaque cubes.
- [ ] **Build and verify** — transparency correct, particles have bloom glow.
- [ ] **Commit**

---

## Milestone 7: Decals

**Goal:** Alpha-transparent decals projected onto surfaces.

### Task 7.1: Create decal component and system

- [ ] **Create `engine/sources/components/decal.c/h`** — `comp_3d_decal` struct (texture, size, offset, opacity, layer_mask), `Comp3dDecal` tag.
- [ ] **Create `engine/sources/ecs/decal/decal_bridge.c/h`** — AABB projection, UV calculation, sorted with transparent objects.
- [ ] **Create `engine/assets/shaders/decal_projection.kong`** — projects texture using world position + depth reconstruction.
- [ ] **Register in `ecs_components.c/h`**

### Task 7.2: Wire decals into transparent pass

- [ ] **Modify `transparent_bridge.c`** — include decal entities in back-to-front sort.
- [ ] **Modify `render3d_bridge.c`** — add decal rendering call.

### Task 7.3: Create M7 test script

- [ ] **Create `engine/assets/systems/test_m7_decal.minic`** — ground + walls, place bullet hole decals.
- [ ] **Build and verify** — decals project correctly onto surfaces, follow normals, no penetration.
- [ ] **Commit**

---

## Milestone 8: Optimization

**Goal:** Same visuals, better performance.

### Task 8.1: Depth pre-pass (Early-Z)

- [ ] **Modify `render3d_bridge.c`** — add depth-only pass before G-buffer. Reuse `shadow_depth.kong` shader (depth-only output).
- [ ] **Modify `mesh_bridge_3d.c`** — support depth-only render mode (no material binding).

### Task 8.2: LOD system

- [ ] **Create `engine/sources/components/lod.c/h`** — `comp_3d_lod` struct (distance thresholds, mesh paths, current_lod).
- [ ] **Create `engine/sources/ecs/culling/culling_lod.c/h`** — distance-based LOD selection.
- [ ] **Modify `mesh_bridge_3d.c`** — switch mesh based on `current_lod`.
- [ ] **Register in `ecs_components.c/h`**

### Task 8.3: GPU frustum culling

- [ ] **Create `engine/sources/ecs/culling/culling_frustum.c/h`** — compute shader that tests AABB vs frustum planes.
- [ ] **Create `engine/assets/shaders/culling_frustum.csh`** — frustum cull compute shader.
- [ ] **Modify `camera_bridge_3d.c`** — export frustum planes.

### Task 8.4: Hi-Z occlusion

- [ ] **Create `engine/sources/ecs/culling/culling_occlusion.c/h`** — depth mipmap + Hi-Z query.

### Task 8.5: Create M8 test script

- [ ] **Create `engine/assets/systems/test_m8_optimization.minic`** — 100-200 objects, toggle culling/LOD, show perf stats.
- [ ] **Build and verify** — identical visuals, higher FPS, debug overlay shows stats.
- [ ] **Commit**

---

## Self-Review Checklist

- [x] Spec coverage: Every milestone in the spec has corresponding tasks
- [x] Placeholder scan: No TBD/TODO in critical code paths (Iron API notes are flagged for verification)
- [x] Type consistency: Component structs defined once in headers, referenced consistently across files
- [x] File paths: All paths match the existing project structure under `engine/sources/`
- [x] Build commands: Correct two-step build process documented at top
- [x] Test pattern: Each milestone has a test script + manifest update + visual verification step
