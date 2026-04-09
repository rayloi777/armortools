# Fix: Perspective Distortion in scene3d_test

## Problem

The 3D camera in scene3d_test produces incorrect perspective distortion. Two root causes identified.

### Root Cause 1: FOV Degrees/Radians Mismatch

The Minic API accepts FOV in degrees (e.g., `camera_3d_create(60.0, ...)`), but `mat4_persp()` in the Iron engine expects radians. The value passes through unconverted:

```
scene3d_test.minic  → 60.0 (degrees)
scene_3d_api.c      → cam->fov = 60.0
camera_bridge_3d.c  → g_camera_3d_data->fov = 60.0
mat4_persp()        → tanf(60.0 / 2) → nonsense (expects radians)
```

The Iron engine stores FOV in radians internally (e.g., cube test uses `fov = 0.85` ≈ 48.7°).

### Root Cause 2: Non-Normalized Quaternion in step()

`scene3d_test.minic` step() sets raw quaternion values that don't form a unit quaternion:

```c
rx = sin(g_angle * 1.3) * 0.707;
ry = sin(g_angle);
rw = cos(g_angle * 0.5);
// rx² + ry² + rz² + rw² ≠ 1.0 → scaling/shearing distortion
```

## Fix

### 1. Convert degrees to radians at the API boundary

In `engine/sources/core/scene_3d_api.c`, convert FOV from degrees to radians in three functions:

- `minic_camera_3d_create()` — `fov = deg * (PI / 180.0f)`
- `minic_camera_3d_set_fov()` — same conversion
- `minic_camera_3d_perspective()` — same conversion

### 2. Fix default FOV in camera_bridge_3d_init()

In `engine/sources/ecs/camera_bridge_3d.c`, the initial camera data uses `fov = 60.0f`. Since this flows directly to `mat4_persp`, it must be radians:

- `camera_bridge_3d_init()` — `.fov = 60.0f * (PI / 180.0f)`

### 3. Fix quaternion in scene3d_test.minic step()

Replace the non-normalized quaternion with a proper Y-axis rotation:

```c
float half = g_angle * 0.5;
float rx = 0.0;
float ry = sin(half);
float rz = 0.0;
float rw = cos(half);
```

This produces a clean unit quaternion for rotation around Y.

## Files Changed

| File | Change |
|---|---|
| `engine/sources/core/scene_3d_api.c` | Degrees→radians conversion in 3 functions |
| `engine/sources/ecs/camera_bridge_3d.c` | Fix default fov to radians |
| `engine/assets/systems/scene3d_test.minic` | Fix quaternion to proper Y-axis rotation |

## Scope

- Minimal: only touches the conversion boundary and the test script
- No changes to `base/`
- Scripts continue using degrees (intuitive); engine internals stay in radians (correct)
