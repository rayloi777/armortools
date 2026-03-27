#pragma once

#include <stdint.h>

typedef struct sprite_renderer sprite_renderer_t;

sprite_renderer_t *sprite_renderer_create(void);
void sprite_renderer_destroy(sprite_renderer_t *sr);
void sprite_renderer_draw(sprite_renderer_t *sr);
void sprite_renderer_set_texture(sprite_renderer_t *sr, const char *texture_name);
void sprite_renderer_set_scale(sprite_renderer_t *sr, float scale_x, float scale_y);
void sprite_renderer_set_position(sprite_renderer_t *sr, float x, float y);
