# 3D Deferred Pipeline Integration Design

**Date:** 2026-04-12
**Status:** Approved
**Approach:** Direct rewrite of render3d_bridge, visual milestone testing

---

## Context

The engine currently has a basic forward 3D rendering pipeline (`render3d_bridge.c` + `mesh_bridge_3d.c` + `camera_bridge_3d.c` + directional light). The goal is to replace this with a full deferred rendering pipeline as described in `3D_PLAN.md`.

**Strategy:** Directly rewrite `render3d_bridge.c` into the deferred pipeline. No parallel forward path. Each milestone produces testable visual output via `.minic` test scripts.

---

## Integration Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Integration method | Direct rewrite | Cleanest architecture, no dual-pipeline complexity |
| Test method | .minic test scripts per milestone | Lightweight, matches existing systems.manifest flow |
| Phasing unit | Visual milestones | Each milestone produces a visible, verifiable result |
| Planning scope | All milestones at once | Complete roadmap for the full pipeline |

---

## Visual Milestones

```
M1: G-Buffer Debug View        → See normal/depth/albedo channels
M2: Deferred PBR Lighting      → See lit PBR scene (no shadows)
M3: CSM Directional Shadows    → See shadows from directional light
M4: Point + Spot Lights        → See multiple light types + shadows
M5: Post-Processing            → See SSAO + Bloom
M6: Transparent + Particles    → See alpha objects + glowing particles
M7: Decals                     → See projected decals on surfaces
M8: Optimization               → Same visuals, better performance
```

Each milestone has its own `.minic` test script loaded via `systems.manifest`.

---

## M1: G-Buffer Debug View

**Goal:** Rewrite `render3d_bridge.c` to render scene into G-buffer (gbuffer0 + gbuffer1 + depth), then output debug visualization.

**Render Flow:**
```
render_path_set_target → "gbuffer0" + "gbuffer1" + "main" (depth)
render_path_submit_draw("mesh_gbuffer")  // MRT output
→ Debug: display gbuffer channels to screen
```

**New Files:**

| File | Purpose |
|------|---------|
| `engine/sources/ecs/deferred/deferred_gbuffer.c/h` | G-buffer creation and management |
| `engine/assets/shaders/mesh_gbuffer.kong` | MRT geometry shader |
| `engine/assets/shaders/debug_gbuffer.kong` | Debug visualization shader |
| `engine/assets/scripts/test_m1_gbuffer.minic` | Test script |

**Modified Files:**

| File | Change |
|------|--------|
| `render3d_bridge.c/h` | Rewrite render commands to deferred pipeline |
| `mesh_bridge_3d.c` | Use new gbuffer shader |
| `ecs_components.c/h` | Add material component |
| `game_engine.c` | Initialize gbuffer render targets |

**Test Script** (`test_m1_gbuffer.minic`):
- Create ground plane + several cubes
- Keys 1-4 switch display: Normal, Depth, Albedo, Roughness/Metallic

**Expected Visuals:**
- Key 1: Object normals as colors (up = green, right = red, etc.)
- Key 2: Depth (near = white, far = black)
- Key 3: Albedo colors
- Key 4: Roughness/metallic grayscale

**Risk:** Forward rendering is replaced. Until M2, no lit scene — only debug channel views.

---

## M2: Deferred PBR Lighting

**Goal:** Read G-buffer + depth, compute PBR lighting. Directional light only, no shadows.

**Render Flow:**
```
Pass 1: G-Buffer (from M1)
Pass 2: Deferred Lighting
  → Read gbuffer0 (normal, roughness, metallic)
  → Read gbuffer1 (albedo, ao)
  → Read main (depth → reconstruct world position)
  → Output to "buf" (RGBA16F)
Pass 3: Present → "buf" to screen
```

**New Files:**

| File | Purpose |
|------|---------|
| `engine/sources/ecs/deferred/deferred_lighting.c/h` | Lighting pass |
| `engine/sources/core/material_api.c/h` | Minic material API |
| `engine/sources/core/material_system.c/h` | Material management |
| `engine/sources/core/light_api.c/h` | Minic light API |
| `engine/assets/shaders/deferred_light.kong` | Deferred PBR shader |
| `engine/assets/shaders/fullscreen_quad.kong` | Fullscreen quad for present |
| `engine/assets/scripts/test_m2_lighting.minic` | Test script |

**Modified Files:**

| File | Change |
|------|--------|
| `render3d_bridge.c` | Add lighting pass + present pass |
| `ecs_components.c/h` | Add material component fields |
| `runtime_api.c` | Register material + light APIs |

**Test Script** (`test_m2_lighting.minic`):
- Create ground + cubes + sphere
- Different materials: metallic red, rough gray, smooth blue
- One directional light from upper-left
- Keys 1-4 switch debug/lit view

**Expected Visuals:**
- Correct PBR lighting (metallic reflections, soft diffuse on rough surfaces)
- Dark faces without direct light
- No shadows (added in M3)
- Debug view still accessible via key press

---

## M3: CSM Directional Shadows

**Goal:** Add Cascaded Shadow Maps for directional light shadows.

**Render Flow:**
```
Pass 0: Shadow Maps (new)
  → Directional light VP × 3 cascades
  → Render to shadow_0 (2048), shadow_1 (1024), shadow_2 (512)
  → mesh_depth_only shader (depth-only)

Pass 1: G-Buffer (unchanged)
Pass 2: Deferred Lighting (updated)
  → Read shadow maps
  → Compute shadow factor per pixel
  → Apply to lighting result
Pass 3: Present (unchanged)
```

**New Files:**

| File | Purpose |
|------|---------|
| `engine/sources/ecs/shadow/shadow_directional.c/h` | CSM shadow pass |
| `engine/assets/shaders/shadow_depth.kong` | Shadow depth shader |
| `engine/assets/scripts/test_m3_shadow.minic` | Test script |

**Modified Files:**

| File | Change |
|------|--------|
| `render3d_bridge.c` | Add shadow pass (before gbuffer) |
| `deferred_lighting.c` | Shadow sampling + cascade selection |
| `deferred_light.kong` | Add shadow uniform + PCF sampling |
| `ecs_components.c/h` | Extend directional_light (shadow bias, etc.) |
| `mesh_bridge_3d.c` | Support depth-only render mode |

**Test Script** (`test_m3_shadow.minic`):
- Multiple cubes at different heights
- Ground plane receives shadows
- Key S: toggle shadow cascade debug (different colors per cascade)
- Key +/-: adjust shadow bias

**Expected Visuals:**
- Cubes cast shadows on ground
- Shadows sharp near camera, soft far away (cascade resolution decreases)
- No shadow acne (correct bias)
- Cascade debug: red/green/blue regions

---

## M4: Point + Spot Lights

**Goal:** Add Point Light and Spot Light with shadows.

**Render Flow:**
```
Pass 0: Shadow Maps (expanded)
  → Directional CSM (from M3)
  → Point Lights × 8 → shadow_cube_[0-7] (1024²×6 faces)
  → Spot Lights × 4 → shadow_spot_[0-3] (1024²)

Pass 1: G-Buffer (unchanged)
Pass 2: Deferred Lighting (updated)
  → Point light traversal (quadratic attenuation + cubemap shadow)
  → Spot light traversal (angular attenuation + perspective shadow)
Pass 3: Present (unchanged)
```

**New Files:**

| File | Purpose |
|------|---------|
| `engine/sources/ecs/shadow/shadow_point.c/h` | Point light cubemap shadow |
| `engine/sources/ecs/shadow/shadow_spot.c/h` | Spot light shadow map |
| `engine/sources/components/light.c/h` | Point/Spot light components |
| `engine/assets/shaders/shadow_cubemap.kong` | Point shadow shader |
| `engine/assets/shaders/shadow_spot.kong` | Spot shadow shader |
| `engine/assets/scripts/test_m4_lights.minic` | Test script |

**Modified Files:**

| File | Change |
|------|--------|
| `render3d_bridge.c` | Add point/spot shadow passes |
| `deferred_lighting.c` | Point/spot light computation |
| `deferred_light.kong` | Point/spot lighting + shadow |
| `ecs_components.c/h` | Register point/spot light components |
| `light_api.c` | Add point/spot creation APIs |

**Test Script** (`test_m4_lights.minic`):
- Indoor scene: walls + ground + cubes
- 1 directional light (window)
- 2 point lights (indoor lamps, different colors)
- 1 spotlight (ceiling downward)
- Keys 1-4 toggle each light
- Key D: show light position debug spheres

**Expected Visuals:**
- Point lights illuminate surroundings with correct distance falloff
- Spotlight has cone-shaped beam
- Each light casts shadows
- Mixed multi-light (directional + indoor)

---

## M5: Post-Processing (SSAO + Bloom)

**Goal:** Add SSAO and Bloom post-processing.

**Render Flow:**
```
Pass 0-2: Shadow → G-Buffer → Lighting (unchanged)
Pass 3a: SSAO
  → Read depth + normal → ao buffer
  → Multiply AO into lighting result
Pass 3b: Bloom
  → Bloom_down: progressive downsample (1/2, 1/4, 1/8, 1/16)
  → Bloom_up: progressive upsample + composite
  → Write to "last"
Pass 4: Present → "last" to screen

Note: Transparent pass (M6) will be inserted between Pass 3a (SSAO)
and Pass 3b (Bloom) so particles write to HDR before bloom runs.
```

**Design Decision:** TAA is deferred to a later optimization phase. It requires history frame buffers + motion vectors, which adds significant complexity without a clear visual milestone.

**New Files:**

| File | Purpose |
|------|---------|
| `engine/sources/ecs/deferred/deferred_postfx.c/h` | Post-processing management |
| `engine/assets/shaders/postfx_ssao.kong` | SSAO shader |
| `engine/assets/shaders/postfx_bloom_down.kong` | Bloom downsample |
| `engine/assets/shaders/postfx_bloom_up.kong` | Bloom upsample |
| `engine/assets/shaders/postfx_compositor.kong` | Final composite |
| `engine/assets/scripts/test_m5_postfx.minic` | Test script |

**Modified Files:**

| File | Change |
|------|--------|
| `render3d_bridge.c` | Add post-processing pass chain |
| `deferred_lighting.c` | Multiply SSAO result into lighting |
| `game_engine.c` | Initialize post-fx render targets |

**Test Script** (`test_m5_postfx.minic`):
- Outdoor scene: ground + multiple objects + directional light
- Key A: toggle SSAO (compare corner occlusion)
- Key B: toggle Bloom
- Key +/-: adjust SSAO radius / Bloom intensity
- One high-emissive object triggers bloom

**Expected Visuals:**
- SSAO on: soft occlusion shadows at object intersections and corners
- Bloom on: glow around emissive objects
- Both can be toggled independently for comparison

---

## M6: Transparent Objects + Particles

**Goal:** Add Forward Transparent Pass after Deferred Lighting for alpha-blended objects and additive particles.

**Render Flow:**
```
Pass 0-2: Shadow → G-Buffer → Lighting (unchanged)
Pass 3a: SSAO (from M5, unchanged)
Pass 3.5: Transparent Forward (new)
  → Read "buf" depth for depth test (LESS, depth_write = false)
  → Alpha blend (SRC_ALPHA, ONE_MINUS_SRC_ALPHA)
  → Particles use Additive blend
  → Writes to HDR "buf" (after SSAO, before Bloom)
Pass 3b: Bloom (from M5, reads HDR including transparent particles)
Pass 4: Present
```

**Design Note:** Transparent pass is inserted between SSAO (Pass 3a) and Bloom (Pass 3b) so particles write to HDR buffer before bloom runs, allowing emissive particles to trigger bloom correctly.

**New Files:**

| File | Purpose |
|------|---------|
| `engine/sources/ecs/transparent_bridge.c/h` | Transparent forward pass |
| `engine/sources/ecs/particle_system.c/h` | Particle system |
| `engine/sources/components/transparency.c/h` | Transparent + particle components |
| `engine/assets/shaders/transparent_forward.kong` | Transparent object shader |
| `engine/assets/shaders/particle_additive.kong` | Particle additive shader |
| `engine/assets/scripts/test_m6_transparent.minic` | Test script |

**Modified Files:**

| File | Change |
|------|--------|
| `render3d_bridge.c` | Insert transparent pass before Bloom |
| `deferred_postfx.c` | Adjust pass order |
| `ecs_components.c/h` | Add transparency/particle tags |
| `runtime_api.c` | Register transparent/particle APIs |

**Test Script** (`test_m6_transparent.minic`):
- Semi-transparent glass cube (alpha = 0.3)
- Glowing particles (additive, emissive)
- Opaque cubes for contrast
- Key A: adjust alpha
- Key E: adjust emissive strength

**Expected Visuals:**
- Glass cube shows opaque objects behind it
- Particles have bloom glow (HDR + Additive)
- Correct depth test (transparent doesn't occlude opaque)

---

## M7: Decals

**Goal:** Add alpha-transparent decals projected onto scene surfaces.

**Render Flow:**
```
Pass 0-2: Shadow → G-Buffer → Lighting (unchanged)
Pass 3a: SSAO (unchanged)
Pass 3.5: Transparent Forward (expanded)
  → Transparent objects
  → Decals (sorted back-to-front with transparent objects)
Pass 3b: Bloom (unchanged)
Pass 4: Present
```

**Decal Principle:** An invisible AABB box projects texture onto scene surfaces within the box. Uses world position + depth reconstruction for UV calculation.

**New Files:**

| File | Purpose |
|------|---------|
| `engine/sources/ecs/decal/decal_bridge.c/h` | Decal system |
| `engine/sources/components/decal.c/h` | Decal component |
| `engine/assets/shaders/decal_projection.kong` | Projection shader |
| `engine/assets/scripts/test_m7_decal.minic` | Test script |

**Modified Files:**

| File | Change |
|------|--------|
| `render3d_bridge.c` | Add decal to transparent pass |
| `transparent_bridge.c` | Sort includes decal entities |
| `ecs_components.c/h` | Add decal component + tag |
| `runtime_api.c` | Register decal API |

**Test Script** (`test_m7_decal.minic`):
- Ground + wall + cubes
- Decals at various positions (bullet hole texture, marker texture)
- Key D: add random decal
- Key +/-: adjust decal size
- Key O: adjust opacity

**Expected Visuals:**
- Decals correctly project onto ground/wall surfaces
- Decals follow surface normals (don't penetrate through walls)
- Multiple decals don't conflict
- Correct opacity

---

## M8: Optimization

**Goal:** Same visuals, better performance. Add Early-Z, LOD, Hi-Z Occlusion Culling, DrawProceduralIndirect.

**Render Flow Changes:**
```
Pass 0.5: Depth Pre-pass (new)
  → mesh_depth_only → "main" depth buffer
  → Early-Z lets G-buffer pass skip occluded fragments

Pass 1: G-Buffer (benefits from Early-Z)
Pass 2: Lighting (unchanged)
Pass 3a: SSAO → Pass 3.5: Transparent → Pass 3b: Bloom → Pass 4: Present (unchanged)

LOD System:
  → CPU frustum culling → select LOD level
  → comp_3d_lod auto-switches mesh by distance

Hi-Z Occlusion:
  → Depth mipmap chain
  → GPU CS: Hi-Z query for large objects
  → Fully occluded objects skipped

DrawProceduralIndirect:
  → Culling results write to indirect args buffer
  → GPU reads directly, reducing CPU overhead
```

**New Files:**

| File | Purpose |
|------|---------|
| `engine/sources/ecs/culling/culling_frustum.c/h` | GPU frustum culling |
| `engine/sources/ecs/culling/culling_lod.c/h` | LOD system |
| `engine/sources/ecs/culling/culling_occlusion.c/h` | Hi-Z occlusion |
| `engine/sources/components/lod.c/h` | LOD component |
| `engine/assets/shaders/culling_frustum.csh` | Frustum cull compute shader |
| `engine/assets/scripts/test_m8_optimization.minic` | Test script |

**Modified Files:**

| File | Change |
|------|--------|
| `render3d_bridge.c` | Add depth pre-pass + culling |
| `mesh_bridge_3d.c` | LOD mesh switching |
| `ecs_components.c/h` | LOD component |
| `camera_bridge_3d.c` | Frustum planes export |

**Test Script** (`test_m8_optimization.minic`):
- Many objects (100-200 cubes/spheres)
- Various distances (test LOD)
- Key F: toggle frustum culling (show culled count)
- Key O: toggle occlusion culling
- Key L: toggle LOD (debug show LOD level)
- Key P: show perf stats (draw calls, triangles)

**Expected Visuals:**
- Visually identical to M7
- Higher FPS (fewer draw calls, fewer fragment shader invocations)
- Debug overlay shows culling/LOD stats

---

## Complete File Summary

### New Files (per milestone)

| Milestone | New Files |
|-----------|-----------|
| M1 | deferred_gbuffer.c/h, mesh_gbuffer.kong, debug_gbuffer.kong, test_m1_gbuffer.minic |
| M2 | deferred_lighting.c/h, material_api.c/h, material_system.c/h, light_api.c/h, deferred_light.kong, fullscreen_quad.kong, test_m2_lighting.minic |
| M3 | shadow_directional.c/h, shadow_depth.kong, test_m3_shadow.minic |
| M4 | shadow_point.c/h, shadow_spot.c/h, light.c/h, shadow_cubemap.kong, shadow_spot.kong, test_m4_lights.minic |
| M5 | deferred_postfx.c/h, postfx_ssao.kong, postfx_bloom_down.kong, postfx_bloom_up.kong, postfx_compositor.kong, test_m5_postfx.minic |
| M6 | transparent_bridge.c/h, particle_system.c/h, transparency.c/h, transparent_forward.kong, particle_additive.kong, test_m6_transparent.minic |
| M7 | decal_bridge.c/h, decal.c/h, decal_projection.kong, test_m7_decal.minic |
| M8 | culling_frustum.c/h, culling_lod.c/h, culling_occlusion.c/h, lod.c/h, culling_frustum.csh, test_m8_optimization.minic |

### Modified Files (cumulative)

| File | Milestones |
|------|------------|
| `render3d_bridge.c/h` | M1, M2, M3, M4, M5, M6, M7, M8 |
| `ecs_components.c/h` | M1, M2, M3, M4, M6, M7, M8 |
| `runtime_api.c` | M2, M4, M6, M7 |
| `game_engine.c` | M1, M5 |
| `mesh_bridge_3d.c` | M1, M3, M8 |
| `deferred_lighting.c` | M2, M3, M4, M5 |
| `deferred_light.kong` | M2, M3, M4 |
| `camera_bridge_3d.c` | M8 |
| `transparent_bridge.c` | M7 |
| `deferred_postfx.c` | M6 |
