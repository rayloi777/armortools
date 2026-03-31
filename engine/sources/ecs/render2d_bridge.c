#include "render2d_bridge.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "ecs_bridge.h"
#include "flecs.h"
#include <iron_draw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static game_world_t *g_render2d_world = NULL;

static ecs_query_t *g_sys_2d_rect_query = NULL;
static ecs_query_t *g_sys_2d_circle_query = NULL;
static ecs_query_t *g_sys_2d_line_query = NULL;
static ecs_query_t *g_sys_2d_text_query = NULL;
static ecs_query_t *g_sys_2d_sprite_query = NULL;

typedef enum {
    R2D_SPRITE,
    R2D_RECT,
    R2D_CIRCLE,
    R2D_LINE,
    R2D_TEXT,
} r2d_type_t;

typedef struct {
    r2d_type_t type;
    int layer;
    union {
        struct { gpu_texture_t *texture; float x, y; float scale_x, scale_y; float src_width, src_height; float pivot_x, pivot_y; bool flip_x, flip_y; } sprite;
        struct { float x, y, w, h; bool filled; float strength; uint32_t color; } rect;
        struct { float cx, cy, radius; int segments; bool filled; float strength; uint32_t color; } circle;
        struct { float x0, y0, x1, y1; float strength; uint32_t color; } line;
        struct { char *text; float x, y; char *font_path; int font_size; uint32_t color; } text;
    };
} r2d_item_t;

static r2d_item_t *s_items = NULL;
static int s_capacity = 0;
static int s_prev_count = 0; // items from previous frame, nearly sorted

// Insertion sort for nearly-sorted data (O(n) best case when frame order is stable)
static void r2d_insertion_sort(r2d_item_t *items, int count) {
    for (int i = 1; i < count; i++) {
        r2d_item_t tmp = items[i];
        int j = i - 1;
        while (j >= 0 && items[j].layer > tmp.layer) {
            items[j + 1] = items[j];
            j--;
        }
        items[j + 1] = tmp;
    }
}

static void ensure_capacity(int needed) {
    if (needed < s_capacity) return;
    int new_cap = s_capacity == 0 ? 128 : s_capacity;
    while (new_cap <= needed) new_cap *= 2;
    r2d_item_t *new_items = realloc(s_items, sizeof(r2d_item_t) * new_cap);
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
    qdesc.terms[0].id = ecs_component_comp_2d_rect();
    g_sys_2d_rect_query = ecs_query_init(ecs, &qdesc);

    qdesc.terms[0].id = ecs_component_comp_2d_circle();
    g_sys_2d_circle_query = ecs_query_init(ecs, &qdesc);

    qdesc.terms[0].id = ecs_component_comp_2d_line();
    g_sys_2d_line_query = ecs_query_init(ecs, &qdesc);

    qdesc.terms[0].id = ecs_component_comp_2d_text();
    g_sys_2d_text_query = ecs_query_init(ecs, &qdesc);

    memset(&qdesc, 0, sizeof(qdesc));
    qdesc.terms[0].id = ecs_component_comp_2d_position();
    qdesc.terms[1].id = ecs_component_comp_2d_sprite();
    g_sys_2d_sprite_query = ecs_query_init(ecs, &qdesc);

    printf("Render2d Bridge initialized\n");
}

void sys_2d_shutdown(void) {
    if (g_sys_2d_rect_query) { ecs_query_fini(g_sys_2d_rect_query); g_sys_2d_rect_query = NULL; }
    if (g_sys_2d_circle_query) { ecs_query_fini(g_sys_2d_circle_query); g_sys_2d_circle_query = NULL; }
    if (g_sys_2d_line_query) { ecs_query_fini(g_sys_2d_line_query); g_sys_2d_line_query = NULL; }
    if (g_sys_2d_text_query) { ecs_query_fini(g_sys_2d_text_query); g_sys_2d_text_query = NULL; }
    if (g_sys_2d_sprite_query) { ecs_query_fini(g_sys_2d_sprite_query); g_sys_2d_sprite_query = NULL; }
    free(s_items);
    s_items = NULL;
    s_capacity = 0;
    g_render2d_world = NULL;
    printf("Render2d Bridge shutdown\n");
}

void sys_2d_draw(void) {
    if (!g_render2d_world) return;

    // Reuse pre-allocated buffer; only reset count per frame, no malloc/free
    int count = 0;
    ensure_capacity(128); // ensure initial allocation

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_render2d_world);
    if (!ecs) return;

    // First pass: count total visible items to do a single ensure_capacity
    int total_estimate = 0;
    // (Skip estimate for simplicity — ensure_capacity handles per-item growth efficiently)

    if (g_sys_2d_sprite_query) {
        ecs_iter_t it = ecs_query_iter(ecs, g_sys_2d_sprite_query);
        while (ecs_query_next(&it)) {
            comp_2d_position *pos = ecs_field(&it, comp_2d_position, 0);
            comp_2d_sprite *spr = ecs_field(&it, comp_2d_sprite, 1);
            for (int i = 0; i < it.count; i++) {
                if (!spr[i].visible || !spr[i].render_object) continue;
                gpu_texture_t *tex = (gpu_texture_t *)spr[i].render_object;
                float src_w = spr[i].src_width >= 1.0f ? spr[i].src_width : (float)tex->width;
                float src_h = spr[i].src_height >= 1.0f ? spr[i].src_height : (float)tex->height;
                ensure_capacity(count);
                r2d_item_t *item = &s_items[count++];
                item->type = R2D_SPRITE;
                item->layer = spr[i].layer;
                item->sprite.texture = tex;
                item->sprite.x = pos[i].x;
                item->sprite.y = pos[i].y;
                item->sprite.scale_x = spr[i].scale_x;
                item->sprite.scale_y = spr[i].scale_y;
                item->sprite.src_width = src_w;
                item->sprite.src_height = src_h;
                item->sprite.pivot_x = spr[i].pivot_x;
                item->sprite.pivot_y = spr[i].pivot_y;
                item->sprite.flip_x = spr[i].flip_x;
                item->sprite.flip_y = spr[i].flip_y;
            }
        }
    }

    if (g_sys_2d_rect_query) {
        ecs_iter_t it = ecs_query_iter(ecs, g_sys_2d_rect_query);
        while (ecs_query_next(&it)) {
            comp_2d_rect *r = ecs_field(&it, comp_2d_rect, 0);
            for (int i = 0; i < it.count; i++) {
                if (!r[i].visible) continue;
                ensure_capacity(count);
                r2d_item_t *item = &s_items[count++];
                item->type = R2D_RECT;
                item->layer = r[i].layer;
                item->rect.x = r[i].x;
                item->rect.y = r[i].y;
                item->rect.w = r[i].width;
                item->rect.h = r[i].height;
                item->rect.filled = r[i].filled;
                item->rect.strength = r[i].strength;
                item->rect.color = r[i].color;
            }
        }
    }

    if (g_sys_2d_circle_query) {
        ecs_iter_t it = ecs_query_iter(ecs, g_sys_2d_circle_query);
        while (ecs_query_next(&it)) {
            comp_2d_circle *c = ecs_field(&it, comp_2d_circle, 0);
            for (int i = 0; i < it.count; i++) {
                if (!c[i].visible) continue;
                ensure_capacity(count);
                r2d_item_t *item = &s_items[count++];
                item->type = R2D_CIRCLE;
                item->layer = c[i].layer;
                item->circle.cx = c[i].cx;
                item->circle.cy = c[i].cy;
                item->circle.radius = c[i].radius;
                item->circle.segments = c[i].segments;
                item->circle.filled = c[i].filled;
                item->circle.strength = c[i].strength;
                item->circle.color = c[i].color;
            }
        }
    }

    if (g_sys_2d_line_query) {
        ecs_iter_t it = ecs_query_iter(ecs, g_sys_2d_line_query);
        while (ecs_query_next(&it)) {
            comp_2d_line *l = ecs_field(&it, comp_2d_line, 0);
            for (int i = 0; i < it.count; i++) {
                if (!l[i].visible) continue;
                ensure_capacity(count);
                r2d_item_t *item = &s_items[count++];
                item->type = R2D_LINE;
                item->layer = l[i].layer;
                item->line.x0 = l[i].x0;
                item->line.y0 = l[i].y0;
                item->line.x1 = l[i].x1;
                item->line.y1 = l[i].y1;
                item->line.strength = l[i].strength;
                item->line.color = l[i].color;
            }
        }
    }

    if (g_sys_2d_text_query) {
        ecs_iter_t it = ecs_query_iter(ecs, g_sys_2d_text_query);
        while (ecs_query_next(&it)) {
            comp_2d_text *t = ecs_field(&it, comp_2d_text, 0);
            for (int i = 0; i < it.count; i++) {
                if (!t[i].visible) continue;
                ensure_capacity(count);
                r2d_item_t *item = &s_items[count++];
                item->type = R2D_TEXT;
                item->layer = t[i].layer;
                item->text.text = t[i].text;
                item->text.x = t[i].x;
                item->text.y = t[i].y;
                item->text.font_path = t[i].font_path;
                item->text.font_size = t[i].font_size;
                item->text.color = t[i].color;
            }
        }
    }

    if (count == 0) return;

    // Insertion sort: O(n) for nearly-sorted data (stable between frames)
    // Much faster than qsort's O(n log n) for the common case
    r2d_insertion_sort(s_items, count);

    uint32_t prev_color = draw_get_color();
    for (int i = 0; i < count; i++) {
        r2d_item_t *item = &s_items[i];
        switch (item->type) {
        case R2D_SPRITE: {
            gpu_texture_t *tex = item->sprite.texture;
            float draw_w = item->sprite.src_width * item->sprite.scale_x;
            float draw_h = item->sprite.src_height * item->sprite.scale_y;
            float draw_x = item->sprite.x - (draw_w * item->sprite.pivot_x);
            float draw_y = item->sprite.y - (draw_h * item->sprite.pivot_y);
            float sw = item->sprite.src_width;
            float sh = item->sprite.src_height;
            if (item->sprite.flip_x || item->sprite.flip_y) {
                draw_scaled_sub_image(tex,
                    item->sprite.flip_x ? sw : 0,
                    item->sprite.flip_y ? sh : 0,
                    item->sprite.flip_x ? -sw : sw,
                    item->sprite.flip_y ? -sh : sh,
                    draw_x, draw_y, draw_w, draw_h);
            } else {
                draw_scaled_sub_image(tex, 0, 0, sw, sh, draw_x, draw_y, draw_w, draw_h);
            }
            break;
        }
        case R2D_RECT:
            draw_set_color(item->rect.color);
            if (item->rect.filled) {
                draw_filled_rect(item->rect.x, item->rect.y, item->rect.w, item->rect.h);
            } else {
                draw_rect(item->rect.x, item->rect.y, item->rect.w, item->rect.h, item->rect.strength);
            }
            break;
        case R2D_CIRCLE:
            draw_set_color(item->circle.color);
            if (item->circle.filled) {
                draw_filled_circle(item->circle.cx, item->circle.cy, item->circle.radius, item->circle.segments);
            } else {
                draw_circle(item->circle.cx, item->circle.cy, item->circle.radius, item->circle.segments, item->circle.strength);
            }
            break;
        case R2D_LINE:
            draw_set_color(item->line.color);
            draw_line(item->line.x0, item->line.y0, item->line.x1, item->line.y1, item->line.strength);
            break;
        case R2D_TEXT:
            draw_set_color(item->text.color);
            if (item->text.font_path) {
                draw_font_t *font = data_get_font(item->text.font_path);
                if (font) {
                    draw_font_init(font);
                    draw_set_font(font, item->text.font_size);
                }
            }
            if (item->text.text) {
                draw_string(item->text.text, item->text.x, item->text.y);
            }
            break;
        }
    }
    draw_set_color(prev_color);
}
