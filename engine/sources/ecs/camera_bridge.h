#pragma once

#include <stdint.h>
#include "../core/camera2d.h"

struct game_world_t;

void camera_bridge_set_world(struct game_world_t *world);
void camera_bridge_init(void);
void camera_bridge_shutdown(void);
camera2d_t *camera_bridge_get_camera(void);
