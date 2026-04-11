# MINIC_README.md 更新設計

## 目標

深入參考 engine 的實際系統腳本，更新 `MINIC_README.md` 文件，使其反映當前 API 的真實使用模式。

## 現狀分析

現有 `MINIC_README.md` 的問題：
- `draw` 回調文件中未提及 `draw_ui`（實際使用的 UI 繪製回調）
- Query API 僅有 `query_foreach` 回調方式，未記錄 iterator-style pattern
- 缺少 3D Camera API
- 缺少 Mouse lock/unlock 功能
- 缺少 string conversion 函數 (`str_float`, `str_int`)
- Complete Example 仍使用 `int`-based 舊模式，非 `id` 類型

## 變更項目

### 1. 新增 `draw_ui` 回調

實際系統使用 `draw_ui` 而非 `draw` 來繪製 UI 疊加層：

```c
int draw_ui(void) {
    draw_set_font("font.ttf", 16);
    draw_set_color(0xffffffff);
    draw_string("Arrows+Space = Player", 10.0, 10.0);
    draw_fps(10.0, 32.0);
    return 0;
}
```

`draw` 和 `draw_ui` 的區別：
- `draw`: 用於 2D 渲染（在 3D 場景上）
- `draw_ui`: 用於 UI 疊加層（在最上層繪製 HUD）

### 2. 更新 `id` 類型說明

所有實際系統已遷移到 `id` 類型（64-bit Flecs entity ID）：

```c
id g_player = 0;
id g_pos_comp = 0;

int init(void) {
    g_pos_comp = component_lookup("comp_2d_position");
    g_player = entity_create();
    entity_add(g_player, g_pos_comp);
}
```

`id` 類型特點：
- 64-bit 無符號整數，直接存储 Flecs entity/component ID
- 支持比較運算子 (`==`, `!=`, `<`, `>`, `<=`, `>=`)
- 不支持算術運算
- 適用於 `entity_create()`, `component_lookup()`, `query_iter_entity_id()` 等場景

### 3. 擴展 Query API — Iterator Pattern

實際系統使用 iterator-style query 遍歷實體：

```c
// 創建 query（兩種方式）
int q = query_create("FrogVelocity");  // 由組件名創建

// 或手動創建並指定組件
int q = query_new();
query_with(q, g_health_comp);
query_find(q);

// Iterator-style iteration
query_iter_begin(q);
while (query_iter_next(q)) {
    int count = query_iter_count(q);
    for (int i = 0; i < count; i++) {
        id entity = query_iter_entity_id(q, i);
        void *data = query_iter_comp_ptr(q, i, 0);  // comp_index = 0
        // ... process entity
    }
}

// 獲取 entity 數量
int count = query_find(q);
for (int i = 0; i < count; i++) {
    id e = query_get(q, i);
    // ...
}
```

### 4. 新增 3D Camera API

`camera3d_control_system.minic` 展示了 3D 相機控制：

```c
// 設置位置和朝向
camera_3d_set_position(cam, x, y, z);
camera_3d_look_at(cam, target_x, target_y, target_z);

// 查找實體
id cam = entity_find_by_name("main_camera");

// 讀取當前位置
float cx = camera_3d_get_x(cam);
float cy = camera_3d_get_y(cam);
float cz = camera_3d_get_z(cam);
```

### 5. 新增 Mouse Lock/Unlock（FPS 風格控制）

滑鼠鎖定用於 FPS 攝影機控制：

```c
// 按下右鍵開始鎖定
if (mouse_started("right") != 0.0) {
    mouse_lock();
}
// 釋放右鍵解除鎖定
if (mouse_released("right") != 0.0) {
    mouse_unlock();
}
// 鎖定後讀取 delta（相對移動）
if (mouse_down("right") != 0.0) {
    float dx = mouse_dx();
    float dy = mouse_dy();
    g_yaw = g_yaw - dx * sensitivity;
    g_pitch = g_pitch - dy * sensitivity;
}
```

`mouse_lock()` 將滑鼠捕獲在視窗內，適用於 FPS 控制。`mouse_dx()`/`mouse_dy()` 返回自上一幀以來的相對移動。

### 6. 新增 String Conversion 函數

用於將數值轉換為字串以便顯示：

```c
// 數值轉字串
char *s = str_float(float_value);
char *s = str_int(int_value);

// 使用範例
draw_string(str_float(g_speed), 70.0, 70.0);
draw_string(str_int(g_level), m + 30.0, y);
```

### 7. 新增 Gamepad 左搖桿支援

`vampire_system.minic` 展示了搖桿控制：

```c
// 左搖桿（支援 WASD fallback）
float glx = gamepad_stick_left_x(0);
float gly = gamepad_stick_left_y(0);
if (glx * glx + gly * gly > 0.01) { dx = glx; dy = gly; }
```

### 8. 更新 Complete Example

將過時的 `int`-based 示例替換為實際的 `frog_system.minic` 模式：

```c
// FrogSystem - 使用 id 類型和 iterator pattern
id g_pos_comp = 0;
id g_sprite_comp = 0;
id g_vel_comp = 0;

int g_vel_query = -1;
float g_dt = 0.016;
id g_player = 0;

int init(void) {
    g_pos_comp = component_lookup("comp_2d_position");
    g_sprite_comp = component_lookup("comp_2d_sprite");
    g_vel_comp = component_register("comp_frog_velocity", 8);
    component_add_field(g_vel_comp, "vx", TYPE_FLOAT, 0);
    component_add_field(g_vel_comp, "vy", TYPE_FLOAT, 4);

    int i = 0;
    while (i < AI_FROG_COUNT) {
        id e = entity_create();
        entity_add(e, g_pos_comp);
        entity_add(e, g_sprite_comp);
        entity_add(e, g_vel_comp);
        // ... 設置初始值 ...
        i = i + 1;
    }

    g_player = entity_create();
    entity_add(g_player, g_pos_comp);
    entity_add(g_player, g_sprite_comp);
    // ... 設置玩家 ...
    g_vel_query = query_create("comp_frog_velocity");
    return 0;
}

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

    query_foreach(g_vel_query, ai_frog_step);

    // Player logic ...

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

### 9. 重新組織文件結構

1. Upstream Sync（保持）
2. Overview（保持）
3. **Script Lifecycle** — `init`/`step`/`draw`/`draw_ui`（新增 `draw_ui`）
4. **Language Syntax** — Types, Variables, Operators, Control Flow, Functions, Comments, Literals, Type Casting
5. **Engine ECS API** — Components, Batch Field Access, Entities, Queries（更新為 iterator pattern）
6. **System API**（保持）
7. **Input API** — Keyboard, Mouse, Mouse Lock, Gamepad（擴展）
8. **2D Drawing API**（保持）
9. **Camera API** — 2D + 3D（新增 3D 部分）
10. **Sprite API**（保持）
11. **Built-in Constants**（保持）
12. **Iron Engine API** — Math, Strings, Arrays, Files, JSON, Scene, UI, GC, String Conversion（新增）
13. **Complete Example** — 更新的 frog_system（替換）
14. **Performance Tips**（保持）

## 預期產出

更新後的 `MINIC_README.md` 文件，全面反映 engine 中實際使用的 Minic API 模式。