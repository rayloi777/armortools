# Fix: Camera3D Horizontal Stutter on Right-Click Drag

**Date:** 2026-04-09
**Status:** Approved

## Problem

When holding right mouse button to look around in the 3D camera, horizontal (yaw) rotation stutters while vertical (pitch) rotation is smooth.

**Root cause:** `mouse_dx()` returns position-based deltas (`x - mouse_last_x`) when the cursor is unlocked. macOS mouse acceleration makes horizontal cursor deltas inconsistent, causing jerky yaw rotation. Vertical deltas are less affected because macOS acceleration is less aggressive vertically.

## Solution

Lock the mouse cursor when right-click is held. When locked, `mouse_dx()`/`mouse_dy()` use raw input deltas (`movement_x`/`movement_y`) which are consistent and unaffected by OS cursor acceleration.

## Changes

### 1. `engine/sources/core/runtime_api.c`

Add two minic bindings exposing existing Iron API functions:

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

Register alongside existing mouse functions:

```c
minic_register_native("mouse_lock", minic_mouse_lock);
minic_register_native("mouse_unlock", minic_mouse_unlock);
```

### 2. `engine/assets/systems/camera3d_control_system.minic`

Track right-button held state and lock/unlock cursor accordingly:

- When `mouse_started("right")` fires → call `mouse_lock()`
- When `mouse_released("right")` fires → call `mouse_unlock()`
- Existing mouse look code (`mouse_dx()`/`mouse_dy()`) unchanged — it automatically benefits from raw input when cursor is locked

## Scope

- Two new C binding functions (~10 lines) in `runtime_api.c`
- ~5 lines changed in `camera3d_control_system.minic`
- No new state, config, or architectural changes
