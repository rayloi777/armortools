// ../../make --run

#include <stdio.h>
#include <iron.h>

void render_commands() {
	render_path_set_target("", NULL, NULL, GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH, 0xff6495ed, 1.0);
	render_path_draw_meshes("mesh");
}

void scene_ready() {
	transform_t *t = scene_camera->base->transform;
	t->loc         = vec4_create(0, 0, 5, 1.0);
	t->rot         = quat_create(0, 0, 0, 1);
	transform_build_matrix(t);
	camera_object_build_proj(scene_camera, (f32)sys_w() / (f32)sys_h());
	camera_object_build_mat(scene_camera);
}

void ready() {
	gc_unroot(render_path_commands);
	render_path_commands = render_commands;
	gc_root(render_path_commands);

	gc_unroot(data_cached_scene_raws);
	data_cached_scene_raws = any_map_create();
	gc_root(data_cached_scene_raws);
	
	scene_t *scene = GC_ALLOC_INIT(
	    scene_t,
	    {.name    = "Scene",
	     .objects = any_array_create_from_raw(
	         (void *[]){
	             GC_ALLOC_INIT(
	                 obj_t, {.name = "Cube", .type = "mesh_object", .data_ref = "Cube", .material_ref = "MyMaterial", .visible = true, .spawn = true}),
	             GC_ALLOC_INIT(obj_t, {.name = "Camera", .type = "camera_object", .data_ref = "MyCamera", .visible = true, .spawn = true}),
	         },
	         2),
	     .camera_datas = any_array_create_from_raw(
	         (void *[]){
	             GC_ALLOC_INIT(camera_data_t, {.name = "MyCamera", .near_plane = 0.01, .far_plane = 100.0, .fov = 0.85}),
	         },
	         1),
	     .camera_ref     = "Camera",
	     .world_ref      = "MyWorld",
	     .world_datas    = any_array_create_from_raw((void *[]){GC_ALLOC_INIT(world_data_t, {.name = "MyWorld", .color = 0xff000000})}, 1),
	     .material_datas = any_array_create_from_raw(
	         (void *[]){
	             GC_ALLOC_INIT(material_data_t,
	                           {.name     = "MyMaterial",
	                            .shader   = "MyShader",
	                            .contexts = any_array_create_from_raw(
	                                (void *[]){
	                                    GC_ALLOC_INIT(material_context_t, {.name = "mesh"}),
	                                },
	                                1)}),
	         },
	         1),
	     .shader_datas = any_array_create_from_raw(
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
	                                                                      },
	                                                                      1),
	                                                                  .constants = any_array_create_from_raw(
	                                                                      (void *[]){
	                                                                          GC_ALLOC_INIT(shader_const_t,
	                                                                                        {.name = "WVP", .type = "mat4", .link = "_world_view_proj_matrix"}),
	                                                                      },
	                                                                      1),
	                                                                  .depth_attachment = "D32"}),
	                                               },
	                                               1)}),
	         },
	         1),
	     .mesh_datas = any_array_create_from_raw(
	         (void *[]){
	             GC_ALLOC_INIT(mesh_data_t,
	                           {.name        = "Cube",
	                            .scale_pos   = 1.0,
	                            .vertex_arrays = any_array_create_from_raw(
	                                (void *[]){
	                                    GC_ALLOC_INIT(vertex_array_t,
	                                                  {.attrib = "pos",
	                                                   .data   = "short4norm",
	                                                   .values = i16_array_create_from_raw(
	                                                       (i16[]){
	                                                           32767,  32767,  32767, 32767,
	                                                          -32767,  32767,  32767, 32767,
	                                                          -32767, -32767,  32767, 32767,
	                                                           32767, -32767,  32767, 32767,
	                                                           32767,  32767, -32767, 32767,
	                                                          -32767,  32767, -32767, 32767,
	                                                          -32767, -32767, -32767, 32767,
	                                                           32767, -32767, -32767, 32767,
	                                                       },
	                                                       32)
	                                    }),
	                                },
	                                1),
	                            .index_array = u32_array_create_from_raw(
	                                (u32[]){
	                                    0, 1, 2, 0, 2, 3,
	                                    4, 5, 6, 4, 6, 7,
	                                    0, 4, 5, 0, 5, 1,
	                                    2, 6, 7, 2, 7, 3,
	                                    0, 2, 6, 0, 6, 4,
	                                    1, 5, 7, 1, 7, 3
	                                },
	                                36)
	                           }),
	         },
	         1)
	    });

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
char *tr(char *id, any_map_t *vars) {
	return id;
}
i32 pipes_get_constant_location(char *s) {
	return 0;
}
