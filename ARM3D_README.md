# .arm File Format

## 概述

`.arm` 是 ArmorTools 的二進制序列化格式，基於 msgpack 並擴展了 typed array 支援。用於 3D 場景和繪圖專案。

```
Blender ──────► io_export_arm.py ──────► .arm (Scene format)
ArmorPaint ───► export_arm.c ─────────► .arm (Project format)

.arm ───► armpack_decode() ───► scene_t / project_t ───► Iron scene graph ───► ECS entities
```

### 兩種格式

| 格式 | 來源 | 用途 |
|------|------|------|
| **Scene format** | Blender 匯出 | 3D 網格幾何 + 場景物件，用於引擎渲染 |
| **Project format** | ArmorPaint 匯出 | 完整專案資料：材質、圖層、筆刷、貼圖、網格 |

兩者使用相同的 armpack 二進制編碼，但結構不同。

---

## 用戶指南

### Blender 匯出（Scene Format）

1. **安裝Addon**
   將 `base/tools/io_export_arm.py` 安裝為 Blender addon：
   ```
   Blender → Edit → Preferences → Add-ons → Install → 選擇 io_export_arm.py
   ```

2. **匯出場景**
   ```
   File → Export → Armory (.arm)
   ```

3. **匯出選項**
   - Apply Modifiers — 套用修飾符
   - Include UVs — 匯出 UV 座標
   - Include Vertex Colors — 匯出頂點顏色
   - Include Tangents — 匯出 tangent（用於法線貼圖）

4. **輸出內容**
   - 網格幾何（頂點位置、法線、UV、頂點顏色）
   - 物件層級結構（transform hierarchy）
   - **不含** 材質、紋理

5. **引擎資產位置**
   匯出後放置於 `engine/assets/` 目錄，構建時自動複製到 `build/data/`。

---

### ArmorPaint 匯出（Project Format）

| 匯出類型 | 功能表路徑 | 副檔名 |
|----------|-----------|--------|
| 專案檔 | File → Save | `.arm` |
| 材質 | File → Export → Material | `.arm` |
| 筆刷 | File → Export → Brush | `.arm` |
| 色彩盤 | File → Export → Swatches | `.arm` |
| 網格 | File → Export → Mesh | `.arm` |

**貼圖打包選項：**
- `pack_assets_on_save` — 將紋理嵌入 .arm 檔案（LZ4 壓縮）
- 關閉時 — 紋理保持為獨立檔案

---

## 格式詳解

### Binary Encoding

- 基於 msgpack，擴展 typed array
- 類型標籤：

| Tag | Type |
|-----|------|
| `0xdf` | map |
| `0xdd` | array |
| `0xdb` | string |
| `0xca` | f32 |
| `0xd2` | i32 |
| `0xd1` | i16 |
| `0xc4` | u8 |
| `0xc0` | null |
| `0xc2` | false |
| `0xc3` | true |

- 頂點資料使用 `i16` 打包以節省空間
- 大型資料塊（>4096 bytes）單獨分配，小型內聯

**相關程式碼：**
- 編碼：`base/sources/iron_armpack.c` — `armpack_encode_*()`
- 解碼：`base/sources/iron_armpack.c` — `armpack_decode()`

---

### Scene Format（Blender 匯出）

#### 頂層 map 結構

| Key | Type | Description |
|-----|------|-------------|
| `name` | string | 場景名稱 |
| `objects` | array[map] | 場景物件層級 |
| `mesh_datas` | array[map] | 網格幾何資料 |

#### Object 結構

| Key | Type | Description |
|-----|------|-------------|
| `name` | string | 物件名稱 |
| `type` | string | `"mesh_object"` 或 `"camera_object"` |
| `data_ref` | string | 參照的 mesh_data 名稱 |
| `transform` | array[f32] | 4x4 列主序 transform 矩陣 |
| `dimensions` | array[f32] | AABB 半邊界 [x, y, z] |
| `visible` | bool | 可見性 |
| `spawn` | bool | 生成標記 |
| `children` | array[map] | 子物件 |

#### Mesh Data 結構

| Key | Type | Description |
|-----|------|-------------|
| `name` | string | 網格名稱 |
| `scale_pos` | f32 | 位置打包比例 |
| `scale_tex` | f32 | UV 打包比例 |
| `vertex_arrays` | array[map] | 頂點屬性陣列 |
| `index_array` | array[i32] | 三角形索引 |

#### Vertex Array 屬性

| attrib | data format | Description |
|--------|-------------|-------------|
| `pos` | short4norm | 位置：xyz + 打包的法線 z |
| `nor` | short2norm | 法線：xy |
| `tex` | short2norm | UV 座標 |
| `tex1` | short2norm | 次要 UV（可選） |
| `col` | short4norm | 頂點顏色 RGBA |
| `tang` | short4norm | Tangent（可選） |

**打包規則：**
- `pos` 的 xyz 乘以 `invscale_pos = (1/scale_pos) * 32767`，壓縮為 `i16`
- `tex` 的 uv 乘以 `invscale_tex = (1/scale_tex) * 32767`，壓縮為 `i16`
- 讀取時反向還原

---

### Project Format（ArmorPaint 匯出）

#### 頂層 map 結構（25+ 欄位）

| Key | Type | Description |
|-----|------|-------------|
| `version` | string | 格式版本 |
| `assets` | array[string] | 紋理檔案路徑 |
| `is_bgra` | bool | 像素格式標記 |
| `packed_assets` | array[map] | 內嵌紋理資料 |
| `envmap` | string | 環境圖路徑 |
| `envmap_strength` | f32 | 環境圖強度 |
| `envmap_angle` | i32 | 環境圖旋轉角度 |
| `envmap_blur` | bool | 環境圖模糊 |
| `camera_world` | array[f32] | 4x4 相機 Transform |
| `camera_origin` | array[f32] | 相機原點 [x, y, z] |
| `camera_fov` | f32 | 視野角度 |
| `swatches` | array[map] | 色彩盤資料 |
| `brush_nodes` | array[map] | 筆刷節點圖 |
| `brush_icons` | array[buffer] | 筆刷預覽圖 |
| `material_nodes` | array[map] | 材質節點圖 |
| `material_groups` | array[map] | 材質群組節點圖 |
| `material_icons` | array[buffer] | 材質預覽圖 |
| `font_assets` | array[string] | 字體檔案路徑 |
| `layer_datas` | array[map] | 繪圖圖層資料 |
| `mesh_datas` | array[map] | 網格幾何 |
| `mesh_assets` | array[string] | 網格檔案路徑 |
| `mesh_icons` | array[buffer] | 網格預覽圖 |
| `atlas_objects` | array[i32] | Atlas 物件索引 |
| `atlas_names` | array[string] | Atlas 物件名稱 |
| `script_datas` | array[string] | 腳本資料（已廢棄） |

#### Layer Data 結構（28 欄位）

| Key | Type | Description |
|-----|------|-------------|
| `name` | string | 圖層名稱 |
| `res` | i32 | 紋理解析度 |
| `bpp` | i32 | 每像素位元數（8/16/32） |
| `texpaint` | buffer | 繪圖紋理（LZ4 壓縮） |
| `uv_scale` | f32 | UV 縮放 |
| `uv_rot` | f32 | UV 旋轉 |
| `uv_type` | i32 | UV 映射類型 |
| `decal_mat` | array[f32] | Decal 投影矩陣 |
| `opacity_mask` | f32 | 透明度/遮罩值 |
| `fill_layer` | i32 | 填充材質索引 |
| `object_mask` | i32 | 物件選取遮罩 |
| `blending` | i32 | 混合模式 |
| `parent` | i32 | 父圖層索引 |
| `visible` | bool | 圖層可見性 |
| `texpaint_nor` | buffer | 法線貼圖紋理 |
| `texpaint_pack` | buffer | 打包的 ORM 紋理 |
| `paint_base` | bool | 繪製 base color 標記 |
| `paint_opac` | bool | 繪製 opacity 標記 |
| `paint_occ` | bool | 繪製 occlusion 標記 |
| `paint_rough` | bool | 繪製 roughness 標記 |
| `paint_met` | bool | 繪製 metallic 標記 |
| `paint_nor` | bool | 繪製 normal 標記 |
| `paint_nor_blend` | bool | 法線混合標記 |
| `paint_height` | bool | 繪製 height 標記 |
| `paint_height_blend` | bool | Height 混合標記 |
| `paint_emis` | bool | 繪製 emission 標記 |
| `paint_subs` | bool | 繪製 subsurface 標記 |
| `uv_map` | i32 | UV map 索引 |

---

## 引擎載入流程

### 載入路徑

```
.arm file on disk
  ↓ iron_file_load_blob()
  ↓ data_get_blob() — cached read
  ↓ armpack_decode() — binary → C structs
  ↓ data_get_scene_raw() — cached scene_t*
  ↓ scene_create() — initializes Iron scene graph
  ↓ asset_loader_load_scene() — syncs to ECS entities
```

### 關鍵函數

| Function | Location | Purpose |
|----------|----------|---------|
| `armpack_decode()` | `base/sources/iron_armpack.c` | Binary → typed C structs |
| `data_get_scene_raw()` | `base/sources/engine.c:1987` | Load + cache .arm scene |
| `scene_create()` | `base/sources/engine.c` | Build Iron scene graph |
| `asset_loader_load_scene()` | `engine/sources/core/asset_loader.c` | Sync to ECS entities |

### 匯出路徑（Export）

```
Blender: io_export_arm.py → packb() → .arm binary
ArmorPaint: export_arm.c → util_encode_scene()/util_encode_project() → armpack_encode_* → .arm binary
```

### 全域狀態

| Variable | Type | Set by |
|----------|------|--------|
| `_scene_root` | `object_t*` | `scene_create()` |
| `scene_meshes` | `any_array_t*` | `scene_create()` |
| `scene_cameras` | `any_array_t*` | `scene_create()` |
| `scene_camera` | `camera_object_t*` | `scene_create()` |
| `data_cached_scene_raws` | `any_map_t*` | `data_get_scene_raw()` |

---

## 開發者參考

### 檔案清單

| File | Role |
|------|------|
| `base/tools/io_export_arm.py` | Blender addon exporter |
| `paint/sources/export_arm.c` | ArmorPaint exporter |
| `paint/sources/util_encode.c` | Scene/project encoding |
| `base/sources/iron_armpack.c` | Binary encoder/decoder |
| `base/sources/iron_armpack.h` | Decoder API header |
| `base/sources/engine.c` | Scene loading, `data_get_scene_raw()` |
| `engine/sources/core/asset_loader.c` | ECS sync |

### 建置整合

- `.arm` 檔案放置於 `engine/assets/` 會在 `../base/make macos metal` 時自動複製到 `build/data/`
- ArmorPaint 資產建置：`cd paint && ../base/make`

### Debug Tools

使用 `armpack_decode_to_json()` 可將二進制 .arm 轉換為 JSON 方便除錯：

```c
#include "iron_armpack.h"
buffer_t *b = iron_load_blob("path/to/file.arm");
char *json = armpack_decode_to_json(b);
printf("%s\n", json);
```
