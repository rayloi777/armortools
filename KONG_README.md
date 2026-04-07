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

```kong
#[set(sampler_name)]
var my_sampler: sampler;

#[set(texture_name)]
var my_texture: tex2d;

#[pipe]
struct pipe {
    vertex = test_vert;
    fragment = test_frag;
}
```

- `#[set(name)]` - Binds a resource (sampler, texture, constant buffer) to a descriptor set
- `#[pipe]` - Marks a render pipeline definition that groups vertex and fragment shaders

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

// Vector
dot(a: float3, b: float3) -> float
cross(a: float3, b: float3) -> float3
normalize(v: float3) -> float3
reflect(v: float3, n: float3) -> float3
length(v: float3) -> float
distance(a: float3, b: float3) -> float
```

### Intrinsics

```kong
ddx(x: float) -> float    // Partial derivative in x
ddy(x: float) -> float    // Partial derivative in y
group_id() -> int3         // Thread group ID (compute shaders)
dispatch_thread_id() -> int3  // Global thread ID (compute shaders)
vertex_id() -> int        // Vertex index
```

## Complete Example

```kong
struct vert_in {
    pos: float3;
    uv: float2;
}

struct vert_out {
    pos: float4;
    uv: float2;
}

#[set(tex)]
var diffuse_texture: tex2d;
var diffuse_sampler: sampler;

fun test_vert(input: vert_in): vert_out {
    var output: vert_out;
    output.pos = float4(input.pos, 1.0);
    output.uv = input.uv;
    return output;
}

fun test_frag(input: vert_out): float4 {
    return sample(diffuse_texture, diffuse_sampler, input.uv);
}

#[pipe]
struct pipe {
    vertex = test_vert;
    fragment = test_frag;
}
```

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
