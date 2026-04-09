# Camera3D Horizontal Stutter Fix — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix jerky horizontal camera rotation when right-click dragging by locking the cursor to enable raw mouse input.

**Architecture:** Expose `iron_mouse_lock()`/`iron_mouse_unlock()` as minic bindings in `runtime_api.c`, then use them in `camera3d_control_system.minic` to lock the cursor during right-click drag. When locked, `mouse_dx()`/`mouse_dy()` automatically use raw OS deltas instead of position-based ones.

**Tech Stack:** C (engine bindings), Minic (game script)

---

### Task 1: Add mouse_lock/mouse_unlock minic bindings

**Files:**
- Modify: `engine/sources/core/runtime_api.c`

- [ ] **Step 1: Add include for iron_system.h**

Add `#include <iron_system.h>` after the existing `#include <iron_input.h>` at line 18:

```c
#include <iron_input.h>
#include <iron_system.h>
```

This is needed because `iron_mouse_lock()` and `iron_mouse_unlock()` are declared in `iron_system.h`, not `iron_input.h`.

- [ ] **Step 2: Add the two binding functions**

Insert these two functions after `minic_mouse_wheel_delta()` (after line 1165), before `minic_gamepad_down`:

```c
static minic_val_t minic_mouse_lock(void) {
    iron_mouse_lock();
    return minic_val_int(0);
}

static minic_val_t minic_mouse_unlock(void) {
    iron_mouse_unlock();
    return minic_val_int(0);
}
```

- [ ] **Step 3: Register the new functions**

Add these two registration calls after the existing `minic_register_native("mouse_wheel_delta", ...)` line (line 1381), before the gamepad registrations:

```c
    minic_register_native("mouse_lock", minic_mouse_lock);
    minic_register_native("mouse_unlock", minic_mouse_unlock);
```

- [ ] **Step 4: Commit**

```bash
git add engine/sources/core/runtime_api.c
git commit -m "feat(runtime_api): expose mouse_lock/mouse_unlock to minic scripts"
```

---

### Task 2: Update camera3d_control_system.minic to lock cursor on right-click

**Files:**
- Modify: `engine/assets/systems/camera3d_control_system.minic`

- [ ] **Step 1: Add cursor lock/unlock calls in the step() function**

Replace the mouse look section (lines 30–39) with cursor-lock-aware version. The new code locks the cursor when right-click starts and unlocks when it's released:

```minic
    // --- Mouse look (right-click drag) ---
    if (mouse_started("right") != 0.0) {
        mouse_lock();
    }
    if (mouse_released("right") != 0.0) {
        mouse_unlock();
    }
    if (mouse_down("right") != 0.0) {
        float dx = mouse_dx();
        float dy = mouse_dy();
        g_yaw = g_yaw - dx * g_sensitivity;
        g_pitch = g_pitch - dy * g_sensitivity;
        // Clamp pitch to avoid gimbal issues
        if (g_pitch > 1.5533) g_pitch = 1.5533;   // ~89 degrees
        if (g_pitch < -1.5533) g_pitch = -1.5533;
    }
```

This replaces lines 30–39 of the original file. The `mouse_started`/`mouse_released` calls detect button transitions (press/release) so we lock exactly once on press and unlock exactly once on release.

- [ ] **Step 2: Commit**

```bash
git add engine/assets/systems/camera3d_control_system.minic
git commit -m "fix(camera3d_control): lock cursor during right-click drag for smooth rotation"
```

---

### Task 3: Build and verify

- [ ] **Step 1: Export assets**

```bash
cd /Users/rayloi/Documents/GitHub/armortools/engine && ../base/make macos metal
```

Expected: completes without errors. The minic script is exported as a data asset.

- [ ] **Step 2: Compile C code**

```bash
cd /Users/rayloi/Documents/GitHub/armortools/engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build
```

Expected: `** BUILD SUCCEEDED **`

- [ ] **Step 3: Run and manually test**

```bash
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame
```

Verify:
1. Hold right mouse button → cursor should disappear (locked)
2. Drag horizontally → camera yaw rotates smoothly, no stuttering
3. Drag vertically → camera pitch rotates smoothly (unchanged)
4. Release right button → cursor reappears at original position
