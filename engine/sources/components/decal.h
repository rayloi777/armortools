#pragma once

typedef struct {
    float size_x, size_y, size_z;  // AABB half-extents for projection volume
    float opacity;
    float color_r, color_g, color_b;
    int   channel;       // which render channel to project onto (0 = gbuffer0)
} comp_3d_decal;
