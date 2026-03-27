# 2D 青蛙測試計劃 (Plan A)

## 目標

驗證 ECS 系統 + 渲染 2D 圖片 - 1000 隻青蛙在畫面反彈。

## 資源

- `frog_idle.png` - 透明背景青蛙 PNG（amake 自動轉換為 .k 格式）

## 架構

```
┌─────────────────────────────────────────────────────────────┐
│  frog_system.minic                                         │
│                                                             │
│  init():                                                   │
│      frog_tex = sprite_load("frog_idle.k")               │
│      創建1000個實體，每個帶Transform2D, Velocity2D         │
│                                                             │
│  step():                                                   │
│      遍歷查詢，更新位置 + 邊界反彈                         │
│      draw_begin(NULL, true, 0x1a1a2e)                      │
│      for each frog: sprite_draw(frog_tex, x, y, w, h)     │
│      draw_end()                                            │
└─────────────────────────────────────────────────────────────┘
```

## API 設計

| C 函數 | Minic 函數 | 簽名 |
|--------|------------|------|
| `data_get_image` | `sprite_load` | `(name) -> tex` |
| `draw_scaled_image` | `sprite_draw` | `(tex, x, y, w, h)` |
| `draw_begin` | `draw_begin` | `(target, clear, color)` |
| `draw_end` | `draw_end` | `()` |

## 實現步驟

### Phase 1: 添加 2D 繪圖 API

**1.1 修改 `engine/sources/core/runtime_api.c`**

添加 wrapper 函數：

```c
// 加載 2D 貼圖
static minic_val_t minic_sprite_load(minic_val_t *args, int argc) {
    if (argc < 1) return minic_val_void();
    const char *name = (const char*)args[0].p;
    gpu_texture_t *tex = data_get_image((char*)name);
    return minic_val_ptr(tex);
}

// 繪製縮放後的貼圖
static minic_val_t minic_sprite_draw(minic_val_t *args, int argc) {
    if (argc < 5) return minic_val_void();
    void *tex = (void*)args[0].p;
    float x = (float)minic_val_to_d(args[1]);
    float y = (float)minic_val_to_d(args[2]);
    float w = (float)minic_val_to_d(args[3]);
    float h = (float)minic_val_to_d(args[4]);
    draw_scaled_image(tex, x, y, w, h);
    return minic_val_void();
}

// 開始 2D 繪製
static minic_val_t minic_draw_begin(minic_val_t *args, int argc) {
    void *target = (argc > 0 && args[0].p) ? args[0].p : NULL;
    bool clear = (argc < 2) ? true : (bool)minic_val_to_d(args[1]);
    int color = (argc < 3) ? 0 : (int)minic_val_to_d(args[2]);
    draw_begin(target, clear, color);
    return minic_val_void();
}

// 結束 2D 繪製
static minic_val_t minic_draw_end(minic_val_t *args, int argc) {
    (void)argc; (void)args;
    draw_end();
    return minic_val_void();
}
```

**1.2 在 `runtime_api_register()` 註冊**

```c
minic_register_native("sprite_load", minic_sprite_load);
minic_register_native("sprite_draw", minic_sprite_draw);
minic_register_native("draw_begin", minic_draw_begin);
minic_register_native("draw_end", minic_draw_end);
```

### Phase 2: 重寫 frog_system.minic

```c
int g_tex = -1;
int g_pos_comp = -1;
int g_vel_comp = -1;
int g_query = -1;

int init() {
    printf("FrogSystem: Init\n");
    
    g_tex = sprite_load("frog_idle.k");
    if (g_tex == 0) {
        printf("FrogSystem: Failed to load frog_idle.k\n");
        return -1;
    }
    
    g_pos_comp = component_lookup("Transform2D");
    g_vel_comp = component_lookup("Velocity2D");
    
    if (g_pos_comp < 0 || g_vel_comp < 0) {
        printf("FrogSystem: Components not found\n");
        return -1;
    }
    
    g_query = query_new();
    query_with(g_query, g_pos_comp);
    query_with(g_query, g_vel_comp);
    query_find(g_query);
    
    // 創建1000個青蛙
    int i = 0;
    while (i < 1000) {
        int e = entity_create();
        entity_add(e, g_pos_comp);
        entity_add(e, g_vel_comp);
        
        void *pos = entity_get(e, g_pos_comp);
        void *vel = entity_get(e, g_vel_comp);
        
        // 隨機位置
        comp_set_float(g_pos_comp, pos, "x", rand_float() * 1280.0);
        comp_set_float(g_pos_comp, pos, "y", rand_float() * 720.0);
        
        // 隨機速度 (-200 到 200)
        float vx = (rand_float() - 0.5) * 400.0;
        float vy = (rand_float() - 0.5) * 400.0;
        comp_set_float(g_vel_comp, vel, "vx", vx);
        comp_set_float(g_vel_comp, vel, "vy", vy);
        
        i = i + 1;
    }
    
    printf("FrogSystem: Created 1000 frogs\n");
    return 0;
}

int step() {
    float dt = sys_delta();
    
    int count = query_find(g_query);
    int i = 0;
    
    // 更新位置 + 邊界反彈
    while (i < count) {
        int e = query_get(g_query, i);
        void *pos = entity_get(e, g_pos_comp);
        void *vel = entity_get(e, g_vel_comp);
        
        float px = comp_get_float(g_pos_comp, pos, "x");
        float py = comp_get_float(g_pos_comp, pos, "y");
        float vx = comp_get_float(g_vel_comp, vel, "vx");
        float vy = comp_get_float(g_vel_comp, vel, "vy");
        
        px = px + vx * dt;
        py = py + vy * dt;
        
        // X 邊界反彈
        if (px < 0.0) { px = 0.0; vx = -vx; }
        if (px > 1280.0) { px = 1280.0; vx = -vx; }
        
        // Y 邊界反彈
        if (py < 0.0) { py = 0.0; vy = -vy; }
        if (py > 720.0) { py = 720.0; vy = -vy; }
        
        comp_set_float(g_pos_comp, pos, "x", px);
        comp_set_float(g_pos_comp, pos, "y", py);
        comp_set_float(g_vel_comp, vel, "vx", vx);
        comp_set_float(g_vel_comp, vel, "vy", vy);
        
        i = i + 1;
    }
    
    // 繪製
    draw_begin(0, 1, 0x1a1a2e);
    
    i = 0;
    while (i < count) {
        int e = query_get(g_query, i);
        void *pos = entity_get(e, g_pos_comp);
        
        float px = comp_get_float(g_pos_comp, pos, "x");
        float py = comp_get_float(g_pos_comp, pos, "y");
        
        sprite_draw(g_tex, px, py, 64.0, 64.0);
        i = i + 1;
    }
    
    draw_end();
    return 0;
}
```

### Phase 3: 資源配置

**3.1 確保 `project.js` 包含 PNG 資源**

```js
project.add_assets("assets/*.png", {destination: "data/{name}"});
```

## 關鍵文件

| 文件 | 操作 |
|------|------|
| `engine/sources/core/runtime_api.c` | 修改 - 添加 sprite_load, sprite_draw, draw_begin, draw_end |
| `engine/assets/systems/frog_system.minic` | 重寫 - 使用新 API |
| `engine/assets/frog_idle.png` | 添加（用戶提供） |

## 查詢 API 說明

現有的 `movement_system.minic` 使用：
- `query_new()` - 創建新查詢
- `query_with(q, comp_id)` - 添加組件條件
- `query_find(q)` - 執行查詢，返回匹配實體數
- `query_get(q, index)` - 獲取第 index 個匹配的實體

## 構建和測試

```bash
# 1. 確保 frog_idle.png 在 engine/assets/

# 2. 構建 base
cd base && ./make macos metal

# 3. 構建 engine
cd engine && ../base/make macos metal

# 4. 運行測試
# 應該看到 1000 隻青蛙在屏幕內隨機移動並反彈
```

## 驗證清單

- [ ] `runtime_api.c` 添加了 4 個新 API
- [ ] `frog_system.minic` 使用 `sprite_load/draw_begin/sprite_draw/draw_end`
- [ ] `project.js` 配置了 PNG 資源
- [ ] 構建成功
- [ ] 運行時看到 1000 隻青蛙在屏幕內移動/反彈
