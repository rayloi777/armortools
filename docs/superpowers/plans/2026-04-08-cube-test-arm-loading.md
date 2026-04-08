# Cube Test: Load cube.arm + Texture Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace hardcoded mesh data in the cube test with `data_get_scene_raw("cube")` loading, and add texture support using the existing `texture.png` asset.

**Architecture:** Load `cube.arm` via Iron's `data_get_scene_raw()` to get mesh data and scene objects, then supplement the loaded `scene_t` with camera, material, shader, and world data that the `.arm` file lacks. Update the Kong shader to sample a texture instead of outputting solid red.

**Tech Stack:** C (Iron engine API), Kong shader language

---

## File Structure

| File | Action | Responsibility |
|------|--------|---------------|
| `base/tests/cube/sources/main.c` | Modify | Replace hardcoded scene construction with `.arm` loading + pipeline data |
| `base/tests/cube/shaders/mesh.kong` | Modify | Add texture sampler, tex coords, `sample()` call |

---

### Task 1: Update mesh.kong shader for texture support

**Files:**
- Modify: `base/tests/cube/shaders/mesh.kong`

- [ ] **Step 1: Rewrite mesh.kong with texture sampling**

Replace the entire file content with:

```kong

#[set(everything)]
const constants: {
	WVP: float4x4;
};

#[set(everything)]
const sampler_linear: sampler;

#[set(everything)]
const my_texture: tex2d;

struct vert_in {
	pos: float4;
	tex: float2;
}

struct vert_out {
	pos: float4;
	tex: float2;
}

fun mesh_vert(input: vert_in): vert_out {
	var output: vert_out;
	output.tex = input.tex;
	output.pos = constants.WVP * float4(input.pos.xyz, 1.0);
	return output;
}

fun mesh_frag(input: vert_out): float4 {
	return sample(my_texture, sampler_linear, input.tex);
}

#[pipe]
struct pipe {
	vertex = mesh_vert;
	fragment = mesh_frag;
}
```

Changes from original:
- Added `sampler_linear: sampler` and `my_texture: tex2d` constants
- Added `tex: float2` to `vert_in` and `vert_out`
- `mesh_vert` passes through `tex` coordinates
- `mesh_frag` samples the texture instead of returning solid red

- [ ] **Step 2: Commit shader changes**

```bash
git add base/tests/cube/shaders/mesh.kong
git commit -m "feat(cube-test): add texture sampling to mesh shader"
```

---

### Task 2: Rewrite main.c to load cube.arm + add texture wiring

**Files:**
- Modify: `base/tests/cube/sources/main.c`

- [ ] **Step 1: Rewrite main.c**

Replace the entire file content with:

```c
// ../../make --run

#include <stdio.h>
#include <iron.h>

void render_commands() {
	// Use _gpu_begin directly — render_path_set_target calls gpu_viewport
	// with znear=0.1/zfar=100.0 which breaks Metal's depth range
	_gpu_begin(NULL, NULL, NULL, GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH, 0xff6495ed, 1.0);
	render_path_submit_draw("mesh");
	gpu_end();
}

void scene_ready() {
	transform_t *t = scene_camera->base->transform;
	t->loc         = vec4_create(0, 0, 5.0, 1.0);
	t->rot         = quat_create(0, 0, 0, 1);
	transform_build_matrix(t);
	camera_object_build_proj(scene_camera, (f32)sys_w() / (f32)sys_h());
	camera_object_build_mat(scene_camera);
	render_path_current_w = sys_w();
	render_path_current_h = sys_h();
}

void ready() {
	gc_unroot(render_path_commands);
	render_path_commands = render_commands;
	gc_root(render_path_commands);

	gc_unroot(data_cached_scene_raws);
	data_cached_scene_raws = any_map_create();
	gc_root(data_cached_scene_raws);

	// Load cube.arm — provides mesh data, objects, transforms
	scene_t *scene = data_get_scene_raw("cube");

	// Supplement with pipeline data not stored in cube.arm
	scene->camera_datas = any_array_create_from_raw(
	    (void *[]){
	        GC_ALLOC_INIT(camera_data_t, {.name = "MyCamera", .near_plane = 0.01, .far_plane = 100.0, .fov = 0.85}),
	    },
	    1);
	scene->camera_ref     = "Camera";
	scene->world_ref      = "MyWorld";
	scene->world_datas    = any_array_create_from_raw((void *[]){GC_ALLOC_INIT(world_data_t, {.name = "MyWorld", .color = 0xff000000})}, 1);
	scene->material_datas = any_array_create_from_raw(
	    (void *[]){
	        GC_ALLOC_INIT(material_data_t,
	                      {.name     = "MyMaterial",
	                       .shader   = "MyShader",
	                       .contexts = any_array_create_from_raw(
	                           (void *[]){
	                               GC_ALLOC_INIT(material_context_t, {.name          = "mesh",
	                                                                  .bind_textures = any_array_create_from_raw(
	                                                                      (void *[]){
	                                                                          GC_ALLOC_INIT(bind_tex_t, {.name = "my_texture", .file = "texture.k"}),
	                                                                      },
	                                                                      1)}),
	                           },
	                           1)}),
	    },
	    1);
	scene->shader_datas = any_array_create_from_raw(
	    (void *[]){
	        GC_ALLOC_INIT(shader_data_t, {.name     = "MyShader",
	                                       .contexts = any_array_create_from_raw(
	                                           (void *[]){
	                                               GC_ALLOC_INIT(shader_context_t,
	                                                             {.name            = "mesh",
	                                                              .vertex_shader   = "mesh.vert",
	                                                              .fragment_shader = "mesh.frag",
	                                                              .compare_mode    = "less",
	                                                              .cull_mode       = "none",
	                                                              .depth_write     = true,
	                                                              .vertex_elements = any_array_create_from_raw(
	                                                                  (void *[]){
	                                                                      GC_ALLOC_INIT(vertex_element_t, {.name = "pos", .data = "short4norm"}),
	                                                                      GC_ALLOC_INIT(vertex_element_t, {.name = "tex", .data = "short2norm"}),
	                                                                  },
	                                                                  2),
	                                                              .constants = any_array_create_from_raw(
	                                                                  (void *[]){
	                                                                      GC_ALLOC_INIT(shader_const_t,
	                                                                                    {.name = "WVP", .type = "mat4", .link = "_world_view_proj_matrix"}),
	                                                                  },
	                                                                  1),
	                                                              .texture_units = any_array_create_from_raw(
	                                                                  (void *[]){
	                                                                      GC_ALLOC_INIT(tex_unit_t, {.name = "my_texture"}),
	                                                                  },
	                                                                  1),
	                                                              .depth_attachment = "D32"}),
                                           },
                                           1)}),
        },
        1);

	any_map_set(data_cached_scene_raws, scene->name, scene);
	scene_create(scene);
	scene_ready();
}

void _kickstart() {
	iron_window_options_t *ops =
	    GC_ALLOC_INIT(iron_window_options_t, {.title     = "Cube",
	                                          .width     = 1280,
	                                          .height    = 720,
	                                          .x         = -1,
	                                          .y         = -1,
	                                          .features  = IRON_WINDOW_FEATURES_RESIZABLE | IRON_WINDOW_FEATURES_MINIMIZABLE | IRON_WINDOW_FEATURES_MAXIMIZABLE,
	                                          .mode      = IRON_WINDOW_MODE_WINDOW,
	                                          .frequency = 60,
	                                          .vsync     = true,
	                                          .depth_bits = 32});
	sys_start(ops);
	ready();
	iron_start();
}

//

any_map_t *ui_children;
any_map_t *ui_nodes_custom_buttons;
i32        pipes_offset;
char      *strings_check_internet_connection() {
    return "";
}
void  console_error(char *s) {}
void  console_info(char *s) {}
char *tr(char *id) {
    return id;
}
i32 pipes_get_constant_location(char *s) {
    return 0;
}
```

Key changes from original:
- Removed the entire `scene_t` GC_ALLOC_INIT block with hardcoded mesh vertex/index arrays (~100 lines)
- Added `scene_t *scene = data_get_scene_raw("cube")` to load from `.arm`
- Added camera, material, shader, world data to the loaded scene
- Material context now has `bind_textures` with `bind_tex_t` referencing `"texture.k"`
- Shader context now has `texture_units` with `tex_unit_t` named `"my_texture"`
- Shader vertex_elements now includes both `pos` (short4norm) and `tex` (short2norm)
- Everything else (render_commands, scene_ready, _kickstart, stubs) unchanged

- [ ] **Step 2: Commit main.c changes**

```bash
git add base/tests/cube/sources/main.c
git commit -m "feat(cube-test): load cube.arm instead of hardcoded mesh, add texture support"
```

---

### Task 3: Build and verify

**Files:** None (build only)

- [ ] **Step 1: Export assets and shaders**

```bash
cd /Users/user/Documents/GitHub/armortools/base/tests/cube && ../../make macos metal
```

Expected: completes without errors, exports `mesh.vert.metal`, `mesh.frag.metal`, `cube.arm`, `texture.k` to `build/out/data/`.

- [ ] **Step 2: Compile**

```bash
cd /Users/user/Documents/GitHub/armortools/base/tests/cube/build && xcodebuild -project test.xcodeproj -configuration Release build
```

Expected: `** BUILD SUCCEEDED **`

- [ ] **Step 3: Run and visually verify**

```bash
/Users/user/Documents/GitHub/armortools/base/tests/cube/build/build/Release/test.app/Contents/MacOS/test
```

Expected: a window showing a cube at z=5 with `texture.png` applied (not solid red).
