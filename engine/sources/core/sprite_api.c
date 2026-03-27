#include "sprite_api.h"
#include "../global.h"
#include <stdlib.h>

typedef struct sprite_renderer {
    gpu_texture_t *texture;
    float x, y;
    float scale_x, scale_y;
    float width, height;
    bool visible;
} sprite_renderer_t;

sprite_renderer_t *sprite_renderer_create(void) {
    sprite_renderer_t *sr = malloc(sizeof(sprite_renderer_t));
    sr->texture = NULL;
    sr->x = 0;
    sr->y = 0;
    sr->scale_x = 1.0f;
    sr->scale_y = 1.0f;
    sr->width = 0;
    sr->height = 0;
    sr->visible = true;
    return sr;
}

void sprite_renderer_destroy(sprite_renderer_t *sr) {
    if (sr) {
        free(sr);
    }
}

void sprite_renderer_set_texture(sprite_renderer_t *sr, const char *texture_name) {
    if (!sr || !texture_name) return;
    sr->texture = data_get_image((char*)texture_name);
    if (sr->texture) {
        sr->width = (float)sr->texture->width;
        sr->height = (float)sr->texture->height;
    }
}

void sprite_renderer_set_scale(sprite_renderer_t *sr, float scale_x, float scale_y) {
    if (!sr) return;
    sr->scale_x = scale_x;
    sr->scale_y = scale_y;
}

void sprite_renderer_set_position(sprite_renderer_t *sr, float x, float y) {
    if (!sr) return;
    sr->x = x;
    sr->y = y;
}

void sprite_renderer_draw(sprite_renderer_t *sr) {
    if (!sr || !sr->visible || !sr->texture) return;
    
    float draw_w = sr->width * sr->scale_x;
    float draw_h = sr->height * sr->scale_y;
    
    draw_scaled_image(sr->texture, sr->x, sr->y, draw_w, draw_h);
}
