# ECS 功能完整化計劃

## 概述

將 engine/ 的 ECS 功能從當前 60% 完成度提升至 95%，專注於核心 ECS 功能，暫不包含渲染、Prefab 系統。

**核心原則：不修改 base/ 目錄，所有新代碼放在 engine/ 目錄。**

---

## 現狀評估

| 模組 | 狀態 | 說明 |
|------|------|------|
| **ECS World** | ✅ 完成 | game_world_t 封裝正常運作 |
| **內建組件** | ✅ 完成 | TransformPosition/Rotation/Scale, RenderMesh 等 |
| **Entity API** | ✅ 完成 | CRUD 操作正常 |
| **Component API** | ⚠️ 部分 | 缺少 field count/info 獲取 |
| **System API** | ⚠️ 部分 | minic_system_get_entity() 返回0 |
| **Query API** | ❌ 未實現 | 完全缺失 |
| **Game Loop** | ✅ 完成 | delta/time/frame 正常 |
| **Input 系統** | ❌ 未實現 | 完全缺失 |
| **Prefab 系統** | ❌ 未實現 | 可延後 |
| **Minic 綁定** | ⚠️ 部分 | 缺少 ~10個實用函數 |

---

## Phase 1: Query API (高優先級)

### 目標
實現從 Minic 腳本創建和迭代 Flecs 查詢的能力。

### 當前狀態
- `query_api.c/h` 存在但完全是 stub
- 沒有任何查詢功能實現
- 沒有任何 Minic 綁定

### 需要實現

**query_api.h:**
```c
typedef struct {
    ecs_query_t *query;
    char filter[256];
    bool valid;
} runtime_query_t;

int query_create(const char *filter_expr);
void query_destroy(int query_id);
bool query_next(int query_id);
int query_count(int query_id);
void *query_get(int query_id, int index, const char *component_name);
int query_get_field_id(int query_id, const char *component_name);
```

**Minic 綁定:**
```c
minic_register("query_create", "i(p)", ...);
minic_register("query_destroy", "v(i)", ...);
minic_register("query_next", "i(i)", ...);
minic_register("query_count", "i(i)", ...);
minic_register("query_get", "p(i,i,p)", ...);
```

**Minic 使用示例:**
```c
// 創建查詢：所有具有 Position 和 Velocity 組件的實體
int move_q = query_create("Position, Velocity");

// 迭代查詢
while (query_next(move_q)) {
    int count = query_count(move_q);
    int i = 0;
    while (i < count) {
        // 獲取組件數據指針
        void *pos_data = query_get(move_q, i, "Position");
        void *vel_data = query_get(move_q, i, "Velocity");
        
        // 使用 comp_get/set 操作字段
        float x = comp_get_float(pos_comp_id, pos_data, "x");
        
        i = i + 1;
    }
}

// 銷毀查詢
query_destroy(move_q);
```

### 實現步驟

1. 在 `query_api.h` 中定義 `runtime_query_t` 結構
2. 在 `query_api.c` 中實現查詢表管理
3. 實現 `query_create()` - 使用 `ecs_query_desc_t` 創建 Flecs 查詢
4. 實現 `query_destroy()` - 釋放查詢資源
5. 實現 `query_iter_t` 結構保存迭代狀態
6. 實現 `query_next()` - 使用 `ecs_query_next()` 迭代
7. 實現 `query_count()` - 返回 `it->count`
8. 實現 `query_get()` - 使用 `ecs_field()` 獲取組件數據
9. 在 `runtime_api_register()` 中添加 Minic 綁定

### 預計工時
2-3 天

---

## Phase 2: System API 修復 (中優先級)

### 目標
修復 `minic_system_get_entity()` 總是返回 0 的問題。

### 當前問題

在 `system_api.c` 中，`minic_system_trampoline` 和 `minic_system_get_entity` 無法正確獲取當前系統的實體。

### 需要修復

```c
// 當前任務: ECS_SYSTEM 結構中存儲當前迭代的實體
typedef struct {
    const char *name;
    ecs_entity_t entity;
    ecs_query_t *query;
    int phase;
    void *ctx;
    bool enabled;
    ecs_entity_t last_entity;  // 新增：跟踪最後處理的實體
} ECS_SYSTEM;

// trampoline 需要存儲當前迭代上下文
static void minic_system_trampoline(ecs_iter_t *it) {
    // 存儲當前實體到系統上下文
    ECS_SYSTEM *sys = (ECS_SYSTEM *)it->ctx;
    // 遍歷並設置 last_entity
}
```

### 實現步驟

1. 在 `ECS_SYSTEM` 結構中添加 `last_entity` 和 `current_iter` 字段
2. 修改 `minic_system_trampoline` 在每次迭代時更新這些字段
3. 實現 `minic_system_get_entity()` 返回 `current_system->last_entity`
4. 添加 `minic_system_get_count()` 返回 `it->count`

### 預計工時
1 天

---

## Phase 3: Input 系統 (中優先級)

### 目標
實現鍵盤和鼠標輸入追蹤，與 Iron 的輸入系統整合。

### 當前狀態
- `input.c/h` 存在但完全是 stub
- 沒有任何輸入處理

### 需要實現

**input.h:**
```c
void input_init(void);
void input_shutdown(void);

bool input_key_down(const char *key);
bool input_key_pressed(const char *key);
bool input_key_released(const char *key);

float input_mouse_x(void);
float input_mouse_y(void);
float input_mouse_delta_x(void);
float input_mouse_delta_y(void);
bool input_mouse_button_down(int button);
bool input_mouse_button_pressed(int button);
bool input_mouse_button_released(int button);

float input_get_axis(const char *axis);

// 快捷方式
bool input_left(void);
bool input_right(void);
bool input_up(void);
bool input_down(void);
```

### 實現策略

Iron 的 base 已經有鍵盤追蹤，使用 `keyboard_down()` 從 minic_api.c。我們需要包裝這些並添加狀態追蹤（pressed/released）。

```c
// input.c
static bool g_key_states[256];
static bool g_key_prev_states[256];
static float g_mouse_x, g_mouse_y;
static float g_mouse_delta_x, g_mouse_delta_y;

void input_update(void) {
    // 每幀調用，保存上一幀狀態
    memcpy(g_key_prev_states, g_key_states, sizeof(g_key_states));
}

bool input_key_pressed(const char *key) {
    int code = key_to_code(key);
    return g_key_states[code] && !g_key_prev_states[code];
}

bool input_key_released(const char *key) {
    int code = key_to_code(key);
    return !g_key_states[code] && g_key_prev_states[code];
}
```

### 整合游戲循環

在 `game_loop_update()` 中調用 `input_update()`:

```c
void game_loop_update(void) {
    input_update();  // 新增：更新輸入狀態
    g_delta_time = sys_delta();
    g_time += g_delta_time;
    g_frame_count++;
    game_world_progress(g_loop_world, g_delta_time);
}
```

### Minic 綁定

```c
minic_register("input_key_down", "i(p)", (minic_ext_fn_raw_t)input_key_down);
minic_register("input_key_pressed", "i(p)", (minic_ext_fn_raw_t)input_key_pressed);
minic_register("input_key_released", "i(p)", (minic_ext_fn_raw_t)input_key_released);
minic_register("input_mouse_x", "f()", (minic_ext_fn_raw_t)input_mouse_x);
minic_register("input_mouse_y", "f()", (minic_ext_fn_raw_t)input_mouse_y);
minic_register("input_mouse_delta_x", "f()", (minic_ext_fn_raw_t)input_mouse_delta_x);
minic_register("input_mouse_delta_y", "f()", (minic_ext_fn_raw_t)input_mouse_delta_y);
```

### Minic 使用示例

```c
float main() {
    int player = entity_create("Player");
    entity_add(player, pos_comp);
    entity_add(player, vel_comp);
    
    while (1) {
        // 玩家輸入
        if (input_key_down("W") || input_key_down("Up")) {
            comp_set_float(vel_comp, vel_data, "z", 5.0);
        } else {
            comp_set_float(vel_comp, vel_data, "z", 0.0);
        }
        
        // 應用速度到位置
        void *pos = entity_get(player, pos_comp);
        void *vel = entity_get(player, vel_comp);
        float vz = comp_get_float(vel_comp, vel, "z");
        
        float px = comp_get_float(pos_comp, pos, "x");
        float pz = comp_get_float(pos_comp, pos, "z");
        comp_set_float(pos_comp, pos, "x", px + 0.0 * sys_delta());
        comp_set_float(pos_comp, pos, "z", pz + vz * sys_delta());
        
        // 旋轉相機
        if (input_mouse_button_down(1)) {  // 右鍵
            float dx = input_mouse_delta_x();
            float cam_rot = comp_get_float(cam_comp, cam_data, "y");
            comp_set_float(cam_comp, cam_data, "y", cam_rot + dx * 0.1);
        }
    }
    
    return 0.0;
}
```

### 預計工時
2-3 天

---

## Phase 4: 補充 Minic 綁定 (低優先級)

### 目標
添加實用但缺失的 Minic 函數。

### 缺少的函數

| 函數 | 簽名 | 說明 |
|------|------|------|
| `entity_is_valid` | `i(i)` | 檢查實體是否有效 |
| `entity_get_name` | `p(i)` | 獲取實體名稱 |
| `entity_set_name` | `v(i,p)` | 設置實體名稱 |
| `entity_get_parent` | `i(i)` | 獲取父實體 |
| `entity_set_parent` | `v(i,i)` | 設置父實體 |
| `component_get_alignment` | `i(i)` | 獲取組件對齊 |
| `component_get_field_count` | `i(i)` | 獲取字段數量 |
| `component_get_field_info` | `i(i,i,p,i)` | 獲取字段詳細信息 |
| `system_get_entity` | `i()` | 獲取當前系統實體 |
| `system_get_count` | `i()` | 獲取當前系統匹配的實體數 |

### 實現步驟

這些都是簡單的包裝函數，在 `runtime_api.c` 中添加:

```c
// entity_is_valid
static int minic_entity_is_valid(uint64_t entity) {
    return entity_is_valid(g_runtime_world, entity) ? 1 : 0;
}

// entity_get_name  
static const char *minic_entity_get_name(uint64_t entity) {
    return entity_get_name(g_runtime_world, entity);
}

// component_get_field_count
static int minic_component_get_field_count(uint64_t comp_id) {
    return component_get_field_count(comp_id);
}
```

然後在 `runtime_api_register()` 中註冊。

### 預計工時
1 天

---

## Phase 5: 動態組件改進 (低優先級)

### 當前限制

- 最多 256 個動態組件
- 每個組件最多 16 字段
- 只支持 4字節類型 (INT, FLOAT, BOOL, PTR)

### 可選改進

1. **動態擴展限制**:
```c
#define MAX_DYNAMIC_COMPONENTS 256  // 改為動態分配
#define MAX_FIELDS_PER_COMPONENT 16
```

2. **添加 STRING 類型支持**:
```c
case DYNAMIC_TYPE_STRING:
    field_size = strlen((char*)value) + 1;
    strcpy((char*)comp_data + offset, (char*)value);
    break;
```

### 預計工時
1-2 天（如果要做）

---

## Phase 6: Prefab 系統 (可延後)

### 目標
實現 JSON 格式的 Prefab 載入/保存/實例化。

### JSON 格式

```json
{
    "version": "1.0",
    "type": "entity",
    "name": "Player",
    "active": true,
    "transform": {
        "position": [0, 1, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1]
    },
    "components": [
        {
            "type": "Health",
            "current": 100.0,
            "max": 100.0
        }
    ],
    "children": []
}
```

### 需要實現

```c
// prefab.h
uint64_t prefab_load(const char *path);
int prefab_save(uint64_t entity, const char *path);
uint64_t prefab_instantiate(uint64_t prefab_entity);

// Minic 綁定
minic_register("prefab_load", "i(p)", (minic_ext_fn_raw_t)prefab_load);
minic_register("prefab_save", "i(i,p)", (minic_ext_fn_raw_t)prefab_save);
minic_register("prefab_instantiate", "i(i)", (minic_ext_fn_raw_t)prefab_instantiate);
```

### 預計工時
3-5 天

---

## 實施順序

```
Week 1: Phase 1 - Query API
Week 2: Phase 2 - System API 修復 + Phase 3 Input 系統
Week 3: Phase 4 - 補充 Minic 綁定
Week 4: Phase 5 - 動態組件改進 (可選)
```

---

## 驗收標準

完成後，應該能夠編寫完整的遊戲腳本:

```c
float main() {
    // 創建組件
    int pos_comp = component_register("Position", 12);
    component_add_field(pos_comp, "x", TYPE_FLOAT, 0);
    component_add_field(pos_comp, "y", TYPE_FLOAT, 4);
    component_add_field(pos_comp, "z", TYPE_FLOAT, 8);
    
    int vel_comp = component_register("Velocity", 12);
    // ... 添加字段
    
    int health_comp = component_register("Health", 8);
    component_add_field(health_comp, "current", TYPE_FLOAT, 0);
    component_add_field(health_comp, "max", TYPE_FLOAT, 4);
    
    // 創建實體
    int player = entity_create("Player");
    entity_set_name(player, "Player");
    entity_add(player, pos_comp);
    entity_add(player, vel_comp);
    entity_add(player, health_comp);
    
    // 初始化數據
    float init_pos[3] = {0.0, 1.0, 0.0};
    entity_set_data(player, pos_comp, init_pos);
    
    // 創建查詢
    int move_q = query_create("Position, Velocity");
    int health_q = query_create("Health, ?EntityActive");
    
    // 遊戲循環
    while (entity_is_valid(player)) {
        // 玩家輸入
        if (input_key_down("W")) {
            comp_set_float(vel_comp, vel_data, "z", 5.0);
        }
        
        // 移動系統
        while (query_next(move_q)) {
            int count = query_count(move_q);
            for (int i = 0; i < count; i++) {
                void *pos = query_get(move_q, i, "Position");
                void *vel = query_get(move_q, i, "Velocity");
                
                float vz = comp_get_float(vel_comp, vel, "z");
                float px = comp_get_float(pos_comp, pos, "x");
                float pz = comp_get_float(pos_comp, pos, "z");
                
                comp_set_float(pos_comp, pos, "x", px + 0.0 * sys_delta());
                comp_set_float(pos_comp, pos, "z", pz + vz * sys_delta());
            }
        }
        
        // 健康系統
        while (query_next(health_q)) {
            int count = query_count(health_q);
            for (int i = 0; i < count; i++) {
                void *health = query_get(health_q, i, "Health");
                float current = comp_get_float(health_comp, health, "current");
                if (current < 100.0) {
                    comp_set_float(health_comp, health, "current", current + 1.0 * sys_delta());
                }
            }
        }
    }
    
    query_destroy(move_q);
    query_destroy(health_q);
    
    return 0.0;
}
```

---

## 文件清單

### 需要修改的現有文件

| 文件 | 修改內容 |
|------|----------|
| `engine/sources/core/runtime_api.c` | 添加 Minic 綁定 |
| `engine/sources/core/system_api.c` | 修復 get_entity |
| `engine/sources/core/game_loop.c` | 添加 input_update 調用 |

### 需要實現的新文件

| 文件 | 內容 |
|------|------|
| `engine/sources/core/query_api.c` | Query API 實現 |
| `engine/sources/core/query_api.h` | Query 類型定義 |
| `engine/sources/core/input.c` | Input 系統實現 |

### 需要創建的頭文件

無

---

## 技術參考

- [Flecs Queries](https://www.flecs.dev/flecs/md_docs_2Queries.html) - Flecs 查詢文檔
- [Flecs Query API](https://github.com/SanderMertens/flecs/blob/master/include/flecs.h) - 查詢相關 API
- [Iron Input](https://github.com/armory3d/iron) - Iron 輸入系統參考
