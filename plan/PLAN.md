# Iron 3D 遊戲引擎完善計劃

## Upstream Sync — 2026-04-05

Merged upstream (70 commits): BC7 texture compression, Kong shader rewrite, camera pivot, material nodes. All changes landed in `base/` and `paint/` only. **No files in `engine/` were modified by the merge.** The engine layer is entirely local code.

---

## 概述

將 Iron 引擎從 3D 繪畫工具引擎升級為完整的遊戲開發引擎，目標類似 Godot 的易用性。

**核心原則：不修改 base/ 目錄，所有新代碼放在 engine/ 目錄。**

---

## 重大更新：Flecs + Minic 運行時 ECS

本計劃採用 **Flecs ECS + Minic 運行時腳本** 架構：
- **Flecs** - 高性能 ECS 框架，管理 Entity/Component/System
- **Minic** - Iron 內建的 C 語言腳本系統，用於運行時加載
- **運行時定義** - 組件、實體、系統都可在運行時通過 Minic 腳本定義

### 為何使用 Minic？

| 項目 | 自建 JS 引擎 | Minic |
|------|-------------|-------|
| JS 引擎 | ~3000 行 | 0 行（已有）|
| API 綁定 | ~1500 行 | 0 行（已有 500+）|
| 遊戲 API | ~100 行 | ~100 行 |
| **總計** | **~5500 行** | **~400 行** |

Minic 已有 Iron 的完整 API 綁定（500+ 函數），可直接使用。

---

## 技術棧

| 模組 | 技術選擇 |
|------|----------|
| 渲染 | Metal / Vulkan / Direct3D12 / WebGPU (base 已支援) |
| **ECS 架構** | **Flecs** (現代 ECS，支援多執行緒) |
| **腳本系統** | **Minic** (C 語言腳本，運行時加載) |
| 物理 | Jolt Physics |
| UI | 即時模式 (base 已支援) |
| 著色器 | Kong (base 已內建) |

---

## 運行時 ECS 架構

```
┌─────────────────────────────────────────────────────────────┐
│                    運行時加載流程                            │
│                                                             │
│  .minic 腳本 ──► minic_eval() ──► 調用 C API ──► Flecs     │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  components.minic   ──► 運行時註冊組件類型          │   │
│  │  entities.minic     ──► 運行時創建實體              │   │
│  │  systems.minic      ──► 運行時註冊系統              │   │
│  │  game.minic         ──► 遊戲入口，include 上述腳本  │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 架構圖

```
┌─────────────────────────────────────────────────────────────┐
│                    Runtime ECS with Minic                    │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              Minic Scripts (.minic)                  │   │
│  │  ┌───────────────┐ ┌───────────────┐ ┌────────────┐ │   │
│  │  │components.minic│ │systems.minic │ │entities.minic│  │
│  │  └───────┬───────┘ └───────┬───────┘ └──────┬─────┘ │   │
│  └──────────┼─────────────────┼────────────────┼───────┘   │
│             │                 │                │           │
│             ▼                 ▼                ▼           │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              C API (註冊到 minic)                   │   │
│  │  component_register()  system_register()  entity_create() │
│  └─────────────────────┬───────────────────────────────┘   │
│                        │                                   │
│                        ▼                                   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              Flecs ECS World                         │   │
│  │  Components: Position, Health, PlayerInput, ...     │   │
│  │  Systems: HealthRegen, Movement, RenderBridge, ...  │   │
│  └─────────────────────┬───────────────────────────────┘   │
│                        │                                   │
│                        ▼                                   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              Iron Bridge (C)                         │   │
│  │  MeshRenderer → object_t → Iron renderer            │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

---

## 目錄結構

```
armortools/
├── base/                      # Iron 引擎核心 (不修改)
│   ├── sources/
│   │   ├── iron.h            # 所有 API 入口
│   │   ├── libs/
│   │   │   ├── minic.h       # Minic 腳本引擎
│   │   │   ├── minic.c
│   │   │   ├── minic_ext.c
│   │   │   └── minic_api.c   # 500+ Iron API 綁定
│   │   └── ...
│   └── project.js
│
└── engine/                    # 遊戲引擎層 (新)
    ├── project.js
    ├── sources/
    │   ├── game_engine.h/c   # 統一 API 入口
    │   │
    │   ├── core/             # 核心 API (運行時)
    │   │   ├── runtime_api.h/c      # 運行時 API (約 300 行)
    │   │   ├── entity_api.h/c       # Entity API
    │   │   ├── component_api.h/c    # Component API
    │   │   ├── system_api.h/c       # System API
    │   │   ├── query_api.h/c       # Query API
    │   │   ├── game_loop.h/c       # 遊戲循環
    │   │   ├── input.h/c           # 輸入系統
    │   │   └── prefab.h/c          # Prefab 載入/保存
    │   │
    │   ├── ecs/               # Flecs ECS 核心
    │   │   ├── flecs/                 # Flecs 庫
    │   │   ├── ecs_world.h/c          # World 管理
    │   │   ├── ecs_components.h/c     # 內建組件
    │   │   ├── ecs_dynamic.h/c        # 動態組件
    │   │   └── ecs_bridge.h/c        # Iron 橋接
    │   │
    │   └── components/          # 內建組件定義
    │       ├── transform.h/c
    │       ├── mesh_renderer.h/c
    │       └── camera.h/c
    │
    ├── assets/
    │   ├── scripts/
    │   │   ├── game.minic       # 遊戲入口
    │   │   ├── components.minic # 組件定義
    │   │   ├── entities.minic   # 實體創建
    │   │   └── systems.minic    # 系統定義
    │   └── systems/
    │       ├── mouse_system.minic    # 滑鼠輸入測試
    │       ├── gamepad_system.minic  # 手柄輸入測試
    │       ├── movement_system.minic # 移動系統示例
    │       └── health_system.minic   # 生命值系統示例
    │
    └── shaders/
```

---

## 第一階段：核心框架 + Flecs + Minic 整合 (第 1-6 週)

**目標：建立可運行的最小遊戲引擎，支援運行時腳本定義**

### 1.1 項目框架搭建 (第 1 週)

**任務清單**:

| # | 任務 | 檔案 | 狀態 |
|---|------|------|------|
| 1.1.1 | 創建 engine/ 目錄結構 | `engine/` | ✓ 完成 |
| 1.1.2 | 下載 Flecs 單頭文件 | `sources/ecs/flecs/flecs.h` | ✓ 完成 |
| 1.1.3 | 編寫 project.js 構建配置 | `engine/project.js` | ✓ 完成 |
| 1.1.4 | 創建 game_engine.h/c 入口 | `engine/sources/game_engine.h/c` | ✓ 完成 |
| 1.1.5 | 測試編譯 (macOS Metal) | - | ✓ 完成 |

**project.js 示例**:

```js
let project = new Project("IronGame");
project.add_project("../base");
project.add_include_dir("sources");
project.add_cfiles(
    "sources/game_engine.c",
    "sources/core/runtime_api.c",
    "sources/core/entity_api.c",
    "sources/core/component_api.c",
    "sources/core/system_api.c",
    "sources/core/game_loop.c",
    "sources/core/input.c",
    "sources/core/prefab.c",
    "sources/ecs/ecs_world.c",
    "sources/ecs/ecs_components.c",
    "sources/ecs/ecs_dynamic.c",
    "sources/ecs/ecs_bridge.c",
    "sources/components/transform.c",
    "sources/components/mesh_renderer.c",
    "sources/components/camera.c"
);
project.add_assets("assets/scripts/*.minic", {destination: "data/{name}"});
return project;
```

**驗證方法**:

- 編譯成功：`cd engine && ../base/make macos metal`
- 運行測試：顯示空白窗口

**估算工時**: 3-5 天

---

### 1.2 Flecs World 核心 (第 2 週)

**任務清單**:

| # | 任務 | 檔案 | 狀態 |
|---|------|------|------|
| 1.2.1 | 創建 ecs_world.h 類型定義 | `engine/sources/ecs/ecs_world.h` | ✓ 完成 |
| 1.2.2 | 實現 ecs_world.c 初始化 (Flecs v4.0.0) | `engine/sources/ecs/ecs_world.c` | ✓ 完成 |
| 1.2.3 | 下載 flecs.c 實現 | `engine/sources/ecs/flecs/flecs.c` | ✓ 完成 |
| 1.2.4 | 實現 ecs_dynamic.c 動態組件 | `engine/sources/ecs/ecs_dynamic.h/c` | ✓ 完成 |
| 1.2.5 | 測試：創建 Flecs World | - | ✓ 完成 |

**API 函數清單**:

```c
// ecs_world.h
typedef struct {
    ecs_world_t *world;
    ecs_entity_t bridge_system;
    f32 delta_time;
    f32 time;
    u64 frame_count;
} game_world_t;

game_world_t *game_world_create(void);
void game_world_destroy(game_world_t *world);
void game_world_progress(game_world_t *world, f32 dt);
ecs_world_t *game_world_get_ecs(game_world_t *world);

// ecs_components.h - 內建組件
typedef struct { f32 x, y, z; } Position;
typedef struct { f32 x, y, z, w; } Rotation;  // quaternion
typedef struct { f32 x, y, z; } Scale;
typedef struct { char *value; } Name;
typedef struct { bool value; } Active;
typedef struct { object_t *object; transform_t *transform; bool dirty; } IronObject;
```

**驗證方法**:

- 創建 game_world
- 確認 Flecs 正常工作
- 打印 `game_world->time`

**估算工時**: 5-7 天

---

### 1.3 運行時 API - Component (第 2-3 週)

**任務清單**:

| # | 任務 | 檔案 | 狀態 |
|---|------|------|------|
| 1.3.1 | 創建 component_registry.h 類型 | `engine/sources/core/component_registry.h` | ✓ 完成 |
| 1.3.2 | 實現 component_registry.c | `engine/sources/core/component_registry.c` | ✓ 完成 |
| 1.3.3 | 實現 component_api.c 運行時 API | `engine/sources/core/component_api.c` | ✓ 完成 |
| 1.3.4 | 測試：運行時註冊組件 | - | ✓ 完成 |

**API 函數清單**:

```c
// component_registry.h
typedef struct {
    char name[64];
    size_t size;
    ecs_entity_t flecs_id;
    size_t field_count;
    struct {
        char name[32];
        int type;      // TYPE_INT, TYPE_FLOAT, TYPE_PTR
        size_t offset;
    } fields[16];
} component_type_t;

// 運行時註冊組件
int component_register(const char *name, size_t size);
void component_add_field(int type_id, const char *name, int type, size_t offset);
component_type_t *component_get_by_name(const char *name);
component_type_t *component_get_by_id(int id);

// minic 註冊函數
void component_api_register(void);
```

**Minic 腳本示例**:

```c
// assets/scripts/components.minic

// 定義 Health 組件
let health_type = component_register("Health", 16);
component_add_field(health_type, "current", TYPE_FLOAT, 0);
component_add_field(health_type, "max", TYPE_FLOAT, 4);
component_add_field(health_type, "regen_rate", TYPE_FLOAT, 8);
component_add_field(health_type, "invulnerable", TYPE_INT, 12);

// 定義 PlayerInput 組件
let input_type = component_register("PlayerInput", 12);
component_add_field(input_type, "move_x", TYPE_FLOAT, 0);
component_add_field(input_type, "move_y", TYPE_FLOAT, 4);
component_add_field(input_type, "jump", TYPE_INT, 8);
```

**估算工時**: 7-10 天

---

### 1.4 運行時 API - Entity (第 3 週)

**任務清單**:

| # | 任務 | 檔案 | 狀態 |
|---|------|------|------|
| 1.4.1 | 實現 entity_api.c | `engine/sources/core/entity_api.c` | ✓ 完成 |
| 1.4.2 | 實現父子層級關係 | `engine/sources/core/entity_api.c` | ✓ 完成 |
| 1.4.3 | 實現便捷創建函數 | `engine/sources/core/entity_api.c` | ✓ 完成 |
| 1.4.4 | 測試：運行時創建實體 | - | ✓ 完成 |

**API 函數清單**:

```c
// entity_api.h
ecs_entity_t entity_create(const char *name);
void entity_destroy(ecs_entity_t entity);

void *entity_add_component(ecs_entity_t entity, const char *comp_name);
void *entity_get_component(ecs_entity_t entity, const char *comp_name);
bool entity_has_component(ecs_entity_t entity, const char *comp_name);
void entity_remove_component(ecs_entity_t entity, const char *comp_name);

void entity_set_parent(ecs_entity_t child, ecs_entity_t parent);
ecs_entity_t entity_get_parent(ecs_entity_t entity);

void entity_set_active(ecs_entity_t entity, bool active);
bool entity_is_active(ecs_entity_t entity);

const char *entity_get_name(ecs_entity_t entity);

ecs_entity_t entity_find(const char *name);

// 便捷函數
void entity_add_transform(ecs_entity_t entity);
void entity_add_mesh_renderer(ecs_entity_t entity, const char *mesh, const char *mat);
void entity_add_script(ecs_entity_t entity, const char *script_path);

// minic 註冊函數
void entity_api_register(void);
```

**Minic 腳本示例**:

```c
// assets/scripts/entities.minic

// 創建玩家
let player = entity_create("Player");
let pos = entity_add_component(player, "Position");
pos.x = 0.0; pos.y = 1.0; pos.z = 0.0;
let health = entity_add_component(player, "Health");
health.current = 100.0; health.max = 100.0;
let input = entity_add_component(player, "PlayerInput");
entity_add_transform(player);
entity_add_mesh_renderer(player, "models/player.glb", "materials/pbr.json");

// 創建敵人
let enemy = entity_create("Enemy");
let e_pos = entity_add_component(enemy, "Position");
e_pos.x = 10.0; e_pos.y = 0.0; e_pos.z = 0.0;
let e_health = entity_add_component(enemy, "Health");
e_health.current = 50.0; e_health.max = 50.0;
```

**估算工時**: 5-7 天

---

### 1.5 運行時 API - System (第 3-4 週)

**任務清單**:

| # | 任務 | 檔案 | 狀態 |
|---|------|------|------|
| 1.5.1 | 創建 system_api.h 類型 | `engine/sources/core/system_api.h` | ✓ 完成 |
| 1.5.2 | 實現 system_api.c | `engine/sources/core/system_api.c` | ✓ 完成 |
| 1.5.3 | 實現 query_api.c | `engine/sources/core/query_api.h/c` | ✓ 完成 |
| 1.5.4 | 測試：運行時註冊系統 | - | ✓ 完成 |

**API 函數清單**:

```c
// system_api.h
#define PHASE_UPDATE 0
#define PHASE_PRE_RENDER 1
#define PHASE_RENDER 2

typedef struct {
    char name[64];
    ecs_entity_t system_entity;
    ecs_query_t *query;
    int phase;
} runtime_system_t;

int system_register(const char *name, void *fn, const char *query, int phase);
void system_unregister(const char *name);
runtime_system_t *system_get(const char *name);

// minic 函數包裝
typedef struct {
    void *fn_ptr;
    int argc;
    void *argv[16];
} minic_system_ctx_t;

void system_call(const char *name, f32 dt);

void system_api_register(void);

// query_api.h
typedef struct {
    ecs_query_t *query;
    ecs_iter_t *iter;
} runtime_query_t;

int query_create(const char *filter);
void query_destroy(int query_id);
bool query_next(int query_id);
void *query_get(int query_id, int index, const char *comp_name);
int query_count(int query_id);

void query_api_register(void);
```

**Minic 腳本示例**:

```c
// assets/scripts/systems.minic

// 健康回復系統
function health_regen(it) {
    let i = 0;
    while (i < it.count) {
        let health = it.get(i, "Health");
        if (health.current < health.max) {
            health.current = health.current + health.regen_rate * sys_delta();
            if (health.current > health.max) {
                health.current = health.max;
            }
        }
        i = i + 1;
    }
}
system_register("HealthRegen", health_regen, "Health, ?Active", PHASE_UPDATE);

// 移動系統
function movement(it) {
    let i = 0;
    while (i < it.count) {
        let pos = it.get(i, "Position");
        let input = it.get(i, "PlayerInput");
        
        pos.x = pos.x + input.move_x * 5.0 * sys_delta();
        pos.z = pos.z + input.move_y * 5.0 * sys.delta;
        
        i = i + 1;
    }
}
system_register("Movement", movement, "Position, PlayerInput", PHASE_UPDATE);

// 渲染橋接系統
function render_bridge(it) {
    let i = 0;
    while (i < it.count) {
        let pos = it.get(i, "Position");
        let rot = it.get(i, "Rotation");
        let iron = it.get(i, "IronObject");
        
        transform_set_position(iron.transform, pos.x, pos.y, pos.z);
        transform_set_rotation(iron.transform, rot.x, rot.y, rot.z, rot.w);
        
        i = i + 1;
    }
}
system_register("RenderBridge", render_bridge, "Position, Rotation, IronObject", PHASE_PRE_RENDER);
```

**估算工時**: 7-10 天

---

### 1.6 Iron Bridge (第 4 週)

**任務清單**:

| # | 任務 | 檔案 | 狀態 |
|---|------|------|------|
| 1.6.1 | 實現 ecs_bridge.c | `engine/sources/ecs/ecs_bridge.h/c` | ✓ 完成 |
| 1.6.2 | 實現 MeshRenderer 組件 | `engine/sources/components/mesh_renderer.h/c` | ✓ 完成 |
| 1.6.3 | 實現 Transform 同步 | `engine/sources/components/transform.h/c` | ✓ 完成 |
| 1.6.4 | 測試：渲染帶 MeshRenderer 的 Entity | - | ✓ 完成 |

**Bridge 工作流程**:

```
MeshRenderer 組件被添加 (entity_add_mesh_renderer)
       │
       ▼
Flecs 檢測到 IronObject.dirty == true
       │
       ▼
Bridge System 運行 (Phase: PreRender)
       │
       ▼
if (IronObject.object == NULL) {
    IronObject.object = object_create(OBJECT_TYPE_MESH);
    IronObject.transform = transform_create(IronObject.object);
    scene_add_object(scene_get_active(), IronObject.object);
}
       │
       ▼
同步 Position/Rotation/Scale → Iron transform
```

**估算工時**: 5-7 天

---

### 1.7 遊戲循環 (第 4-5 週)

**任務清單**:

| # | 任務 | 檔案 | 狀態 |
|---|------|------|------|
| 1.7.1 | 實現 game_loop.c | `engine/sources/core/game_loop.h/c` | ✓ 完成 |
| 1.7.2 | 實現 update_phase.h | `engine/sources/core/update_phase.h` | ✓ 完成 |
| 1.7.3 | 連接到 Iron 更新回調 | `engine/sources/game_engine.c` | ✓ 完成 |
| 1.7.4 | 測試固定時間步長 | - | ✓ 完成 |

**API 函數清單**:

```c
// game_loop.h
typedef enum {
    GAME_PHASE_INPUT,
    GAME_PHASE_PRE_UPDATE,
    GAME_PHASE_UPDATE,
    GAME_PHASE_POST_UPDATE,
    GAME_PHASE_PRE_RENDER,
    GAME_PHASE_RENDER,
    GAME_PHASE_POST_RENDER,
} game_update_phase_t;

void game_loop_init(game_world_t *world);
void game_loop_shutdown(void);
void game_loop_update(void);  // 連接到 Iron 的 update callback
f32 game_loop_get_delta_time(void);
f32 game_loop_get_time(void);
u64 game_loop_get_frame_count(void);
```

**估算工時**: 3-5 天

---

### 1.8 輸入系統 (第 5 週)

**任務清單**:

| # | 任務 | 檔案 | 狀態 |
|---|------|------|------|
| 1.8.1 | 實現鍵盤追蹤 Minic 包裝 | `engine/sources/core/runtime_api.c` | ✓ 完成 |
| 1.8.2 | 實現滑鼠追蹤 Minic 包裝 | `engine/sources/core/runtime_api.c` | ✓ 完成 |
| 1.8.3 | 實現遊戲手柄 Minic 包裝 | `engine/sources/core/runtime_api.c` | ✓ 完成 |
| 1.8.4 | 連接到 Iron 輸入事件 | `engine/sources/core/runtime_api.c` | ✓ 完成 |
| 1.8.5 | 測試：Minic 讀取輸入 | `engine/assets/systems/mouse_system.minic` | ✓ 完成 |
| 1.8.6 | 測試：Gamepad 讀取輸入 | `engine/assets/systems/gamepad_system.minic` | ✓ 完成 |

**API 函數清單**:

```c
// runtime_api.c - 鍵盤 API
minic_val_t minic_keyboard_down(minic_val_t *args, int argc);
minic_val_t minic_keyboard_started(minic_val_t *args, int argc);
minic_val_t minic_keyboard_released(minic_val_t *args, int argc);

// runtime_api.c - 滑鼠 API
minic_val_t minic_mouse_down(minic_val_t *args, int argc);
minic_val_t minic_mouse_started(minic_val_t *args, int argc);
minic_val_t minic_mouse_released(minic_val_t *args, int argc);
minic_val_t minic_mouse_x(minic_val_t *args, int argc);
minic_val_t minic_mouse_y(minic_val_t *args, int argc);
minic_val_t minic_mouse_dx(minic_val_t *args, int argc);
minic_val_t minic_mouse_dy(minic_val_t *args, int argc);
minic_val_t minic_mouse_wheel_delta(minic_val_t *args, int argc);

// runtime_api.c - 遊戲手柄 API
minic_val_t minic_gamepad_down(minic_val_t *args, int argc);
minic_val_t minic_gamepad_started(minic_val_t *args, int argc);
minic_val_t minic_gamepad_released(minic_val_t *args, int argc);
minic_val_t minic_gamepad_stick_left_x(minic_val_t *args, int argc);
minic_val_t minic_gamepad_stick_left_y(minic_val_t *args, int argc);
minic_val_t minic_gamepad_stick_right_x(minic_val_t *args, int argc);
minic_val_t minic_gamepad_stick_right_y(minic_val_t *args, int argc);
minic_val_t minic_gamepad_stick_delta_x(minic_val_t *args, int argc);
minic_val_t minic_gamepad_stick_delta_y(minic_val_t *args, int argc);
```

**Minic 使用示例**:

```c
// 鍵盤
int shift_down = keyboard_down("shift");
int ctrl_down = keyboard_down("control");
int a_pressed = keyboard_started("a");
int space_released = keyboard_released("space");

// 滑鼠
int left_down = mouse_down("left");
int left_started = mouse_started("left");
int mouse_x = mouse_x();
int mouse_y = mouse_y();
int wheel = mouse_wheel_delta();

// 遊戲手柄 (Xbox 布局)
int a_down = gamepad_down("a");
int b_started = gamepad_started("b");
float lx = gamepad_stick_left_x(0);
float ly = gamepad_stick_left_y(0);
float rx = gamepad_stick_right_x(0);
float ry = gamepad_stick_right_y(0);
float dx = gamepad_stick_delta_x(0);
float dy = gamepad_stick_delta_y(0);
```

**估算工時**: 3-5 天 ✓ 完成

**API 函數清單**:

```c
// input.h
bool input_key_down(const char *key);      // minic 已有 keyboard_down
bool input_key_pressed(const char *key);
bool input_key_released(const char *key);

float input_mouse_x(void);
float input_mouse_y(void);
float input_mouse_delta_x(void);
float input_mouse_delta_y(void);
bool input_mouse_button_down(int button);
bool input_mouse_button_pressed(int button);

vec2_t input_get_axis(const char *axis);   // "Horizontal", "Vertical"

// 快捷方式
#define input_left() input_key_down("A") || input_key_down("Left")
#define input_right() input_key_down("D") || input_key_down("Right")
#define input_forward() input_key_down("W") || input_key_down("Up")
#define input_backward() input_key_down("S") || input_key_down("Down")
```

**估算工時**: 3-5 天

---

### 1.9 Prefab 系統 (第 5-6 週)

**任務清單**:

| # | 任務 | 檔案 | 狀態 |
|---|------|------|------|
| 1.9.1 | 創建 prefab.h/c 類型定義 | `engine/sources/core/prefab.h/c` | 待辦 |
| 1.9.2 | 實現 prefab_load.c (JSON → Flecs) | `engine/sources/core/prefab_load.c` | 待辦 |
| 1.9.3 | 實現 prefab_save.c (Flecs → JSON) | `engine/sources/core/prefab_save.c` | 待辦 |
| 1.9.4 | 測試：Prefab 載入/保存 | - | 待辦 |

**Prefab JSON 格式**:

```json
{
    "version": "1.0",
    "type": "entity",
    "name": "Player",
    "uuid": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
    "active": true,
    "transform": {
        "position": [0, 1, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1]
    },
    "components": [
        {
            "type": "MeshRenderer",
            "mesh": "assets/models/player.glb",
            "material": "assets/materials/pbr.json",
            "cast_shadows": true
        },
        {
            "type": "Health",
            "current": 100.0,
            "max": 100.0,
            "regen_rate": 1.0
        }
    ],
    "children": []
}
```

**估算工時**: 5-7 天

---

### 1.10 Minic 整合 (第 6 週)

**任務清單**:

| # | 任務 | 檔案 | 狀態 |
|---|------|------|------|
| 1.10.1 | 實現 runtime_api.c 統一註冊 | `engine/sources/core/runtime_api.c` | ✓ 完成 |
| 1.10.2 | 創建遊戲入口腳本 | `engine/assets/scripts/game.minic` | ✓ 完成 |
| 1.10.3 | 整合所有 API | `engine/sources/game_engine.c` | ✓ 完成 |
| 1.10.4 | 測試：完整運行 | - | ✓ 完成 |

**runtime_api.c 示例**:

```c
// engine/sources/core/runtime_api.c

void runtime_api_register(void) {
    // 註冊所有運行時 API
    component_api_register();
    entity_api_register();
    system_api_register();
    query_api_register();
    
    // 遊戲循環 API
    minic_register("sys_delta", "f()", (minic_ext_fn_raw_t)game_loop_get_delta_time);
    minic_register("sys_time", "f()", (minic_ext_fn_raw_t)game_loop_get_time);
    minic_register("sys_frame", "i()", (minic_ext_fn_raw_t)game_loop_get_frame_count);
    
    // 輸入 API
    minic_register("input_key_down", "i(p)", (minic_ext_fn_raw_t)input_key_down);
    minic_register("input_axis", "p(p)", (minic_ext_fn_raw_t)input_get_axis);
    
    // 類型常量
    minic_register("TYPE_INT", "i()", NULL);
    minic_register("TYPE_FLOAT", "i()", NULL);
    minic_register("TYPE_PTR", "i()", NULL);
    
    minic_register("PHASE_UPDATE", "i()", NULL);
    minic_register("PHASE_PRE_RENDER", "i()", NULL);
    minic_register("PHASE_RENDER", "i()", NULL);
    
    // Prefab API
    minic_register("prefab_load", "p(p)", (minic_ext_fn_raw_t)prefab_load);
    minic_register("prefab_save", "i(p,p)", (minic_ext_fn_raw_t)prefab_save);
    minic_register("prefab_instantiate", "i(p)", (minic_ext_fn_raw_t)prefab_instantiate);
}
```

**game.minic 示例**:

```c
// assets/scripts/game.minic

// 1. 註冊組件
include("components.minic");

// 2. 創建實體
include("entities.minic");

// 3. 註冊系統
include("systems.minic");

// 4. 遊戲配置
printf("Game loaded: %s\n", "Hello World");
```

**估算工時**: 3-5 天

---

## 第一階段交付成果

### 功能清單

| 功能 | 狀態 |
|------|------|
| Flecs World 初始化 | ✓ |
| 運行時 Component 註冊 (Minic) | ✓ |
| 運行時 Entity 創建 (Minic) | ✓ |
| 運行時 System 註冊 (Minic) | ✓ |
| Transform 組件 | ✓ |
| MeshRenderer 組件 | ✓ |
| Iron Bridge (Flecs → Iron) | ✓ |
| 2D Sprite System (ECS) | ✓ |
| 2D Camera System | ✓ |
| 2D 渲染 (render2d_bridge) | ✓ |
| 遊戲循環 | ✓ |
| 鍵盤輸入 (Minic) | ✓ |
| 滑鼠輸入 (Minic) | ✓ |
| 遊戲手柄輸入 (Minic) | ✓ |
| Prefab 載入/保存 | 待辦 |

### 驗收標準

1. **編譯通過** - 無錯誤、無警告
2. **渲染測試** - 運行 `game.minic` 創建 Entity，渲染到屏幕
3. **熱重載** - 修改 `.minic` 文件後無需重編譯即可生效

### 測試腳本

```c
// assets/scripts/game.minic

int init(void) {
    printf("=== Game Initialization ===\n");
    
    int pos_comp = component_register("Position", 12);
    component_add_field(pos_comp, "x", TYPE_FLOAT, 0);
    component_add_field(pos_comp, "y", TYPE_FLOAT, 4);
    component_add_field(pos_comp, "z", TYPE_FLOAT, 8);
    
    int vel_comp = component_register("Velocity", 12);
    component_add_field(vel_comp, "vx", TYPE_FLOAT, 0);
    component_add_field(vel_comp, "vy", TYPE_FLOAT, 4);
    component_add_field(vel_comp, "vz", TYPE_FLOAT, 8);
    
    int i = 0;
    while (i < 5) {
        int e = entity_create();
        entity_add(e, pos_comp);
        entity_add(e, vel_comp);
        
        void* pos = entity_get(e, pos_comp);
        comp_set_float(pos_comp, pos, "x", i * 10.0);
        comp_set_float(pos_comp, pos, "y", i * 5.0);
        comp_set_float(pos_comp, pos, "z", 0.0);
        i = i + 1;
    }
    
    printf("=== Game Init Complete ===\n");
    return 0;
}
```

```c
// assets/systems/mouse_system.minic

int init(void) {
    g_query = query_new();
    return 0;
}

int step() {
    int left_down = mouse_down("left");
    int left_started = mouse_started("left");
    int right_down = mouse_down("right");
    
    if (left_down > 0 || left_started > 0 || right_down > 0) {
        printf("[MouseSystem] left: down=%d started=%d | pos=(%d,%d) delta=(%d,%d) wheel=%d\n",
               left_down, left_started, mouse_x(), mouse_y(), mouse_dx(), mouse_dy(), mouse_wheel_delta());
    }
    return 0;
}
```

```c
// assets/systems/gamepad_system.minic

int step() {
    int a_started = gamepad_started("a");
    int b_started = gamepad_started("b");
    float lx = gamepad_stick_left_x(0);
    float ly = gamepad_stick_left_y(0);
    
    if (a_started || b_started) {
        printf("[GamepadSystem] buttons: a=%d b=%d\n", a_started, b_started);
    }
    if (lx != 0.0 || ly != 0.0) {
        printf("[GamepadSystem] sticks: left=(%d,%d)\n", lx * 100, ly * 100);
    }
    return 0;
}
```

**估算總工時**: 4-6 週

---

## 第二階段：物理系統 (第 7-10 週)

### 2.1 Jolt Physics 整合

| # | 任務 | 檔案 |
|---|------|------|
| 2.1.1 | 下載 Jolt Physics | `physics/jolt/` |
| 2.1.2 | 實現 PhysicsBridge | `physics/physics_bridge.c` |
| 2.1.3 | 實現 RigidBody 組件 | `rigid_body.h/c` |
| 2.1.4 | 實現 Collider 組件 | `collider.h/c` |
| 2.1.5 | 實現 PhysicsSync | `physics/physics_sync.c` |

### 2.2 碰撞事件

| # | 任務 | 檔案 |
|---|------|------|
| 2.2.1 | 定義碰撞事件結構 | `collision_events.h` |
| 2.2.2 | 實現事件分發 | `collision_events.c` |
| 2.2.3 | Minic 碰撞 API | `minic_register("on_collide", ...)` |

---

## 第三階段：編輯器 (第 11-16 週)

### 3.1 編輯器框架

| # | 任務 | 檔案 |
|---|------|------|
| 3.1.1 | 創建編輯器入口 | `editor/main.c` |
| 3.1.2 | 實現面板系統 | `panel_manager.c` |
| 3.1.3 | 實現 SceneView | `panels/scene_view.c` |
| 3.1.4 | 實現 Hierarchy | `panels/hierarchy.c` |
| 3.1.5 | 實現 Inspector | `panels/inspector.c` |

### 3.2 Gizmos

| # | 任務 | 檔案 |
|---|------|------|
| 3.2.1 | Translate Gizmo | `gizmos/translate_gizmo.c` |
| 3.2.2 | Rotate Gizmo | `gizmos/rotate_gizmo.c` |
| 3.2.3 | Scale Gizmo | `gizmos/scale_gizmo.c` |

---

## 第四階段：性能優化 (第 17-20 週)

### 4.1 多執行緒

| # | 任務 |
|---|------|
| 4.1.1 | Flecs scheduler 配置 |
| 4.1.2 | 系統並行化 |
| 4.1.3 | 渲染線程分離 |

### 4.2 內存管理

| # | 任務 |
|---|------|
| 4.2.1 | 對象池 |
| 4.2.2 | 組件數組優化 |
| 4.2.3 | 熱路徑優化 |

---

## 統一 Prefab 格式

```json
{
    "version": "1.0",
    "type": "entity",
    "name": "Player",
    "uuid": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
    "active": true,
    "tag": "Player",
    "layer": "Default",
    "transform": {
        "position": [0, 1, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1]
    },
    "components": [
        {
            "type": "MeshRenderer",
            "mesh": "assets/models/player.glb",
            "material": "assets/materials/pbr.json",
            "cast_shadows": true
        },
        {
            "type": "Health",
            "current": 100.0,
            "max": 100.0,
            "regen_rate": 1.0
        }
    ],
    "children": [
        {
            "name": "CameraMount",
            "active": true,
            "transform": {"position": [0, 0.6, 0]},
            "components": []
        }
    ]
}
```

---

## 階段總結

| 階段 | 週次 | 估算工時 |
|------|------|----------|
| 第一階段 | 1-6 | 4-6 週 |
| 第二階段 | 7-10 | 4-5 週 |
| 第三階段 | 11-16 | 5-6 週 |
| 第四階段 | 17-20 | 4-5 週 |
| **總計** | **1-20** | **17-22 週** |

### 代碼量估算

| 模組 | 行數 |
|------|------|
| Flecs World | ~200 |
| 運行時 API (Component/Entity/System/Query) | ~800 |
| Iron Bridge | ~300 |
| 遊戲循環 + 輸入 | ~300 |
| Prefab | ~400 |
| 內建組件 | ~200 |
| **Phase 1 總計** | **~2200 行** |

---

## 技術參考

- [Flecs ECS](https://github.com/SanderMertens/flecs) - 現代 ECS 框架
- [Flecs Hub](https://github.com/flecs-hub) - 遊戲組件庫集合
- [Jolt Physics](https://github.com/jrouwe/JoltPhysics) - 現代 C++ 物理引擎
- [Minic API](https://github.com/armory3d/minic) - Iron 內建腳本引擎
- [Godot ECS](https://docs.godotengine.org/en/stable/development/core_and_bindings/ecs.html) - Godot ECS 設計參考
