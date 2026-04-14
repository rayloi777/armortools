#pragma once

#include <stdbool.h>

// Blend modes for transparent rendering
#define BLEND_MODE_ALPHA    0
#define BLEND_MODE_ADDITIVE 1

typedef struct {
    float opacity;       // 0.0 = invisible, 1.0 = fully opaque
    int   blend_mode;    // BLEND_MODE_ALPHA or BLEND_MODE_ADDITIVE
    bool  two_sided;     // render both faces
} comp_3d_transparency;

typedef struct {
    float lifetime;        // remaining seconds
    float max_lifetime;    // initial lifetime
    float velocity_x, velocity_y, velocity_z;
    float size;
    float color_r, color_g, color_b, color_a;
    bool  alive;
} comp_3d_particle;
