# Spec: ARM3D_README.md

## Overview

Document the `.arm` file format used throughout ArmorTools — its purpose, how it is generated from both Blender and ArmorPaint, and how the engine loads and parses it. The document serves both end users (artists) and developers.

## File Location

`ARM3D_README.md` at project root.

## Format Summary

`.arm` is a binary serialization format based on msgpack with typed array extensions. It has two distinct variants:

| Variant | Source | Primary Use |
|---------|--------|-------------|
| **Scene format** | Blender via `io_export_arm.py` | 3D mesh geometry + scene objects for engine rendering |
| **Project format** | ArmorPaint via `export_arm.c` | Full project data: materials, paint layers, brushes, textures, meshes |

Both use the same armpack binary encoding but contain different data structures.

## Section 1: 概述

- Brief description: binary msgpack-based asset format for 3D scenes and paint projects
- Two use cases side-by-side
- ASCII diagram showing format flow:
  - Blender → io_export_arm.py → .arm (scene)
  - ArmorPaint → export_arm.c → .arm (project)
  - Engine → armpack_decode() → scene_t / project_t

## Section 2: 用戶指南

### 2.1 Blender 匯出 (Scene Format)

- Install `base/tools/io_export_arm.py` as Blender addon
- Export via File → Export → Armory (.arm)
- Key options: apply modifiers, include UVs, include vertex colors
- Output: mesh geometry + object hierarchy (no materials/textures)
- File location for engine assets

### 2.2 ArmorPaint 匯出 (Project Format)

- `.arm` project file (File → Save)
- Material export (File → Export → Material)
- Brush export (File → Export → Brush)
- Swatches export (File → Export → Swatches)
- Mesh export (File → Export → Mesh)
- Packing options: pack assets on save vs separate files

## Section 3: 格式詳解

### 3.1 Binary Encoding

- Based on msgpack with typed array extensions
- Custom encoder/decoder: `armpack_encode_*` / `armpack_decode()` in `base/sources/iron_armpack.c`
- Type tags: 0xdf=map, 0xdd=array, 0xdb=string, 0xca=f32, 0xd2=i32, 0xd1=i16, 0xc4=u8
- Vertex arrays stored as typed i16/f32 for space efficiency

### 3.2 Scene Format (Blender-exported)

**Top-level map keys:**

| Key | Type | Description |
|-----|------|-------------|
| `name` | string | Scene name |
| `objects` | array[map] | Scene object hierarchy |
| `mesh_datas` | array[map] | Mesh geometry data |

**Object map keys:**

| Key | Type | Description |
|-----|------|-------------|
| `name` | string | Object struct name |
| `type` | string | "mesh_object" or "camera_object" |
| `data_ref` | string | Reference to mesh_data name |
| `transform` | array[f32] | 4x4 column-major transform matrix |
| `dimensions` | array[f32] | AABB half-extents [x,y,z] |
| `visible` | bool | Visibility flag |
| `spawn` | bool | Spawn flag |
| `children` | array[map] | Child objects |

**Mesh data map keys:**

| Key | Type | Description |
|-----|------|-------------|
| `name` | string | Mesh name |
| `scale_pos` | f32 | Position packing scale |
| `scale_tex` | f32 | UV packing scale |
| `vertex_arrays` | array[map] | Vertex attribute arrays |
| `index_array` | array[i32] | Triangle indices |

**Vertex array attribute types:**

| attrib | data format | Description |
|--------|-------------|-------------|
| `pos` | short4norm | Position: xyz + packed normal z |
| `nor` | short2norm | Normal: xy |
| `tex` | short2norm | UV coordinates |
| `tex1` | short2norm | Secondary UV (optional) |
| `col` | short4norm | Vertex color RGBA |
| `tang` | short4norm | Tangent (optional) |

**Packing:** Positions and UVs are scaled to i16 range [-32767, 32767] using `scale_pos`/`scale_tex` factors to preserve precision.

### 3.3 Project Format (ArmorPaint-exported)

**Top-level map keys (25+ fields):**

| Key | Type | Description |
|-----|------|-------------|
| `version` | string | Format version |
| `assets` | array[string] | Texture file paths |
| `is_bgra` | bool | Pixel format flag |
| `packed_assets` | array[map] | Embedded texture data |
| `envmap` | string | Environment map path |
| `envmap_strength` | f32 | Environment map strength |
| `envmap_angle` | i32 | Environment map rotation |
| `envmap_blur` | bool | Environment blur flag |
| `camera_world` | array[f32] | 4x4 camera transform |
| `camera_origin` | array[f32] | Camera origin [x,y,z] |
| `camera_fov` | f32 | Field of view |
| `swatches` | array[map] | Color swatches |
| `brush_nodes` | array[map] | Brush node graphs |
| `brush_icons` | array[buffer] | Brush preview icons |
| `material_nodes` | array[map] | Material node graphs |
| `material_groups` | array[map] | Material group node graphs |
| `material_icons` | array[buffer] | Material preview icons |
| `font_assets` | array[string] | Font file paths |
| `layer_datas` | array[map] | Paint layer data |
| `mesh_datas` | array[map] | Mesh geometry |
| `mesh_assets` | array[string] | Mesh file paths |
| `mesh_icons` | array[buffer] | Mesh preview icons |
| `atlas_objects` | array[i32] | Atlas object indices |
| `atlas_names` | array[string] | Atlas object names |
| `script_datas` | array[string] | Script data (deprecated) |

**Layer data map keys (28 fields):**

| Key | Type | Description |
|-----|------|-------------|
| `name` | string | Layer name |
| `res` | i32 | Texture resolution |
| `bpp` | i32 | Bits per pixel (8/16/32) |
| `texpaint` | buffer | Paint texture (LZ4 compressed) |
| `uv_scale` | f32 | UV scale |
| `uv_rot` | f32 | UV rotation |
| `uv_type` | i32 | UV mapping type |
| `decal_mat` | array[f32] | Decal projection matrix |
| `opacity_mask` | f32 | Opacity/mask value |
| `fill_layer` | i32 | Fill material index |
| `object_mask` | i32 | Object selection mask |
| `blending` | i32 | Blend mode |
| `parent` | i32 | Parent layer index |
| `visible` | bool | Layer visibility |
| `texpaint_nor` | buffer | Normal map texture |
| `texpaint_pack` | buffer | Packed ORM texture |
| `paint_base` | bool | Paint base color flag |
| `paint_opac` | bool | Paint opacity flag |
| `paint_occ` | bool | Paint occlusion flag |
| `paint_rough` | bool | Paint roughness flag |
| `paint_met` | bool | Paint metallic flag |
| `paint_nor` | bool | Paint normal flag |
| `paint_nor_blend` | bool | Normal blend flag |
| `paint_height` | bool | Paint height flag |
| `paint_height_blend` | bool | Height blend flag |
| `paint_emis` | bool | Paint emission flag |
| `paint_subs` | bool | Paint subsurface flag |
| `uv_map` | i32 | UV map index |

## Section 4: 引擎載入流程

### Loading Path

```
.arm file on disk
  ↓ iron_file_load_blob()
  ↓ data_get_blob() — cached read
  ↓ armpack_decode() — binary → C structs
  ↓ data_get_scene_raw() — cached scene_t*
  ↓ scene_create() — initializes Iron scene graph
  ↓ asset_loader_load_scene() — syncs to ECS entities
```

### Key Functions

| Function | Location | Purpose |
|----------|----------|---------|
| `armpack_decode()` | `base/sources/iron_armpack.c` | Binary → typed C structs |
| `data_get_scene_raw()` | `base/sources/engine.c:1987` | Load + cache .arm scene |
| `scene_create()` | `base/sources/engine.c` | Build Iron scene graph |
| `asset_loader_load_scene()` | `engine/sources/core/asset_loader.c` | Sync to ECS entities |

### Encoding Path (Export)

```
Blender: io_export_arm.py → packb() → .arm binary
ArmorPaint: export_arm.c → util_encode_scene()/util_encode_project() → armpack_encode_* → .arm binary
```

### Global State

| Variable | Type | Set by |
|----------|------|--------|
| `_scene_root` | `object_t*` | `scene_create()` |
| `scene_meshes` | `any_array_t*` | `scene_create()` |
| `scene_cameras` | `any_array_t*` | `scene_create()` |
| `scene_camera` | `camera_object_t*` | `scene_create()` |
| `data_cached_scene_raws` | `any_map_t*` | `data_get_scene_raw()` |

## Section 5: 開發者參考

### File Inventory

| File | Role |
|------|------|
| `base/tools/io_export_arm.py` | Blender addon exporter |
| `paint/sources/export_arm.c` | ArmorPaint exporter |
| `paint/sources/util_encode.c` | Scene/project encoding |
| `base/sources/iron_armpack.c` | Binary encoder/decoder |
| `base/sources/iron_armpack.h` | Decoder API header |
| `base/sources/engine.c` | Scene loading, `data_get_scene_raw()` |
| `engine/sources/core/asset_loader.c` | ECS sync |

### Build Integration

- `.arm` files placed in `engine/assets/` are exported to `build/data/` by `../base/make macos metal`
- Project assets exported to `paint/build/data/` by `../base/make`
