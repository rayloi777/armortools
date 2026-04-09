# camera3d_control_system Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a free-fly FPS camera control system in minic that drives the 3D test scene's camera via WASD + mouse look.

**Architecture:** A new `camera3d_control_system.minic` finds the camera entity created by `scene3d_test.minic` via `entity_find_by_name("main_camera")`. It tracks yaw/pitch euler angles as globals, computes forward/right vectors each frame with `sin`/`cos`, reads keyboard/mouse input, and updates the camera position and look-at target. No C code changes.

**Tech Stack:** Minic scripting language, Flecs ECS entity naming, existing Iron input/camera C APIs.

---

## File Structure

| File | Action | Responsibility |
|------|--------|----------------|
| `engine/assets/systems/scene3d_test.minic` | Modify | Name camera entity, remove orbit code, update HUD |
| `engine/assets/systems/camera3d_control_system.minic` | Create | Full camera control logic (init/step/draw) |
| `engine/assets/systems.manifest` | Modify | Register the new system after scene3d_test |

---

### Task 1: Modify scene3d_test.minic

**Files:**
- Modify: `engine/assets/systems/scene3d_test.minic`

- [ ] **Step 1: Add entity_set_name after camera creation**

Replace lines 17-19 (the camera creation block):

```
    g_cam = camera_3d_create(60.0, 0.1, 100.0);
    camera_3d_set_position(g_cam, 0.0, 2.0, 5.0);
    camera_3d_look_at(g_cam, 0.0, 0.0, 0.0);
    camera_3d_perspective(g_cam, 60.0, 0.1, 100.0);
```

With:

```
    g_cam = camera_3d_create(60.0, 0.1, 100.0);
    camera_3d_set_position(g_cam, 0.0, 2.0, 5.0);
    camera_3d_look_at(g_cam, 0.0, 0.0, 0.0);
    camera_3d_perspective(g_cam, 60.0, 0.1, 100.0);
    entity_set_name(g_cam, "main_camera");
```

- [ ] **Step 2: Remove commented-out orbit code**

Delete lines 47-52 (the commented orbit block):

```
    // Orbit camera around origin
    //float dist = 5.0;
    //float cx = sin(g_angle) * dist;
    //float cz = cos(g_angle) * dist;
    //camera_3d_set_position(g_cam, cx, 2.0, cz);
    //camera_3d_look_at(g_cam, 0.0, 0.0, 0.0);
```

- [ ] **Step 3: Update draw() HUD text**

Replace the draw() function:

```
int draw(void) {
    draw_set_font("font.ttf", 16);
    draw_set_color(0xffffffff);
    draw_string("3D Scene Test", 10.0, 10.0);
    draw_string("WASD=Move  Q/E=Up/Down  RMB=Look  Scroll=Speed", 10.0, 30.0);
    draw_fps(10.0, 50.0);
    return 0;
}
```

- [ ] **Step 4: Commit**

```bash
git add engine/assets/systems/scene3d_test.minic
git commit -m "feat(scene3d_test): name camera entity for control system hookup"
```

---

### Task 2: Create camera3d_control_system.minic

**Files:**
- Create: `engine/assets/systems/camera3d_control_system.minic`

- [ ] **Step 1: Write the complete script**

Create `engine/assets/systems/camera3d_control_system.minic` with the following content:

```
// camera3d_control_system — free-fly FPS camera controls
// Finds "main_camera" entity and drives it with WASD + mouse look.

id g_cam = 0;
float g_yaw = 0.0;       // horizontal angle (radians)
float g_pitch = -0.3805;  // vertical angle (radians) — matches camera at (0,2,5) looking at (0,0,0)
float g_speed = 5.0;
float g_sensitivity = 0.003;

// Position tracked locally (read from entity at init, updated each frame)
float g_pos_x = 0.0;
float g_pos_y = 2.0;
float g_pos_z = 5.0;

int init(void) {
    g_cam = entity_find_by_name("main_camera");
    if (g_cam == 0) {
        printf("[camera3d_control] Warning: 'main_camera' not found\n");
        return 0;
    }
    printf("[camera3d_control] Camera found, controls active\n");
    return 0;
}

int step(void) {
    if (g_cam == 0) return 0;

    float dt = sys_delta_time();

    // --- Mouse look (right-click drag) ---
    if (mouse_down("right") != 0.0) {
        float dx = mouse_dx();
        float dy = mouse_dy();
        g_yaw = g_yaw - dx * g_sensitivity;
        g_pitch = g_pitch - dy * g_sensitivity;
        // Clamp pitch to avoid gimbal issues
        if (g_pitch > 1.5533) g_pitch = 1.5533;   // ~89 degrees
        if (g_pitch < -1.5533) g_pitch = -1.5533;
    }

    // --- Mouse wheel speed adjustment ---
    float wheel = mouse_wheel_delta();
    if (wheel > 0.0) {
        g_speed = g_speed + 0.5;
    }
    if (wheel < 0.0) {
        g_speed = g_speed - 0.5;
    }
    if (g_speed < 1.0) g_speed = 1.0;
    if (g_speed > 50.0) g_speed = 50.0;

    // --- Compute forward and right vectors from yaw/pitch ---
    // Forward direction (where camera looks)
    float fwd_x = -sin(g_yaw) * cos(g_pitch);
    float fwd_y =  sin(g_pitch);
    float fwd_z = -cos(g_yaw) * cos(g_pitch);

    // Right direction (perpendicular on XZ plane)
    float right_x =  cos(g_yaw);
    float right_z = -sin(g_yaw);

    // --- WASD + QE movement ---
    float move_x = 0.0;
    float move_y = 0.0;
    float move_z = 0.0;

    if (keyboard_down("w") != 0.0) {
        move_x = move_x + fwd_x;
        move_z = move_z + fwd_z;
    }
    if (keyboard_down("s") != 0.0) {
        move_x = move_x - fwd_x;
        move_z = move_z - fwd_z;
    }
    if (keyboard_down("d") != 0.0) {
        move_x = move_x + right_x;
        move_z = move_z + right_z;
    }
    if (keyboard_down("a") != 0.0) {
        move_x = move_x - right_x;
        move_z = move_z - right_z;
    }
    if (keyboard_down("e") != 0.0) {
        move_y = move_y + 1.0;
    }
    if (keyboard_down("q") != 0.0) {
        move_y = move_y - 1.0;
    }

    // Normalize horizontal movement to prevent faster diagonal speed
    float move_len = sqrt(move_x * move_x + move_z * move_z);
    if (move_len > 0.001) {
        move_x = move_x / move_len;
        move_z = move_z / move_len;
    }

    // Apply movement
    float velocity = g_speed * dt;
    g_pos_x = g_pos_x + move_x * velocity;
    g_pos_y = g_pos_y + move_y * velocity;
    g_pos_z = g_pos_z + move_z * velocity;

    // --- Update camera ---
    camera_3d_set_position(g_cam, g_pos_x, g_pos_y, g_pos_z);
    camera_3d_look_at(g_cam, g_pos_x + fwd_x, g_pos_y + fwd_y, g_pos_z + fwd_z);

    return 0;
}

int draw(void) {
    draw_set_font("font.ttf", 14);
    draw_set_color(0xffcccccc);
    draw_string("Speed:", 10.0, 70.0);
    draw_string(str_float1(g_speed), 70.0, 70.0);
    return 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add engine/assets/systems/camera3d_control_system.minic
git commit -m "feat: add camera3d_control_system.minic — FPS camera controls"
```

---

### Task 3: Register in systems.manifest

**Files:**
- Modify: `engine/assets/systems.manifest`

- [ ] **Step 1: Add camera3d_control_system after scene3d_test**

The manifest should read:

```
# Systems manifest — one system per line
# Lines starting with # are comments (disabled)
# Format: SystemName [path/to/script.minic]
#   If path omitted, defaults to data/systems/<name>.minic

# keyboard_test
#frog_system
perf_overlay
draw_ui_ext_system
# vampire_system
scene3d_test
camera3d_control_system
```

The new line is `camera3d_control_system` placed directly after `scene3d_test`. This ensures the camera entity is created before the control system's `init()` tries to find it.

- [ ] **Step 2: Commit**

```bash
git add engine/assets/systems.manifest
git commit -m "feat: register camera3d_control_system in manifest"
```

---

### Task 4: Build and verify

**Files:** None (build + manual test)

- [ ] **Step 1: Export assets and compile**

```bash
cd engine && ../base/make macos metal
cd build && xcodebuild -project IronGame.xcodeproj -configuration Release build
```

Expected: Build succeeds with no errors. Console output should show:
```
[minic_system] Loaded 'camera3d_control_system' from 'data/systems/camera3d_control_system.minic' (step=..., init=..., draw=...)
[camera3d_control] Camera found, controls active
```

- [ ] **Step 2: Run and test controls**

```bash
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame
```

Verify:
1. Cube renders and auto-rotates (existing behavior)
2. HUD shows control hint text and speed value
3. Hold right-click + drag mouse → camera rotates
4. W/A/S/D moves camera horizontally relative to look direction
5. Q moves down, E moves up
6. Mouse scroll changes speed (reflected in HUD)
7. Looking straight up/down does not flip or break

- [ ] **Step 3: Final commit if any fixes needed**

If fixes were needed during testing, commit them:
```bash
git add engine/assets/systems/camera3d_control_system.minic
git commit -m "fix: camera control adjustments from testing"
```
