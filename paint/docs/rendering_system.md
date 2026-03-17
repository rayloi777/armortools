# ArmorPaint 渲染系統分析

本文檔記錄 ArmorPaint 3D 繪畫引擎的渲染架構與運作流程。

---

## 1. 架構總覽

ArmorPaint 基於 Iron 3D 引擎，採用**延遲渲染 (Deferred Rendering)** 搭配 **多渲染目標 (MRT)** 架構，專為 3D PBR 材質繪畫設計。

---

## 2. 主渲染循環

```
render_path_paint_begin() → render_path_paint_draw() → render_path_paint_end()
```

主要入口點位於 `render_path_paint.c`，由 `render_path_forward_commands()` 調用：

```c
void render_path_forward_commands() {
    render_path_paint_live_brush_dirty();
    render_path_base_commands(render_path_forward_draw_forward);
}
```

---

## 3. 圖層系統

每個圖層 (`slot_layer_t`) 維護多個紋理：

| 紋理 | 用途 | 格式 |
|------|------|------|
| `texpaint` | 基礎顏色 + 透明度 | RGBA32/64/128 |
| `texpaint_nor` | 法線圖 + 材質 ID | RGBA32/64/128 |
| `texpaint_pack` | 遮蔽、粗糙度、金屬度、高度 | RGBA32/64/128 |
| `texpaint_preview` | 圖層縮圖預覽 | RGBA32 |
| `texpaint_sculpt` | 雕刻模式深度 | - |

### 圖層創建

```c
render_target_t *t = render_target_create();
t->name = string("texpaint%s", ext);
t->width = config_get_texture_res_x();
t->height = config_get_texture_res_y();
raw->texpaint = render_path_create_render_target(t)->_image;
```

---

## 4. 筆刷渲染管線

### 4.1 輸入處理

當用戶繪畫時，筆刷輸出節點接收：
- **Position**: 壓感筆 UV 座標
- **Radius**: 筆刷大小 (受壓力影響)
- **Scale**: 筆刷縮放因子
- **Angle**: 筆刷旋轉角度
- **Opacity**: 筆刷透明度 (受壓力影響)
- **Hardness**: 筆刷邊緣硬度
- **Stencil**: 模板紋理 (可選)

### 4.2 渲染流程

```
用戶輸入 (UV座標 + 壓力)
       ↓
brush_output_node_run() 處理輸入
       ↓
make_paint.c 動態生成著色器
       ↓
make_brush.c 計算筆刷形狀
       ↓
渲染到圖層紋理 (4個MRT附件)
       ↓
混合模式處理
```

### 4.3 筆刷形狀計算

```c
// 膠囊距離計算 (make_brush.c)
var h: float = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
dist = length(pa - ba * h);
if (dist > constants.brush_radius) { discard; }
```

### 4.4 深度剔除

防止繪製到背面：

```c
if (sp.z > sample_lod(gbufferD, sampler_linear, sp.xy, 0.0).r - 0.00008) { discard; }
```

---

## 5. 動態著色器生成

`make_paint.c` 在運行時動態生成筆刷著色器：

### 5.1 著色器上下文

```c
shader_context_t props = {
    .name = "paint",
    .depth_write = false,
    .compare_mode = "always",
    .cull_mode = "none",
    .color_attachments = make_paint_color_attachments()  // 4 MRT attachments
};
```

### 5.2 頂點著色器

投影到螢幕空間：

```c
node_shader_write_vert(kong, "var tpos: float2 = float2(input.tex.x * 2.0 - 1.0, (1.0 - input.tex.y) * 2.0 - 1.0);");
node_shader_write_vert(kong, "output.pos = float4(tpos, 0.0, 1.0);");
```

### 5.3 片段著色器輸出

4 個顏色附件：
- **output[0]**: 基礎顏色 (RGBA)
- **output[1]**: 法線/切線 + 材質 ID
- **output[2]**: 遮蔽、粗糙度、金屬度、高度
- **output[3]**: 筆觸遮罩 (R8)

### 5.4 混合模式

```c
node_shader_write_frag(kong, string("output[0] = float4(%s, max(str, sample_undo.a));",
    make_material_blend_mode(kong, context_raw->brush_blending, "sample_undo.rgb", "basecol", "opacity")));
```

---

## 6. 渲染目標

在 `render_path_paint_init()` 中創建特殊渲染目標：

| 目標 | 用途 |
|------|------|
| `texpaint_blend0` | 筆觸遮罩累加器 |
| `texpaint_blend1` | 臨時混合緩衝 |
| `texpaint_colorid` | 物件 ID 渲染 |
| `texpaint_picker` | 視口顏色拾取 |
| `texpaint_nor_picker` | 法線拾取 |
| `texpaint_pack_picker` | PBR 通道拾取 |
| `texpaint_posnortex_picker0/1` | 位置/法線拾取 |

---

## 7. 對稱繪畫

```c
void render_path_paint_commands_symmetry() {
    if (context_raw->sym_x || context_raw->sym_y || context_raw->sym_z) {
        // 鏡像網格變換並重繪
        t->scale = vec4_create(-sx, sy, sz, 1.0);  // X 軸鏡像
        transform_build_matrix(t);
        render_path_paint_commands_paint(false);
    }
}
```

---

## 8. 即時筆刷預覽

"Live Brush" 功能提供即時預覽：

1. 創建臨時 `_live` 圖層
2. 複製當前圖層狀態
3. 渲染筆刷操作
4. 透過 `render_path_paint_use_live_layer()` 交換圖層

---

## 9. 圖層合成

使用 GPU 管線 `pipes_merge` 和著色器 `layer_merge.kong` 合併圖層：

```c
_gpu_begin(l0->texpaint, NULL, NULL, GPU_CLEAR_NONE, 0, 0.0);
gpu_set_pipeline(pipes_merge);
gpu_set_texture(pipes_tex0, l1->texpaint);
gpu_set_texture(pipes_texmask, mask);
gpu_set_float(pipes_opac, slot_layer_get_opacity(l1));
gpu_set_int(pipes_blending, l1->blending);
gpu_draw();
```

### 支援的混合模式

- Normal: Mix
- Darken: Darken, Multiply, Burn
- Lighten: Lighten, Screen, Dodge
- Contrast: Overlay, Soft Light
- Arithmetic: Add
- Special: Blend ID 102 (法線混合), 103 (高度混合)

---

## 10. 後處理效果

`compositor_pass.kong`:

### 10.1 靜態顆粒 (Static Grain)

```c
var x: float = (input.tex.x + 4.0) * (input.tex.y + 4.0) * 10.0;
var g: float = (((x % 13.0) + 1.0) * ((x % 123.0) + 1.0) % 0.01) - 0.005;
color.rgb = color.rgb + (float3(g, g, g) * constants.grain_strength);
```

### 10.2 暗角 (Vignette)

```c
color.rgb = color.rgb * ((1.0 - constants.vignette_strength) + 
    constants.vignette_strength * pow(16.0 * input.tex.x * input.tex.y * (1.0 - input.tex.x) * (1.0 - input.tex.y), 0.2));
```

### 10.3 調色 (Tonemapping)

```c
color.rgb = tonemap_filmic(color.rgb);
```

### 其他效果

- **SSAO**: 螢幕空間環境遮蔽 (`ssao_pass.kong`)
- **Bloom**: HDR 光暈 (`bloom_downsample_pass.kong`, `bloom_upsample_pass.kong`)
- **TAA**: 時間性抗鋸齒

---

## 11. Undo 系統

Undo 系統儲存圖層的先前狀態：
- `texpaint_undo%d`: 圖層紋理的 Undo 狀態
- `texpaint_nor_undo%d`: 法線的 Undo 狀態
- `texpaint_pack_undo%d`: PBR 包的 Undo 狀態

繪畫時，先前狀態會被取樣並與新筆觸混合：

```c
node_shader_add_texture(kong, "texpaint_undo", "_texpaint_undo");
node_shader_write_frag(kong, "var sample_undo: float4 = sample_lod(texpaint_undo, sampler_linear, sample_tc, 0.0);");
```

---

## 12. 關鍵資料結構

### context_t

```c
typedef struct context {
    vec4_t           paint_vec;           // 當前繪畫位置
    f32              last_paint_x, last_paint_y;
    f32              brush_radius;        // 當前筆刷半徑
    f32              brush_opacity;       // 當前筆刷透明度
    f32              brush_hardness;      // 當前筆刷硬度
    blend_type_t     brush_blending;      // 當前混合模式
    tool_type_t      tool;                // 當前工具 (筆刷、橡皮擦等)
    slot_brush      *brush;               // 活躍筆刷
    slot_layer      *layer;               // 活躍圖層
    slot_material   *material;            // 活躍材質
} context_t;
```

### brush_output_node_t

```c
typedef struct brush_output_node {
    struct logic_node *base;
    struct ui_node    *raw;
} brush_output_node_t;
```

---

## 13. 渲染流程總結

```
1. 用戶輸入 (壓感筆 UV + 壓力)
       ↓
2. brush_output_node 處理輸入
       ↓
3. 動態生成筆刷著色器
       ↓
4. 渲染筆觸到圖層紋理
       ↓
5. 混合模式處理
       ↓
6. 對稱處理 (如啟用)
       ↓
7. UV 膨脹 (可選)
       ↓
8. 儲存 Undo 狀態
       ↓
9. 合成所有可見圖層
       ↓
10. 後處理 (顆粒、暗角、調色、SSAO、Bloom、TAA)
       ↓
11. 顯示到螢幕
```

---

## 14. 檔案位置參考

| 檔案 | 功能 |
|------|------|
| `render_path_paint.c` | 主繪畫渲染路徑 |
| `make_paint.c` | 動態生成筆刷著色器 |
| `make_brush.c` | 筆刷形狀計算 |
| `brush_output_node.c` | 筆刷節點處理 |
| `render_path_paint_commands.c` | 繪畫指令 |
| `layer_merge.kong` | 圖層合併著色器 |
| `compositor_pass.kong` | 後處理著色器 |
| `ssao_pass.kong` | SSAO 效果 |
| `bloom_*.kpng` | Bloom 效果 |

---

*本文檔由 AI 系統自動生成，分析版本基於 armortools 專案。*
