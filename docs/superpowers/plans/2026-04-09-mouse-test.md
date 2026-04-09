# mouse_test.minic Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create a visual HUD test script that displays real-time mouse input state on screen, verifying buttons, cursor position, deltas, and scroll wheel work correctly.

**Architecture:** Single new minic script using 2D draw API. Follows the same pattern as `keyboard_test.minic` — poll input in `step()`, render state in `draw()`. Flash counters track started/released events for a few frames so brief clicks remain visible.

**Tech Stack:** Minic scripting language, Iron 2D draw API (`draw_filled_rect`, `draw_string`, `draw_set_color`, `draw_set_font`, `draw_fps`), mouse input API (`mouse_down`, `mouse_started`, `mouse_released`, `mouse_x`, `mouse_y`, `mouse_dx`, `mouse_dy`, `mouse_wheel_delta`).

---

## File Structure

| File | Action | Responsibility |
|------|--------|----------------|
| `engine/assets/systems/mouse_test.minic` | Create | Mouse input visual test script |
| `engine/assets/systems.manifest` | Modify | Add commented-out `mouse_test` entry |

---

### Task 1: Create mouse_test.minic

**Files:**
- Create: `engine/assets/systems/mouse_test.minic`

- [ ] **Step 1: Write the complete script**

Create `engine/assets/systems/mouse_test.minic` with the following content:

```
// MouseTest — visual HUD showing real-time mouse input state

// Flash counters for started/released events (15 frames ≈ 0.25s at 60fps)
int g_flash_left_started = 0;
int g_flash_left_released = 0;
int g_flash_right_started = 0;
int g_flash_right_released = 0;
int g_flash_middle_started = 0;
int g_flash_middle_released = 0;

int init(void) {
    printf("MouseTest: Init\n");
    return 0;
}

int step(void) {
    // Detect started/released events and set flash counters
    if (mouse_started("left") > 0) g_flash_left_started = 15;
    if (mouse_released("left") > 0) g_flash_left_released = 15;
    if (mouse_started("right") > 0) g_flash_right_started = 15;
    if (mouse_released("right") > 0) g_flash_right_released = 15;
    if (mouse_started("middle") > 0) g_flash_middle_started = 15;
    if (mouse_released("middle") > 0) g_flash_middle_released = 15;

    // Decrement flash counters
    if (g_flash_left_started > 0) g_flash_left_started = g_flash_left_started - 1;
    if (g_flash_left_released > 0) g_flash_left_released = g_flash_left_released - 1;
    if (g_flash_right_started > 0) g_flash_right_started = g_flash_right_started - 1;
    if (g_flash_right_released > 0) g_flash_right_released = g_flash_right_released - 1;
    if (g_flash_middle_started > 0) g_flash_middle_started = g_flash_middle_started - 1;
    if (g_flash_middle_released > 0) g_flash_middle_released = g_flash_middle_released - 1;

    return 0;
}

int draw(void) {
    draw_set_font("font.ttf", 18);
    draw_set_color(0xffffffff);
    draw_string("Mouse Test", 10.0, 10.0);

    // --- Button state boxes ---
    float box_y = 40.0;
    float box_w = 90.0;
    float box_h = 30.0;
    float gap = 20.0;

    // LEFT button
    float bx = 10.0;
    if (mouse_down("left") > 0) {
        draw_set_color(0xff00cc00);
    } else if (g_flash_left_started > 0) {
        draw_set_color(0xff00ff00);
    } else if (g_flash_left_released > 0) {
        draw_set_color(0xffcc8800);
    } else {
        draw_set_color(0xff444444);
    }
    draw_filled_rect(bx, box_y, box_w, box_h);
    draw_set_color(0xffffffff);
    draw_string("LEFT", bx + 18.0, box_y + 7.0);
    if (g_flash_left_started > 0) {
        draw_set_color(0xff00ff00);
        draw_string("*", bx + 70.0, box_y + 7.0);
    }
    if (g_flash_left_released > 0) {
        draw_set_color(0xffcc8800);
        draw_string("^", bx + 70.0, box_y + 7.0);
    }

    // RIGHT button
    bx = bx + box_w + gap;
    if (mouse_down("right") > 0) {
        draw_set_color(0xff00cc00);
    } else if (g_flash_right_started > 0) {
        draw_set_color(0xff00ff00);
    } else if (g_flash_right_released > 0) {
        draw_set_color(0xffcc8800);
    } else {
        draw_set_color(0xff444444);
    }
    draw_filled_rect(bx, box_y, box_w, box_h);
    draw_set_color(0xffffffff);
    draw_string("RIGHT", bx + 12.0, box_y + 7.0);
    if (g_flash_right_started > 0) {
        draw_set_color(0xff00ff00);
        draw_string("*", bx + 70.0, box_y + 7.0);
    }
    if (g_flash_right_released > 0) {
        draw_set_color(0xffcc8800);
        draw_string("^", bx + 70.0, box_y + 7.0);
    }

    // MIDDLE button
    bx = bx + box_w + gap;
    if (mouse_down("middle") > 0) {
        draw_set_color(0xff00cc00);
    } else if (g_flash_middle_started > 0) {
        draw_set_color(0xff00ff00);
    } else if (g_flash_middle_released > 0) {
        draw_set_color(0xffcc8800);
    } else {
        draw_set_color(0xff444444);
    }
    draw_filled_rect(bx, box_y, box_w, box_h);
    draw_set_color(0xffffffff);
    draw_string("MIDDLE", bx + 6.0, box_y + 7.0);
    if (g_flash_middle_started > 0) {
        draw_set_color(0xff00ff00);
        draw_string("*", bx + 70.0, box_y + 7.0);
    }
    if (g_flash_middle_released > 0) {
        draw_set_color(0xffcc8800);
        draw_string("^", bx + 70.0, box_y + 7.0);
    }

    // --- Info text ---
    float ty = box_y + box_h + 20.0;
    draw_set_font("font.ttf", 16);
    draw_set_color(0xffcccccc);

    draw_string("Position:", 10.0, ty);
    draw_set_color(0xffffffff);
    draw_string(str_float1(mouse_x()), 110.0, ty);
    draw_string(", ", 160.0, ty);
    draw_string(str_float1(mouse_y()), 170.0, ty);

    ty = ty + 22.0;
    draw_set_color(0xffcccccc);
    draw_string("Delta:", 10.0, ty);
    draw_set_color(0xffffffff);
    draw_string(str_float1(mouse_dx()), 110.0, ty);
    draw_string(", ", 160.0, ty);
    draw_string(str_float1(mouse_dy()), 170.0, ty);

    ty = ty + 22.0;
    draw_set_color(0xffcccccc);
    draw_string("Wheel:", 10.0, ty);
    draw_set_color(0xffffffff);
    draw_string(str_float1(mouse_wheel_delta()), 110.0, ty);

    // Legend
    ty = ty + 30.0;
    draw_set_color(0xff00cc00);
    draw_string("= held", 10.0, ty);
    draw_set_color(0xff00ff00);
    draw_string("  * = just pressed", 70.0, ty);
    draw_set_color(0xffcc8800);
    draw_string("  ^ = just released", 250.0, ty);

    draw_set_color(0xffffffff);
    draw_fps(10.0, 580.0);
    return 0;
}
```

- [ ] **Step 2: Commit**

```bash
git add engine/assets/systems/mouse_test.minic
git commit -m "feat: add mouse_test.minic — visual mouse input test"
```

---

### Task 2: Add to systems.manifest

**Files:**
- Modify: `engine/assets/systems.manifest`

- [ ] **Step 1: Add commented-out entry**

Add `# mouse_test` after the existing `# keyboard_test` comment line (line 6). The manifest should read:

```
# Systems manifest — one system per line
# Lines starting with # are comments (disabled)
# Format: SystemName [path/to/script.minic]
#   If path omitted, defaults to data/systems/<name>.minic

# keyboard_test
# mouse_test
#frog_system
perf_overlay
draw_ui_ext_system
# vampire_system
scene3d_test
camera3d_control_system
```

- [ ] **Step 2: Commit**

```bash
git add engine/assets/systems.manifest
git commit -m "feat: add mouse_test entry to systems.manifest"
```

---

### Task 3: Build and verify

**Files:** None (build + manual test)

- [ ] **Step 1: Uncomment mouse_test in manifest for testing**

Temporarily edit `engine/assets/systems.manifest` to uncomment `mouse_test`:
```
mouse_test
```
(Change `# mouse_test` to `mouse_test`)

- [ ] **Step 2: Export assets and compile**

```bash
cd engine && ../base/make macos metal
cd build && xcodebuild -project IronGame.xcodeproj -configuration Release build
```

Expected: Build succeeds.

- [ ] **Step 3: Run and test**

```bash
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame
```

Verify:
1. "Mouse Test" title appears on screen
2. Three button boxes (LEFT, RIGHT, MIDDLE) — dark when idle, bright green when held
3. Click left mouse button → LEFT box turns green, `*` flash appears
4. Release left mouse button → LEFT box goes dark, `^` flash appears briefly
5. Same for right and middle buttons
6. Position values update as cursor moves
7. Delta values change while cursor moves, go to 0 when still
8. Scroll wheel shows non-zero values when scrolling
9. Legend at bottom explains color meanings
10. FPS counter visible

- [ ] **Step 4: Re-comment mouse_test in manifest**

After testing, comment it back out:
```
# mouse_test
```

Commit any fixes if needed:
```bash
git add engine/assets/systems/mouse_test.minic engine/assets/systems.manifest
git commit -m "fix: mouse_test adjustments from testing"
```
