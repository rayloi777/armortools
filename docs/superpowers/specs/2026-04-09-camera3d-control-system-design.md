# camera3d_control_system.minic Design

**Date:** 2026-04-09
**Status:** Draft

## Summary

Add a new minic system `camera3d_control_system` that provides free-fly / FPS-style camera controls for the 3D scene test. The system reads keyboard and mouse input each frame to move and rotate a named camera entity.

## Requirements

- Right-click drag to rotate camera (mouse look)
- WASD for horizontal movement on the XZ plane
- Q/E for vertical movement (down/up)
- Mouse scroll to adjust movement speed
- HUD overlay showing controls and current speed

## Architecture

### Cross-System Communication

Each minic system has an isolated `minic_ctx_t`, so globals cannot be shared directly. Systems communicate through named entities:

1. `scene3d_test.minic` names its camera with `entity_set_name(g_cam, "main_camera")`
2. `camera3d_control_system.minic` finds it with `entity_find_by_name("main_camera")`

### Orientation Tracking (Yaw/Pitch)

The system tracks camera orientation as two float globals: `g_yaw` (horizontal rotation, radians) and `g_pitch` (vertical rotation, radians, clamped to +/-89 degrees). Each frame, forward and right vectors are computed from yaw/pitch using `sin`/`cos`:

```
forward_x = -sin(yaw) * cos(pitch)
forward_y =  sin(pitch)
forward_z = -cos(yaw) * cos(pitch)
```

Movement is applied to position using these vectors, then `camera_3d_set_position()` and `camera_3d_look_at()` sync the camera.

### No New C Code Required

All logic is in minic. The system uses existing APIs: `keyboard_down`, `mouse_started`, `mouse_down`, `mouse_dx`, `mouse_dy`, `mouse_wheel_delta`, `camera_3d_set_position`, `camera_3d_look_at`, `entity_find_by_name`, `sin`, `cos`, `sqrt`, `sys_delta_time`.

## Controls

| Input | Action |
|-------|--------|
| Right-click drag | Rotate camera (yaw/pitch) |
| W | Move forward |
| S | Move backward |
| A | Move left (strafe) |
| D | Move right (strafe) |
| Q | Move down |
| E | Move up |
| Mouse scroll up | Increase speed |
| Mouse scroll down | Decrease speed |

## Script Structure

```
camera3d_control_system.minic
├── Globals
│   ├── g_cam: id           -- camera entity (found by name)
│   ├── g_yaw: float        -- horizontal angle (radians, default 0)
│   ├── g_pitch: float      -- vertical angle (radians, default ~-22 deg)
│   ├── g_speed: float      -- movement speed (default 5.0)
│   └── g_sensitivity: float -- mouse look sensitivity (default 0.003)
├── init()
│   └── Find camera via entity_find_by_name("main_camera")
│       Set initial yaw/pitch from starting position (0, 2, 5) looking at (0, 0, 0)
├── step()
│   ├── If no camera found, skip
│   ├── Read mouse input (right-click drag → update yaw/pitch)
│   ├── Read mouse wheel (adjust speed)
│   ├── Compute forward/right vectors from yaw/pitch
│   ├── Read WASD/QE keys → compute position delta
│   ├── Update camera position
│   └── Set look-at target = position + forward vector
└── draw()
    └── Display controls help text and current speed
```

## Changes to Existing Files

### scene3d_test.minic

- Add `entity_set_name(g_cam, "main_camera")` after camera creation
- Remove commented-out orbit code (lines 47-52)
- Update draw() text to reflect new camera controls

### systems.manifest

Add `camera3d_control_system` after `scene3d_test` to ensure the camera exists before the control system tries to find it:

```
scene3d_test
camera3d_control_system
```

## Limitations

- No `atan2` available in minic, so initial yaw/pitch are hardcoded to match the default camera position (0, 2, 5) looking at (0, 0, 0). The approximate values are yaw=0, pitch=-0.38 rad (~-22 degrees).
- No quaternion math in minic — using yaw/pitch euler angles instead. Gimbal lock is not a practical concern for this use case.
