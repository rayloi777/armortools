#pragma once

#include <stdbool.h>

typedef struct {
    float fov;
    float near_plane;
    float far_plane;
    bool  perspective;
    bool  active;
} comp_3d_camera;

void camera_init(void);
void camera_shutdown(void);
