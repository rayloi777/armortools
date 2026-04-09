# 2D Game Engine Extension Plan

## Upstream Sync — 2026-04-05

Merged upstream (70 commits): BC7 texture compression, Kong shader rewrite, camera pivot, material nodes. All changes landed in `base/` and `paint/` only. **No files in `engine/` were modified by the merge.**

---

## Table of Contents
1. [ECS Architecture Overview](#ecs-architecture-overview)
2. [ECS Component Reference](#ecs-component-reference)
3. [ECS Bridge Pattern](#ecs-bridge-pattern)
4. [Dynamic ECS & Minic](#dynamic-ecs--minic)
5. [2D Feature Plans](#2d-feature-plans)
6. [Implementation Order](#implementation-order)
7. [Deep Architecture Analysis](#deep-architecture-analysis)

---

# ECS Architecture Overview

## Layer Structure

```
┌─────────────────────────────────────────────────────────────┐
│                    Game Engine                               │
│         (game_loop.c, game_engine.c)                        │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              High-Level APIs                                 │
│   entity_api.c  │  system_api.c  │  query_api.c            │
│   component_api.c  │  runtime_api.c                          │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│           ECS Bridge Layer (ecs_bridge.c)                    │
│   - Syncs ECS entities to Iron Engine scene graph           │
│   - Creates bridge_system for transform propagation          │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│           ECS World Layer                                   │
│   ecs_world.c  │  ecs_components.c  │  ecs_dynamic.c       │
│   - game_world_t wrapper                                   │
│   - Builtin component registration                          │
│   - Dynamic component management                            │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              Flecs Core (flecs.c/h)                         │
│   - ecs_init() / ecs_fini() / ecs_progress()              │
│   - ecs_component_init() / ecs_system_init()                │
│   - ecs_query_init() / ecs_field()                         │
└─────────────────────────────────────────────────────────────┘
```

---

# ECS Component Reference

## Static Components (ecs_components.h)

```c
// Position in world space
typedef struct { float x, y, z; } TransformPosition;

// Quaternion rotation
typedef struct { float x, y, z, w; } TransformRotation;

// Scale
typedef struct { float x, y, z; } TransformScale;

// Entity name string
typedef struct { char *value; } EntityName;

// Active flag (for enable/disable)
typedef struct { bool value; } EntityActive;

// Bridge component - holds Iron Engine object references
typedef struct {
    void *object;      // object_t* - Iron scene graph node
    void *transform;   // transform_t* - Iron transform node
    bool dirty;        // Sync flag - needs update?
} RenderObject;

// 3D mesh rendering data
typedef struct {
    char *mesh_path;
    char *material_path;
    bool cast_shadows;
    bool receive_shadows;
} RenderMesh;

// Script component for minic integration
typedef struct {
    char *script_path;
    void *script_ctx;
    void *update_fn;
    bool initialized;
} EntityScript;
```

---

# ECS Bridge Pattern

## Architecture

```
ECS Entity                              Iron Engine Scene Graph
──────────                              ──────────────────────
TransformPosition ──────────────────►  transform_t (loc, rot, scale)
TransformRotation ──────────────────►
TransformScale ─────────────────────►
RenderObject ───────────────────────►  object_t (scene graph node)
    ├── object: NULL ───────────────►      mesh_object_t ──► mesh_data, material
    └── dirty: true ───────────────►      transform_build_matrix()
RenderMesh ────────────────────────►  asset loading (mesh, material)
```

## bridge_system() Implementation

```c
// ecs_bridge.c
static void bridge_system(ecs_iter_t *it) {
    TransformPosition *pos = ecs_field(it, TransformPosition, 1);
    TransformRotation *rot = ecs_field(it, TransformRotation, 2);
    TransformScale *scale = ecs_field(it, TransformScale, 3);
    RenderObject *render_obj = ecs_field(it, RenderObject, 4);
    RenderMesh *mesh = ecs_field(it, RenderMesh, 5);

    for (int i = 0; i < it->count; i++) {
        // Create Iron object if needed
        if (render_obj[i].object == NULL && mesh[i].mesh_path != NULL) {
            mesh_data_t *mesh_data = data_get_mesh(mesh[i].mesh_path, mesh[i].mesh_path);
            // ... create mesh_object_t ...
        }
        
        // Sync transform if dirty
        if (render_obj[i].transform != NULL && render_obj[i].dirty) {
            transform_t *t = (transform_t *)render_obj[i].transform;
            t->loc.x = pos[i].x; t->loc.y = pos[i].y; t->loc.z = pos[i].z;
            // ... copy rotation, scale ...
            t->dirty = true;
            transform_build_matrix(t);
            render_obj[i].dirty = false;
        }
    }
}
```

## System Registration

```c
void ecs_bridge_init(void) {
    ecs_system_desc_t sys_desc = {0};
    sys_desc.query.terms[0].id = ecs_component_TransformPosition();
    sys_desc.query.terms[1].id = ecs_component_TransformRotation();
    sys_desc.query.terms[2].id = ecs_component_TransformScale();
    sys_desc.query.terms[3].id = ecs_component_RenderObject();
    sys_desc.query.terms[4].id = ecs_component_RenderMesh();
    sys_desc.callback = bridge_system;
    
    g_bridge_system = ecs_system_init(ecs, &sys_desc);
    ecs_add_pair(ecs, g_bridge_system, EcsDependsOn, EcsPreStore);
}
```

---

# Dynamic ECS & Minic

## Overview

| Aspect | Static ECS | Dynamic ECS |
|--------|------------|-------------|
| Components | Compile-time C structs | Runtime created |
| Definition | ecs_components.h | ecs_dynamic.c |
| Flexibility | Fixed types | Can add new types at runtime |
| Scripting | Via EntityScript | Constructor/destructor hooks |

## Minic System Integration

All 2D systems are **Minic Systems** that run every frame automatically:
- Trigger: Every frame via `ecs_progress()`
- Control: Script-based (camera position, sprite animation, etc.)

---

# 2D Feature Plans

## Part 1: Sprite System

### Design

```c
// RenderSprite (ecs_components.h)
typedef struct {
    char *texture_path;           // Path to texture asset
    
    // Atlas region (for sprite sheet)
    float region_x, region_y;     // Top-left of frame in atlas
    float src_width, src_height; // Original frame size
    
    // Transform
    float pivot_x, pivot_y;      // Anchor point (0-1)
    float scale_x, scale_y;       // Scale multiplier
    
    // Flip
    bool flip_x, flip_y;
    
    // Render order
    int layer;                    // Z-order (0=背景, 100=前景, 200=UI)
    
    // State
    bool visible;
    
    // Cached (rendered size after scale applied)
    float width;                  // src_width * scale_x
    float height;                 // src_height * scale_y
    
    void *render_object;          // Internal: bridge reference
} RenderSprite;
```

### Layer Values

| Layer | Purpose |
|-------|---------|
| 0-9 | Background |
| 10-50 | Background decoration |
| 51-100 | Game objects |
| 101-150 | Foreground decoration |
| 151-200 | UI/HUD |

### Bridge System (sprite_bridge.c)

```c
static void sprite_bridge_system(ecs_iter_t *it) {
    TransformPosition *pos = ecs_field(it, TransformPosition, 1);
    RenderSprite *sprite = ecs_field(it, RenderSprite, 2);
    
    for (int i = 0; i < it->count; i++) {
        // Create sprite if needed
        if (sprite[i].render_object == NULL && sprite[i].texture_path != NULL) {
            sprite[i].render_object = sprite_object_create(...);
        }
        
        // Sync position and properties
        if (sprite[i].render_object != NULL) {
            sprite_renderer_set_position(sprite[i].render_object, pos[i].x, pos[i].y);
            sprite_renderer_set_scale(sprite[i].render_object, sprite[i].scale_x, sprite[i].scale_y);
            sprite_renderer_set_flip(sprite[i].render_object, sprite[i].flip_x, sprite[i].flip_y);
            sprite_renderer_set_layer(sprite[i].render_object, sprite[i].layer);
            
            // Update cached dimensions
            sprite[i].width = sprite[i].src_width * sprite[i].scale_x;
            sprite[i].height = sprite[i].src_height * sprite[i].scale_y;
        }
    }
}
```

### sprite_api.c Enhancement

```c
// sprite_api.h (enhanced)
typedef struct sprite_renderer sprite_renderer_t;

sprite_renderer_t *sprite_renderer_create(void);
void sprite_renderer_destroy(sprite_renderer_t *sr);
void sprite_renderer_draw(sprite_renderer_t *sr);
void sprite_renderer_set_texture(sprite_renderer_t *sr, const char *texture_name);
void sprite_renderer_set_scale(sprite_renderer_t *sr, float scale_x, float scale_y);
void sprite_renderer_set_position(sprite_renderer_t *sr, float x, float y);
void sprite_renderer_set_flip(sprite_renderer_t *sr, bool flip_x, bool flip_y);
void sprite_renderer_set_layer(sprite_renderer_t *sr, int layer);
void sprite_renderer_set_region(sprite_renderer_t *sr, float x, float y, float w, float h);
void sprite_renderer_set_visible(sprite_renderer_t *sr, bool visible);

// Sorting
void sprite_renderer_sort_by_layer(void);
```

---

## Part 2: 2D Camera System

### Design

```c
// Camera2D (ecs_components.h)
typedef struct {
    float x, y;              // Camera position in world space
    float zoom;             // Zoom level (1.0 = default)
    float rotation;          // Rotation in radians
    float smoothing;         // Follow smoothing (0-1)
    
    // Target (for follow)
    float target_x, target_y;
    bool has_target;
    
    // Bounds (optional)
    float bounds_min_x, bounds_min_y;
    float bounds_max_x, bounds_max_y;
    bool has_bounds;
    
    // Aspect ratio
    float aspect_ratio;
} Camera2D;
```

### camera2d Functions

```c
// camera2d.h
typedef struct camera2d camera2d_t;

camera2d_t *camera2d_create(float x, float y);
void camera2d_destroy(camera2d_t *cam);

void camera2d_set_position(camera2d_t *cam, float x, float y);
void camera2d_set_zoom(camera2d_t *cam, float zoom);
void camera2d_set_rotation(camera2d_t *cam, float rotation);
void camera2d_set_smoothing(camera2d_t *cam, float smoothing);

void camera2d_follow(camera2d_t *cam, float target_x, float target_y);
void camera2d_set_bounds(camera2d_t *cam, float min_x, float min_y, float max_x, float max_y);

mat3_t camera2d_get_transform(camera2d_t *cam);
void camera2d_apply(camera2d_t *cam);  // Calls draw_set_transform()
```

### Camera Transform

```c
mat3_t camera2d_get_transform(camera2d_t *cam) {
    mat3_t t = mat3_identity();
    
    // Translate to negative position (world moves opposite to camera)
    t = mat3_multmat(t, mat3_translation(-cam->x, -cam->y));
    
    // Apply zoom (scale from origin)
    t = mat3_multmat(t, mat3_scale(mat3_identity(), vec4_create(cam->zoom, cam->zoom, 0, 0)));
    
    // Apply rotation
    t = mat3_multmat(t, mat3_rotation(cam->rotation));
    
    return t;
}
```

---

## Part 3: Tilemap System

### Design

```c
// TilemapRenderer (ecs_components.h)
typedef struct {
    char *tilemap_path;      // Path to tilemap data
    int active_layer;        // Which layer to render
    bool visible;
} TilemapRenderer;

// Tilemap data (runtime)
typedef struct {
    char *name;
    int width, height;       // Map size in tiles
    int tile_width, tile_height;
    any_array_t *tilesets;  // tileset_t[]
    any_array_t *layers;    // tilemap_layer_t[]
} tilemap_t;

typedef struct {
    char *name;
    int *tiles;             // Tile indices (width * height)
    bool visible;
    float opacity;
} tilemap_layer_t;

typedef struct {
    char *name;
    int first_gid;
    int tile_width, tile_height;
    int columns, rows;
    gpu_texture_t *image;
} tileset_t;
```

### tilemap Functions

```c
// tilemap.h
tilemap_t *tilemap_load(const char *path);
void tilemap_destroy(tilemap_t *map);

void tilemap_set_layer(tilemap_t *map, int layer_index);
int tilemap_get_tile(tilemap_t *map, int layer, int x, int y);
void tilemap_set_tile(tilemap_t *map, int layer, int x, int y, int tile_id);

void tilemap_render(tilemap_t *map, int layer_index, mat3_t camera_transform);
void tilemap_render_all(tilemap_t *map, mat3_t camera_transform);
```

---

## Part 4: 2D Physics System

### Design

```c
// Physics2DBody (ecs_components.h)
typedef struct {
    body2d_type_t type;     // STATIC, DYNAMIC, KINEMATIC
    vec2_t velocity;
    vec2_t force;
    float mass;
    float restitution;       // Bounciness
    float friction;
    aabb2d_t aabb;           // Axis-aligned bounding box
    bool enabled;
    bool awake;
} Physics2DBody;

typedef enum {
    BODY2D_STATIC = 0,
    BODY2D_DYNAMIC = 1,
    BODY2D_KINEMATIC = 2
} body2d_type_t;

typedef struct { vec2_t min; vec2_t max; } aabb2d_t;
```

### physics2d Functions

```c
// physics2d.h
typedef struct physics2d_world physics2d_world_t;

physics2d_world_t *physics2d_world_create(void);
void physics2d_world_destroy(physics2d_world_t *world);

body2d_t *physics2d_world_add_body(physics2d_world_t *world, body2d_t *body);
void physics2d_world_remove_body(physics2d_world_t *world, body2d_t *body);

void physics2d_world_update(physics2d_world_t *world, float dt);

typedef void (*collision_callback_t)(uint64_t entity_a, uint64_t entity_b, 
                                      vec2_t normal, float depth);
void physics2d_world_set_collision_callback(physics2d_world_t *world, 
                                              collision_callback_t cb);

// Collision detection
bool aabb2d_intersect(aabb2d_t a, aabb2d_t b);
bool aabb2d_contains_point(aabb2d_t a, vec2_t p);
```

---

# Implementation Order

## Phase 1: Sprite System
- [x] Add `RenderSprite` to ecs_components.h
- [x] Register in ecs_components.c
- [x] Enhance sprite_api.c (region, flip, layer, width/height cache)
- [x] Create sprite_bridge.c
- [x] Add sprite rendering to game loop
- [x] Sort sprites by layer before rendering

## Phase 2: 2D Camera
- [x] Add `Camera2D` to ecs_components.h
- [x] Create camera2d.c/h
- [x] Implement follow, zoom, bounds
- [x] Integrate with draw_set_transform()
- [x] Add camera update to game loop

## Phase 3: Tilemap
- [ ] Create tilemap.h/c
- [ ] Implement JSON/TMX loader
- [ ] Add `TilemapRenderer` component
- [ ] Render via iron_draw with camera culling

## Phase 4: 2D Physics
- [ ] Add `Physics2DBody` to ecs_components.h
- [ ] Implement aabb2d collision
- [ ] Create physics2d.c/h
- [ ] Add collision callbacks

---

# File Structure

```
engine/sources/
├── core/
│   ├── sprite_api.c/h          (enhance)
│   ├── camera2d.c/h            (NEW)
│   └── tilemap.c/h             (NEW)
├── ecs/
│   ├── ecs_components.h        (enhance: add RenderSprite, Camera2D, TilemapRenderer, Physics2DBody)
│   ├── ecs_components.c        (enhance: register new components)
│   ├── ecs_bridge.c            (reference)
│   ├── sprite_bridge.c         (NEW)
│   ├── camera2d_bridge.c       (NEW)
│   ├── tilemap_bridge.c        (NEW)
│   ├── physics2d.c             (NEW)
│   └── ecs_dynamic.c/h         (reference)
└── game_loop.c                 (enhance: sprite render, camera apply)
```

---

# Reference Files

| File | Purpose |
|------|---------|
| `ecs_bridge.c` | Example of ECS→scene graph bridge |
| `ecs_components.h/c` | Component definitions |
| `ecs_dynamic.c/h` | Dynamic ECS + minic hooks |
| `entity_api.c/h` | High-level entity API |
| `system_api.c/h` | System management |
| `query_api.c/h` | Query functionality |
| `sprite_api.c/h` | Existing sprite implementation |
| `flecs/flecs.h` | Flecs library API |
| `iron_draw.h` | 2D rendering API |

---

# Key Flecs API

## World Management
```c
ecs_world_t *ecs_init(void);
void ecs_fini(ecs_world_t *world);
void ecs_progress(ecs_world_t *world, float delta_time);
```

## Component Management
```c
ecs_entity_t ecs_component_init(ecs_world_t *world, const ecs_component_desc_t *desc);
ecs_id_t ecs_get_id(ecs_world_t *world, ecs_entity_t entity, ecs_id_t component);
```

## System Management
```c
ecs_entity_t ecs_system_init(ecs_world_t *world, const ecs_system_desc_t *desc);
void ecs_add_pair(ecs_world_t *world, ecs_entity_t entity, ecs_entity_t relationship, ecs_entity_t target);
```

## Iteration
```c
void *ecs_field(ecs_iter_t *it, size_t size, int32_t index);
bool ecs_query_next(ecs_iter_t *it);
```

---

# Deep Architecture Analysis

> 基於程式碼的深入架構分析，記錄各層實際實作細節。

## Five-Layer Architecture

```
┌─────────────────────────────────────────────────────┐
│  Minic Scripts (.minic)                              │  ← 遊戲邏輯
│  frog_system / movement / health / gamepad / mouse   │
├─────────────────────────────────────────────────────┤
│  Core API (engine/sources/core/)                     │  ← C API 註冊層
│  runtime_api → entity/component/system/query/sprite  │
├─────────────────────────────────────────────────────┤
│  Flecs ECS (engine/sources/ecs/)                     │  ← 實體組件系統
│  ecs_world / ecs_dynamic / ecs_components            │
├─────────────────────────────────────────────────────┤
│  Bridge Systems (engine/sources/ecs/*_bridge.c)      │  ← 同步到 Iron 引擎
│  ecs_bridge / sprite_bridge / render2d / camera      │
├─────────────────────────────────────────────────────┤
│  Iron Engine (base/)                                 │  ← 渲染 / 平台 / UI（不可修改）
└─────────────────────────────────────────────────────┘
```

## Startup Flow (`game_engine.c`)

`_kickstart()` 是程式入口（line 194-220），按以下順序初始化：

1. **`game_engine_init()`**（line 112-149）：
   - System API init
   - 建立遊戲世界（`game_world_create()`）
   - 設定 ECS Bridge、Sprite Bridge、Camera Bridge、Render2D Bridge
   - 建立動態組件欄位快取 & ID 快取
   - 註冊 Runtime API（`runtime_api_register()`）
   - 輸入系統（鍵盤、滑鼠、Gamepad）
   - 遊戲迴圈初始化

2. **載入 Minic 腳本**：
   ```c
   minic_system_load("MinicTest", "data/systems/minic_test.minic");
   minic_system_load("MinicBench", "data/systems/minic_bench.minic");
   ```

3. **`game_engine_start()`** — 啟動 Iron 主迴圈

## Game Loop (`game_loop.c`)

每幀執行順序：

```
game_loop_update() (line 28)
  ├── 計算 delta_time
  ├── game_world_progress()        // 推進 Flecs ECS
  ├── minic_system_call_step()     // 執行所有 Minic step() 函數
  ├── render frame:
  │   ├── draw_begin()             // Iron 清屏 (0xff1a1a2e)
  │   ├── camera2d_apply()         // 套用 2D 相機變換
  │   ├── sys_2d_draw()            // 批次 2D 渲染（render2d_bridge）
  │   ├── minic_system_call_draw() // 執行 Minic draw() 函數
  │   └── draw_end()
  └── 更新 frame_count / time
```

## Core API Layer (`engine/sources/core/`)

### runtime_api.c — 統一註冊入口（line 987-1115）

調用順序：
1. `component_api_register()` → 18 個 API
2. `entity_api_register()` → 12 個 API
3. `system_api_register()` → 7 個 API
4. `query_api_register()` → 6 個 API
5. 直接註冊 ~100+ 個額外 API（輸入、繪圖、相機等）

**註冊模式**：
- `minic_register("name", "i(p,i)", func_ptr)` — 帶型別簽名
- `minic_register_native("name", native_func)` — 直接 C 函數（更快，跳過型別分派）

### API Modules

| 模組 | 檔案 | 功能 |
|------|------|------|
| Entity API | `entity_api.c` | create/destroy/add_component/remove_component/has/get/set_data |
| Component API | `component_api.c` | register/lookup/add_field/set-get_float/int/ptr/bool |
| System API | `system_api.c` | create(enable/disable)/phase 管理/最多 64 系統 |
| Query API | `query_api.c` | query_new/with/find/iter/foreach/foreach_batch |
| Sprite API | `sprite_api.c` | 紋理載入 + 快取 |
| Minic System | `minic_system.c` | 腳本載入/管理/step/draw 調用 |

## ECS Layer (`engine/sources/ecs/`)

### ecs_world.c — Flecs 世界管理
- `game_world_create()` — 建立 Flecs world（line 8）
- `game_world_progress()` — 每幀推進 ECS，傳入 delta_time（line 40）
- 全域狀態：`delta_time`, `time`, `frame_count`

### ecs_dynamic.c — 執行期動態組件
- **最大 64 個動態組件**（`MAX_DYNAMIC_COMPONENTS`）
- 組件 ID 雜湊表 — O(1) 查找
- 欄位快取（`field_cache`）— 加速欄位存取
- Minic 建構子/解構子掛勾

### ecs_components.c/h — 內建組件定義

**2D Transform**：`comp_2d_position` (x,y,z) / `comp_2d_rotation` (四元數) / `comp_2d_scale` (x,y,z)

**2D Rendering**：`comp_2d_sprite` / `comp_2d_rect` / `comp_2d_circle` / `comp_2d_line` / `comp_2d_text`

**2D Camera**：`comp_2d_camera` (x,y,zoom,rotation,smoothing,target,bounds)

**Generic**：`EntityName` / `EntityActive` / `EntityScript` / `RenderObject` / `RenderMesh`

## Bridge Systems (ECS ↔ Iron)

所有 Bridge 遵循統一模式：**偵測 ECS 組件變化 → 同步到 Iron 引擎物件**。

### ecs_bridge.c — 3D 變換同步
- 以 Flecs 系統運行（`EcsPreStore` 階段）
- `bridge_system()` — 更新 Transform → `transform_build_matrix()`
- Iron API：`mesh_object_create()`, `object_set_parent()`, `data_get_mesh()`

### sprite_bridge.c — 精靈圖生命週期
- `sprite_bridge_create_sprite()` — 載入紋理（`data_get_image()`）→ 設定 render_object
- `sprite_bridge_destroy_sprite()` — 清理 Iron 物件

### render2d_bridge.c — 批次 2D 渲染（核心）
- `sys_2d_draw()` — 主渲染函數（line 116-307）
- **流程**：
  1. 為每種 2D 原型（sprite/rect/circle/line/text）建立 Flecs query
  2. 收集所有可見實體到 render items 陣列
  3. **Insertion sort 按 layer 排序**（穩定 O(n)，適合幾乎有序的幀資料）
  4. 按排序順序發出 Iron 繪圖呼叫
- **動態緩衝區**：render items 陣列動態增長，每幀重用
- Iron 繪圖 API：
  - `draw_scaled_sub_image()` — 精靈
  - `draw_filled_rect()` / `draw_rect()` — 矩形
  - `draw_filled_circle()` / `draw_circle()` — 圓形
  - `draw_line()` — 線段
  - `draw_string()` — 文字

### camera_bridge.c — 2D 相機
- `camera_bridge_init()` — 建立 640×360 相機（`camera2d_create()`）
- `camera_bridge_get_camera()` — 返回活躍相機

## Minic Script System

### 載入機制（`minic_system.c`）
1. `minic_system_load(name, path)` — 讀取檔案 → 建立 context → 執行腳本
2. 查找 `init()`、`step()`、`draw()` 標準函數
3. 最多 16 個系統（`MAX_MINIC_SYSTEMS`）
4. 每幀：`minic_system_call_step()` → `minic_system_call_draw()`

### 腳本範例

**frog_system.minic** — 2D ECS 效能測試：
- 動態組件註冊：`component_register("FrogVelocity", 8)`
- 批次欄位操作：`comp_set_floats()`, `comp_add_floats()`
- C 端迭代：`query_foreach(query, "comp_set_float(id, ...)")`

**movement_system.minic** — WASD 移動 + Query：
- `query_new()` / `query_with(q, "comp_2d_position")`
- `keyboard_down("w")` → 修改 position

### Minic 解釋器（`base/sources/libs/minic.c`）
- **Tree-walking interpreter** — 78K 行 C
- **值系統**：`minic_val_t` — type(i32/f32/ptr/bool/char) + union
- **效能優化**：雜湊函數分派(M1)、雜湊變數查找(M2)、Arena save/restore(M3)、函數體 token 快取(M4, 2.9x)

## Performance Optimizations

| 優化 | 位置 | 效果 |
|------|------|------|
| 函數分派雜湊表 | `minic_ext.c` M1 | O(1) 函數查找 |
| 變數查找雜湊 | `minic_ext.c` M2 | O(1) 變數存取 |
| Arena save/restore | `minic_ext.c` M3 | 正確記憶體管理 |
| 函數體 token 快取 | `minic.c` M4 | 2.9x 函數呼叫加速 |
| 欄位快取 | `ecs_dynamic.c` | 快速組件欄位存取 |
| 組件 ID 快取 | `ecs_dynamic.c` | flecs_id → component O(1) 映射 |
| 批次 API | `runtime_api.c` | `comp_set_floats()` 批次處理 |
| C 端迭代 | `query_api.c` | `query_foreach()` 避免 Minic 迴圈開銷 |
| Insertion sort | `render2d_bridge.c` | 穩定 O(n) 幀排序 |
| 紋理快取 | `sprite_api.c` | 避免重複載入 |

## Key File Index

| 類別 | 檔案 | 說明 |
|------|------|------|
| 入口 | `engine/sources/game_engine.c` | init/shutdown/start/kickstart |
| 遊戲迴圈 | `engine/sources/core/game_loop.c` | 每幀 update + render |
| API 註冊 | `engine/sources/core/runtime_api.c` | 100+ Minic API 綁定 |
| Entity API | `engine/sources/core/entity_api.c` | 實體 CRUD |
| Component API | `engine/sources/core/component_api.c` | 組件註冊/存取 |
| System API | `engine/sources/core/system_api.c` | 系統管理 |
| Query API | `engine/sources/core/query_api.c` | 查詢引擎 |
| ECS 世界 | `engine/sources/ecs/ecs_world.c` | Flecs 生命週期 |
| 動態組件 | `engine/sources/ecs/ecs_dynamic.c` | 執行期組件 |
| 內建組件 | `engine/sources/ecs/ecs_components.c/h` | 所有組件定義 |
| 3D Bridge | `engine/sources/ecs/ecs_bridge.c` | Transform → Iron |
| Sprite Bridge | `engine/sources/ecs/sprite_bridge.c` | 精靈圖生命週期 |
| 2D 渲染 | `engine/sources/ecs/render2d_bridge.c` | 批次 2D 繪圖 |
| 相機 Bridge | `engine/sources/ecs/camera_bridge.c` | 2D 相機 |
| Minic 載入 | `engine/sources/core/minic_system.c` | 腳本管理 |
| Sprite API | `engine/sources/core/sprite_api.c` | 紋理快取 |
| Minic 解釋器 | `base/sources/libs/minic.c/h` | Tree-walking interpreter |
| Minic 擴展 | `base/sources/libs/minic_ext.c` | 效能優化 |

## Current Limitations

1. **無熱載入**：腳本僅在啟動時載入，無檔案監視機制
2. **最大 64 動態組件 / 16 Minic 系統** — 有固定上限
3. **2D 相機固定 640×360** — 寫死在 `camera_bridge_init()`
4. **Layer 排序用 Insertion sort** — 適合幾乎有序場景，大量新增實體時可能退化
5. **2D 無批次合批** — 每個 render item 獨立發出 draw call（未使用 instancing）
