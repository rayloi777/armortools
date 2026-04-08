#pragma once

#include <stdbool.h>

typedef struct {
    float fov;
    float near_plane;
    float far_plane;
    bool  perspective;
    bool  active;
    float ortho_left, ortho_right, ortho_bottom, ortho_top;
} comp_3d_camera;

void camera_init(void);
void camera_shutdown(void);
