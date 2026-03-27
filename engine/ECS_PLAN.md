# ECS 功能完整化計劃

## 概述

將 engine/ 的 ECS 功能完成至 95%，專注於核心 ECS 功能，暫不包含渲染、Prefab 系統。

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
| **Game Loop** | ✅ 完成 | sys_delta/time/frame 正常 |
| **Iron Input in Minic** | ✅ 完成 | keyboard_down/started/released 已有 |
| **Minic 2階段初始化** | ✅ 完成 | minic_ctx_create/run/get_fn |
| **Minic System 框架** | ❌ 待實現 | 分離 system 文件 |
| **game.minic update()** | ❌ 待實現 | 主腳本協調 |
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
- `component_get_id()` / `component_get_name()` / `component_get_size()` - 獲取值件信息
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
    comp_set_float(comp_id, data, "x", 1.0f);
    comp_set_float(comp_id, data, "y", 2.0f);
    comp_set_float(comp_id, data, "z", 3.0f);
}

void Position_dtor(void* data, void* comp_id) {
    // 清理資源
}
```

### Game Loop
- `sys_delta()` - 獲取時間增量
- `sys_time()` - 獲取總時間
- `sys_frame()` - 獲取幀計數

### Iron Input API (Minic 中已有)
- `keyboard_down(key)` - 按住返回 1
- `keyboard_started(key)` - 剛按下返回 1
- `keyboard_released(key)` - 剛釋放返回 1
- `mouse_view_x()` - 鼠標 X 位置
- `mouse_view_y()` - 鼠標 Y 位置

### Minic 2階段初始化
- `minic_ctx_create()` - 創建 Minic 上下文（立即返回 ctx）
- `minic_ctx_run()` - 運行腳本
- `minic_ctx_get_fn()` - 從 C 調用 Minic 函數

---

## 實現計劃

### Phase 1: Minic System 框架 (核心)

**目標:** 實現分離的 System 文件和 update() 協調機制

**文件結構:**
```
engine/assets/scripts/
├── systems/
│   ├── movement_system.minic   → 有 update()
│   └── health_system.minic    → 有 update()
└── game.minic                 → 有 update() 協調調用
```

**實現內容:**

| 步驟 | 檔案 | 內容 |
|------|------|------|
| 1.1 | `minic_system.h` | System 加載/卸載 API 聲明 |
| 1.2 | `minic_system.c` | `load_system()`, `call_system_update()` |
| 1.3 | `runtime_api.c` | 註冊 System 包裝函數 |

**System 加載流程:**
```c
// 1. 載入 system 文件
load_system("movement", "systems/movement_system.minic");
//   - 讀取文件
//   - minic_ctx_create() + minic_ctx_run()
//   - 找到 "update" 函數指針
//   - 註冊為 "movement_update" 包裝函數

// 2. 主腳本調用
// game.minic::update() {
//     movement_update();
//     health_update();
// }
```

**遊戲循環:**
```
game_loop_update()
├── input_update()              ← Iron 自動每幀更新
├── ecs_progress()           ← C Systems
└── game.minic::update()     ← Minic update
    ├── movement_update()      ← 調用 movement_system.minic::update()
    └── health_update()       ← 調用 health_system.minic::update()
```

**Minic System 格式:**
```minic
// systems/movement_system.minic
void update() {
    float dt = sys_delta();
    
    if (keyboard_down("W")) {
        // 移動邏輯
    }
}
```

**預計工時:** 1-2 天

---

### Phase 2: Thread-Local Context (可選)

**目標:** 確保多線程安全

**當前狀態:** `sys_delta()`, `sys_time()`, `sys_frame()` 已有

**如需 TLS:**
```c
#if defined(_MSC_VER)
    #define TLS __declspec(thread)
#else
    #define TLS _Thread_local
#endif

static TLS float g_tls_delta_time = 0.0f;
float sys_delta(void) { return g_tls_delta_time; }
```

**預計工時:** 0.5 天（可跳過，如不需多線程）

---

### Phase 3: 重構遊戲腳本 (驗證)

**目標:** 使用新框架重構示例遊戲

**game.minic:**
```minic
void update() {
    movement_update();
    health_update();
}

float main() {
    return 0.0;
}
```

**systems/movement_system.minic:**
```minic
void update() {
    float dt = sys_delta();
    
    if (keyboard_down("W")) {
        // 移動
    }
}
```

**預計工時:** 0.5 天

---

### Phase 4: 多線程 ECS (可延後)

**目標:** 啟用 Flecs 多線程 System

```c
// system_api.c
desc.multi_threaded = true;  // 啟用 worker threads
```

**預計工時:** 0.5 天

---

## 完整實現順序

| Phase | 內容 | 工時 | 優先 |
|-------|------|------|------|
| **1** | Minic System 框架 | 1-2 天 | **核心** |
| 2 | Thread-Local Context | 0.5 天 | 可選 |
| 3 | 重構遊戲腳本 | 0.5 天 | 驗證 |
| 4 | 多線程 ECS | 0.5 天 | 可延後 |

**總計核心工作:** 1-2 天

---

## 驗收標準

完成後，應該能夠編寫分離的 Minic System:

```minic
// systems/movement_system.minic
void update() {
    float dt = sys_delta();
    
    if (keyboard_down("W")) {
        // 移動邏輯
    }
}
```

```minic
// game.minic
void update() {
    movement_update();
    health_update();
}

float main() {
    load_system("movement", "systems/movement_system.minic");
    load_system("health", "systems/health_system.minic");
    return 0.0;
}
```

---

## 文件清單

### 需要修改的現有文件

| 文件 | 修改內容 |
|------|----------|
| `engine/sources/core/runtime_api.c` | 添加 System 包裝函數註冊 |

### 需要實現的新文件

| 文件 | 內容 |
|------|------|
| `engine/sources/core/minic_system.h` | System 加載 API 聲明 |
| `engine/sources/core/minic_system.c` | System 加載實現 |

---

## 技術參考

- [Flecs Queries](https://www.flecs.dev/flecs/md_docs_2Queries.html) - Flecs 查詢文檔
- [Flecs Query API](https://github.com/SanderMertens/flecs/blob/master/include/flecs.h) - 查詢相關 API
- [Iron Input](https://github.com/armory3d/iron) - Iron 輸入系統參考

---

## Git 提交記錄

| Commit | 描述 |
|--------|------|
| `5ae01f55` | Update ECS_PLAN.md with lifecycle callbacks documentation |
| `c3de5dc1` | Complete ECS lifecycle callbacks and Query API thread safety |
| `ef4ccc8c` | Fix minic FFI for ECS component access |
| `b03311c1` | Complete Entity and Component API |
| `d6e65c2e` | Fix System API: system_get_entity and system_create |
| `045fca96` | Implement Query API for ECS |

---

## 實現細節

### Minic System 加載機制

```c
// minic_system.c
typedef struct {
    char name[64];
    minic_ctx_t *ctx;
    void *update_fn;  // "update" 函數指針
} minic_system_t;

static minic_system_t g_systems[16];
static int g_system_count = 0;

int load_system(const char *name, const char *path) {
    // 1. 讀取 .minic 文件
    char *source = read_file(path);
    
    // 2. 創建 Minic 上下文
    minic_ctx_t *ctx = minic_ctx_create(source);
    
    // 3. 運行腳本（只初始化，不執行遊戲循環）
    minic_ctx_run(ctx);
    
    // 4. 查找 "update" 函數
    void *fn = minic_ctx_get_fn(ctx, "update");
    
    // 5. 註冊包裝函數
    char wrapper_name[128];
    snprintf(wrapper_name, sizeof(wrapper_name), "%s_update", name);
    minic_register(wrapper_name, "v()", wrapper_fn);
    
    // 6. 保存
    strcpy(g_systems[g_system_count].name, name);
    g_systems[g_system_count].ctx = ctx;
    g_systems[g_system_count].update_fn = fn;
    g_system_count++;
    
    return 0;
}
```

### 調用流程

```c
// game_loop_update() 中
void game_loop_update() {
    // ... input_update, ecs_progress ...
    
    // 調用所有已加載的 Minic System
    for (int i = 0; i < g_system_count; i++) {
        if (g_systems[i].update_fn) {
            minic_call_fn(g_systems[i].update_fn, NULL, 0);
        }
    }
}
```

### Iron Input 機制

Iron 在每幀結尾自動調用 `input_end_frame()`:
```c
// iron_sys.c:470
input_end_frame();
```

`keyboard_started()` 只在按下那一幀返回 true，下一幀自動變回 false。
