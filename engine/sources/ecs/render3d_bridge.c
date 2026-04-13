#include "render3d_bridge.h"
#include "ecs_world.h"
#include "deferred/deferred_gbuffer.h"
#include "shadow/shadow_directional.h"
#include <iron.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

// Whether any 3D content has been rendered this frame.
// When true, the 2D overlay should NOT clear the framebuffer.
static bool g_3d_rendered = false;

// Debug visualization mode: 0=Normal, 1=Depth, 2=Albedo, 3=RoughMet
static int g_debug_mode = 0;

// Light view-projection matrix for shadow mapping
static mat4_t g_light_vp;

static void compute_light_vp(void) {
    // Read light direction from material constants
    float ldx = -0.5f, ldy = -0.7f, ldz = -0.5f;
    if (scene_meshes != NULL && scene_meshes->length > 0) {
        mesh_object_t *m = (mesh_object_t *)scene_meshes->buffer[0];
        if (m && m->material) {
            material_context_t *mc = material_data_get_context(m->material, "mesh");
            if (mc && mc->bind_constants) {
                for (int i = 0; i < mc->bind_constants->length; i++) {
                    bind_const_t *bc = (bind_const_t *)mc->bind_constants->buffer[i];
                    if (bc->name && bc->vec) {
                        if (strcmp(bc->name, "light_dir") == 0 && bc->vec->length >= 3) {
                            ldx = bc->vec->buffer[0];
                            ldy = bc->vec->buffer[1];
                            ldz = bc->vec->buffer[2];
                        }
                    }
                }
            }
        }
    }

    // Normalize light direction
    float len = sqrtf(ldx * ldx + ldy * ldy + ldz * ldz);
    if (len > 0.0001f) { ldx /= len; ldy /= len; ldz /= len; }

    // Build light view matrix using Iron's convention: view = inv(world)
    // Same approach as camera_object_build_mat() in engine.c
    vec4_t eye = vec4_create(-ldx * 5.0f, -ldy * 5.0f, -ldz * 5.0f, 1.0f);
    vec4_t target = vec4_create(0.0f, 0.0f, 0.0f, 1.0f);
    vec4_t up = vec4_create(0.0f, 1.0f, 0.0f, 0.0f);

    vec4_t fwd = vec4_norm(vec4_sub(target, eye));
    vec4_t right = vec4_norm(vec4_cross(fwd, up));
    vec4_t real_up = vec4_cross(right, fwd);

    // Build camera world transform: rotation axes as columns, position at m30/m31/m32
    mat4_t light_world = mat4_identity();
    light_world.m00 = right.x; light_world.m01 = right.y; light_world.m02 = right.z;
    light_world.m10 = real_up.x; light_world.m11 = real_up.y; light_world.m12 = real_up.z;
    light_world.m20 = -fwd.x; light_world.m21 = -fwd.y; light_world.m22 = -fwd.z;
    light_world.m30 = eye.x; light_world.m31 = eye.y; light_world.m32 = eye.z;

    // View = inverse of world transform (same as camera_object_build_mat)
    mat4_t view = mat4_inv(light_world);

    // Orthographic projection
    float ortho_size = 3.0f;
    mat4_t proj = mat4_ortho(-ortho_size, ortho_size, -ortho_size, ortho_size, 0.1f, 15.0f);

    g_light_vp = mat4_mult_mat(view, proj);
}

// Update cam_pos in the material bind_constants to match the current camera position.
static void update_cam_pos_material(void) {
    if (!scene_camera || !scene_camera->base || !scene_camera->base->transform) return;
    if (!scene_meshes || scene_meshes->length == 0) return;

    vec4_t cam_loc = scene_camera->base->transform->loc;

    // Get material from the first mesh object's material pointer
    mesh_object_t *mesh_obj = (mesh_object_t *)scene_meshes->buffer[0];
    if (!mesh_obj || !mesh_obj->material) return;

    material_context_t *mctx = material_data_get_context(mesh_obj->material, "mesh");
    if (!mctx || !mctx->bind_constants) return;

    for (int i = 0; i < mctx->bind_constants->length; i++) {
        bind_const_t *bc = (bind_const_t *)mctx->bind_constants->buffer[i];
        if (!bc->name || !bc->vec) continue;
        if (strcmp(bc->name, "cam_pos") == 0 && bc->vec->length >= 3) {
            bc->vec->buffer[0] = cam_loc.x;
            bc->vec->buffer[1] = cam_loc.y;
            bc->vec->buffer[2] = cam_loc.z;
        }
        else if (strcmp(bc->name, "point_light_pos0") == 0 && bc->vec->length >= 3) {
            bc->vec->buffer[0] = 2.0f;
            bc->vec->buffer[1] = 2.0f;
            bc->vec->buffer[2] = 0.0f;
        }
        else if (strcmp(bc->name, "point_light_strength0") == 0) {
            bc->vec->buffer[0] = 5.0f;
        }
        else if (strcmp(bc->name, "num_point_lights") == 0) {
            bc->vec->buffer[0] = 1.0f;
        }
    }
}

// Helper: set a bind_constant float value by name on the first mesh's material
static void set_material_const(const char *name, float value) {
    if (!scene_meshes || scene_meshes->length == 0) return;
    mesh_object_t *m = (mesh_object_t *)scene_meshes->buffer[0];
    if (!m || !m->material) return;
    material_context_t *mc = material_data_get_context(m->material, "mesh");
    if (!mc || !mc->bind_constants) return;
    for (int i = 0; i < mc->bind_constants->length; i++) {
        bind_const_t *bc = (bind_const_t *)mc->bind_constants->buffer[i];
        if (bc->name && bc->vec && strcmp(bc->name, name) == 0) {
            bc->vec->buffer[0] = value;
            return;
        }
    }
}

static void sys_3d_render_commands(void) {
    int w = sys_w();
    int h = sys_h();
    if (w <= 0 || h <= 0) return;

    gbuffer_resize(w, h);
    gbuffer_t *gb = gbuffer_get();
    if (!gb || !gb->initialized) return;

    shadow_data_t *sd = shadow_get();
    if (!sd || !sd->initialized) return;

    // Update camera position in material before rendering
    update_cam_pos_material();

    // Compute light VP matrix
    compute_light_vp();

    // --- Shadow Pass: Render depth from light perspective ---
    gpu_texture_t *shadow_targets[1] = { sd->shadow_map };
    gpu_begin(shadow_targets, 1, sd->shadow_depth,
              GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH, 0xffffffff, 1.0f);

    camera_object_t *main_cam = scene_camera;
    mat4_t saved_v = main_cam->v;
    mat4_t saved_p = main_cam->p;
    bool saved_frustum = main_cam->data->frustum_culling;

    // Swap camera VP to light perspective
    main_cam->v = g_light_vp;
    main_cam->p = mat4_identity();
    main_cam->data->frustum_culling = false; // disable culling during shadow pass

    // Set shadow_pass = 1 so shader outputs depth as color
    set_material_const("shadow_pass", 1.0f);
    set_material_const("shadow_enabled", 0.0f);

    // Rebuild mesh transforms
    for (int i = 0; scene_meshes && i < scene_meshes->length; i++) {
        mesh_object_t *mesh = (mesh_object_t *)scene_meshes->buffer[i];
        if (mesh && mesh->base && mesh->base->transform) {
            transform_build_matrix(mesh->base->transform);
        }
    }

    render_path_submit_draw("mesh");

    // Restore camera
    main_cam->v = saved_v;
    main_cam->p = saved_p;
    main_cam->data->frustum_culling = saved_frustum;
    camera_object_build_proj(main_cam, (f32)w / (f32)h);
    camera_object_build_mat(main_cam);

    gpu_end();

    // --- G-Buffer Pass: Render scene with PBR + shadow sampling ---
    gpu_texture_t *targets[1] = { gb->gbuffer0 };
    gpu_begin(targets, 1, gb->depth_target,
              GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH, 0xff000000, 1.0f);

    // Set shadow_pass = 0 so shader outputs normal PBR color
    set_material_const("shadow_pass", 0.0f);
    set_material_const("shadow_enabled", 1.0f);

    // Bind shadow map texture via material's bind_textures/runtime system.
    // uniforms_set_material_consts() will match "_shadow_map" by name and
    // call gpu_set_texture(shader_unit, material_runtime_texture) during draw.
    if (scene_meshes != NULL && scene_meshes->length > 0) {
        mesh_object_t *m = (mesh_object_t *)scene_meshes->buffer[0];
        if (m && m->material && m->material->_) {
            material_context_t *mc = material_data_get_context(m->material, "mesh");
            if (mc && mc->_ && mc->_->textures && mc->_->textures->length > 0) {
                mc->_->textures->buffer[0] = sd->shadow_map;
            }
        }
    }

    // Set light VP matrix as 4 vec4 shader constants for shadow coordinate calculation
    if (scene_meshes != NULL && scene_meshes->length > 0) {
        mesh_object_t *m = (mesh_object_t *)scene_meshes->buffer[0];
        if (m && m->material) {
            material_context_t *mc = material_data_get_context(m->material, "mesh");
            if (mc && mc->bind_constants) {
                for (int i = 0; i < mc->bind_constants->length; i++) {
                    bind_const_t *bc = (bind_const_t *)mc->bind_constants->buffer[i];
                    if (!bc->name || !bc->vec) continue;
                    if (strcmp(bc->name, "light_vp_r0") == 0 && bc->vec->length >= 4) {
                        bc->vec->buffer[0] = g_light_vp.m00; bc->vec->buffer[1] = g_light_vp.m10;
                        bc->vec->buffer[2] = g_light_vp.m20; bc->vec->buffer[3] = g_light_vp.m30;
                    }
                    else if (strcmp(bc->name, "light_vp_r1") == 0 && bc->vec->length >= 4) {
                        bc->vec->buffer[0] = g_light_vp.m01; bc->vec->buffer[1] = g_light_vp.m11;
                        bc->vec->buffer[2] = g_light_vp.m21; bc->vec->buffer[3] = g_light_vp.m31;
                    }
                    else if (strcmp(bc->name, "light_vp_r2") == 0 && bc->vec->length >= 4) {
                        bc->vec->buffer[0] = g_light_vp.m02; bc->vec->buffer[1] = g_light_vp.m12;
                        bc->vec->buffer[2] = g_light_vp.m22; bc->vec->buffer[3] = g_light_vp.m32;
                    }
                    else if (strcmp(bc->name, "light_vp_r3") == 0 && bc->vec->length >= 4) {
                        bc->vec->buffer[0] = g_light_vp.m03; bc->vec->buffer[1] = g_light_vp.m13;
                        bc->vec->buffer[2] = g_light_vp.m23; bc->vec->buffer[3] = g_light_vp.m33;
                    }
                }
            }
        }
    }

    render_path_submit_draw("mesh");
    gpu_end();

    // Pass 3: Display
    _gpu_begin(NULL, NULL, NULL, GPU_CLEAR_COLOR, 0xff1a1a2e, 1.0f);
    gpu_texture_t *display_tex = NULL;
    switch (g_debug_mode) {
        case 1: display_tex = gb->depth_target; break;  // Depth
        case 3: display_tex = sd->shadow_map; break;    // Shadow map
        default: display_tex = gb->gbuffer0; break;      // Color/Normal/Albedo
    }
    if (display_tex) {
        draw_scaled_image(display_tex, 0.0f, 0.0f, (float)w, (float)h);
    }
    gpu_end();

    g_3d_rendered = true;
}

void sys_3d_set_world(game_world_t *world) {
    (void)world;
}

void sys_3d_init(void) {
    int w = sys_w();
    int h = sys_h();
    gbuffer_init(w > 0 ? w : 1280, h > 0 ? h : 720);
    shadow_init(2048);
    render_path_commands = sys_3d_render_commands;
    render_path_current_w = w;
    render_path_current_h = h;
    printf("3D Render Bridge: initialized (deferred G-buffer pipeline)\n");
}

void sys_3d_shutdown(void) {
    shadow_shutdown();
    gbuffer_shutdown();
    render_path_commands = NULL;
    g_3d_rendered = false;
    g_debug_mode = 0;
    printf("3D Render Bridge: shutdown\n");
}

void sys_3d_draw(void) {
    // 3D rendering is handled by render_path_commands callback,
    // invoked during sys_render() -> scene_render_frame().
}

bool sys_3d_was_rendered(void) {
    return g_3d_rendered;
}

void sys_3d_reset_frame(void) {
    g_3d_rendered = false;
}

void sys_3d_set_debug_mode(int mode) {
    if (mode < 0) mode = 0;
    if (mode > 4) mode = 4;
    g_debug_mode = mode;
}

int sys_3d_get_debug_mode(void) {
    return g_debug_mode;
}
