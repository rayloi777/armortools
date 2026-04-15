# Bounding Sphere Frustum Culling Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the quarter-radius hack with correctly computed minimum bounding spheres for accurate frustum culling.

**Architecture:** Implement Ritter's algorithm to compute a tight bounding sphere from mesh vertex data at load time. Store the radius via the existing `obj->raw->dimensions` field so Iron's `transform_compute_radius()` chain produces the correct bounding sphere radius in world space. Remove all quarter-radius save/restore logic.

**Tech Stack:** C (engine layer), Iron engine APIs (`mesh_data_get_vertex_array`, `transform_compute_dim`, `transform_compute_radius`)

---

### Task 1: Implement Ritter's bounding sphere function

**Files:**
- Modify: `engine/sources/core/scene_3d_api.c` (add static function before `fix_mesh_dimensions`)

This task adds the core algorithm. No visible behavior change yet.

- [ ] **Step 1: Add `calculate_bounding_sphere_radius` function**

Add this static function at the top of `scene_3d_api.c`, after the `#define DEG_TO_RAD` block and before the `extern game_world_t *g_runtime_world;` declaration:

```c
// Compute a tight bounding sphere radius from mesh vertex positions using Ritter's algorithm.
// The vertex buffer stores INT16-packed positions with stride 4 (x,y,z,w).
// To get world-space coordinates: divide by 32767 and multiply by scale_pos.
static float calculate_bounding_sphere_radius(mesh_data_t *mesh_data) {
    vertex_array_t *positions = mesh_data_get_vertex_array(mesh_data, "pos");
    if (!positions || !positions->values || positions->values->length < 4) return 1.0f;

    float scale = mesh_data->scale_pos / 32767.0f;
    float *buf = positions->values->buffer;
    int count = positions->values->length;

    // Step 1: Find the point farthest from the first vertex
    float x0 = buf[0] * scale, y0 = buf[1] * scale, z0 = buf[2] * scale;
    float max_dist_sq = 0.0f;
    float p1x = x0, p1y = y0, p1z = z0;
    for (int i = 4; i < count; i += 4) {
        float x = buf[i] * scale, y = buf[i+1] * scale, z = buf[i+2] * scale;
        float dx = x - x0, dy = y - y0, dz = z - z0;
        float d2 = dx*dx + dy*dy + dz*dz;
        if (d2 > max_dist_sq) { max_dist_sq = d2; p1x = x; p1y = y; p1z = z; }
    }

    // Step 2: Find the point farthest from P1
    max_dist_sq = 0.0f;
    float p2x = p1x, p2y = p1y, p2z = p1z;
    for (int i = 0; i < count; i += 4) {
        float x = buf[i] * scale, y = buf[i+1] * scale, z = buf[i+2] * scale;
        float dx = x - p1x, dy = y - p1y, dz = z - p1z;
        float d2 = dx*dx + dy*dy + dz*dz;
        if (d2 > max_dist_sq) { max_dist_sq = d2; p2x = x; p2y = y; p2z = z; }
    }

    // Step 3: Initial sphere from P1-P2 diameter
    float cx = (p1x + p2x) * 0.5f;
    float cy = (p1y + p2y) * 0.5f;
    float cz = (p1z + p2z) * 0.5f;
    float dx = p2x - p1x, dy = p2y - p1y, dz = p2z - p1z;
    float radius = sqrtf(dx*dx + dy*dy + dz*dz) * 0.5f;

    // Step 4: Expand sphere to include all vertices
    for (int i = 0; i < count; i += 4) {
        float x = buf[i] * scale, y = buf[i+1] * scale, z = buf[i+2] * scale;
        float ex = x - cx, ey = y - cy, ez = z - cz;
        float dist = sqrtf(ex*ex + ey*ey + ez*ez);
        if (dist > radius) {
            float new_r = (radius + dist) * 0.5f;
            float ratio = (new_r - radius) / dist;
            cx += ex * ratio;
            cy += ey * ratio;
            cz += ez * ratio;
            radius = new_r;
        }
    }

    return radius > 0.001f ? radius : 1.0f;
}
```

- [ ] **Step 2: Build and verify compilation**

Run:
```bash
cd /Users/rayloi/Documents/GitHub/armortools/engine && ../base/make macos metal
cd /Users/rayloi/Documents/GitHub/armortools/engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | tail -5
```

Expected: Build succeeds (function is static, not yet called).

- [ ] **Step 3: Commit**

```bash
git add engine/sources/core/scene_3d_api.c
git commit -m "feat(culling): add Ritter's bounding sphere algorithm"
```

---

### Task 2: Replace AABB logic with bounding sphere in `scene_3d_api.c`

**Files:**
- Modify: `engine/sources/core/scene_3d_api.c:22-55` (replace `fix_mesh_dimensions`)

This changes the Minic-script mesh loading path (`mesh_load_arm` / `mesh_add_arm`) to use bounding spheres.

- [ ] **Step 1: Replace `fix_mesh_dimensions` function body**

Replace the existing `fix_mesh_dimensions` function (lines 22-55) with:

```c
// Fix bounding sphere dimensions on mesh objects so frustum culling uses accurate bounds.
// Computes a tight bounding sphere via Ritter's algorithm and stores the radius in
// obj->raw->dimensions. Iron's transform_compute_dim() multiplies each dimension by scale,
// then transform_compute_radius() computes sqrt(dx²+dy²+dz²). Setting all three slots to
// radius/sqrt(3) produces the correct bounding sphere radius: r/sqrt(3) * sqrt(3*s²) = r*s.
static void fix_mesh_dimensions(int start_index) {
    if (!scene_meshes) return;
    for (int i = start_index; i < scene_meshes->length; i++) {
        mesh_object_t *m = (mesh_object_t *)scene_meshes->buffer[i];
        if (!m || !m->base || !m->data) continue;
        object_t *obj = m->base;

        float radius = calculate_bounding_sphere_radius(m->data);

        // Store radius/sqrt(3) in each dimension slot so Iron's radius chain
        // produces the correct bounding sphere radius after scale multiplication
        float dim_val = radius / sqrtf(3.0f);

        if (obj->raw == NULL) {
            obj->raw = GC_ALLOC_INIT(obj_t, {
                .name = "",
                .dimensions = GC_ALLOC_INIT(f32_array_t, {
                    .buffer = gc_alloc(sizeof(float) * 3),
                    .length = 3,
                    .capacity = 3
                })
            });
        } else if (obj->raw->dimensions == NULL) {
            obj->raw->dimensions = GC_ALLOC_INIT(f32_array_t, {
                .buffer = gc_alloc(sizeof(float) * 3),
                .length = 3,
                .capacity = 3
            });
        }

        obj->raw->dimensions->buffer[0] = dim_val;
        obj->raw->dimensions->buffer[1] = dim_val;
        obj->raw->dimensions->buffer[2] = dim_val;
        obj->transform->dirty = true;
        transform_build_matrix(obj->transform);
    }
}
```

- [ ] **Step 2: Build and verify**

Run:
```bash
cd /Users/rayloi/Documents/GitHub/armortools/engine && ../base/make macos metal
cd /Users/rayloi/Documents/GitHub/armortools/engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | tail -5
```

Expected: Build succeeds.

- [ ] **Step 3: Commit**

```bash
git add engine/sources/core/scene_3d_api.c
git commit -m "feat(culling): use bounding sphere in fix_mesh_dimensions"
```

---

### Task 3: Replace AABB logic with bounding sphere in `mesh_bridge_3d.c`

**Files:**
- Modify: `engine/sources/ecs/mesh_bridge_3d.c:137-153` (AABB fix in `mesh_bridge_3d_create_mesh`)

This changes the ECS bridge path (entities created via `mesh_create()` Minic call).

- [ ] **Step 1: Add the same `calculate_bounding_sphere_radius` function to `mesh_bridge_3d.c`**

Add this static function near the top of `mesh_bridge_3d.c`, after the `#include` block and before `static game_world_t *g_mesh_3d_world`:

```c
// Compute a tight bounding sphere radius from mesh vertex positions using Ritter's algorithm.
// The vertex buffer stores INT16-packed positions with stride 4 (x,y,z,w).
// To get world-space coordinates: divide by 32767 and multiply by scale_pos.
static float calculate_bounding_sphere_radius(mesh_data_t *mesh_data) {
    vertex_array_t *positions = mesh_data_get_vertex_array(mesh_data, "pos");
    if (!positions || !positions->values || positions->values->length < 4) return 1.0f;

    float scale = mesh_data->scale_pos / 32767.0f;
    float *buf = positions->values->buffer;
    int count = positions->values->length;

    float x0 = buf[0] * scale, y0 = buf[1] * scale, z0 = buf[2] * scale;
    float max_dist_sq = 0.0f;
    float p1x = x0, p1y = y0, p1z = z0;
    for (int i = 4; i < count; i += 4) {
        float x = buf[i] * scale, y = buf[i+1] * scale, z = buf[i+2] * scale;
        float dx = x - x0, dy = y - y0, dz = z - z0;
        float d2 = dx*dx + dy*dy + dz*dz;
        if (d2 > max_dist_sq) { max_dist_sq = d2; p1x = x; p1y = y; p1z = z; }
    }

    max_dist_sq = 0.0f;
    float p2x = p1x, p2y = p1y, p2z = p1z;
    for (int i = 0; i < count; i += 4) {
        float x = buf[i] * scale, y = buf[i+1] * scale, z = buf[i+2] * scale;
        float dx = x - p1x, dy = y - p1y, dz = z - p1z;
        float d2 = dx*dx + dy*dy + dz*dz;
        if (d2 > max_dist_sq) { max_dist_sq = d2; p2x = x; p2y = y; p2z = z; }
    }

    float cx = (p1x + p2x) * 0.5f;
    float cy = (p1y + p2y) * 0.5f;
    float cz = (p1z + p2z) * 0.5f;
    float dx = p2x - p1x, dy = p2y - p1y, dz = p2z - p1z;
    float radius = sqrtf(dx*dx + dy*dy + dz*dz) * 0.5f;

    for (int i = 0; i < count; i += 4) {
        float x = buf[i] * scale, y = buf[i+1] * scale, z = buf[i+2] * scale;
        float ex = x - cx, ey = y - cy, ez = z - cz;
        float dist = sqrtf(ex*ex + ey*ey + ez*ez);
        if (dist > radius) {
            float new_r = (radius + dist) * 0.5f;
            float ratio = (new_r - radius) / dist;
            cx += ex * ratio;
            cy += ey * ratio;
            cz += ez * ratio;
            radius = new_r;
        }
    }

    return radius > 0.001f ? radius : 1.0f;
}
```

- [ ] **Step 2: Replace the AABB block in `mesh_bridge_3d_create_mesh`**

Replace the existing AABB block (lines 137-153 in `mesh_bridge_3d_create_mesh`):

```c
            // Set correct bounding sphere dimensions so frustum culling uses accurate bounds.
            if (mesh_obj->base->raw == NULL) {
                float radius = calculate_bounding_sphere_radius(mesh_data);
                float dim_val = radius / sqrtf(3.0f);
                obj_t *o_raw = GC_ALLOC_INIT(obj_t, {
                    .name = "",
                    .dimensions = GC_ALLOC_INIT(f32_array_t, {
                        .buffer = gc_alloc(sizeof(float) * 3),
                        .length = 3,
                        .capacity = 3
                    })
                });
                o_raw->dimensions->buffer[0] = dim_val;
                o_raw->dimensions->buffer[1] = dim_val;
                o_raw->dimensions->buffer[2] = dim_val;
                mesh_obj->base->raw = o_raw;
                mesh_obj->base->transform->dirty = true;
                transform_build_matrix(mesh_obj->base->transform);
            }
```

- [ ] **Step 3: Build and verify**

Run:
```bash
cd /Users/rayloi/Documents/GitHub/armortools/engine && ../base/make macos metal
cd /Users/rayloi/Documents/GitHub/armortools/engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | tail -5
```

Expected: Build succeeds.

- [ ] **Step 4: Commit**

```bash
git add engine/sources/ecs/mesh_bridge_3d.c
git commit -m "feat(culling): use bounding sphere in mesh_bridge_3d"
```

---

### Task 4: Remove quarter-radius hack from `render3d_bridge.c`

**Files:**
- Modify: `engine/sources/ecs/render3d_bridge.c`

This removes all quarter-radius save/restore logic while keeping the custom sphere-in-frustum test.

- [ ] **Step 1: Remove `g_saved_radii` and `g_saved_radii_count` static variables**

Delete these two lines at the top of the file (lines 18-19):

```c
static float *g_saved_radii = NULL;
static int g_saved_radii_count = 0;
```

- [ ] **Step 2: Remove the radius quartering block**

Delete the entire block from line 228 to line 245 (the block between `set_material_const("shadow_enabled", 1.0f);` and the shadow map binding). This is the block starting with:

```c
    // Tighten bounding sphere radii before G-buffer pass so frustum culling
    // actually works. Iron's transform_compute_radius uses full diagonal which
    // makes the conservative +radius*2 test never cull anything.
    {
        int mesh_count = scene_meshes ? scene_meshes->length : 0;
        if (g_saved_radii_count < mesh_count) {
            ...
        for (int i = 0; i < mesh_count; i++) {
            ...
            t->radius *= 0.25f;
        }
    }
```

- [ ] **Step 3: Remove the radius restore block**

Delete the radius restore code that appears after the stats collection block (between the stats collection loop and the `// --- Post-Processing ---` comment). This is the block that restores `g_saved_radii[i]`:

```c
    // Restore bounding sphere radii
    for (int i = 0; scene_meshes && i < scene_meshes->length; i++) {
        mesh_object_t *mesh = (mesh_object_t *)scene_meshes->buffer[i];
        transform_t *t = mesh->base->transform;
        t->radius = g_saved_radii[i];
    }
```

Note: The visibility restore block (`mesh->base->visible = true; mesh->base->culled = false;`) stays — it's needed because our custom culling sets `visible=false` on culled meshes.

- [ ] **Step 4: Update the comment on the custom frustum culling block**

Change the comment above the custom frustum culling block from:

```c
    // Custom frustum culling: Iron's built-in cull test uses a very conservative
    // +radius*2 threshold. We set our own tighter culling by checking each mesh
    // against the camera's frustum planes using a standard sphere test.
```

to:

```c
    // Custom frustum culling using correctly computed bounding spheres.
    // Iron's built-in cull test uses a conservative +radius*2 threshold,
    // so we apply our own standard sphere-in-frustum test (dist + radius < 0).
```

- [ ] **Step 5: Update the inline comment about quartered radius**

In the custom culling block, change:

```c
                // Use the quartered radius as bounding sphere
                float radius = t->radius;
```

to:

```c
                // Use the bounding sphere radius (computed via Ritter's algorithm at load time)
                float radius = t->radius;
```

- [ ] **Step 6: Build and verify**

Run:
```bash
cd /Users/rayloi/Documents/GitHub/armortools/engine && ../base/make macos metal
cd /Users/rayloi/Documents/GitHub/armortools/engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | tail -5
```

Expected: Build succeeds.

- [ ] **Step 7: Commit**

```bash
git add engine/sources/ecs/render3d_bridge.c
git commit -m "refactor(culling): remove quarter-radius hack, use bounding spheres"
```

---

### Task 5: Verify frustum culling end-to-end

**Files:**
- None (runtime verification only)

- [ ] **Step 1: Run the M8 optimization test**

Run:
```bash
cd /Users/rayloi/Documents/GitHub/armortools/engine/build && ./build/Release/IronGame.app/Contents/MacOS/IronGame
```

Expected behavior:
- The overlay shows "Meshes: 50 total" with non-zero rendered/culled counts
- When camera faces the cube grid: rendered should be significantly less than 50 (only visible cubes)
- When camera moves behind all objects: culled should be close to 50

- [ ] **Step 2: Verify no quarter-radius references remain**

Run:
```bash
grep -rn "0.25" engine/sources/ecs/render3d_bridge.c
grep -rn "g_saved_radii" engine/sources/ecs/render3d_bridge.c
grep -rn "quarter" engine/sources/ecs/render3d_bridge.c
```

Expected: No matches found.
