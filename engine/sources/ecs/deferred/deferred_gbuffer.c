#include "deferred_gbuffer.h"
#include <iron.h>
#include <stdio.h>

static gbuffer_t g_gbuffer = {0};

void gbuffer_init(int width, int height) {
    if (g_gbuffer.initialized) gbuffer_shutdown();

    g_gbuffer.gbuffer0 = gpu_create_render_target(width, height, GPU_TEXTURE_FORMAT_RGBA64);
    g_gbuffer.gbuffer1 = gpu_create_render_target(width, height, GPU_TEXTURE_FORMAT_RGBA64);
    g_gbuffer.depth_target = gpu_create_render_target(width, height, GPU_TEXTURE_FORMAT_D32);

    g_gbuffer.width = width;
    g_gbuffer.height = height;
    g_gbuffer.initialized = true;

    printf("G-Buffer initialized: %dx%d\n", width, height);
}

void gbuffer_resize(int width, int height) {
    if (!g_gbuffer.initialized) return;
    if (g_gbuffer.width == width && g_gbuffer.height == height) return;
    gbuffer_shutdown();
    gbuffer_init(width, height);
}

void gbuffer_shutdown(void) {
    if (!g_gbuffer.initialized) return;
    g_gbuffer.gbuffer0 = NULL;
    g_gbuffer.gbuffer1 = NULL;
    g_gbuffer.depth_target = NULL;
    g_gbuffer.initialized = false;
    printf("G-Buffer shutdown\n");
}

gbuffer_t *gbuffer_get(void) {
    return &g_gbuffer;
}
