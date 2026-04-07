#pragma once

#include <stdbool.h>

struct game_world_t;

void sys_3d_set_world(struct game_world_t *world);
void sys_3d_init(void);
void sys_3d_shutdown(void);
void sys_3d_draw(void);
bool sys_3d_was_rendered(void);
void sys_3d_reset_frame(void);
