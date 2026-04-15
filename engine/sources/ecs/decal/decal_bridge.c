#include "decal_bridge.h"
#include "../ecs_world.h"
#include "../ecs_components.h"
#include "flecs.h"
#include <iron.h>
#include <const_data.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static game_world_t *g_decal_world = NULL;
static ecs_query_t *g_decal_query = NULL;
static gpu_pipeline_t *g_decal_pipeline = NULL;

extern gpu_vertex_structure_t postfx_vertex_structure;

static gpu_pipeline_t *create_decal_pipeline(void) {
    gpu_shader_t *vert = sys_get_shader("decal_projection.vert");
    gpu_shader_t *frag = sys_get_shader("decal_projection.frag");
    printf("Decal: shader load (vert=%p frag=%p)\n", (void *)vert, (void *)frag);
    if (!vert || !frag) {
        printf("Decal: failed to load shaders — skipping pipeline creation\n");
        return NULL;
    }

    gpu_pipeline_t *pipe = (gpu_pipeline_t *)calloc(1, sizeof(gpu_pipeline_t));
    gpu_pipeline_init(pipe);

    // Screen-aligned vertex layout (pos: float2)
    pipe->input_layout = &postfx_vertex_structure;
    pipe->color_attachment[0] = GPU_TEXTURE_FORMAT_RGBA64;
    pipe->blend_source = GPU_BLEND_ONE;
    pipe->blend_destination = GPU_BLEND_INV_SOURCE_ALPHA;
    pipe->alpha_blend_source = GPU_BLEND_ONE;
    pipe->alpha_blend_destination = GPU_BLEND_INV_SOURCE_ALPHA;

    pipe->vertex_shader = vert;
    pipe->fragment_shader = frag;

    printf("Decal: vert shader ptr=%p fragment=%p\n", (void *)vert, (void *)frag);
    printf("Decal: compiling pipeline...\n");
    gpu_pipeline_compile(pipe);
    printf("Decal: pipeline compiled ok\n");
    return pipe;
}

void decal_bridge_init(game_world_t *world) {
    g_decal_world = world;
    if (!world) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(world);

    ecs_query_desc_t desc = {0};
    desc.terms[0].id = ecs_component_comp_3d_decal();
    desc.terms[1].id = ecs_component_comp_3d_position();
    g_decal_query = ecs_query_init(ecs, &desc);

    g_decal_pipeline = create_decal_pipeline();

    if (g_decal_pipeline) {
        printf("Decal Bridge initialized\n");
    } else {
        printf("Decal Bridge initialized (no pipeline — decals disabled)\n");
    }
}

void decal_bridge_shutdown(void) {
    if (g_decal_query) { ecs_query_fini(g_decal_query); g_decal_query = NULL; }
    // Pipeline freed by Iron's GC
    g_decal_pipeline = NULL;
    g_decal_world = NULL;
    printf("Decal Bridge shutdown\n");
}

void decal_bridge_render(gpu_texture_t *depth_tex, gpu_texture_t *scene_tex, int w, int h) {
    if (!g_decal_world || !g_decal_query || !g_decal_pipeline) return;
    if (!scene_camera) return;

    // Check if any decals exist
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_decal_world);
    if (!ecs) return;

    bool has_decals = false;
    ecs_iter_t it = ecs_query_iter(ecs, g_decal_query);
    while (ecs_query_next(&it)) {
        if (it.count > 0) { has_decals = true; break; }
    }
    if (!has_decals) return;

    // Compute inverse view-projection matrix for world position reconstruction
    mat4_t vp = mat4_mult_mat(scene_camera->v, scene_camera->p);
    mat4_t inv_vp = mat4_inv(vp);

    // Prepare screen-aligned vertex data
    if (const_data_screen_aligned_vb == NULL) {
        const_data_create_screen_aligned_data();
    }

    // Set pipeline FIRST (it clears textures)
    gpu_set_pipeline(g_decal_pipeline);

    // Set constants: inverse view-projection matrix rows (offset 0, 16, 32, 48 bytes)
    gpu_set_float(0, inv_vp.m00);
    gpu_set_float(4, inv_vp.m10);
    gpu_set_float(8, inv_vp.m20);
    gpu_set_float(12, inv_vp.m30);

    gpu_set_float(16, inv_vp.m01);
    gpu_set_float(20, inv_vp.m11);
    gpu_set_float(24, inv_vp.m21);
    gpu_set_float(28, inv_vp.m31);

    gpu_set_float(32, inv_vp.m02);
    gpu_set_float(36, inv_vp.m12);
    gpu_set_float(40, inv_vp.m22);
    gpu_set_float(44, inv_vp.m32);

    gpu_set_float(48, inv_vp.m03);
    gpu_set_float(52, inv_vp.m13);
    gpu_set_float(56, inv_vp.m23);
    gpu_set_float(60, inv_vp.m33);

    // Decal position and size from first decal entity
    it = ecs_query_iter(ecs, g_decal_query);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            const comp_3d_decal *decal = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_decal());
            const comp_3d_position *pos = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_position());
            if (!decal || !pos) continue;

            // decal_pos (offset 64)
            gpu_set_float(64, pos->x);
            gpu_set_float(68, pos->y);
            gpu_set_float(72, pos->z);

            // decal_half (offset 76)
            gpu_set_float(76, decal->size_x);
            gpu_set_float(80, decal->size_y);
            gpu_set_float(84, decal->size_z);

            // decal_color (offset 88)
            gpu_set_float(88, decal->color_r);
            gpu_set_float(92, decal->color_g);
            gpu_set_float(96, decal->color_b);

            // decal_opacity (offset 100)
            gpu_set_float(100, decal->opacity);

            // screen dimensions (offset 104, 108)
            gpu_set_float(104, (float)w);
            gpu_set_float(108, (float)h);

            // near/far (offset 112, 116)
            gpu_set_float(112, 0.1f);
            gpu_set_float(116, 100.0f);

            // Set textures (after pipeline!)
            gpu_set_texture(0, depth_tex);
            gpu_set_texture(1, scene_tex);

            gpu_set_vertex_buffer(const_data_screen_aligned_vb);
            gpu_set_index_buffer(const_data_screen_aligned_ib);
            gpu_draw();
        }
    }
}
