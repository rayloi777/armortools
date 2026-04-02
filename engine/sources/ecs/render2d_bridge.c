#include "render2d_bridge.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "ecs_bridge.h"
#include "flecs.h"
#include "camera_bridge.h"
#include "../core/camera2d.h"
#include <iron_draw.h>
#include <iron_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static game_world_t *g_render2d_world = NULL;
static ecs_query_t *g_sys_2d_sprite_query = NULL;

typedef struct {
    int layer;
    gpu_texture_t *texture;
    float x, y;
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

    printf("Render2d Bridge initialized\n");
}

void sys_2d_shutdown(void) {
    if (g_sys_2d_sprite_query) { ecs_query_fini(g_sys_2d_sprite_query); g_sys_2d_sprite_query = NULL; }
    free(s_items);
    s_items = NULL;
    s_capacity = 0;
    g_render2d_world = NULL;
    printf("Render2d Bridge shutdown\n");
}

void sys_2d_draw(void) {
    if (!g_render2d_world) return;

    int count = 0;
    ensure_capacity(128);

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_render2d_world);
    if (!ecs) return;

    if (g_sys_2d_sprite_query) {
        ecs_iter_t it = ecs_query_iter(ecs, g_sys_2d_sprite_query);
        while (ecs_query_next(&it)) {
            comp_2d_position *pos = ecs_field(&it, comp_2d_position, 0);
            comp_2d_sprite *spr = ecs_field(&it, comp_2d_sprite, 1);
            for (int i = 0; i < it.count; i++) {
                if (!spr[i].visible || !spr[i].render_object) continue;
                gpu_texture_t *tex = (gpu_texture_t *)spr[i].render_object;
                ensure_capacity(count);
                sprite_item_t *item = &s_items[count++];
                item->layer = spr[i].layer;
                item->texture = tex;
                item->x = pos[i].x;
                item->y = pos[i].y;
                item->scale_x = spr[i].scale_x;
                item->scale_y = spr[i].scale_y;
                item->src_width = spr[i].src_width >= 1.0f ? spr[i].src_width : (float)tex->width;
                item->src_height = spr[i].src_height >= 1.0f ? spr[i].src_height : (float)tex->height;
                item->pivot_x = spr[i].pivot_x;
                item->pivot_y = spr[i].pivot_y;
                item->flip_x = spr[i].flip_x;
                item->flip_y = spr[i].flip_y;
            }
        }
    }

    if (count == 0) return;

    camera2d_t *cam = camera_bridge_get_camera();
    float cam_x = camera2d_get_x(cam);
    float cam_y = camera2d_get_y(cam);
    float cam_zoom = camera2d_get_zoom(cam);
    float half_w = iron_window_width() / 2.0f;
    float half_h = iron_window_height() / 2.0f;

    for (int i = 0; i < count; i++) {
        sprite_item_t *item = &s_items[i];
        float scaled_w = item->src_width * item->scale_x * cam_zoom;
        float scaled_h = item->src_height * item->scale_y * cam_zoom;
        float screen_x = (item->x - cam_x) * cam_zoom + half_w;
        float screen_y = (item->y - cam_y) * cam_zoom + half_h;
        float draw_x = screen_x - (scaled_w * item->pivot_x);
        float draw_y = screen_y - (scaled_h * item->pivot_y);
        float sw = item->src_width;
        float sh = item->src_height;
        if (item->flip_x || item->flip_y) {
            draw_scaled_sub_image(item->texture,
                item->flip_x ? sw : 0,
                item->flip_y ? sh : 0,
                item->flip_x ? -sw : sw,
                item->flip_y ? -sh : sh,
                draw_x, draw_y, scaled_w, scaled_h);
        } else {
            draw_scaled_sub_image(item->texture, 0, 0, sw, sh, draw_x, draw_y, scaled_w, scaled_h);
        }
    }
}
