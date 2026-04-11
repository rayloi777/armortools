# MINIC_README.md Update Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Update MINIC_README.md to reflect actual Minic API patterns used in engine systems, including draw_ui callback, id type, iterator-style queries, 3D camera, mouse lock, and string conversion functions.

**Architecture:** Read actual Minic systems in engine/assets/systems/ to verify API patterns, then rewrite MINIC_README.md sections to match reality.

**Tech Stack:** MINIC_README.md (documentation)

---

## File Structure

- **Modify:** `MINIC_README.md` — Main documentation file to update

No new files or tests needed — this is purely documentation update based on existing code patterns.

---

## Task Map

| Section | Changes |
|---------|---------|
| Script Lifecycle | Add `draw_ui` callback explanation |
| id Type | Expand with actual usage patterns |
| Query API | Add iterator-style pattern documentation |
| Input API | Add mouse lock/unlock section |
| Camera API | Add 3D camera functions |
| Iron Engine API | Add string conversion functions |
| Complete Example | Replace with id-based frog_system pattern |

---

## Tasks

### Task 1: Update Script Lifecycle — Add draw_ui Callback

**Files:**
- Modify: `MINIC_README.md:20-42` (Script Lifecycle section)

- [ ] **Step 1: Read current Script Lifecycle section**

Locate lines 20-42 in MINIC_README.md to understand current structure.

- [ ] **Step 2: Add draw_ui callback documentation**

Replace the Script Lifecycle section content with:

```markdown
## Script Lifecycle

A script can define up to four callback functions:

```c
int init(void) {
    // Called once when the script loads
    return 0;
}

int step(void) {
    // Called every frame (update logic)
    return 0;
}

int draw(void) {
    // Called every frame (render 2D overlay in 3D scene)
    return 0;
}

int draw_ui(void) {
    // Called every frame after all 3D rendering (UI/HUD overlay)
    return 0;
}
```

All four are optional. `draw_ui` is used for game HUD, debug overlays, and UI elements that should render on top of everything else. Return values are currently ignored.
```

- [ ] **Step 3: Commit**

```bash
git add MINIC_README.md
git commit -m "docs(minic): add draw_ui callback to Script Lifecycle"
```

---

### Task 2: Expand id Type Documentation

**Files:**
- Modify: `MINIC_README.md:49-66` (Types section with id type)

- [ ] **Step 1: Read current Types section**

Locate the id type documentation (around lines 49-66).

- [ ] **Step 2: Update id type with actual usage patterns**

Replace the existing id type content with:

```markdown
| `id`    | 64-bit entity/component ID (Flecs `ecs_id_t`)        |

The `id` type stores 64-bit Flecs entity/component IDs natively (`uint64_t`). It supports comparison operators (`==`, `!=`, `<`, `>`, `<=`, `>=`) via a fast path that compares `uint64_t` directly. It does **not** support arithmetic. Use it for variables that hold entity or component IDs returned by `entity_create()`, `component_lookup()`, `query_iter_entity_id()`, `entity_find_by_name()`, etc.

```c
// Example: id variables for entities and components
id g_player = 0;
id g_pos_comp = 0;
id g_camera = 0;

int init(void) {
    g_pos_comp = component_lookup("comp_2d_position");
    g_player = entity_create();
    entity_add(g_player, g_pos_comp);
    g_camera = entity_find_by_name("main_camera");
}
```

**Global `id` variables must be initialized to `0`** — uninitialized `id` variables may cause undefined behavior when passed to ECS functions.
```

- [ ] **Step 3: Commit**

```bash
git add MINIC_README.md
git commit -m "docs(minic): expand id type documentation with init requirement"
```

---

### Task 3: Expand Query API — Add Iterator-Style Pattern

**Files:**
- Modify: `MINIC_README.md:281-313` (Queries section)

- [ ] **Step 1: Read current Queries section**

Locate lines 281-313 to understand current query documentation.

- [ ] **Step 2: Add iterator-style query patterns**

Replace the Queries section with:

```markdown
### Queries

Queries iterate over all entities that have a specific set of components. Two patterns are available.

**Pattern 1: Callback-style (query_foreach)**

```c
// Create a query for entities with a specific component
int q = query_create("FrogVelocity");

// Callback called once per entity
int ai_frog_step(id entity, void *vel) {
    float vx = comp_get_float(g_vel_comp, vel, "vx");
    float vy = comp_get_float(g_vel_comp, vel, "vy");
    // ... process entity ...
    return 0;
}
query_foreach(q, ai_frog_step);
```

**Pattern 2: Iterator-style (direct iteration in Minic)**

```c
// Create query from component name
int q = query_create("EnemyBody");

// Or create and configure manually
int q2 = query_new();
query_with(q2, g_health_comp);
query_find(q2);

// Iterator loop
query_iter_begin(q);
while (query_iter_next(q)) {
    int count = query_iter_count(q);
    for (int i = 0; i < count; i++) {
        id entity = query_iter_entity_id(q, i);
        void *data = query_iter_comp_ptr(q, i, 0);  // comp_index = 0
        // ... process entity ...
    }
}

// Get entity by index (old style, still works)
int count = query_find(q);
for (int i = 0; i < count; i++) {
    id e = query_get(q, i);
    void *data = entity_get(e, comp_id);
    // ...
}
```

**Deferred entity destruction:** When destroying entities during iteration, collect them in a buffer and destroy after the loop:

```c
id g_dead_enemies[200];
int g_dead_enemy_count = 0;

void destroy_dead(void) {
    int i = 0;
    while (i < g_dead_enemy_count) {
        entity_destroy(g_dead_enemies[i]);
        i++;
    }
    g_dead_enemy_count = 0;
}

// During query iteration:
if (should_destroy) {
    if (g_dead_enemy_count < 200) {
        g_dead_enemies[g_dead_enemy_count] = query_iter_entity_id(q, i);
        g_dead_enemy_count++;
    }
}
destroy_dead();  // called after iteration completes
```
```

- [ ] **Step 3: Commit**

```bash
git add MINIC_README.md
git commit -m "docs(minic): add iterator-style query pattern documentation"
```

---

### Task 4: Add Mouse Lock/Unlock to Input API

**Files:**
- Modify: `MINIC_README.md:334-378` (Input API section)

- [ ] **Step 1: Read current Input API section**

Locate lines 334-378 for the Mouse section.

- [ ] **Step 2: Add mouse lock/unlock after Mouse section**

After the existing mouse section (ending with `mouse_wheel_delta()`), add:

```markdown
### Mouse Lock (FPS Controls)

For FPS-style camera control, lock the mouse to the window while dragging:

```c
// Begin lock on right-click start
if (mouse_started("right") != 0.0) {
    mouse_lock();
}
// End lock on right-click release
if (mouse_released("right") != 0.0) {
    mouse_unlock();
}
// Read relative movement while locked
if (mouse_down("right") != 0.0) {
    float dx = mouse_dx();
    float dy = mouse_dy();
    g_yaw = g_yaw - dx * sensitivity;
    g_pitch = g_pitch - dy * sensitivity;
    // Clamp pitch to prevent gimbal lock
    if (g_pitch > 1.5533) g_pitch = 1.5533;
    if (g_pitch < -1.5533) g_pitch = -1.5533;
}
```

`mouse_lock()` captures the mouse cursor within the window. `mouse_dx()`/`mouse_dy()` return relative movement since the last frame. Use `mouse_wheel_delta()` to adjust movement speed.
```

- [ ] **Step 3: Commit**

```bash
git add MINIC_README.md
git commit -m "docs(minic): add mouse lock/unlock for FPS controls"
```

---

### Task 5: Add 3D Camera API to Camera API Section

**Files:**
- Modify: `MINIC_README.md:403-414` (Camera API section)

- [ ] **Step 1: Read current Camera API section**

Locate lines 403-414.

- [ ] **Step 2: Add 3D camera functions after existing 2D section**

After the existing 2D Camera API content, add:

```markdown
### 3D Camera

```c
// Set camera position and look target
camera_3d_set_position(cam, x, y, z);
camera_3d_look_at(cam, target_x, target_y, target_z);

// Get camera position
float cx = camera_3d_get_x(cam);
float cy = camera_3d_get_y(cam);
float cz = camera_3d_get_z(cam);

// Find camera by name
id cam = entity_find_by_name("main_camera");
if (cam == 0) {
    printf("[camera] Warning: 'main_camera' not found\n");
    return 0;
}
```

Note: Camera movement is typically done by reading/tracking a world position and calling `camera_3d_set_position()` each frame, rather than storing camera position internally.
```

- [ ] **Step 3: Commit**

```bash
git add MINIC_README.md
git commit -m "docs(minic): add 3D camera API documentation"
```

---

### Task 6: Add String Conversion Functions to Iron Engine API

**Files:**
- Modify: `MINIC_README.md:453-544` (Iron Engine API section)

- [ ] **Step 1: Read Iron Engine API section**

Locate around lines 453-544.

- [ ] **Step 2: Add string conversion section after Strings subsection**

After the Strings subsection (ending with `i32_to_string_hex`), add:

```markdown
### String Conversion

```c
// Convert numeric values to strings for display
char *s = str_float(float_value);   // e.g., "3.14"
char *s = str_int(int_value);       // e.g., "42"

// Usage with draw_string
draw_string(str_float(g_speed), 70.0, 70.0);
draw_string(str_int(g_level), m + 30.0, y);
```
```

- [ ] **Step 3: Commit**

```bash
git add MINIC_README.md
git commit -m "docs(minic): add string conversion functions"
```

---

### Task 7: Replace Complete Example with id-Based frog_system

**Files:**
- Modify: `MINIC_README.md:547-726` (Complete Example section)

- [ ] **Step 1: Read current Complete Example section**

Locate lines 547-726.

- [ ] **Step 2: Replace with actual frog_system.minic pattern**

Replace the entire Complete Example section with:

```markdown
## Complete Example

A bouncing frog system with 5000 AI frogs and 1 player-controlled frog, using `id` type, `query_foreach`, `draw_ui`, keyboard input, and camera follow:

```c
// FrogSystem - 5000 AI frogs + 1 player
// Keyboard: Arrows to move, Space to jump

float GRAVITY = 2200.0;
float GROUND_Y = 20.0;
float JUMP_VEL = 600.0;
float AI_MOVE_SPEED = 200.0;
float WALL_LEFT = 0.0;
float WALL_RIGHT = 2000.0;
int AI_FROG_COUNT = 5000;

// Component IDs (id type for ECS handles)
id g_pos_comp = 0;
id g_sprite_comp = 0;
id g_vel_comp = 0;

// Query for AI frogs (entities with FrogVelocity)
int g_vel_query = -1;
float g_dt = 0.016;

// Player state (no FrogVelocity — excluded from query)
id g_player = 0;
float g_pvx = 0.0;
float g_pvy = 0.0;
int g_pground = 1;
float g_pdir = 1.0;
int g_pjump_press = 0;

int init(void) {
    printf("FrogSystem: Init\n");
    g_pos_comp = component_lookup("comp_2d_position");
    g_sprite_comp = component_lookup("comp_2d_sprite");
    g_vel_comp = component_register("comp_frog_velocity", 8);
    component_add_field(g_vel_comp, "vx", TYPE_FLOAT, 0);
    component_add_field(g_vel_comp, "vy", TYPE_FLOAT, 4);

    // Spawn AI frogs
    int i = 0;
    while (i < AI_FROG_COUNT) {
        id e = entity_create();
        entity_add(e, g_pos_comp);
        entity_add(e, g_sprite_comp);
        entity_add(e, g_vel_comp);
        void *pos = entity_get(e, g_pos_comp);
        comp_set_float(g_pos_comp, pos, "x", 100.0 + i * 80.0);
        comp_set_float(g_pos_comp, pos, "y", GROUND_Y);
        float dir = 1.0;
        if (i % 2 == 1) { dir = 0.0 - 1.0; }
        void *vel = entity_get(e, g_vel_comp);
        comp_set_float(g_vel_comp, vel, "vx", dir * AI_MOVE_SPEED);
        comp_set_float(g_vel_comp, vel, "vy", 0.0);
        void *spr = entity_get(e, g_sprite_comp);
        comp_set_float(g_sprite_comp, spr, "pivot_x", 0.5);
        comp_set_float(g_sprite_comp, spr, "pivot_y", 1.0);
        comp_set_float(g_sprite_comp, spr, "scale_x", 1.0);
        comp_set_float(g_sprite_comp, spr, "scale_y", 1.0);
        comp_set_bool(g_sprite_comp, spr, "visible", 1);
        comp_set_int(g_sprite_comp, spr, "layer", 30);
        r2d_sprite_create(e, "frog_idle.k");
        i = i + 1;
    }

    // Player frog (no FrogVelocity)
    g_player = entity_create();
    entity_add(g_player, g_pos_comp);
    entity_add(g_player, g_sprite_comp);
    void *ppos = entity_get(g_player, g_pos_comp);
    comp_set_float(g_pos_comp, ppos, "x", 640.0);
    comp_set_float(g_pos_comp, ppos, "y", GROUND_Y);
    void *pspr = entity_get(g_player, g_sprite_comp);
    comp_set_float(g_sprite_comp, pspr, "pivot_x", 0.5);
    comp_set_float(g_sprite_comp, pspr, "pivot_y", 1.0);
    comp_set_float(g_sprite_comp, pspr, "scale_x", 2.0);
    comp_set_float(g_sprite_comp, pspr, "scale_y", 2.0);
    comp_set_bool(g_sprite_comp, pspr, "visible", 1);
    comp_set_int(g_sprite_comp, pspr, "layer", 35);
    r2d_sprite_create(g_player, "frog_idle.k");

    g_vel_query = query_create("comp_frog_velocity");
    printf("FrogSystem: ready\n");
    return 0;
}

// Callback for query_foreach — called once per AI frog
int ai_frog_step(id entity, void *vel) {
    float dt = g_dt;
    float vx = comp_get_float(g_vel_comp, vel, "vx");
    float vy = comp_get_float(g_vel_comp, vel, "vy");
    vy = vy + GRAVITY * dt;

    void *pos = entity_get(entity, g_pos_comp);
    if (!pos) { return 0; }

    float px = comp_get_float(g_pos_comp, pos, "x");
    float py = comp_get_float(g_pos_comp, pos, "y");
    px = px + vx * dt;
    py = py + vy * dt;

    if (py >= GROUND_Y) {
        py = GROUND_Y;
        vy = 0.0 - JUMP_VEL * (0.5 + 0.5 * rand_float());
        vx = 0.0 - vx;
    }
    if (px < WALL_LEFT) { px = WALL_LEFT; vx = 0.0 - vx; }
    if (px > WALL_RIGHT) { px = WALL_RIGHT; vx = 0.0 - vx; }

    comp_set_float(g_pos_comp, pos, "x", px);
    comp_set_float(g_pos_comp, pos, "y", py);
    comp_set_float(g_vel_comp, vel, "vx", vx);
    comp_set_float(g_vel_comp, vel, "vy", vy);
    return 0;
}

int step(void) {
    float dt = sys_delta_time();
    if (dt <= 0.0) { dt = 0.016; }
    if (dt > 0.05) { dt = 0.05; }
    g_dt = dt;

    // Update all AI frogs via query_foreach
    query_foreach(g_vel_query, ai_frog_step);

    // Player movement
    g_pvx = 0.0;
    if (keyboard_down("left")) { g_pdir = 0.0 - 1.0; }
    if (keyboard_down("right")) { g_pdir = 1.0; }
    if (keyboard_down("left") || keyboard_down("right")) {
        g_pvx = g_pdir * AI_MOVE_SPEED;
    }
    void *pspr = entity_get(g_player, g_sprite_comp);
    if (pspr) {
        comp_set_bool(g_sprite_comp, pspr, "flip_x", g_pdir < 0.0);
    }

    // Jump
    int space = keyboard_down("space");
    if (space) {
        if (g_pground && g_pjump_press == 0) {
            g_pvy = 0.0 - JUMP_VEL;
            g_pground = 0;
        }
    }
    g_pjump_press = space;
    g_pvy = g_pvy + GRAVITY * dt;

    // Apply player movement
    void *p = entity_get(g_player, g_pos_comp);
    if (p) {
        float px = comp_get_float(g_pos_comp, p, "x");
        float py = comp_get_float(g_pos_comp, p, "y");
        px = px + g_pvx * dt;
        py = py + g_pvy * dt;
        if (py >= GROUND_Y) {
            py = GROUND_Y;
            g_pvy = 0.0;
            g_pground = 1;
        }
        if (px < WALL_LEFT) { px = WALL_LEFT; g_pdir = 1.0; }
        if (px > WALL_RIGHT) { px = WALL_RIGHT; g_pdir = 0.0 - 1.0; }
        comp_set_float(g_pos_comp, p, "x", px);
        comp_set_float(g_pos_comp, p, "y", py);
        camera_follow(px, py);
    }

    return 0;
}

int draw_ui(void) {
    draw_set_font("font.ttf", 16);
    draw_set_color(0xffffffff);
    draw_string("Arrows+Space = Player | AI frogs bounce", 10.0, 10.0);
    draw_fps(10.0, 32.0);
    return 0;
}
```
```

- [ ] **Step 3: Commit**

```bash
git add MINIC_README.md
git commit -m "docs(minic): replace complete example with id-based frog_system"
```

---

### Task 8: Verify All Sections

**Files:**
- Modify: `MINIC_README.md` (review whole file)

- [ ] **Step 1: Read entire MINIC_README.md**

Verify all changes from Tasks 1-7 are present and consistent.

- [ ] **Step 2: Check for gaps**

Verify:
- Script Lifecycle has `draw_ui`
- id type has initialization requirement
- Query API has iterator pattern
- Input API has mouse lock
- Camera API has 3D section
- Iron Engine API has string conversion
- Complete Example uses id type and draw_ui

- [ ] **Step 3: Final commit if needed**

If any fixes needed, commit as:
```bash
git add MINIC_README.md
git commit -m "docs(minic): fix MINIC_README consistency issues"
```

---

## Self-Review Checklist

- [ ] All 7 tasks present
- [ ] No "TBD", "TODO", placeholder content
- [ ] Code blocks for all implementation steps
- [ ] Exact file paths in all tasks
- [ ] Commit messages for all steps
- [ ] Complete Example reflects actual frog_system.minic patterns

---

## Execution Options

Plan complete and saved to `docs/superpowers/plans/2026-04-11-minic-readme-update.md`.

**Two execution options:**

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints

**Which approach?**