# Engine 3D Render Pipeline — 重構計劃

**Date:** 2026-04-12
**Status:** Draft
**Approach:** Unity-style multi-layer culling + full deferred rendering

---

## 1. 架構總覽

### Layer 架構

```
┌─────────────────────────────────────────────────────┐
│  Minic Scripts (.minic)                              │
│  遊戲邏輯腳本 — 創建實體、添加組件、查詢系統          │
└──────────────────────────────┬──────────────────────┘
                               ↓ calls C API registered via minic_register()
┌─────────────────────────────────────────────────────┐
│  Core API (core/)                                    │
│  entity_api · light_api · camera_api · culling_api   │
│  render_api                                          │
└──────────────────────────────┬──────────────────────┘
                               ↓ operates on
┌─────────────────────────────────────────────────────┐
│  Flecs ECS (ecs/)                                    │
│  Systems: culling · shadow · gbuffer · lighting      │
│  Components: position, rotation, scale, mesh, light   │
└──────────────────────────────┬──────────────────────┘
                               ↓ syncs to
┌─────────────────────────────────────────────────────┐
│  Iron Engine (base/)                                 │
│  GPU render targets · render_path · mesh_object      │
└─────────────────────────────────────────────────────┘
```

### ECS World — 所有 3D 組件

```c
// Transform
comp_3d_position     — float x, y, z
comp_3d_rotation     — float x, y, z, w (quaternion)
comp_3d_scale        — float x, y, z

// Mesh
comp_3d_mesh_renderer — mesh_path, material_path
RenderObject3D        — iron_mesh_object ptr, iron_transform ptr
comp_3d_lod           — distances[4], mesh_lod0/1/2/3 paths, current_lod

// Camera
comp_3d_camera        — fov, near, far, perspective
comp_3d_ortho         — left, right, bottom, top

// Lights
comp_3d_directional_light — dir(xyz), color(rgb), strength, enabled, cast_shadows, shadow_bias, normal_offset_bias, shadow_radius
comp_3d_point_light       — pos(xyz), color(rgb), strength, range, enabled, cast_shadows, shadow_bias
comp_3d_spot_light        — pos(xyz), dir(xyz), color(rgb), strength, range, inner_cone, outer_cone, enabled, cast_shadows, shadow_bias

// Rendering Tags
Tag: CompRenderable      — 可被渲染的實體
Tag: CompShadowCaster    — 投射陰影
Tag: CompShadowReceiver  — 接收陰影
Tag: CompVisible         — 當前幀可見（culling 結果）
```

### ECS System 執行順序

```
1.  sys_culling_frustum    // GPU frustum culling → writes CompVisible
2.  sys_culling_lod        // LOD selection → updates comp_3d_lod.current_lod
3.  sys_shadow_directional  // CSM × 4 directional lights
4.  sys_shadow_point        // Cubemap shadows × 8 point lights
5.  sys_shadow_spot         // Shadow maps × 4 spot lights
6.  sys_gbuffer            // Geometry pass → gbuffer0 + gbuffer1 + main(depth)
7.  sys_lighting           // Deferred lighting → buf
8.  sys_postfx             // Post-processing → SSAO → Bloom → TAA → last
9.  sys_output             // 2D overlay + present
```

---

## 2. Render Pipeline（5 Pass）

```
┌──────────────────────────────────────────────────────────────────┐
│  Pass 0: Shadow Maps                                             │
│  Directional × 4 × 3 cascades  →  shadow_0/1/2 (2048/1024/512) │
│  Point × 8 × 6 cubemap faces  →  shadow_cube_[0-7] (1024²×6)    │
│  Spot × 4                      →  shadow_spot_[0-3] (1024²)     │
└──────────────────────────────────────────────────────────────────┘
                               ↓
┌──────────────────────────────────────────────────────────────────┐
│  Pass 1: G-Buffer Geometry                                        │
│  world_gbuffer.kong (MRT)                                         │
│  → gbuffer0: Normal.xy(oct) + Roughness + Metallic              │
│  → gbuffer1: Albedo.rgb + AO                                     │
│  → main: Depth (D24S8)                                           │
└──────────────────────────────────────────────────────────────────┘
                               ↓
┌──────────────────────────────────────────────────────────────────┐
│  Pass 2: Deferred Lighting                                       │
│  deferred_light.kong — reads gbuffer0/1 + depth + all shadow maps │
│  → buf: Lit color                                                │
└──────────────────────────────────────────────────────────────────┘
                               ↓
┌──────────────────────────────────────────────────────────────────┐
│  Pass 3: Post-Processing                                          │
│  SSAO → Bloom_down → Bloom_up → TAA → last                      │
└──────────────────────────────────────────────────────────────────┘
                               ↓
┌──────────────────────────────────────────────────────────────────┐
│  Pass 4: 2D Overlay                                               │
│  render2d_bridge sprite batch → screen                            │
└──────────────────────────────────────────────────────────────────┘
```

---

## 3. G-Buffer 格式

| Target    | Format    | Size   | 內容                                      |
|-----------|-----------|--------|-------------------------------------------|
| main      | D24S8     | W×H    | Depth buffer（所有 Pass 共享）             |
| gbuffer0  | RGBA16F   | W×H    | Normal.xy (octahedron), Roughness, Metallic |
| gbuffer1  | RGBA16F   | W×H    | Albedo.rgb, AO                           |
| buf       | RGBA16F   | W×H    | Intermediate / lighting output            |
| last      | RGBA16F   | W×H    | Final composited output (post-fx result)  |

### G-Buffer Channel 分配

**gbuffer0 (RGBA16F):**

| Channel | 內容            | 預設值 |
|---------|----------------|--------|
| R       | Normal.x (oct) | —      |
| G       | Normal.y (oct) | —      |
| B       | Roughness      | 0.5    |
| A       | Metallic       | 0.0    |

**gbuffer1 (RGBA16F):**

| Channel | 內容    | 預設值 |
|---------|---------|--------|
| R       | Albedo.r| 0.8    |
| G       | Albedo.g| 0.8    |
| B       | Albedo.b| 0.8    |
| A       | AO      | 1.0    |

### Octahedron Normal Encoding

```glsl
// Encode
float2 oct_encode(float3 n) {
    n = normalize(n);
    float2 oct = n.xy * (1.0 / (abs(n.x) + abs(n.y) + abs(n.z)));
    oct = oct * 0.5 + 0.5;
    return oct;
}

// Decode
float3 oct_decode(float2 oct) {
    oct = oct * 2.0 - 1.0;
    float3 n;
    n.z = 1.0 - abs(oct.x) - abs(oct.y);
    if (n.z >= 0.0) { n.x = oct.x; n.y = oct.y; }
    else { float2 f2 = float2(1.0 - abs(oct.y), 1.0 - abs(oct.x)) * sign(oct); n.x = f2.x; n.y = f2.y; }
    return normalize(n);
}
```

---

## 4. Cascaded Shadow Maps (CSM)

### 參數

| 參數         | 值                      |
|-------------|------------------------|
| Cascade 數量 | 3                      |
| 分割策略     | 對數分割（Logarithmic）  |
| Cascade 0   | 2048×2048, near → ~4.6m |
| Cascade 1   | 1024×1024, ~4.6m → ~21.5m |
| Cascade 2   | 512×512,   ~21.5m → 100m |
| Filtering   | 4-tap PCF              |
| Max Directional Lights | 4         |

### Logarithmic Split 計算

```
split_i(near, far, i, N) = near * (far / near)^(i / N)

Cascade 0: near → near * (far/near)^(1/3)   →  0.1m → ~4.6m
Cascade 1: above → near * (far/near)^(2/3) →  ~4.6m → ~21.5m
Cascade 2: above → far                       → ~21.5m → 100m
```

### Light Space VP 計算

```c
// Directional light view matrix
mat4 light_view(float3 light_dir, float3 scene_center) {
    float3 up = fabs(light_dir.y) < 0.999 ? float3(0,1,0) : float3(1,0,0);
    float3 neg_dir = -normalize(light_dir);
    return look_at(scene_center, scene_center + neg_dir, up);
}

// Orthographic projection for each cascade
mat4 light_proj(float extent, float cascade_near, float cascade_far) {
    return ortho(-extent, extent, -extent, extent, cascade_near, cascade_far);
}

// Final: SMVP = light_proj * light_view * model_matrix
```

### Single-Pass CSM（Viewport Scissors）

Shadow map atlas 切成 2×2 grid，一次 draw call 輸出 3 個 cascade：

```
┌─────────────────┬─────────────────┐
│  Cascade 0      │  Cascade 1      │
│  2048×2048      │  1024×1024      │
├─────────────────┼─────────────────┤
│  Cascade 2      │                 │
│  512×512        │    free         │
└─────────────────┴─────────────────┘
```

設置 viewport scissors 綁定每個 cascade 區域，用 geometry shader 或 MRT 一次性輸出。

### Cascade 選擇 + 邊界混合

```glsl
float get_cascade_index(float view_depth, float4 cascade_splits) {
    if (view_depth < cascade_splits.x) return 0.0;
    if (view_depth < cascade_splits.y) return 1.0;
    return 2.0;
}

// 邊界 smooth blend 減少接縫
float get_cascade_blend(float view_depth, float4 cascade_splits, float transition = 2.0) {
    float idx = get_cascade_index(view_depth, cascade_splits);
    float split = (idx < 1.0) ? cascade_splits.x : (idx < 2.0) ? cascade_splits.y : cascade_splits.z;
    return smoothstep(split - transition, split, view_depth);
}
```

### 4-tap PCF

```glsl
float shadow_pcf(sampler2D shadow_map, float4 shadow_coord, float bias, float2 shadow_map_size) {
    vec2 texel_size = 1.0 / shadow_map_size;
    float shadow = 0.0;
    shadow += texture(shadow_map, shadow_coord.xy + vec2(-texel_size.x, -texel_size.y)).r;
    shadow += texture(shadow_map, shadow_coord.xy + vec2( texel_size.x, -texel_size.y)).r;
    shadow += texture(shadow_map, shadow_coord.xy + vec2(-texel_size.x,  texel_size.y)).r;
    shadow += texture(shadow_map, shadow_coord.xy + vec2( texel_size.x,  texel_size.y)).r;
    shadow /= 4.0;
    return shadow < shadow_coord.z - bias ? 0.0 : 1.0;
}
```

---

## 5. 燈光系統

### 燈光數量限制

| 類型        | 數量 | 陰影方法              | Shadow Map                    |
|------------|------|---------------------|-------------------------------|
| Directional | 4    | CSM (3 cascades)    | shadow_0/1/2 (2048/1024/512)  |
| Point       | 8    | Shadow Cubemap      | shadow_cube_[0-7] (1024²×6)   |
| Spot        | 4    | Perspective SM      | shadow_spot_[0-3] (1024²)     |

### 組件定義

```c
typedef struct {
    float dir_x, dir_y, dir_z;   // 光照方向（歸一化）
    float color_r, color_g, color_b;
    float strength;
    bool  enabled;
    bool  cast_shadows;
    float shadow_bias;
    float normal_offset_bias;
    float shadow_radius;          // PCF kernel 大小
} comp_3d_directional_light;

typedef struct {
    float pos_x, pos_y, pos_z;   // 世界座標
    float color_r, color_g, color_b;
    float strength;
    float range;                  // 衰減半徑
    bool  enabled;
    bool  cast_shadows;
    float shadow_bias;
} comp_3d_point_light;

typedef struct {
    float pos_x, pos_y, pos_z;   // 世界座標
    float dir_x, dir_y, dir_z;   // 照射方向
    float color_r, color_g, color_b;
    float strength;
    float range;                  // 衰減半徑
    float inner_cone;             // 內圓錐角（rad）
    float outer_cone;             // 外圓錐角（rad）
    bool  enabled;
    bool  cast_shadows;
    float shadow_bias;
} comp_3d_spot_light;
```

### 衰減公式

**Point Light:**
```
att = 1.0 - saturate(dist / range)
att² = att * att  // 二次衰減更物理精確
```

**Spot Light:**
```
spotEffect = dot(L, -spotDir)
angleAtt = smoothstep(outer_cone, inner_cone, spotEffect)
totalAtt = (1 - dist/range)² × angleAtt
```

### Point Light Shadow Cubemap

每個 point light 需要渲染 6 次到 cubemap 的每個面：
- 6 faces: +X, -X, +Y, -Y, +Z, -Z
- 使用 perspective projection (90° FOV) 匹配立方體每面
- 渲染到 `shadow_cube_[light_index]` 的各個 layer

### Tiled / Clustered Lighting

Deferred lighting pass 前，用 ComputeShader 把屏幕分成 16×16 tiles：

```glsl
// Compute shader: 建立 tile light list
// 每個 tile 只遍歷影響它的燈光（平均 2-4 個）
// 結果寫入 light_index_buffer，供 lighting pass 使用
```

省 50-80% 燈光遍歷計算。

### Lighting Shader Uniforms

```c
struct LightData {
    // Directional[4]
    float4 dir[4];              // xyz = direction, w = enabled
    float4 dirColor[4];
    float4 dirSMVP[4 * 3];     // 4 lights × 3 cascades
    float4 dirCascadeSplit[4];

    // Point[8]
    float4 pointPos[8];
    float4 pointColor[8];
    float  pointRange[8];
    bool   pointShadow[8];

    // Spot[4]
    float4 spotPos[4];
    float4 spotDir[4];
    float4 spotColor[4];
    float  spotRange[4];
    float  spotInnerCone[4];
    float  spotOuterCone[4];
    bool   spotShadow[4];
};
```

### PBR 環境光（Ambient / Environment Lighting）

G-buffer 只儲存 material properties（normal, albedo, roughness, metallic, ao），環境光需要另外處理。

#### 三層環境光

| 層次 | 內容 | 計算方式 | 成本 |
|------|------|---------|------|
| Ambient | 簡單常數或天空顏色 | `albedo × ambient × ao` | 極低 |
| SH Irradiance | 球諧函數預計算的間接光 | `albedo × SH_probe(pos, N)` | 低 |
| IBL Reflection | Environment map 反射 | `env_map.sample(reflection_vec, roughness)` | 中 |

#### Phase 1：Unity 風格預設天空（已實現）

```glsl
// Unity 風格藍天灰地
float3 GetSkyAmbient(float3 N, float3 albedo, float ao) {
    float t = N.y * 0.5 + 0.5;  // -1~1 → 0~1
    float3 sky_color = float3(0.5, 0.7, 1.0);    // 藍天
    float3 ground_color = float3(0.2, 0.2, 0.2); // 灰色地
    float3 sky_ambient = lerp(ground_color, sky_color, t);
    return sky_ambient * albedo * ao * ambient_strength;
}
```

#### Phase 2+：SH Irradiance

球諧函數用 27 個 floats（9 coefficients × 3 channels）儲存 irradiance：

```glsl
struct SHIrradiance {
    float3 L00, L1m1, L10, L11;   // Band 0 + 1
    float3 L2m2, L2m1, L20, L21, L22; // Band 2
};

float3 SampleSH(SHIrradiance sh, float3 N) {
    return sh.L00
         + sh.L1m1 * (N.y) + sh.L10 * (N.z) + sh.L11 * (N.x)
         + sh.L20 * (N.y*N.z) + sh.L21 * (N.y*N.x) + sh.L22 * (N.z*N.x)
         + sh.L2m2 * (N.x*N.y) + sh.L2m1 * (N.z*N.z - 1.0/3.0);
}
```

#### Phase 3+：IBL Reflection

Prefiltered environment cubemap（每個 mip level 對應一個 roughness）：

```glsl
// Specular IBL
float3 R = reflect(-V, N);
float mip = roughness * (num_mips - 1);
float3 specular = env_cubemap.SampleLevel(R, mip);

// PBR BRDF
float3 F0 = lerp(float3(0.04), albedo, metallic);
float3 F = FresnelSchlickRoughness(NdotV, F0, roughness);
float3 kD = (1 - F) * (1 - metallic);
float3 ambient = kD * diffuse_sh + specular * F;
```

#### SH Light Probe（Baked GI，Phase 2）

Light Probe 解決物件之間的全局光照（light bounce）。Editor 中放置 probes，烘焙 SH irradiance 係數。

**原理：** 每個 probe 記錄周圍環境的 irradiance，而不是 reflection。

**Probe 放置：**

| 場景類型 | 放置策略 |
|---------|---------|
| 室外 | 每 10m 放置一個，覆蓋整個場景 |
| 室內 | 每個房間 4-8 個，角落和光源附近更密集 |

**Editor 烘焙流程：**

```c
// 1. 在每個 probe 位置發射 rays，採樣周圍環境
// 2. 對採樣結果做 SH projection，得到 9 個係數
// 3. 儲存到 comp_environment_probe.sh

struct SHIrradiance {
    float3 L00, L1m1, L10, L11;   // Band 0+1
    float3 L2m2, L2m1, L20, L21, L22; // Band 2
};
```

**運行時使用：**

```glsl
// 根據 world position 找最近的 probe
comp_environment_probe* probe = find_nearest_probe(world_pos);

// 計算該方向的 irradiance
float3 indirect_light = SampleSH(probe->sh, normal);

// 應用於 PBR
float3 diffuse_indirect = albedo * indirect_light * ao;
color = direct_lighting + diffuse_indirect;
```

**多 Probe 混合：**

```c
// 找最近的 2-4 個 probes，根據 distance 混合
float w0 = 1.0 - smoothstep(0, blend_radius, dist0);
float w1 = 1.0 - smoothstep(0, blend_radius, dist1);
float3 final_sh = (sh0 * w0 + sh1 * w1) / (w0 + w1);
```

**與 Reflection Probe 的區別：**

| | SH Irradiance Probe | Reflection Probe |
|--|---------------------|------------------|
| 用途 | 間接光照（diffuse bounce） | 反射（specular reflection） |
| 採樣方向 | 法線 N | 反射向量 R |
| 內容 | 周圍物體反彈的光 | 環境的反射 |

#### Reflection Probe（Phase 4）

多個位置放置 probe，每個有自己的 SH + prefiltered env cubemap：

```c
typedef struct {
    float3  position;
    float   blend_radius;
    SHIrradiance sh;
    bool    has_envmap;
} comp_environment_probe;
```

#### Component

```c
// 全域天空設定
typedef struct {
    float3 sky_color;       // float3(0.5, 0.7, 1.0) 藍天
    float3 ground_color;    // float3(0.2, 0.2, 0.2) 灰色地
    float  ambient_strength; // 環境光強度
} comp_sky_settings;
```

---

## 6. Culling 系統

### 執行順序

```
1. sys_culling_frustum   // GPU Frustum CS → CompVisible tag
2. sys_culling_lod       // CPU/GPU LOD selection
3. sys_culling_occlusion  // Baked + Hi-Z occlusion
4. DrawProceduralIndirect // 只對 visible 物件 draw
```

### GPU Frustum Culling (ComputeShader)

所有物件的 AABB 與 camera frustum 6 planes 做測試：

```glsl
// Object Data Buffer
struct ObjectBounds {
    float3 center;    // AABB center (world space)
    float3 extents;   // AABB half-extents
    uint   lodMask;
};

// Frustum Plane
struct Plane { float3 n; float d; }; // dot(n,p) + d = 0

// AABB vs Plane outside test
bool aabb_outside_frustum(ObjectBounds obj, Plane planes[6]) {
    for (int i = 0; i < 6; i++) {
        float d = dot(obj.center, planes[i].n) + planes[i].d;
        if (d < -length(obj.extents)) return true; // 完全在外側
    }
    return false;
}

// SV_DispatchThreadID 並行處理所有物件
```

輸出：visible object list buffer (uint array)

### LOD System

```c
typedef struct {
    float distances[4];         // LOD0→LOD1, LOD1→LOD2, LOD2→LOD3 閾值
    char *mesh_lod0_path;
    char *mesh_lod1_path;
    char *mesh_lod2_path;
    char *mesh_lod3_path;
    int   current_lod;          // 運行時選擇的 LOD level
} comp_3d_lod;

int select_lod(float dist, float4 distances) {
    if (dist > distances.w) return 3;
    if (dist > distances.z) return 2;
    if (dist > distances.y) return 1;
    return 0;
}
```

### Baked Occlusion Culling (Cell Grid)

Editor 時預計算每個 cell 的可見物件列表：

```c
struct OcclusionCell {
    float3 min, max;                    // cell 邊界
    uint   visibleObjects[MAX_VIS];      // 預計算可見列表
    uint   numVisible;
};

// Runtime: 根據 camera 位置找 cell，直接用預計算列表
OcclusionCell* cell = get_cell_at(cameraPos);
for (i = 0; i < cell.numVisible; i++)
    add_to_visible_list(cell.visibleObjects[i]);
```

### Hi-Z Occlusion Culling

利用 depth buffer mipmap 做層次化深度檢測：

```glsl
// Build depth mipmap
depth_mip_0 = main.depth (full res)
depth_mip_N = quarter(depth_mip_{N-1})

// GPU CS: 對 visible list 中的大物件做 Hi-Z 查詢
uint hiz_level = log2(bounds_size_in_pixels / 4.0);
float hi_z_depth = sample(depth_mip[hiz_level], bounds_center_uv);
if (bounds_min_z > hi_z_depth) cull; // 被遮擋
```

### DrawProceduralIndirect

GPU-driven draw call：culling 結果寫入 indirect args buffer，GPU 直接讀取 draw。

```glsl
struct DrawIndexedIndirectArgs {
    uint  indexCount;
    uint  instanceCount;
    uint  startIndex;
    int   baseVertex;
    uint  startInstance;
};
```

---

## 7. 透明物件處理（Dual-Render）

### 問題

Deferred Rendering 的 G-buffer 無法做 alpha blending。透明物件需要走獨立的 Forward Pass。

### Pipeline（含透明物件）

```
Shadow → G-Buffer → Lighting → [Transparent Forward] → Post-FX → 2D
```

### Transparent Forward Pass

渲染順序：**back-to-front**（Order-Dependent）

```c
render_path_set_target("buf", NULL, NULL, GPU_CLEAR_NONE, 0, 0.0f);
// depth_test = LESS, depth_write = false
// blending = ALPHA_BLEND (SRC_ALPHA, ONE_MINUS_SRC_ALPHA)

for each transparent_mesh (sorted back-to-front):
    render_path_submit_draw("mesh_transparent")
```

### 透明類型

| 類型 | 特性 | 渲染方式 |
|------|------|---------|
| Alpha Fade | 簡單淡入淡出 | Standard alpha blend |
| Glass / Ice | 折射 + 反射 | Env map 或 screen-space refraction |
| Smoke / Particle | 體積透明 | Additive / Soft blend |

### HDR 的重要性

- **LDR (RGBA8)**：像素值 [0, 1]，bloom threshold 1.0 → 粒子被 clamp，**無 Bloom**
- **HDR (RGBA16F)**：可超過 1.0 → 粒子 Additive 寫入 HDR buffer，bloom 自動偵測高亮度

Engine G-buffer 已是 HDR（`buf` = RGBA16F），粒子 Additive blend 天然支持 Bloom。

### Component

```c
Tag: CompTransparent  // 有此 tag 的物件走 transparent forward pass

typedef struct {
    float alpha;
    bool  cast_shadows;
} comp_transparency;

Tag: CompParticle           // 粒子物件
Tag: CompGlowingParticle    // 發光粒子（影響 bloom）

typedef struct {
    float  alpha;              // 透明度
    float  emissive_strength;  // 發光強度（需 > bloom_threshold 才生效）
    float3 color;              // 粒子顏色
    int    blend_mode;         // 0=Additive, 1=Alpha, 2=SoftAdd, 3=Multiply
} comp_particle;
```

### Post-FX 執行順序

```
SSAO → Lighting → Particles(Additive) → Bloom → TAA → Compositor
```

### Order-Independent Transparency (OIT)

Standard transparency 必須 back-to-front 排序。OIT 可任意順序：

| 方法 | 原理 | 缺點 |
|------|------|------|
| Depth Peeling | 多層 peel，每層剝離一個透明層 | 4+ passes，昂貴 |
| Linked Lists (A-Buffer) | Fragment 寫 linked list，最後 resolve | 需要 atomic counter |
| Weighted Blended | 用 depth 作為權重，近似準確 | Phase 3 做 |

### 實施順序

- Phase 1：不作透明，專注 deferred opaque rendering
- Phase 2：加入 `CompTransparent` tag + basic alpha fade forward pass
- Phase 3：加入 Weighted Blended OIT

---

## 8. 效能優化

| 優化                  | 節省              | 難度 | 優先級 |
|---------------------|-----------------|------|--------|
| Early-Z + Depth Pre-pass | 30-60% frag shader | 低   | P0     |
| G-Buffer 降精度      | 50% mem bandwidth | 低   | P0     |
| Single-Pass CSM     | 2/3 shadow draws  | 中   | P0     |
| DrawProceduralIndirect | CPU draw overhead | 中   | P0     |
| Tiled Lighting       | 50-80% lighting   | 高   | P1     |
| Hi-Z Occlusion      | 省遮擋物件 draw    | 高   | P1     |
| Variable Rate Shading | 周邊像素著色降低   | 中   | P2     |

### Early-Z + Depth Pre-pass

```c
// Pass 0.5: Depth pre-pass
render_path_set_target("main", NULL, NULL, GPU_CLEAR_DEPTH, 0, 1.0f);
render_path_submit_draw("mesh_depth_only");  // 只寫深度，不算 color

// Pass 1: G-buffer (depth_test = LESS)
// → Early-Z 硬體自動 reject 被遮擋的 fragment
```

### G-Buffer 降精度

```
原本: gbuffer0 = RGBA64 (4×16bit), gbuffer1 = RGBA64, depth = D32
優化: gbuffer0 = RGBA16F (4×16bit), gbuffer1 = RGBA16F, depth = D24S8
→ 省 25-50% 頻寬，精度足夠 PBR
```

---

## 9. Minic API

遊戲腳本透過 C API 控制 3D 系統：

```c
// === 燈光 API ===
id entity_create_light_directional(float dir_x, float dir_y, float dir_z,
                                   float color_r, float color_g, float color_b,
                                   float strength);
id entity_create_light_point(float pos_x, float pos_y, float pos_z,
                             float color_r, float color_g, float color_b,
                             float range, float strength);
id entity_create_light_spot(float pos_x, float pos_y, float pos_z,
                             float dir_x, float dir_y, float dir_z,
                             float color_r, float color_g, float color_b,
                             float range, float inner_cone, float outer_cone);
void light_set_shadow(id light_id, bool cast_shadows, float bias);
void light_set_enabled(id light_id, bool enabled);
void light_set_color(id light_id, float r, float g, float b);

// === 攝影機 API ===
id entity_create_camera3d(float fov, float near, float far);
void camera3d_set_ortho(id camera_id, float left, float right, float bottom, float top);
void camera3d_set_active(id camera_id);  // 成為主攝影機

// === Culling API ===
void culling_set_frustum_enabled(bool enabled);
void culling_set_lod_enabled(bool enabled);
void culling_set_occlusion_enabled(bool enabled);

// === Render API ===
void render_set_pipeline(const char* pipeline);  // "forward", "deferred"
void render_set_shadow_quality(const char* quality);  // "low", "medium", "high"
```

所有 API 透過 `minic_register()` 註冊，遊戲腳本直接調用。

---

## 10. 實施階段

### Phase 1: 基礎建設
- G-Buffer 渲染（gbuffer0, gbuffer1, main depth）
- Directional Light 基本 lighting（無陰影）
- Basic camera + mesh rendering
- 基礎 Frustum Culling（CPU 版本）

### Phase 2: 完整陰影
- CSM for Directional Light（3 cascades, Single-Pass）
- Point Light + Cubemap Shadow
- Spot Light + Perspective Shadow Map
- 完整 3 種燈光 + 陰影

### Phase 3: 高級渲染
- Post-Processing（SSAO, Bloom, TAA）
- Tiled / Clustered Lighting
- Hi-Z Occlusion Culling
- LOD System
- 透明物件 Transparent Forward Pass
- 粒子系統 + Additive Glow + Bloom

### Phase 4: 優化整合
- Early-Z + Depth Pre-pass
- G-Buffer 降精度（RGBA16F + D24S8）
- DrawProceduralIndirect
- Variable Rate Shading

---

## 11. 文件清單

### 新建檔案

| 檔案 | 用途 |
|------|------|
| `engine/sources/ecs/deferred/deferred_gbuffer.c/h` | G-buffer 管理 |
| `engine/sources/ecs/deferred/deferred_lighting.c/h` | Lighting pass |
| `engine/sources/ecs/deferred/deferred_postfx.c/h` | Post-processing |
| `engine/sources/ecs/shadow/shadow_directional.c/h` | CSM shadow pass |
| `engine/sources/ecs/shadow/shadow_point.c/h` | Point cubemap shadow |
| `engine/sources/ecs/shadow/shadow_spot.c/h` | Spot shadow map |
| `engine/sources/ecs/culling/culling_frustum.c/h` | GPU frustum culling CS |
| `engine/sources/ecs/culling/culling_lod.c/h` | LOD system |
| `engine/sources/ecs/culling/culling_occlusion.c/h` | Baked occlusion + Hi-Z |
| `engine/sources/components/light.c/h` | Point/Spot light components |
| `engine/sources/components/lod.c/h` | LOD component |
| `engine/sources/components/transparency.c/h` | Transparent + Particle components |
| `engine/sources/ecs/transparent_bridge.c/h` | Transparent forward pass |
| `engine/sources/ecs/particle_system.c/h` | Particle system |
| `engine/sources/core/light_api.c/h` | Minic light API |
| `engine/sources/core/camera_api.c/h` | Minic camera API |
| `engine/sources/core/culling_api.c/h` | Minic culling API |
| `engine/sources/core/render_api.c/h` | Minic render API |
| `engine/assets/shaders/world_gbuffer.kong` | Geometry pass shader (MRT) |
| `engine/assets/shaders/deferred_light.kong` | Lighting pass shader |
| `engine/assets/shaders/shadow_directional.kong` | CSM shadow shader |
| `engine/assets/shaders/shadow_cubemap.kong` | Point shadow shader |
| `engine/assets/shaders/shadow_spot.kong` | Spot shadow shader |
| `engine/assets/shaders/culling_frustum.csh` | Frustum cull compute shader |
| `engine/assets/shaders/postfx_ssao.kong` | SSAO pass |
| `engine/assets/shaders/postfx_bloom.kong` | Bloom pass |
| `engine/assets/shaders/postfx_taa.kong` | TAA pass |
| `engine/assets/shaders/postfx_compositor.kong` | Compositor |
| `engine/assets/shaders/transparent_forward.kong` | Transparent forward pass |
| `engine/assets/shaders/particle_additive.kong` | Particle additive shader |

### 修改檔案

| 檔案 | 變更 |
|------|------|
| `engine/sources/ecs/render3d_bridge.c/h` | 重寫為 deferred pipeline 主控制器 |
| `engine/sources/ecs/camera_bridge_3d.c/h` | 支援 camera frustum planes 導出 |
| `engine/sources/ecs/mesh_bridge_3d.c/h` | 支援 LOD mesh loading |
| `engine/sources/ecs/light_bridge.c/h` | 支援 Point/Spot light sync |
| `engine/sources/ecs/ecs_components.c/h` | 註冊所有新組件 |
| `engine/sources/ecs/ecs_world.c/h` | 添加新 system registrations |
| `engine/sources/core/runtime_api.c/h` | 註冊所有 Minic API |
| `engine/assets/systems.manifest` | 添加各 ECS system |
| `engine/sources/core/game_engine.c` | 初始化順序更新 |
