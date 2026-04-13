#pragma once

#include <stdbool.h>
#include <iron.h>

typedef struct {
    gpu_texture_t *shadow_map;     // RGBA32 color texture with encoded depth
    gpu_texture_t *shadow_depth;   // D32 depth buffer for depth testing
    int size;
    bool initialized;
} shadow_data_t;

void shadow_init(int size);
void shadow_shutdown(void);
shadow_data_t *shadow_get(void);
