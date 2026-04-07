#pragma once

#include <stdbool.h>

typedef struct {
    char *mesh_path;
    char *material_path;
    bool  visible;
} comp_3d_mesh_renderer;

void mesh_renderer_init(void);
void mesh_renderer_shutdown(void);
