#pragma once

#include <stdint.h>

struct game_world_t;

void camera_bridge_3d_set_world(struct game_world_t *world);
void camera_bridge_3d_init(void);
void camera_bridge_3d_shutdown(void);
void camera_bridge_3d_update(void);
void *camera_bridge_3d_get_active(void);
