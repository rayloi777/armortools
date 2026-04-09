# Perspective Distortion Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix the FOV degrees/radians mismatch and non-normalized quaternion causing incorrect perspective distortion in scene3d_test.

**Architecture:** The Minic API layer accepts FOV in degrees (intuitive for script authors) and converts to radians before storing in ECS components. The Iron engine's `mat4_persp()` always receives radians. The test script quaternion is fixed to produce proper unit quaternions.

**Tech Stack:** C (engine API), Minic scripting

---

### Task 1: Add degrees-to-radians conversion in scene_3d_api.c

**Files:**
- Modify: `engine/sources/core/scene_3d_api.c`

This task adds `#define DEG_TO_RAD` and converts FOV in the three functions that accept it.

- [ ] **Step 1: Add conversion constant and fix `minic_camera_3d_create`**

At the top of `engine/sources/core/scene_3d_api.c`, after the existing `#include` lines, add:

```c
#define DEG_TO_RAD (3.14159265358979323846f / 180.0f)
```

In `minic_camera_3d_create` (line 25), change:

```c
float fov = (argc > 0) ? (float)minic_val_to_d(args[0]) : 60.0f;
```

to:

```c
float fov = (argc > 0) ? (float)minic_val_to_d(args[0]) * DEG_TO_RAD : 60.0f * DEG_TO_RAD;
```

- [ ] **Step 2: Fix `minic_camera_3d_set_fov`**

In `minic_camera_3d_set_fov` (line 65), change:

```c
float fov = (float)minic_val_to_d(args[1]);
```

to:

```c
float fov = (float)minic_val_to_d(args[1]) * DEG_TO_RAD;
```

- [ ] **Step 3: Fix `minic_camera_3d_perspective`**

In `minic_camera_3d_perspective` (line 336), change:

```c
float fov = (float)minic_val_to_d(args[1]);
```

to:

```c
float fov = (float)minic_val_to_d(args[1]) * DEG_TO_RAD;
```

- [ ] **Step 4: Commit**

```bash
git add engine/sources/core/scene_3d_api.c
git commit -m "fix(scene_3d_api): convert FOV from degrees to radians at API boundary"
```

---

### Task 2: Fix default FOV in camera_bridge_3d_init

**Files:**
- Modify: `engine/sources/ecs/camera_bridge_3d.c`

The default FOV in `camera_bridge_3d_init` must be in radians since it flows directly to `mat4_persp`.

- [ ] **Step 1: Add conversion constant and fix default FOV**

At the top of `engine/sources/ecs/camera_bridge_3d.c`, after the `#include` lines, add:

```c
#define DEG_TO_RAD_BRIDGE (3.14159265358979323846f / 180.0f)
```

In `camera_bridge_3d_init` (line 46), change:

```c
.fov = 60.0f,
```

to:

```c
.fov = 60.0f * DEG_TO_RAD_BRIDGE,
```

- [ ] **Step 2: Commit**

```bash
git add engine/sources/ecs/camera_bridge_3d.c
git commit -m "fix(camera_bridge_3d): use radians for default FOV value"
```

---

### Task 3: Fix quaternion in scene3d_test.minic

**Files:**
- Modify: `engine/assets/systems/scene3d_test.minic`

Replace the non-normalized quaternion with a proper Y-axis rotation.

- [ ] **Step 1: Fix rotation quaternion in step()**

In `engine/assets/systems/scene3d_test.minic`, change the step() function's rotation block (lines 38–43):

```c
if (g_mesh != 0) {
    float rx = sin(g_angle * 1.3) * 0.707;
    float ry = sin(g_angle);
    float rz = 0.0;
    float rw = cos(g_angle * 0.5);
    entity_set_rotation(g_mesh, rx, ry, rz, rw);
}
```

to:

```c
if (g_mesh != 0) {
    float half = g_angle * 0.5;
    float rx = 0.0;
    float ry = sin(half);
    float rz = 0.0;
    float rw = cos(half);
    entity_set_rotation(g_mesh, rx, ry, rz, rw);
}
```

- [ ] **Step 2: Commit**

```bash
git add engine/assets/systems/scene3d_test.minic
git commit -m "fix(scene3d_test): use proper unit quaternion for Y-axis rotation"
```

---

### Task 4: Build and verify

**Files:** None (verification only)

- [ ] **Step 1: Rebuild the engine**

```bash
cd engine && ../base/make macos metal && cd build && xcodebuild -project IronGame.xcodeproj -configuration Release build
```

Expected: Build succeeds with no errors.

- [ ] **Step 2: Run and visually verify**

```bash
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame
```

Expected: The cube should display with correct perspective (no extreme distortion), and rotate cleanly around the Y-axis without stretching or shearing.
