#pragma once

#include <stdbool.h>

struct gpu_texture_t;

typedef struct {
    struct gpu_texture_t *gbuffer0;    // RGBA64: Normal.xy (oct) + Roughness + Metallic
    struct gpu_texture_t *gbuffer1;    // RGBA64: Albedo.rgb + AO
    struct gpu_texture_t *depth_target; // D32 depth buffer
    int width;
    int height;
    bool initialized;
} gbuffer_t;

void gbuffer_init(int width, int height);
void gbuffer_resize(int width, int height);
void gbuffer_shutdown(void);
gbuffer_t *gbuffer_get(void);
