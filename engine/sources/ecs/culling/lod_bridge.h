#pragma once

#include "../ecs_world.h"

void lod_bridge_init(game_world_t *world);
void lod_bridge_shutdown(void);
void lod_bridge_update(void);  // Called each frame to update LOD levels
