#pragma once

#include <stdint.h>

struct game_world_t;

void render2d_bridge_set_world(struct game_world_t *world);
void render2d_bridge_init(void);
void render2d_bridge_shutdown(void);
void render2d_bridge_draw(void);
