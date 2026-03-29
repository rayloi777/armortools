#include "sprite_api.h"
#include "../global.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    char path[256];
    gpu_texture_t *texture;
} texture_cache_entry_t;

static texture_cache_entry_t *g_texture_cache = NULL;
static int g_texture_cache_count = 0;
static int g_texture_cache_capacity = 0;

gpu_texture_t *sprite_get_texture(const char *path) {
    if (!path) return NULL;
    for (int i = 0; i < g_texture_cache_count; i++) {
        if (strcmp(g_texture_cache[i].path, path) == 0) {
            return g_texture_cache[i].texture;
        }
    }
    gpu_texture_t *tex = data_get_image((char *)path);
    if (!tex) return NULL;
    if (g_texture_cache_count >= g_texture_cache_capacity) {
        g_texture_cache_capacity = g_texture_cache_capacity == 0 ? 16 : g_texture_cache_capacity * 2;
        g_texture_cache = realloc(g_texture_cache, sizeof(texture_cache_entry_t) * g_texture_cache_capacity);
    }
    strncpy(g_texture_cache[g_texture_cache_count].path, path, 255);
    g_texture_cache[g_texture_cache_count].path[255] = '\0';
    g_texture_cache[g_texture_cache_count].texture = tex;
    g_texture_cache_count++;
    return tex;
}

void sprite_texture_cache_shutdown(void) {
    free(g_texture_cache);
    g_texture_cache = NULL;
    g_texture_cache_count = 0;
    g_texture_cache_capacity = 0;
}
