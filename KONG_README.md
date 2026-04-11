# KONG Shader Compiler

KONG (also called **minikong**) is a shader compiler that transpiles `.kong` shaders into platform-specific formats for various graphics APIs. It is based on [Kongruent](https://github.com/Kode/Kongruent) by RobDangerous.

## Overview

KONG takes a custom shader language (`.kong` files) and compiles them to:

| Backend | Output | File Extension |
|---------|--------|----------------|
| SPIR-V | Binary SPIR-V for Vulkan | `.vert.spirv`, `.frag.spirv` |
| Metal | Metal Shading Language for macOS/iOS | `.vert.metal`, `.frag.metal` |
| HLSL | Direct3D shader bytecode | `.vert.d3d11`, `.frag.d3d11` |
| WGSL | WebGPU Shading Language | `.vert.wgsl`, `.frag.wgsl` |
| C-style | Debug/inspection output | `.c`, `.h` |

The compiler works in three stages:
1. **Tokenize & Parse** - Converts `.kong` source to an AST
2. **IR Generation** - Transforms AST into an opcode IR
3. **Backend Emission** - Generates target language output

## Build System Integration

In `project.js` files, shaders are registered via:

```js
project.add_shaders("shaders/*.kong");
```

The build system (`make.js`) has a `ShaderCompiler` class that automatically compiles all `.kong` files to the appropriate backend format based on the target graphics API:

```bash
# From base directory - automatic platform detection
./make

# Specify platform and graphics backend explicitly
./make macos metal      # macOS with Metal backend
./make linux vulkan     # Linux with Vulkan backend
./make windows direct3d12  # Windows with Direct3D12
```

The compiled shaders are output to `build/temp/` with platform-specific extensions.

## Language Syntax

### Structs

```kong
struct vert_in {
    pos: float3;
    uv: float2;
}

struct vert_out {
    pos: float4;
    uv: float2;
}
```

### Functions

```kong
fun test_vert(input: vert_in): vert_out {
    var output: vert_out;
    output.pos = float4(input.pos, 1.0);
    output.uv = input.uv;
    return output;
}

fun test_frag(input: vert_out): float4 {
    return float4(1.0, 0.0, 0.0, 1.0);
}
```

### Variables

```kong
var counter: int;
var position: float3;
var color: float4;
```

### Attributes

Attributes bind resources and define pipeline structures:

```kong
// Bind a sampler to a descriptor set
#[set(sampler_name)]
var my_sampler: sampler;

// Bind a texture to a descriptor set
#[set(texture_name)]
var my_texture: tex2d;

// Define a render pipeline
#[pipe]
struct pipe {
    vertex = test_vert;
    fragment = test_frag;
}
```

**`#[set(name)]`** - Binds a resource (sampler, texture, constant buffer) to a descriptor set. All `#[set]` declarations with the same name share the same descriptor set, allowing efficient bundling of related resources:

```kong
#[set(everything)]
const constants: {
    SMVP: float4x4;
    envmap_data: float4;
};

#[set(everything)]
const sampler_linear: sampler;

#[set(everything)]
const envmap: tex2d;
```

**`#[pipe]`** - Marks a render pipeline definition that groups vertex and fragment shaders into a single pipeline object.

## Built-in Types

### Scalar Types

| Type | Description |
|------|-------------|
| `float` | 32-bit floating point |
| `int` | 32-bit signed integer |
| `uint` | 32-bit unsigned integer |
| `bool` | Boolean |

### Vector Types

| Type | Description |
|------|-------------|
| `float2` | 2D float vector |
| `float3` | 3D float vector |
| `float4` | 4D float vector |
| `int2` | 2D integer vector |
| `int3` | 3D integer vector |
| `int4` | 4D integer vector |

### Matrix Types

| Type | Description |
|------|-------------|
| `float2x2` | 2x2 float matrix |
| `float3x3` | 3x3 float matrix |
| `float4x4` | 4x4 float matrix |

### Texture and Sampler Types

| Type | Description |
|------|-------------|
| `sampler` | Sampler state |
| `tex2d` | 2D texture |
| `ray` | Ray for raytracing |
| `bvh` | Bounding Volume Hierarchy |

## Built-in Functions

### Texture Sampling

```kong
sample(tex: tex2d, sampler: sampler, uv: float2) -> float4
sample_lod(tex: tex2d, sampler: sampler, uv: float2, lod: float) -> float4
```

The `sample_lod` function is used in all paint shaders for explicit LOD control, enabling efficient multi-level sampling:

```kong
// Sample at specific mip level
var color: float4 = sample_lod(gbuffer0, sampler_linear, input.tex, 0.0);

// Manual mip level selection with interpolation
var lod: float = mip_from_roughness(roughness, 5.0);
var lod0: float = floor(lod);
var lod1: float = ceil(lod);
var lodf: float = lod - lod0;
var lodc0: float3 = envmap_sample(lod0, envmap_coord);
var lodc1: float3 = envmap_sample(lod1, envmap_coord);
var prefiltered_color: float3 = lerp3(lodc0, lodc1, lodf);
```

### Vector Component-wise Operations

KONG provides component-wise operations that operate on all elements of a vector simultaneously. These are heavily used in paint shaders:

```kong
// Component-wise absolute value
abs3(v: float3) -> float3
abs4(v: float4) -> float4

// Component-wise fractional part
frac3(v: float3) -> float3
frac4(v: float4) -> float4

// Component-wise clamp
clamp3(v: float3, min: float, max: float) -> float3
clamp4(v: float4, min: float, max: float) -> float4

// Component-wise min/max
max3(a: float3, b: float3) -> float3
min3(a: float3, b: float3) -> float3
max4(a: float4, b: float4) -> float4
min4(a: float4, b: float4) -> float4

// Component-wise linear interpolation
lerp3(a: float3, b: float3, t: float) -> float3
lerp4(a: float4, b: float4, t: float) -> float4
```

These operations enable efficient per-component arithmetic without explicit loops:

```kong
// Instead of:
x.x = max(0.0, color.x - 0.004);
x.y = max(0.0, color.y - 0.004);
x.z = max(0.0, color.z - 0.004);

// Can write:
var x: float3 = max3(float3(0.0, 0.0, 0.0), color - float3(0.004, 0.004, 0.004));
```

### Math Functions

```kong
// Trigonometry
sin(x: float) -> float
cos(x: float) -> float
tan(x: float) -> float
asin(x: float) -> float
acos(x: float) -> float
atan(x: float) -> float

// Arithmetic
abs(x: float) -> float
floor(x: float) -> float
ceil(x: float) -> float
clamp(x: float, min: float, max: float) -> float
lerp(a: float, b: float, t: float) -> float
pow(x: float, y: float) -> float
sqrt(x: float) -> float
min(a: float, b: float) -> float
max(a: float, b: float) -> float
exp(x: float) -> float
log(x: float) -> float

// Vector
dot(a: float3, b: float3) -> float
cross(a: float3, b: float3) -> float3
normalize(v: float3) -> float3
reflect(v: float3, n: float3) -> float3
length(v: float3) -> float
distance(a: float3, b: float3) -> float

// Swizzle masks - extract single channel as float
v.r  // single float from red channel
v.rgb  // float3 from rgb
v.rgba  // float4
v.xxx  // replicate single channel to float3
v.yy  // replicate single channel to float2
```

### Intrinsics

```kong
ddx(x: float) -> float    // Partial derivative in x
ddy(x: float) -> float     // Partial derivative in y
group_id() -> int3         // Thread group ID (compute shaders)
dispatch_thread_id() -> int3  // Global thread ID (compute shaders)
vertex_id() -> int         // Vertex index
```

### Control Flow

```kong
// discard - discard the current fragment (pixel)
if (tex_sample.r < 0.9) {
    discard;
}

// while loop - for iterative algorithms
var i: int = 0;
while (i < int(max_steps)) {
    hit_coord += step_vec;
    t += ray_step;
    if (delta > 0.0 && delta < 0.2) {
        return t;
    }
    i += 1;
}
```

## Advanced Patterns from Paint Shaders

### Octahedron Normal Encoding

Efficient normal encoding using octahedral projection (used in `deferred_light.kong`, `ssao_pass.kong`):

```kong
fun octahedron_wrap(v: float2): float2 {
    var a: float2;
    if (v.x >= 0.0) { a.x = 1.0; } else { a.x = -1.0; }
    if (v.y >= 0.0) { a.y = 1.0; } else { a.y = -1.0; }

    var r: float2;
    r.x = abs(v.y);
    r.y = abs(v.x);
    r.x = 1.0 - r.x;
    r.y = 1.0 - r.y;
    return r * a;
}

// Decode from GBuffer
var enc: float2 = g0.rg;
var n: float3;
n.z = 1.0 - abs(enc.x) - abs(enc.y);
if (n.z >= 0.0) {
    n.x = enc.x;
    n.y = enc.y;
} else {
    var f2: float2 = octahedron_wrap(enc.xy);
    n.x = f2.x;
    n.y = f2.y;
}
n = normalize(constants.V3 * n);
```

### Whiteout Normal Blend

Blend two normals using the whiteout method for normal map merging (used in `layer_merge.kong`):

```kong
// Whiteout blend two normal maps
var n1: float3 = col0.rgb * float3(2.0, 2.0, 2.0) - float3(1.0, 1.0, 1.0);
var n2: float3 = lerp3(float3(0.5, 0.5, 1.0), col1.rgb, str) * float3(2.0, 2.0, 2.0) - float3(1.0, 1.0, 1.0);
return float4(
    normalize(float3(n1.xy + n2.xy, n1.z * n2.z)) * float3(0.5, 0.5, 0.5) + float3(0.5, 0.5, 0.5),
    max(col1.a, cola.a));
```

### Depth Reconstruction from GBuffer

Reconstruct view-space position from depth texture (used in `deferred_light.kong`, `ssao_pass.kong`):

```kong
// From depth to linear depth
var linear_depth: float = camera_proj.y / (depth - camera_proj.x);

// Full position reconstruction
fun get_pos(eye: float3, eye_look: float3, view_ray: float3, depth: float, camera_proj: float2): float3 {
    var linear_depth: float = camera_proj.y / (depth - camera_proj.x);
    var view_z_dist: float = dot(eye_look, view_ray);
    var wposition: float3 = eye + view_ray * (linear_depth / view_z_dist);
    return wposition;
}
```

### Environment Map Sampling

Convert 3D direction to 2D equirectangular UV and sample:

```kong
const PI: float = 3.14159265358979;
const PI2: float = 6.28318530718;

fun envmap_equirect(normal: float3, angle: float): float2 {
    var phi: float = acos(normal.z);
    var theta: float = atan2(-normal.y, normal.x) + PI + angle;
    return float2(theta / PI2, phi / PI);
}

// Sample environment map
var color: float4 = sample(envmap, sampler_linear, envmap_equirect(-n, angle));
```

### Filmic Tonemapping

Filmic tonemapping operator (used in `world_pass.kong`):

```kong
fun tonemap_filmic(color: float3): float3 {
    var x: float3;
    x.x = max(0.0, color.x - 0.004);
    x.y = max(0.0, color.y - 0.004);
    x.z = max(0.0, color.z - 0.004);
    return (x * (x * 6.2 + 0.5)) / (x * (x * 6.2 + 1.7) + 0.06);
}
```

### Layer Blending Modes

Common blend modes implemented in `layer_merge.kong`:

| Mode | Index | Formula |
|------|-------|---------|
| Mix | 0 | `lerp(base, blend, alpha)` |
| Darken | 1 | `min(base, blend)` |
| Multiply | 2 | `base * blend` |
| Burn | 3 | `1 - (1 - base) / blend` |
| Lighten | 4 | `max(base, blend)` |
| Screen | 5 | `1 - (1 - base) * (1 - blend)` |
| Dodge | 6 | `base / (1 - blend)` |
| Add | 7 | `base + blend` |

### Dual Filter Downsampling

Efficient downsampling using a dual-kernel filter (used in `bloom_downsample_pass.kong`):

```kong
fun downsample_dual_filter(tex_coord: float2, texel_size: float2): float3 {
    var delta: float3 = float3(texel_size.xy, texel_size.x) * float3(0.5, 0.5, -0.5);

    var result: float3;
    result  = sample_lod(tex, sampler_linear, tex_coord,        0.0).rgb * 4.0;
    result += sample_lod(tex, sampler_linear, tex_coord - delta.xy, 0.0).rgb;
    result += sample_lod(tex, sampler_linear, tex_coord - delta.zy, 0.0).rgb;
    result += sample_lod(tex, sampler_linear, tex_coord + delta.zy, 0.0).rgb;
    result += sample_lod(tex, sampler_linear, tex_coord + delta.xy, 0.0).rgb;

    return result * (1.0 / 8.0);
}
```

## Complete Example: world_pass.kong

A full deferred rendering pipeline showing environment mapping with PBR lighting:

```kong
#[set(everything)]
const constants: {
    SMVP: float4x4;
    envmap_data_world: float4; // angle, tonemap_strength, empty, strength
};

#[set(everything)]
const sampler_linear: sampler;

#[set(everything)]
const envmap: tex2d;

const PI: float = 3.1415926535;
const PI2: float = 6.283185307;

struct vert_in {
    pos: float3;
    nor: float3;
}

struct vert_out {
    pos: float4;
    nor: float3;
}

fun world_pass_vert(input: vert_in): vert_out {
    var output: vert_out;
    output.pos = constants.SMVP * float4(input.pos, 1.0);
    output.nor = input.nor;
    return output;
}

fun envmap_equirect(normal: float3, angle: float): float2 {
    var phi: float = acos(normal.z);
    var theta: float = atan2(-normal.y, normal.x) + PI + angle;
    return float2(theta / PI2, phi / PI);
}

fun tonemap_filmic(color: float3): float3 {
    var x: float3;
    x.x = max(0.0, color.x - 0.004);
    x.y = max(0.0, color.y - 0.004);
    x.z = max(0.0, color.z - 0.004);
    return (x * (x * 6.2 + 0.5)) / (x * (x * 6.2 + 1.7) + 0.06);
}

fun world_pass_frag(input: vert_out): float4 {
    var n: float3 = normalize(input.nor);
    var color: float4;
    color.rgb = sample(envmap, sampler_linear, envmap_equirect(-n, constants.envmap_data_world.x)).rgb * constants.envmap_data_world.w;

    // Tonemap with gamma
    color.rgb = lerp3(color.rgb, tonemap_filmic(color.rgb), constants.envmap_data_world.y);

    color.a = 0.0; // Mark as non-opaque
    return color;
}

#[pipe]
struct pipe {
    vertex = world_pass_vert;
    fragment = world_pass_frag;
}
```

## File Structure

| File | Purpose |
|------|---------|
| `base/sources/kong/kong.h` | Header with type definitions, enums, structs, function declarations |
| `base/sources/kong/kong.c` | Core compiler - tokenizer, parser, AST, opcode generation, CLI |
| `base/sources/kong/kong_spirv.c` | SPIR-V backend for Vulkan |
| `base/sources/kong/kong_metal.c` | Metal Shading Language backend |
| `base/sources/kong/kong_hlsl.c` | HLSL backend for Direct3D |
| `base/sources/kong/kong_wgsl.c` | WGSL backend for WebGPU |
| `base/sources/kong/kong_cstyle.c` | C-style output for debugging |
| `base/tools/amake/ashader.c` | CLI wrapper tool used by the build system |

## Shader Locations

- `base/shaders/` - Core engine shaders
- `paint/shaders/` - ArmorPaint application shaders
- `engine/assets/shaders/` - Game engine shaders
- `base/tests/*/shaders/` - Test project shaders

## Origin

KONG is a revision of [Kongruent](https://github.com/Kode/Kongruent) by RobDangerous, specifically commit `1b0f3b70673122e3b20701cdb434781a065555d7`. It was rewritten into a single-file architecture with `kong.c` + `kong.h` as the core, plus separate backend files for each graphics API.