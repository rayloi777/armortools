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

	// cube.arm has no camera object — add one
	any_array_push(scene->objects,
	    GC_ALLOC_INIT(obj_t, {.name = "Camera", .type = "camera_object", .data_ref = "MyCamera", .visible = true, .spawn = true}));

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
