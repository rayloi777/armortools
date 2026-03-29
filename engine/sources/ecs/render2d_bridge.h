#pragma once

#include <stdint.h>

struct game_world_t;

void sys_2d_set_world(struct game_world_t *world);
void sys_2d_init(void);
void sys_2d_shutdown(void);
void sys_2d_draw(void);
