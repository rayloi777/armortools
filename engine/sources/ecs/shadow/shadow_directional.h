#pragma once

#include <stdbool.h>
#include <iron.h>

typedef struct {
    gpu_texture_t *shadow_map;  // Single D32 texture (2048x2048)
    int size;
    bool initialized;
} shadow_data_t;

void shadow_init(int size);
void shadow_shutdown(void);
shadow_data_t *shadow_get(void);
