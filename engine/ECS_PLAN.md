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
| **Entity API** | ✅ 完成 | CRUD 操作正常，含父子關係 |
| **Component API** | ✅ 完成 | field count/info/alignment 全部完成 |
| **System API** | ✅ 完成 | minic_system_get_entity() 正常工作 |
| **Query API** | ✅ 完成 | 支持 AND, OR, NOT 查詢，線程安全 |
| **Lifecycle Callbacks** | ✅ 完成 | ctor/dtor 回調完整實現 |
| **Game Loop** | ✅ 完成 | delta/time/frame 正常 |
| **Minic 綁定** | ✅ 完成 | 全部 ~25 個函數已完成 |
| **Input 系統** | ❌ 未實現 | 完全缺失 |
| **Prefab 系統** | ❌ 未實現 | 可延後 |

**完成度: 90%**

---

## 已完成的功能

### Entity API
- `entity_create()` / `entity_create_named()` - 創建實體
- `entity_destroy()` - 銷毀實體
- `entity_add()` / `entity_remove()` - 添加/移除組件
- `entity_has()` - 檢查是否有組件
- `entity_get()` / `entity_set_data()` - 獲取/設置組件數據
- `entity_is_valid()` - 檢查實體是否有效
- `entity_exists()` - 檢查實體是否存在
- `entity_get_name()` / `entity_set_name()` - 獲取/設置名稱
- `entity_get_parent()` / `entity_set_parent()` - 父子關係

### Component API
- `component_register()` - 註冊動態組件
- `component_add_field()` - 添加字段
- `component_get_id()` / `component_get_name()` / `component_get_size()` - 獲取組件信息
- `component_get_field_count()` / `component_get_field_info()` - 獲取字段信息
- `component_get_alignment()` - 獲取對齊
- `component_set_hooks()` - 設置生命週期回調
- `comp_set/get_int/float/ptr` - 字段讀寫

### System API
- `system_create()` - 創建系統
- `system_destroy()` / `system_enable()` - 銷毀/啟用系統
- `system_get_entity()` - 獲取當前系統匹配的實體
- `system_get_entity_count()` - 獲取匹配的實體數量

### Query API
- `query_create(filter_expr)` - 創建查詢
- `query_destroy(query_id)` - 銷毀查詢
- `query_next(query_id)` - 迭代下一批
- `query_count(query_id)` - 獲取實體數量
- `query_get(query_id, index)` - 獲取實體 ID

### Lifecycle Callbacks (生命週期回調)
- `component_set_hooks("ComponentName", "CtorName", "DtorName")` - 設置回調函數
- **ctor 回調**: 組件添加時自動調用，用於初始化
- **dtor 回調**: 組件移除或實體銷毀時調用，用於清理

**回調函數簽名:**
```c
void Position_ctor(void* data, void* comp_id) {
    comp_set_float(comp_id, data, "x", 1.0f);  // 設置初始值
    comp_set_float(comp_id, data, "y", 2.0f);
    comp_set_float(comp_id, data, "z", 3.0f);
}

void Position_dtor(void* data, void* comp_id) {
    // 清理資源
}
```

**使用方式:**
```c
int pos_comp = component_register("Position", 12);
component_add_field(pos_comp, "x", TYPE_FLOAT, 0);
component_add_field(pos_comp, "y", TYPE_FLOAT, 4);
component_add_field(pos_comp, "z", TYPE_FLOAT, 8);
component_set_hooks("Position", "Position_ctor", "Position_dtor");

int e1 = entity_create();
entity_add(e1, pos_comp);  // 自動調用 Position_ctor
entity_remove(e1, pos_comp);  // 自動調用 Position_dtor
```

### Game Loop
- `sys_delta()` - 獲取時間增量
- `sys_time()` - 獲取總時間
- `sys_frame()` - 獲取幀計數

### Minic API 新增功能
- `minic_ctx_create()` - 創建 Minic 上下文（2階段初始化）
- `minic_ctx_run()` - 運行腳本
- `minic_ctx_get_fn()` - 從 C 調用 Minic 函數

---

## Phase 1: Input 系統 (待實現)

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

### 整合遊戲循環

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

### 預計工時
2-3 天

---

## Phase 2: Prefab 系統 (可延後)

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
Phase 1: Input 系統 (2-3天)
Phase 2: Prefab 系統 (3-5天，可延後)
```

---

## 驗收標準

完成後，應該能夠編寫完整的遊戲腳本:

```c
// 組件回調示例
void Position_ctor(void* data, void* comp_id) {
    comp_set_float(comp_id, data, "x", 0.0f);
    comp_set_float(comp_id, data, "y", 0.0f);
    comp_set_float(comp_id, data, "z", 0.0f);
}

void Health_ctor(void* data, void* comp_id) {
    comp_set_float(comp_id, data, "current", 100.0f);
    comp_set_float(comp_id, data, "max", 100.0f);
}

float main() {
    // 創建組件
    int pos_comp = component_register("Position", 12);
    component_add_field(pos_comp, "x", TYPE_FLOAT, 0);
    component_add_field(pos_comp, "y", TYPE_FLOAT, 4);
    component_add_field(pos_comp, "z", TYPE_FLOAT, 8);
    component_set_hooks("Position", "Position_ctor", NULL);
    
    int health_comp = component_register("Health", 8);
    component_add_field(health_comp, "current", TYPE_FLOAT, 0);
    component_add_field(health_comp, "max", TYPE_FLOAT, 4);
    component_set_hooks("Health", "Health_ctor", NULL);
    
    // 創建實體
    int player = entity_create();
    entity_add(player, pos_comp);
    entity_add(player, health_comp);
    
    // 遊戲循環
    while (entity_is_valid(player)) {
        // 移動系統
        int move_q = query_create("Position");
        while (query_next(move_q)) {
            int count = query_count(move_q);
            for (int i = 0; i < count; i++) {
                int e = query_get(move_q, i);
                void *pos = entity_get(e, pos_comp);
                
                float x = comp_get_float(pos_comp, pos, "x");
                comp_set_float(pos_comp, pos, "x", x + 1.0 * sys_delta());
            }
        }
        query_destroy(move_q);
    }
    
    return 0.0;
}
```

---

## 文件清單

### 需要修改的現有文件

| 文件 | 修改內容 |
|------|----------|
| `engine/sources/core/runtime_api.c` | 已添加 Input 綁定 |
| `engine/sources/core/game_loop.c` | 添加 input_update 調用 |
| `base/sources/libs/minic.c` | 添加 2階段初始化 API |
| `base/sources/libs/minic.h` | 添加 minic_ctx_create/run/get_fn |

### 需要實現的新文件

| 文件 | 內容 |
|------|------|
| `engine/sources/core/input.c` | Input 系統實現 |

---

## 技術參考

- [Flecs Queries](https://www.flecs.dev/flecs/md_docs_2Queries.html) - Flecs 查詢文檔
- [Flecs Query API](https://github.com/SanderMertens/flecs/blob/master/include/flecs.h) - 查詢相關 API
- [Iron Input](https://github.com/armory3d/iron) - Iron 輸入系統參考

---

## Git 提交記錄

| Commit | 描述 |
|--------|------|
| `c3de5dc1` | Complete ECS lifecycle callbacks and Query API thread safety |
| `ef4ccc8c` | Fix minic FFI for ECS component access |
| `b03311c1` | Complete Entity and Component API |
| `d6e65c2e` | Fix System API: system_get_entity and system_create |
| `045fca96` | Implement Query API for ECS |

---

## 實現細節

### Minic 2階段初始化

為了解決生命週期回調需要在腳本運行時訪問 Minic 函數的問題，引入了 2 階段初始化:

```c
// 舊方式（一次性）
minic_ctx_t *ctx = minic_eval(script);  // ctx 在腳本運行完後才返回
// 問題：回調在腳本運行期間觸發，但 ctx 尚未返回

// 新方式（2階段）
minic_ctx_t *ctx = minic_ctx_create(script);  // 立即返回 ctx
minic_ctx_run(ctx);  // 運行腳本，回調可以訪問 ctx
```

### Query API 線程安全

每個查詢有自己的 `cached_it` 迭代器，避免多線程環境下的狀態競爭:

```c
typedef struct {
    uint64_t flecs_query;
    char filter[256];
    bool valid;
    bool iter_started;
    int last_count;
    uint64_t last_entities[256];
    void *cached_it;  // 每個查詢獨立存儲
} runtime_query_t;
```

### 回調函數查找

通過 `minic_ctx_get_fn()` 在運行時查找 Minic 函數:

```c
void *fn = minic_ctx_get_fn(ctx, "Position_ctor");
if (fn) {
    minic_val_t args[2];
    args[0].p = data;
    args[1].p = (void*)comp_id;
    minic_call_fn(fn, args, 2);
}
```
