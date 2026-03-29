#pragma once

#include <stdbool.h>

typedef struct gpu_texture gpu_texture_t;

gpu_texture_t *sprite_get_texture(const char *path);
void sprite_texture_cache_shutdown(void);
