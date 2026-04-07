#pragma once

typedef struct { float x, y, z; } comp_3d_position;
typedef struct { float x, y, z, w; } comp_3d_rotation;
typedef struct { float x, y, z; } comp_3d_scale;

void transform_init(void);
void transform_shutdown(void);
