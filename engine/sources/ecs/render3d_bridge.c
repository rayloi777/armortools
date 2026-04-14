#include "render3d_bridge.h"
#include "ecs_world.h"
#include "deferred/deferred_gbuffer.h"
#include "deferred/deferred_postfx.h"
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

    // --- Post-Processing ---
    postfx_t *pfx = postfx_get();
    if (!pfx || !pfx->initialized) {
        // Fallback: display gbuffer0 directly
        _gpu_begin(NULL, NULL, NULL, GPU_CLEAR_COLOR, 0xff1a1a2e, 1.0f);
        draw_scaled_image(gb->gbuffer0, 0.0f, 0.0f, (float)w, (float)h);
        gpu_end();
        g_3d_rendered = true;
        return;
    }

    postfx_resize(w, h);

    // Debug modes 1,3 show raw textures directly
    if (g_debug_mode == 1 || g_debug_mode == 3) {
        _gpu_begin(NULL, NULL, NULL, GPU_CLEAR_COLOR, 0xff1a1a2e, 1.0f);
        gpu_texture_t *debug_tex = (g_debug_mode == 1) ? gb->depth_target : sd->shadow_map;
        draw_scaled_image(debug_tex, 0.0f, 0.0f, (float)w, (float)h);
        gpu_end();
        g_3d_rendered = true;
        return;
    }

    // Copy lit scene to scene_copy for bloom input
    gpu_texture_t *copy_targets[1] = { pfx->scene_copy };
    gpu_begin(copy_targets, 1, NULL, GPU_CLEAR_COLOR, 0, 1.0f);
    draw_scaled_image(gb->gbuffer0, 0.0f, 0.0f, (float)w, (float)h);
    gpu_end();

    // Pass 3a: SSAO (half-res, reads depth via SSAO shader pipeline)
    if (pfx->ssao_enabled && pfx->ssao_pipeline) {
        gpu_texture_t *ao_targets[1] = { pfx->ao_result };
        gpu_begin(ao_targets, 1, NULL, GPU_CLEAR_COLOR, 0xffffffff, 1.0f);
        draw_set_pipeline(pfx->ssao_pipeline);
        // Set constants for SSAO shader (matching Iron's draw_image layout + custom params)
        // offset 0: pos (float4), offset 16: tex (float4), offset 32: col (float4)
        float hw = (float)(w / 2);
        float hh = (float)(h / 2);
        gpu_set_float4(0, 0.0f, 0.0f, hw / w, hh / h);        // pos: fullscreen at half-res
        gpu_set_float4(16, 0.0f, 0.0f, 1.0f, 1.0f);            // tex: full texture
        gpu_set_float4(32, 1.0f, 1.0f, 1.0f, 1.0f);            // col: white
        gpu_set_float(48, (float)w);                             // screen_w
        gpu_set_float(52, (float)h);                             // screen_h
        gpu_set_float(56, pfx->ssao_radius);                    // radius
        gpu_set_float(60, pfx->ssao_strength);                  // strength
        gpu_set_float(64, 0.1f);                                 // near_plane
        gpu_set_float(68, 100.0f);                               // far_plane
        draw_scaled_image(gb->depth_target, 0.0f, 0.0f, hw, hh);
        draw_set_pipeline(NULL);
        gpu_end();
    }

    // Pass 3b: Bloom downsample chain
    if (pfx->bloom_enabled && pfx->bloom_down_pipeline) {
        // First downsample from scene_copy with brightness extraction
        gpu_texture_t *bd0_targets[1] = { pfx->bloom_down[0] };
        gpu_begin(bd0_targets, 1, NULL, GPU_CLEAR_COLOR, 0, 1.0f);
        draw_set_pipeline(pfx->bloom_down_pipeline);
        gpu_set_float4(0, 0.0f, 0.0f, 1.0f, 1.0f);
        gpu_set_float4(16, 0.0f, 0.0f, 1.0f, 1.0f);
        gpu_set_float4(32, 1.0f, 1.0f, 1.0f, 1.0f);
        gpu_set_float(48, pfx->bloom_threshold);
        draw_scaled_image(pfx->scene_copy, 0.0f, 0.0f, (float)w / 2, (float)h / 2);
        draw_set_pipeline(NULL);
        gpu_end();

        // Progressive downsample with blur
        for (int i = 1; i < 4; i++) {
            gpu_texture_t *bd_targets[1] = { pfx->bloom_down[i] };
            int bw = w >> (i + 1);
            int bh = h >> (i + 1);
            if (bw < 1) bw = 1;
            if (bh < 1) bh = 1;
            int prev_bw = w >> i;
            int prev_bh = h >> i;
            if (prev_bw < 1) prev_bw = 1;
            if (prev_bh < 1) prev_bh = 1;
            gpu_begin(bd_targets, 1, NULL, GPU_CLEAR_COLOR, 0, 1.0f);
            draw_set_pipeline(pfx->bloom_down_pipeline);
            gpu_set_float4(0, 0.0f, 0.0f, 1.0f, 1.0f);
            gpu_set_float4(16, 0.0f, 0.0f, 1.0f, 1.0f);
            gpu_set_float4(32, 1.0f, 1.0f, 1.0f, 1.0f);
            gpu_set_float(48, 0.0f); // no threshold for subsequent passes
            draw_scaled_image(pfx->bloom_down[i - 1], 0.0f, 0.0f, (float)bw, (float)bh);
            draw_set_pipeline(NULL);
            gpu_end();
        }

        // Progressive upsample with tent blur
        if (pfx->bloom_up_pipeline) {
            for (int i = 3; i >= 1; i--) {
                gpu_texture_t *bu_targets[1] = { pfx->bloom_up[i] };
                int bw = w >> i;
                int bh = h >> i;
                if (bw < 1) bw = 1;
                if (bh < 1) bh = 1;
                int src_bw = w >> (i + 1);
                int src_bh = h >> (i + 1);
                if (src_bw < 1) src_bw = 1;
                if (src_bh < 1) src_bh = 1;
                gpu_begin(bu_targets, 1, NULL, GPU_CLEAR_COLOR, 0, 1.0f);
                draw_set_pipeline(pfx->bloom_up_pipeline);
                gpu_set_float4(0, 0.0f, 0.0f, 1.0f, 1.0f);
                gpu_set_float4(16, 0.0f, 0.0f, 1.0f, 1.0f);
                gpu_set_float4(32, 1.0f, 1.0f, 1.0f, 1.0f);
                gpu_set_float(48, 1.0f / (float)src_bw);
                gpu_set_float(52, 1.0f / (float)src_bh);
                draw_scaled_image(pfx->bloom_down[i], 0.0f, 0.0f, (float)bw, (float)bh);
                draw_set_pipeline(NULL);
                gpu_end();
            }

            // Final upsample to half-res bloom result
            gpu_texture_t *bu0_targets[1] = { pfx->bloom_up[0] };
            int src2_bw = w >> 2;
            int src2_bh = h >> 2;
            if (src2_bw < 1) src2_bw = 1;
            if (src2_bh < 1) src2_bh = 1;
            gpu_begin(bu0_targets, 1, NULL, GPU_CLEAR_COLOR, 0, 1.0f);
            draw_set_pipeline(pfx->bloom_up_pipeline);
            gpu_set_float4(0, 0.0f, 0.0f, 1.0f, 1.0f);
            gpu_set_float4(16, 0.0f, 0.0f, 1.0f, 1.0f);
            gpu_set_float4(32, 1.0f, 1.0f, 1.0f, 1.0f);
            gpu_set_float(48, 1.0f / (float)src2_bw);
            gpu_set_float(52, 1.0f / (float)src2_bh);
            draw_scaled_image(pfx->bloom_up[1], 0.0f, 0.0f, (float)w / 2, (float)h / 2);
            draw_set_pipeline(NULL);
            gpu_end();
        }
    }

    // Pass 4: Final composite to screen
    _gpu_begin(NULL, NULL, NULL, GPU_CLEAR_COLOR, 0xff1a1a2e, 1.0f);
    gpu_texture_t *display_tex = NULL;
    switch (g_debug_mode) {
        case 4: display_tex = pfx->ao_result; break;         // SSAO debug
        case 5: display_tex = pfx->bloom_down[2]; break;     // Bloom debug
        default: display_tex = gb->gbuffer0; break;
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
    postfx_init(w > 0 ? w : 1280, h > 0 ? h : 720);
    render_path_commands = sys_3d_render_commands;
    render_path_current_w = w;
    render_path_current_h = h;
    printf("3D Render Bridge: initialized (deferred G-buffer pipeline)\n");
}

void sys_3d_shutdown(void) {
    shadow_shutdown();
    postfx_shutdown();
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
    if (mode > 5) mode = 5;
    g_debug_mode = mode;
}

int sys_3d_get_debug_mode(void) {
    return g_debug_mode;
}
