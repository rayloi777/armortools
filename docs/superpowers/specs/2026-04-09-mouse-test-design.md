# mouse_test.minic Design

**Date:** 2026-04-09
**Status:** Draft

## Summary

Add a new minic system `mouse_test` that displays real-time mouse input state as a visual HUD overlay. Used to verify that mouse buttons, cursor position, deltas, and scroll wheel are all functioning correctly.

## Requirements

- Visual on-screen display of mouse input state
- Show left/right/middle button states (pressed, just pressed, just released)
- Show cursor position (mouse_x, mouse_y)
- Show movement delta (mouse_dx, mouse_dy)
- Show scroll wheel delta (mouse_wheel_delta)
- Flash indicators for started/released events (persist for a few frames)
- FPS counter

## Screen Layout

```
=== Mouse Test ===
[LEFT]  [RIGHT]  [MIDDLE]       ← colored rectangles: bright when down, dark when up
  *                          ← "*" flash on started, "^" flash on released
Position:  (512, 384)
Delta:     (3.2, -1.5)
Wheel:     0.0
```

## Script Structure

```
mouse_test.minic
├── Globals
│   ├── g_flash_left/started/int    -- frame countdown for left started flash
│   ├── g_flash_left_released/int   -- frame countdown for left released flash
│   ├── g_flash_right_started/int   -- (same for right)
│   ├── g_flash_right_released/int
│   ├── g_flash_middle_started/int  -- (same for middle)
│   └── g_flash_middle_released/int
├── step()
│   ├── Read mouse_started for each button → set flash counters (15 frames)
│   ├── Read mouse_released for each button → set flash counters (15 frames)
│   └── Decrement all active flash counters
├── draw()
│   ├── Title "Mouse Test"
│   ├── Button state rectangles (3 boxes)
│   │   ├── Bright color when mouse_down
│   │   ├── Different color when started flash active
│   │   └── Different color when released flash active
│   ├── Position text (mouse_x, mouse_y)
│   ├── Delta text (mouse_dx, mouse_dy)
│   ├── Wheel text (mouse_wheel_delta)
│   └── FPS counter
```

## Implementation Notes

- All drawing uses 2D draw API: `draw_set_color`, `draw_rect`, `draw_string`, `draw_set_font`, `draw_fps`
- Flash duration: 15 frames (~0.25 seconds at 60fps) for started/released indicators
- Button state rectangles at Y=40, sized 80x30 each, spaced 20px apart
- Text info starts at Y=90

## Changes to Existing Files

### systems.manifest

Add `mouse_test` entry (commented out by default, like keyboard_test):
```
# mouse_test
```

No other files modified.
