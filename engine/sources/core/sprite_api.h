#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct sprite_renderer sprite_renderer_t;

extern int sprite_renderer_count;
extern sprite_renderer_t **sprite_renderers;

sprite_renderer_t *sprite_renderer_create(void);
void sprite_renderer_destroy(sprite_renderer_t *sr);
void sprite_renderer_draw(sprite_renderer_t *sr);
void sprite_renderer_set_texture(sprite_renderer_t *sr, const char *texture_name);
void sprite_renderer_set_scale(sprite_renderer_t *sr, float scale_x, float scale_y);
void sprite_renderer_set_position(sprite_renderer_t *sr, float x, float y);
void sprite_renderer_set_flip(sprite_renderer_t *sr, bool flip_x, bool flip_y);
void sprite_renderer_set_layer(sprite_renderer_t *sr, int layer);
void sprite_renderer_set_region(sprite_renderer_t *sr, float x, float y, float w, float h);
void sprite_renderer_set_visible(sprite_renderer_t *sr, bool visible);
void sprite_renderer_set_pivot(sprite_renderer_t *sr, float pivot_x, float pivot_y);

int sprite_renderer_get_layer(sprite_renderer_t *sr);
float sprite_renderer_get_width(sprite_renderer_t *sr);
float sprite_renderer_get_height(sprite_renderer_t *sr);
bool sprite_renderer_is_visible(sprite_renderer_t *sr);

void sprite_renderer_sort_by_layer(void);
bool sprite_renderer_is_valid(sprite_renderer_t *sr);
void sprite_renderer_shutdown(void);
