# Minic Scripting Language

## Upstream Sync — 2026-04-05

Merged upstream (70 commits): BC7 texture compression, Kong shader rewrite, camera pivot, material nodes. All changes landed in `base/` and `paint/` only. **No files in `engine/` were modified by the merge.** Upstream added `MINIC_T_VOID = 5` to minic.h; our `MINIC_T_ID` is now 6.

---

Minic is a lightweight C-like scripting language embedded in the Iron engine. Scripts are written in `.minic` files, loaded at runtime, and compiled to bytecode executed by a register-based VM. No separate compilation step is needed — edit a script and reload to see changes.

## Where Scripts Live

```
engine/assets/scripts/   # Entry-point scripts (game logic)
engine/assets/systems/   # ECS systems (init/step/draw callbacks)
```

Scripts are loaded via `load_and_run_script()` in `game_engine.c`. Each script runs in its own Minic context backed by an 8 MB arena allocator.

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

---

## Language Syntax

### Types

| Type    | Description                                          |
|---------|------------------------------------------------------|
| `int`   | 32-bit signed integer                                |
| `float` | 32-bit floating-point                                |
| `bool`  | Boolean (stored as `int`: 0 or 1)                    |
| `char`  | Character (stored as `int`)                          |
| `id`    | 64-bit entity/component ID (Flecs `ecs_id_t`)        |
| `void`  | No return value                                      |
| `ptr`   | Generic pointer (maps to `void *`)                   |

The `id` type stores 64-bit Flecs entity/component IDs natively (`uint64_t`). It supports comparison operators (`==`, `!=`, `<`, `>`, `<=`, `>=`) via a fast path that compares `uint64_t` directly. It does **not** support arithmetic. Use it for variables that hold entity or component IDs returned by `entity_create()`, `component_lookup()`, `query_foreach()` callbacks, etc.

```c
// Example:
id g_player = entity_create();
id g_pos_comp = component_lookup("comp_2d_position");
entity_add(g_player, g_pos_comp);
```

### Variables

```c
int x = 10;
float speed = 3.5;
bool alive = true;
char *name = "hello";
void *data = NULL;

// Global arrays
int g_entities[128];
float g_deltas[4];
```

### Operators

**Arithmetic:** `+  -  *  /  %`
**Comparison:** `==  !=  <  >  <=  >=`
**Logical:** `&&  ||  !`
**Bitwise:** `&  |  ^  <<  >>` (via host bindings)
**Assignment:** `=  +=  -=  *=  /=  %=`
**Increment/Decrement:** `++  --`
**Address/Dereference:** `&  *` (pointer types)

> **Not supported:** Ternary `?:` — use `if`/`else` instead.

### Control Flow

```c
if (x > 10) {
    // ...
} else if (x == 5) {
    // ...
} else {
    // ...
}

while (i < 10) {
    i++;
    if (i == 3) continue;
    if (i == 8) break;
}

for (int j = 0; j < 5; j++) {
    // ...
}
```

> **Note:** The ternary operator (`?:`) is **not supported**. Use `if`/`else` instead. Using `?:` causes a silent compiler error that skips all subsequent statements in the block.

### Functions

```c
int add(int a, int b) {
    return a + b;
}

void log_msg(char *msg) {
    printf("Message: %s\n", msg);
}

// Forward declarations not needed — define before use.
```

Functions can also be passed as callbacks:
```c
int my_callback(int entity, void *data) {
    return 0;
}
query_foreach(g_query, my_callback);
```

### Comments

```c
// Single-line comment

/*
   Multi-line comment
*/
```

### Literals

```c
int   decimal = 42;
int   hex     = 0xFF;
float f       = 3.14;
char  c       = 'A';
char *s       = "hello\n";
```

Escape sequences in strings and chars: `\n  \t  \\  \"  \0`

### Type Casting

Implicit conversion between `int` and `float` in expressions. Use explicit style when needed:

```c
int i = 10;
float f = i * 1.0;   // int promoted to float
int back = f;         // float truncated to int
```

### Structs and Enums

Structs and enums are declared in C and registered with the Minic runtime via `minic_register_struct()` and `minic_register_enum()`. Scripts access struct fields with `.` notation:

```c
// Registered by the engine:
// vec2_t { x, y }
vec2_t pos;
pos.x = 100.0;
pos.y = 200.0;
```

Enum constants are used as plain identifiers:
```c
// UI_LAYOUT_VERTICAL, UI_LAYOUT_HORIZONTAL are registered enum values
```

---

## Engine ECS API

### Components

```c
// Register a new component type (returns component ID)
int comp_id = component_register("Position", 12);  // name, size_in_bytes

// Add fields to a component
component_add_field(comp_id, "x", TYPE_FLOAT, 0);   // name, type, byte_offset
component_add_field(comp_id, "y", TYPE_FLOAT, 4);
component_add_field(comp_id, "z", TYPE_FLOAT, 8);

// Look up a built-in or previously registered component
int pos_comp = component_lookup("comp_2d_position");

// Finalize: register the component as a Minic struct for field access
component_finalize(comp_id, "Position");

// Get component info
int size = component_get_size(comp_id);
int alignment = component_get_alignment(comp_id);
int field_count = component_get_field_count(comp_id);
```

### Component Field Access

```c
void *data = entity_get(entity_id, comp_id);

// Single field get/set
float x = comp_get_float(comp_id, data, "x");
comp_set_float(comp_id, data, "x", 100.0);

int color = comp_get_int(comp_id, data, "color");
comp_set_int(comp_id, data, "color", 0xff3366ff);

bool visible = comp_get_bool(comp_id, data, "visible");
comp_set_bool(comp_id, data, "visible", 1);

void *ptr = comp_get_ptr(comp_id, data, "texture");
comp_set_ptr(comp_id, data, "texture", my_tex);

// Add to a float field (avoids separate get+set)
comp_add_float(comp_id, data, "x", 1.5);  // data.x += 1.5
```

### Batch Field Access

Batch operations read/write multiple fields in one call, reducing dispatch overhead:

```c
float values[3];
// Set multiple fields at once
values[0] = 1.0; values[1] = 2.0; values[2] = 3.0;
comp_set_floats(comp_id, data, "x,y,z", values);

// Get multiple fields
float out[3];
comp_get_floats(comp_id, data, "x,y,z", out);

// Add to multiple fields
float deltas[2] = { 0.01, 0.01 };
comp_add_floats(comp_id, data, "vx,vy", deltas);
```

### Entities

```c
int e = entity_create();                     // Create entity (returns ID)
int e2 = entity_create_named("player");      // Create with name
entity_destroy(e);                           // Destroy entity

entity_add(e, comp_id);                      // Add component to entity
entity_remove(e, comp_id);                   // Remove component
int has = entity_has(e, comp_id);            // Check component
int exists = entity_exists(e);               // Check entity alive
int valid = entity_is_valid(e);              // Check entity valid

char *name = entity_get_name(e);             // Get entity name
entity_set_name(e, "enemy");                 // Set entity name
int found = entity_find_by_name("player");   // Find by name

int parent = entity_get_parent(e);           // Get parent
entity_set_parent(child, parent);            // Set parent

void *data = entity_get(e, comp_id);         // Get component data pointer
entity_set_data(e, comp_id, data);           // Set component data
```

### Queries

Queries iterate over all entities that have a specific set of components.

```c
// Create a query for entities with a specific component
int q = query_create("BenchVelocity");

// Iterator-style iteration
query_iter_begin(q);
while (query_iter_next(q)) {
    int count = query_iter_count(q);
    for (int i = 0; i < count; i++) {
        int entity = query_iter_entity(q, i);
        void *data = query_iter_comp_ptr(q, i, 0);  // comp_index = 0
        // ... process entity
    }
}

// Callback-style iteration (C iterates, calls your function per entity)
int my_step(int entity, void *data) {
    comp_add_float(g_comp, data, "x", 1.0);
    return 0;
}
query_foreach(q, my_step);

// Batch callback (C iterates, calls once per chunk with contiguous data)
int my_batch(int count, void *data_array) {
    // Process 'count' entities, data is contiguous
    return 0;
}
query_foreach_batch(q, my_batch);
```

---

## System API

```c
float dt    = sys_delta();    // Frame delta time (seconds)
float time  = sys_time();     // Total elapsed time
int frame   = sys_frame();    // Frame counter
float fps   = sys_fps();      // Current FPS
float width = sys_width();    // Window width (float)
float height = sys_height();  // Window height (float)

float r = rand_float();               // Random float [0, 1)
float r = rand_range(10.0, 20.0);     // Random float [min, max)
float t = bench_time();               // High-precision timer (seconds since first call)
```

---

## Input API

### Keyboard

```c
int down     = keyboard_down("w");       // Key held
int pressed  = keyboard_started("space"); // Key just pressed
int released = keyboard_released("esc");  // Key just released
int rep      = keyboard_repeat("a");      // Key repeat (held)
```

Key names: `"a"`-`"z"`, `"0"`-`"9"`, `"space"`, `"enter"`, `"esc"`, `"up"`, `"down"`, `"left"`, `"right"`, `"shift"`, `"ctrl"`, `"tab"`, `"backspace"`, etc.

### Mouse

```c
int held     = mouse_down("left");       // "left", "right", "middle"
int pressed  = mouse_started("left");
int released = mouse_released("left");

float x  = mouse_x();            // Screen position
float y  = mouse_y();
float dx = mouse_dx();           // Movement delta
float dy = mouse_dy();
float wheel = mouse_wheel_delta();

float vx = mouse_view_x();       // View-space position
float vy = mouse_view_y();
```

### Gamepad

```c
int down     = gamepad_down(0, "a");       // (gamepad_index, button)
int pressed  = gamepad_started(0, "a");
int released = gamepad_released(0, "a");

float lx = gamepad_stick_left_x(0);   // Left stick X [-1, 1]
float ly = gamepad_stick_left_y(0);
float rx = gamepad_stick_right_x(0);  // Right stick X
float ry = gamepad_stick_right_y(0);

float dlx = gamepad_stick_delta_x(0); // Stick delta
float dly = gamepad_stick_delta_y(0);
```

---

## 2D Drawing API

```c
draw_begin(NULL, 0, 0);                    // Begin 2D drawing (target, clear, color)
draw_end();                                 // End 2D drawing

draw_set_color(0xffffffff);                 // Set draw color (ABGR)
draw_set_font("font.ttf", 16);             // Set font (returns int)
draw_string("Hello", 10.0, 10.0);          // Draw text at (x, y)

draw_line(0.0, 0.0, 100.0, 100.0, 2.0);   // (x0, y0, x1, y1, strength)
draw_filled_rect(10.0, 10.0, 200.0, 50.0); // (x, y, w, h)
draw_rect(10.0, 10.0, 200.0, 50.0, 1.0);  // (x, y, w, h, strength)
draw_filled_circle(400.0, 300.0, 50.0, 32); // (cx, cy, radius, segments)
draw_circle(400.0, 300.0, 50.0, 1.0, 32);  // (cx, cy, radius, strength, segments)

draw_fps(10.0, 50.0);                      // Draw FPS counter at (x, y)
```

---

## Camera API (2D)

```c
camera_set_position(400.0, 300.0);          // Set camera position
camera_set_zoom(2.0);                       // Set zoom level
camera_set_rotation(0.5);                   // Set rotation (radians)
camera_follow(target_x, target_y);          // Smooth follow target

float cx = camera_get_x();                  // Get current position
float cy = camera_get_y();
float zoom = camera_get_zoom();
```

---

## Sprite API

```c
// Create an ECS sprite entity (adds sprite component and loads texture)
sprite_create(entity_id, "frog_idle.k");

// Low-level: load texture
void *tex = sprite_load("character.k");

// Low-level: draw texture
sprite_draw("character.k", 100.0, 200.0, 64.0, 64.0);  // (path, x, y, w, h)
```

Sprite components support fields: `scale_x`, `scale_y`, `flip_x`, `layer`, `visible`.

---

## Built-in Constants

```c
TYPE_INT      // Component field type: int
TYPE_FLOAT    // Component field type: float
TYPE_BOOL     // Component field type: bool
TYPE_PTR      // Component field type: pointer
TYPE_STRING   // Component field type: string
```

Used with `component_add_field()`:
```c
component_add_field(comp, "hp", TYPE_FLOAT, 0);
component_add_field(comp, "alive", TYPE_BOOL, 4);
```

---

## Iron Engine API (Host Bindings)

Minic has access to hundreds of Iron engine functions registered from C. Below is a categorized summary. All functions are called the same way as native Minic functions.

### Math (`vec2_*`, `vec4_*`, `quat_*`, `mat3_*`, `mat4_*`, `transform_*`)

```c
vec2_t v = vec2_create(1.0, 2.0);
float len = vec2_len(v);
vec2_t n = vec2_norm(v);

vec4_t v4 = vec4_create(1.0, 0.0, 0.0, 1.0);
vec4_t cross = vec4_cross(a, b);
vec4_t lerp = vec4_lerp(a, b, 0.5);

quat_t q = quat_from_euler(0.0, 3.14, 0.0);
mat4_t proj = mat4_persp(fov, aspect, 0.1, 100.0);
mat4_t view = mat4_look(eye, dir, up);

float wx = transform_world_x(t);
```

### Strings

```c
char *s = string_copy("hello");
int len = string_length(s);
int eq = string_equals(s1, s2);
int pos = string_index_of(s, "ll");
char *sub = substring(s, 0, 3);
char *upper = to_upper_case(s);
char *hex = i32_to_string_hex(255);
```

### Arrays

Typed arrays: `i8_array_t`, `u8_array_t`, `i16_array_t`, `u16_array_t`, `i32_array_t`, `u32_array_t`, `f32_array_t`, `any_array_t`, `string_array_t`, `buffer_t`

```c
f32_array_t *arr = f32_array_create(10);
f32_array_push(arr, 3.14);
float v = arr->buffer[0];          // Direct field access
int count = arr->length;
```

Buffer access: `buffer_get_f32(buf, index)`, `buffer_set_i32(buf, index, value)`, etc.

### Files

```c
iron_file_reader_t reader;
iron_file_reader_open(&reader, "data.bin", 0);
int size = iron_file_reader_size(&reader);
// ... read, close

int exists = iron_file_exists("save.dat");
iron_create_directory("saves");
```

### JSON / Armpack

```c
void *json = json_parse(json_string);
json_encode_begin();
json_encode_string("name", "player");
json_encode_f32("hp", 100.0);
char *encoded = json_encode_end();
```

### Scene Management

```c
void *scene = scene_create("MainScene");
scene_set_active(scene);
void *obj = scene_add_mesh_object(scene, mesh_data, transform);
void *cam = scene_add_camera_object(scene, camera_data);
scene_render_frame();
```

### UI

Full ArmorPaint UI system: `ui_begin`, `ui_window`, `ui_button`, `ui_text`, `ui_slider`, `ui_check`, `ui_combo`, `ui_end`, etc. Used primarily in ArmorPaint plugins.

### Garbage Collector

```c
void *mem = gc_alloc(64);
gc_root(mem);
gc_unroot(mem);
gc_free(mem);
```

---

## Complete Example

A bouncing frog system with 15 AI frogs and 1 player-controlled frog, using ECS components, `query_foreach`, keyboard input, camera follow, and 2D rendering:

```c
// frog_system.minic

float GRAVITY = 2200.0;
float GROUND_Y = 560.0;
float JUMP_VEL = 600.0;
float AI_MOVE_SPEED = 200.0;
float WALL_LEFT = 20.0;
float WALL_RIGHT = 1260.0;
int AI_FROG_COUNT = 15;

// Component IDs (stored as float to preserve 64-bit pointer values)
float g_pos_comp = -1.0;
float g_sprite_comp = -1.0;
float g_line_comp = -1.0;
float g_vel_comp = -1.0;

int g_vel_query = -1;
float g_dt = 0.016;

// Player state (no FrogVelocity component — tracked via globals)
float g_player = -1.0;
float g_pvx = 0.0;
float g_pvy = 0.0;
int g_pground = 1;
float g_pdir = 1.0;
int g_pjump_press = 0;

int init(void) {
    printf("FrogSystem: Init\n");

    g_pos_comp = component_lookup("comp_2d_position");
    g_sprite_comp = component_lookup("comp_2d_sprite");
    g_line_comp = component_lookup("comp_2d_line");

    // Register custom velocity component for AI frogs
    g_vel_comp = component_register("FrogVelocity", 8);
    component_add_field(g_vel_comp, "vx", TYPE_FLOAT, 0);
    component_add_field(g_vel_comp, "vy", TYPE_FLOAT, 4);

    // AI frogs (each gets FrogVelocity, so query_foreach finds them)
    int i = 0;
    while (i < AI_FROG_COUNT) {
        float e = entity_create();
        entity_add(e, g_pos_comp);
        entity_add(e, g_sprite_comp);
        entity_add(e, g_vel_comp);

        void *pos = entity_get(e, g_pos_comp);
        comp_set_float(g_pos_comp, pos, "x", 100.0 + i * 80.0);
        comp_set_float(g_pos_comp, pos, "y", GROUND_Y);

        // Alternate directions (no ternary — use if/else)
        float dir = 1.0;
        if (i % 2 == 1) { dir = 0.0 - 1.0; }
        void *vel = entity_get(e, g_vel_comp);
        comp_set_float(g_vel_comp, vel, "vx", dir * AI_MOVE_SPEED);
        comp_set_float(g_vel_comp, vel, "vy", 0.0);

        void *spr = entity_get(e, g_sprite_comp);
        comp_set_float(g_sprite_comp, spr, "pivot_x", 0.5);
        comp_set_float(g_sprite_comp, spr, "pivot_y", 1.0);
        comp_set_float(g_sprite_comp, spr, "scale_x", 2.5);
        comp_set_float(g_sprite_comp, spr, "scale_y", 2.5);
        comp_set_bool(g_sprite_comp, spr, "visible", 1);
        sprite_create(e, "frog_idle.k");

        i = i + 1;
    }

    // Player frog (no FrogVelocity — excluded from query automatically)
    g_player = entity_create();
    entity_add(g_player, g_pos_comp);
    entity_add(g_player, g_sprite_comp);

    void *ppos = entity_get(g_player, g_pos_comp);
    comp_set_float(g_pos_comp, ppos, "x", 640.0);
    comp_set_float(g_pos_comp, ppos, "y", GROUND_Y);

    void *pspr = entity_get(g_player, g_sprite_comp);
    comp_set_float(g_sprite_comp, pspr, "pivot_x", 0.5);
    comp_set_float(g_sprite_comp, pspr, "pivot_y", 1.0);
    comp_set_float(g_sprite_comp, pspr, "scale_x", 3.0);
    comp_set_float(g_sprite_comp, pspr, "scale_y", 3.0);
    comp_set_bool(g_sprite_comp, pspr, "visible", 1);
    sprite_create(g_player, "frog_idle.k");

    g_vel_query = query_create("FrogVelocity");
    return 0;
}

// AI frog callback — called once per entity by query_foreach
int ai_frog_step(int entity, void *vel) {
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

    // Bounce off ground with random height
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

    // AI frogs — batch update via query_foreach
    query_foreach(g_vel_query, ai_frog_step);

    // Player frog — keyboard control
    if (keyboard_down("left")) { g_pdir = 0.0 - 1.0; }
    if (keyboard_down("right")) { g_pdir = 1.0; }
    g_pvx = g_pdir * AI_MOVE_SPEED;

    int space = keyboard_down("space");
    if (space) {
        if (g_pground && g_pjump_press == 0) {
            g_pvy = 0.0 - JUMP_VEL;
            g_pground = 0;
        }
    }
    g_pjump_press = space;

    g_pvy = g_pvy + GRAVITY * dt;

    void *p = entity_get(g_player, g_pos_comp);
    if (p) {
        float px = comp_get_float(g_pos_comp, p, "x");
        float py = comp_get_float(g_pos_comp, p, "y");
        px = px + g_pvx * dt;
        py = py + g_pvy * dt;

        if (py >= GROUND_Y) { py = GROUND_Y; g_pvy = 0.0; g_pground = 1; }
        if (px < WALL_LEFT) { px = WALL_LEFT; g_pdir = 1.0; }
        if (px > WALL_RIGHT) { px = WALL_RIGHT; g_pdir = 0.0 - 1.0; }

        comp_set_float(g_pos_comp, p, "x", px);
        comp_set_float(g_pos_comp, p, "y", py);
        camera_follow(px, py);
    }

    return 0;
}

int draw(void) {
    draw_set_font("font.ttf", 16);
    draw_set_color(0xffffffff);
    draw_string("Arrows+Space = Player | AI frogs bounce", 10.0, 10.0);
    draw_fps(10.0, 32.0);
    return 0;
}
```

---

## Performance Tips

1. **Use `query_foreach`** over manual iteration — C handles the loop, only calling Minic per entity.
2. **Cache `sys_delta_time()`** in a global before calling `query_foreach` — avoids a host call per entity.
3. **Batch field access** with `comp_set_floats` / `comp_add_floats` to reduce dispatch overhead.
4. **Use `comp_add_float`** instead of separate `comp_get_float` + `comp_set_float`.
5. **Cache component IDs** in globals during `init()` — `component_lookup()` is not free.
6. **Global arrays** (`int g_arr[N]`) are the only way to store collections — there are no dynamic arrays in Minic itself (use the host array API for that).
