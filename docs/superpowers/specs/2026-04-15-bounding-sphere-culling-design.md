# Bounding Sphere Frustum Culling

**Goal:** Replace the quarter-radius hack with correctly computed minimum bounding spheres for accurate frustum culling.

**Architecture:** Compute a tight bounding sphere from mesh vertex data at load time using Ritter's algorithm. Store the sphere radius in the existing `obj->raw->dimensions` field so Iron's `transform_compute_radius()` produces a correct value. Remove all quarter-radius save/restore logic from `render3d_bridge.c`.

---

## Background

The current frustum culling has two problems:

1. **Inflated radius:** `transform_compute_radius()` computes `sqrt(dim.x² + dim.y² + dim.z²)` from full AABB dimensions. For a unit cube (dims 2,2,2) this gives ~3.46 instead of the true bounding sphere radius of ~1.73.
2. **Quarter-radius hack:** To compensate, the render bridge quarters the radius before the G-buffer pass and restores it after. This is a fragile magic number.

## Design

### 1. New function: `calculate_bounding_sphere_radius()`

Implement Ritter's algorithm (~30 lines of C) that computes a minimum bounding sphere from vertex positions. The function takes a `mesh_data_t*` and returns a `float` radius.

Steps:
1. Find the point P1 farthest from the first vertex.
2. Find the point P2 farthest from P1.
3. Sphere center = midpoint of P1,P2; radius = half the distance.
4. Iterate all vertices — if a vertex is outside the sphere, extend the sphere to include it.

This runs once at mesh load time. No per-frame cost.

### 2. Storage strategy

Store the computed radius into all three elements of `obj->raw->dimensions->buffer[0..2]`. Since `transform_compute_dim()` does `dim = (buffer[0] * scale.x, buffer[1] * scale.y, buffer[2] * scale.z)` and `transform_compute_radius()` does `sqrt(dim.x² + dim.y² + dim.z²)`, setting all three to the radius value produces `radius * sqrt(scale.x² + scale.y² + scale.z²)`.

For uniform scale (the common case), this equals `radius * scale * sqrt(3)` which is still slightly inflated. To get the exact bounding sphere radius for uniform scale, store the radius divided by `sqrt(3)` in each dimension slot — then `transform_compute_radius()` returns `radius/sqrt(3) * sqrt(3 * scale²) = radius * scale`.

This ensures the final `t->radius` is the correct bounding sphere radius in world space.

### 3. Files changed

| File | Change |
|------|--------|
| `engine/sources/ecs/mesh_bridge_3d.c` | Replace AABB fix in `mesh_bridge_3d_create_mesh()` with bounding sphere computation |
| `engine/sources/core/scene_3d_api.c` | Replace `fix_mesh_dimensions()` AABB logic with bounding sphere |
| `engine/sources/ecs/render3d_bridge.c` | Remove `g_saved_radii`, radius quartering, and radius restore code. Keep custom sphere-in-frustum test with correct radii. |

### 4. What stays the same

- Iron's `transform_compute_radius()` and `transform_compute_dim()` — unchanged in `base/`.
- Custom sphere-in-frustum test (`dist + radius < 0`) in `render3d_bridge.c` — kept as-is.
- Stats collection and Minic bindings — unchanged.
- M8 test script — unchanged.

### 5. Verification

- M8 optimization test: camera facing objects shows reasonable rendered count.
- Camera behind all objects: culled should be ~50.
- No quarter-radius magic number anywhere in the codebase.
