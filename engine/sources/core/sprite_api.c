#include "sprite_api.h"
#include "../global.h"
#include <stdlib.h>
#include <string.h>

sprite_renderer_t **sprite_renderers = NULL;
int sprite_renderer_count = 0;
static int sprite_renderer_capacity = 0;

typedef struct sprite_renderer {
    gpu_texture_t *texture;
    float x, y;
    float scale_x, scale_y;
    float src_width, src_height;
    float pivot_x, pivot_y;
    bool flip_x, flip_y;
    int layer;
    float width, height;
    bool visible;
} sprite_renderer_t;

static void sprite_renderer_update_dimensions(sprite_renderer_t *sr) {
    if (sr) {
        sr->width = sr->src_width * sr->scale_x;
        sr->height = sr->src_height * sr->scale_y;
    }
}

sprite_renderer_t *sprite_renderer_create(void) {
    sprite_renderer_t *sr = malloc(sizeof(sprite_renderer_t));
    memset(sr, 0, sizeof(sprite_renderer_t));
    
    sr->texture = NULL;
    sr->x = 0;
    sr->y = 0;
    sr->scale_x = 1.0f;
    sr->scale_y = 1.0f;
    sr->src_width = 0;
    sr->src_height = 0;
    sr->pivot_x = 0.5f;
    sr->pivot_y = 0.5f;
    sr->flip_x = false;
    sr->flip_y = false;
    sr->layer = 0;
    sr->width = 0;
    sr->height = 0;
    sr->visible = true;
    
    if (sprite_renderer_capacity == 0) {
        sprite_renderer_capacity = 64;
        sprite_renderers = malloc(sizeof(sprite_renderer_t *) * sprite_renderer_capacity);
    }
    if (sprite_renderer_count >= sprite_renderer_capacity) {
        sprite_renderer_capacity *= 2;
        sprite_renderers = realloc(sprite_renderers, sizeof(sprite_renderer_t *) * sprite_renderer_capacity);
    }
    sprite_renderers[sprite_renderer_count++] = sr;
    
    return sr;
}

void sprite_renderer_destroy(sprite_renderer_t *sr) {
    if (!sr) return;
    
    for (int i = 0; i < sprite_renderer_count; i++) {
        if (sprite_renderers[i] == sr) {
            sprite_renderers[i] = sprite_renderers[--sprite_renderer_count];
            free(sr);
            return;
        }
    }
}

void sprite_renderer_set_texture(sprite_renderer_t *sr, const char *texture_name) {
    if (!sr || !texture_name) return;
    sr->texture = data_get_image((char*)texture_name);
    if (sr->texture) {
        sr->src_width = (float)sr->texture->width;
        sr->src_height = (float)sr->texture->height;
        sprite_renderer_update_dimensions(sr);
    }
}

void sprite_renderer_set_scale(sprite_renderer_t *sr, float scale_x, float scale_y) {
    if (!sr) return;
    sr->scale_x = scale_x;
    sr->scale_y = scale_y;
    sprite_renderer_update_dimensions(sr);
}

void sprite_renderer_set_position(sprite_renderer_t *sr, float x, float y) {
    if (!sr) return;
    sr->x = x;
    sr->y = y;
}

void sprite_renderer_set_flip(sprite_renderer_t *sr, bool flip_x, bool flip_y) {
    if (!sr) return;
    sr->flip_x = flip_x;
    sr->flip_y = flip_y;
}

void sprite_renderer_set_layer(sprite_renderer_t *sr, int layer) {
    if (!sr) return;
    sr->layer = layer;
}

void sprite_renderer_set_region(sprite_renderer_t *sr, float x, float y, float w, float h) {
    if (!sr) return;
    sr->src_width = w;
    sr->src_height = h;
    sprite_renderer_update_dimensions(sr);
}

void sprite_renderer_set_visible(sprite_renderer_t *sr, bool visible) {
    if (!sr) return;
    sr->visible = visible;
}

void sprite_renderer_set_pivot(sprite_renderer_t *sr, float pivot_x, float pivot_y) {
    if (!sr) return;
    sr->pivot_x = pivot_x;
    sr->pivot_y = pivot_y;
}

int sprite_renderer_get_layer(sprite_renderer_t *sr) {
    return sr ? sr->layer : 0;
}

float sprite_renderer_get_width(sprite_renderer_t *sr) {
    return sr ? sr->width : 0;
}

float sprite_renderer_get_height(sprite_renderer_t *sr) {
    return sr ? sr->height : 0;
}

bool sprite_renderer_is_visible(sprite_renderer_t *sr) {
    return sr ? sr->visible : false;
}

void sprite_renderer_draw(sprite_renderer_t *sr) {
    if (!sr || !sr->visible || !sr->texture) return;
    
    float draw_w = sr->width;
    float draw_h = sr->height;
    
    float draw_x = sr->x - (draw_w * sr->pivot_x);
    float draw_y = sr->y - (draw_h * sr->pivot_y);
    
    if (sr->flip_x || sr->flip_y) {
        draw_scaled_sub_image(sr->texture, 
                            sr->src_width - (sr->flip_x ? 0 : sr->src_width), 
                            sr->src_height - (sr->flip_y ? 0 : sr->src_height),
                            sr->flip_x ? -sr->src_width : sr->src_width,
                            sr->flip_y ? -sr->src_height : sr->src_height,
                            draw_x, draw_y, draw_w, draw_h);
    } else {
        draw_scaled_sub_image(sr->texture, 
                            0, 0, 
                            sr->src_width, sr->src_height,
                            draw_x, draw_y, draw_w, draw_h);
    }
}

static int sprite_layer_compare(const void *a, const void *b) {
    sprite_renderer_t *sr_a = *(sprite_renderer_t **)a;
    sprite_renderer_t *sr_b = *(sprite_renderer_t **)b;
    return sr_a->layer - sr_b->layer;
}

void sprite_renderer_sort_by_layer(void) {
    if (sprite_renderers && sprite_renderer_count > 1) {
        qsort(sprite_renderers, sprite_renderer_count, sizeof(sprite_renderer_t *), sprite_layer_compare);
    }
}

bool sprite_renderer_is_valid(sprite_renderer_t *sr) {
    if (!sr) return false;
    for (int i = 0; i < sprite_renderer_count; i++) {
        if (sprite_renderers[i] == sr) {
            return true;
        }
    }
    return false;
}

void sprite_renderer_shutdown(void) {
    if (!sprite_renderers) return;
    for (int i = 0; i < sprite_renderer_count; i++) {
        free(sprite_renderers[i]);
        sprite_renderers[i] = NULL;
    }
    free(sprite_renderers);
    sprite_renderers = NULL;
    sprite_renderer_count = 0;
    sprite_renderer_capacity = 0;
}
