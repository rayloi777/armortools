#include "render2d_bridge.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "ecs_bridge.h"
#include "flecs.h"
#include "camera_bridge.h"
#include "../core/camera2d.h"
#include <iron_draw.h>
#include <iron_system.h>
#include <iron_gpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USE_BATCH_PIPELINE 1
#define MAX_SPRITES 8192

static game_world_t *g_render2d_world = NULL;
static ecs_query_t *g_sys_2d_sprite_query = NULL;

static gpu_vertex_structure_t s_batch_structure;
static gpu_buffer_t s_batch_vb;
static gpu_buffer_t s_batch_ib;
static gpu_pipeline_t s_batch_pipeline;
static gpu_shader_t s_batch_vs;
static gpu_shader_t s_batch_fs;
static bool s_batch_initialized = false;

#define FLOATS_PER_VERTEX 9
#define VERTS_PER_SPRITE  4
#define INDICES_PER_SPRITE 6

typedef struct {
    int layer;
    gpu_texture_t *texture;
    float x, y, z;
    float scale_x, scale_y;
    float src_width, src_height;
    float pivot_x, pivot_y;
    bool flip_x, flip_y;
} sprite_item_t;

static sprite_item_t *s_items = NULL;
static int s_capacity = 0;

static void ensure_capacity(int needed) {
    if (needed < s_capacity) return;
    int new_cap = s_capacity == 0 ? 128 : s_capacity;
    while (new_cap <= needed) new_cap *= 2;
    sprite_item_t *new_items = realloc(s_items, sizeof(sprite_item_t) * new_cap);
    if (new_items) {
        s_items = new_items;
        s_capacity = new_cap;
    }
}

static int sprite_compare(const void *a, const void *b) {
    const sprite_item_t *sa = (const sprite_item_t *)a;
    const sprite_item_t *sb = (const sprite_item_t *)b;
    if (sa->layer != sb->layer) return sa->layer - sb->layer;
    if (sa->texture < sb->texture) return -1;
    if (sa->texture > sb->texture) return 1;
    return 0;
}

void sys_2d_set_world(game_world_t *world) {
    g_render2d_world = world;
}

void sys_2d_init(void) {
    if (!g_render2d_world) {
        fprintf(stderr, "Render2d Bridge: world not set\n");
        return;
    }
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_render2d_world);
    if (!ecs) {
        fprintf(stderr, "Render2d Bridge: failed to get ECS world\n");
        return;
    }

    ecs_query_desc_t qdesc = {0};
    qdesc.terms[0].id = ecs_component_comp_2d_position();
    qdesc.terms[1].id = ecs_component_comp_2d_sprite();
    g_sys_2d_sprite_query = ecs_query_init(ecs, &qdesc);

    memset(&s_batch_structure, 0, sizeof(s_batch_structure));
    gpu_vertex_structure_add(&s_batch_structure, "pos", GPU_VERTEX_DATA_F32_3X);
    gpu_vertex_structure_add(&s_batch_structure, "tex", GPU_VERTEX_DATA_F32_2X);
    gpu_vertex_structure_add(&s_batch_structure, "col", GPU_VERTEX_DATA_F32_4X);

    gpu_vertex_buffer_init(&s_batch_vb, MAX_SPRITES * VERTS_PER_SPRITE, &s_batch_structure);

    gpu_index_buffer_init(&s_batch_ib, MAX_SPRITES * INDICES_PER_SPRITE);
    int *indices = (int *)gpu_index_buffer_lock(&s_batch_ib);
    for (int i = 0; i < MAX_SPRITES; i++) {
        int base = i * VERTS_PER_SPRITE;
        indices[i * INDICES_PER_SPRITE + 0] = base + 0;
        indices[i * INDICES_PER_SPRITE + 1] = base + 1;
        indices[i * INDICES_PER_SPRITE + 2] = base + 2;
        indices[i * INDICES_PER_SPRITE + 3] = base + 0;
        indices[i * INDICES_PER_SPRITE + 4] = base + 2;
        indices[i * INDICES_PER_SPRITE + 5] = base + 3;
    }
    gpu_index_buffer_unlock(&s_batch_ib);

    char *dp = data_path();
    char *ext = sys_shader_ext();

    char path_vert[512];
    char path_frag[512];
    strcpy(path_vert, dp);
    strcat(path_vert, "sprite_batch.vert");
    strcat(path_vert, ext);
    strcpy(path_frag, dp);
    strcat(path_frag, "sprite_batch.frag");
    strcat(path_frag, ext);

    buffer_t *vs_buf = iron_load_blob(path_vert);
    buffer_t *fs_buf = iron_load_blob(path_frag);

    if (!vs_buf || !fs_buf) {
        fprintf(stderr, "Render2d Bridge: failed to load sprite_batch shader\n");
        fprintf(stderr, "  vert: %s (%s)\n", path_vert, vs_buf ? "ok" : "MISSING");
        fprintf(stderr, "  frag: %s (%s)\n", path_frag, fs_buf ? "ok" : "MISSING");
        return;
    }

    gpu_shader_init(&s_batch_vs, vs_buf->buffer, vs_buf->length, GPU_SHADER_TYPE_VERTEX);
    gpu_shader_init(&s_batch_fs, fs_buf->buffer, fs_buf->length, GPU_SHADER_TYPE_FRAGMENT);

    gpu_pipeline_init(&s_batch_pipeline);
    s_batch_pipeline.input_layout = &s_batch_structure;
    s_batch_pipeline.vertex_shader = &s_batch_vs;
    s_batch_pipeline.fragment_shader = &s_batch_fs;
    s_batch_pipeline.blend_source = GPU_BLEND_ONE;
    s_batch_pipeline.blend_destination = GPU_BLEND_INV_SOURCE_ALPHA;
    s_batch_pipeline.alpha_blend_source = GPU_BLEND_ONE;
    s_batch_pipeline.alpha_blend_destination = GPU_BLEND_INV_SOURCE_ALPHA;
    gpu_pipeline_compile(&s_batch_pipeline);

    if (s_batch_pipeline.impl.pipeline == NULL) {
        fprintf(stderr, "Render2d Bridge: Pipeline compilation FAILED\n");
        return;
    }

    s_batch_initialized = true;
    printf("Render2d Bridge: batch pipeline initialized\n");
}

void sys_2d_shutdown(void) {
    if (g_sys_2d_sprite_query) { ecs_query_fini(g_sys_2d_sprite_query); g_sys_2d_sprite_query = NULL; }
    if (s_batch_initialized) {
        gpu_buffer_destroy(&s_batch_vb);
        gpu_buffer_destroy(&s_batch_ib);
        gpu_shader_destroy(&s_batch_vs);
        gpu_shader_destroy(&s_batch_fs);
        gpu_pipeline_destroy(&s_batch_pipeline);
        s_batch_initialized = false;
    }
    free(s_items);
    s_items = NULL;
    s_capacity = 0;
    g_render2d_world = NULL;
    printf("Render2d Bridge shutdown\n");
}

void sys_2d_draw(void) {
    if (!g_render2d_world || !s_batch_initialized) return;

    if (!g_sys_2d_sprite_query) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_render2d_world);
    if (!ecs) return;

    int count = 0;
    ensure_capacity(128);

    ecs_iter_t it = ecs_query_iter(ecs, g_sys_2d_sprite_query);
    while (ecs_query_next(&it)) {
        comp_2d_position *pos = ecs_field(&it, comp_2d_position, 0);
        comp_2d_sprite *spr = ecs_field(&it, comp_2d_sprite, 1);
        for (int i = 0; i < it.count; i++) {
            if (!spr[i].visible || !spr[i].render_object) continue;
            ensure_capacity(count);
            sprite_item_t *item = &s_items[count];
            gpu_texture_t *tex = (gpu_texture_t *)spr[i].render_object;
            item->layer = spr[i].layer;
            item->texture = tex;
            item->x = pos[i].x;
            item->y = pos[i].y;
            item->z = pos[i].z;
            item->scale_x = spr[i].scale_x;
            item->scale_y = spr[i].scale_y;
            item->src_width = spr[i].src_width >= 1.0f ? spr[i].src_width : (float)tex->width;
            item->src_height = spr[i].src_height >= 1.0f ? spr[i].src_height : (float)tex->height;
            item->pivot_x = spr[i].pivot_x;
            item->pivot_y = spr[i].pivot_y;
            item->flip_x = spr[i].flip_x;
            item->flip_y = spr[i].flip_y;
            count++;
        }
    }
    ecs_iter_fini(&it);

    if (count == 0) return;

    if (count > MAX_SPRITES) {
        fprintf(stderr, "Render2d Bridge: %d sprites exceed MAX_SPRITES (%d), clamping\n", count, MAX_SPRITES);
        count = MAX_SPRITES;
    }

    qsort(s_items, count, sizeof(sprite_item_t), sprite_compare);

    camera2d_t *cam = camera_bridge_get_camera();
    float cam_x = camera2d_get_x(cam);
    float cam_y = camera2d_get_y(cam);
    float cam_zoom = camera2d_get_zoom(cam);
    float win_w = (float)iron_window_width();
    float win_h = (float)iron_window_height();
    float half_w = win_w / 2.0f;
    float half_h = win_h / 2.0f;
    float inv_w2 = 2.0f / win_w;
    float inv_h2 = 2.0f / win_h;

#if USE_BATCH_PIPELINE
    float *verts = (float *)gpu_vertex_buffer_lock(&s_batch_vb);

    for (int i = 0; i < count; i++) {
        sprite_item_t *item = &s_items[i];
        float scaled_w = item->src_width * item->scale_x * cam_zoom;
        float scaled_h = item->src_height * item->scale_y * cam_zoom;
        float screen_x = (item->x - cam_x) * cam_zoom + half_w;
        float screen_y = (item->y - cam_y) * cam_zoom + half_h;
        float draw_x = screen_x - (scaled_w * item->pivot_x);
        float draw_y = screen_y - (scaled_h * item->pivot_y);

        float x0 = draw_x * inv_w2 - 1.0f;
        float y0 = -(draw_y * inv_h2 - 1.0f);
        float x1 = (draw_x + scaled_w) * inv_w2 - 1.0f;
        float y1 = -((draw_y + scaled_h) * inv_h2 - 1.0f);
        float z = item->z;

        float u0 = 0.0f, v0 = 0.0f;
        float u1 = 1.0f, v1 = 1.0f;
        if (item->flip_x) { float tmp = u0; u0 = u1; u1 = tmp; }
        if (item->flip_y) { float tmp = v0; v0 = v1; v1 = tmp; }

        float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
        float *v = verts + i * VERTS_PER_SPRITE * FLOATS_PER_VERTEX;

        v[0] = x0; v[1] = y1; v[2] = z;
        v[3] = u0; v[4] = v1;
        v[5] = r;  v[6] = g;  v[7] = b; v[8] = a;

        v[9]  = x0; v[10] = y0; v[11] = z;
        v[12] = u0; v[13] = v0;
        v[14] = r;  v[15] = g;  v[16] = b; v[17] = a;

        v[18] = x1; v[19] = y0; v[20] = z;
        v[21] = u1; v[22] = v0;
        v[23] = r;  v[24] = g;  v[25] = b; v[26] = a;
        v[27] = x1; v[28] = y1; v[29] = z;
        v[30] = u1; v[31] = v1;
        v[32] = r;  v[33] = g;  v[34] = b; v[35] = a;
    }
    gpu_vertex_buffer_unlock(&s_batch_vb);

    gpu_set_pipeline(&s_batch_pipeline);
    gpu_set_vertex_buffer(&s_batch_vb);
    gpu_set_index_buffer(&s_batch_ib);

    int batch_start = 0;
    while (batch_start < count) {
        gpu_texture_t *batch_tex = s_items[batch_start].texture;
        int batch_end = batch_start + 1;
        while (batch_end < count && s_items[batch_end].texture == batch_tex) {
            batch_end++;
        }
        int batch_count = batch_end - batch_start;
        gpu_set_texture(0, batch_tex);
        s_batch_ib.count = batch_count * INDICES_PER_SPRITE;
        gpu_draw();
        batch_start = batch_end;
    }
#else
    for (int i = 0; i < count; i++) {
        sprite_item_t *item = &s_items[i];
        float sw = item->src_width * item->scale_x * cam_zoom;
        float sh = item->src_height * item->scale_y * cam_zoom;
        float sx = (item->x - cam_x) * cam_zoom + half_w;
        float sy = (item->y - cam_y) * cam_zoom + half_h;
        float dx = sx - (sw * item->pivot_x);
        float dy = sy - (sh * item->pivot_y);
        draw_scaled_sub_image(item->texture, 0, 0, item->src_width, item->src_height, dx, dy, sw, sh);
    }
#endif
}
