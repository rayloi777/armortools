#include "shadow_directional.h"
#include <stdio.h>

static shadow_data_t g_shadow = {0};

void shadow_init(int size) {
    if (g_shadow.initialized) shadow_shutdown();

    g_shadow.size = size;
    g_shadow.shadow_map = gpu_create_render_target(size, size, GPU_TEXTURE_FORMAT_RGBA64);
    g_shadow.shadow_depth = gpu_create_render_target(size, size, GPU_TEXTURE_FORMAT_D32);
    g_shadow.initialized = true;

    printf("Shadow: initialized (single directional, %dx%d)\n", size, size);
}

void shadow_shutdown(void) {
    if (!g_shadow.initialized) return;
    g_shadow.shadow_map = NULL;
    g_shadow.shadow_depth = NULL;
    g_shadow.size = 0;
    g_shadow.initialized = false;
    printf("Shadow: shutdown\n");
}

shadow_data_t *shadow_get(void) {
    return &g_shadow;
}
