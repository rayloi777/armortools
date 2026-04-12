#pragma once

#include <stdbool.h>

struct game_world_t;

void sys_3d_set_world(struct game_world_t *world);
void sys_3d_init(void);
void sys_3d_shutdown(void);
void sys_3d_draw(void);
bool sys_3d_was_rendered(void);
void sys_3d_reset_frame(void);
void sys_3d_set_debug_mode(int mode);  // 0=Normal, 1=Depth, 2=Albedo, 3=RoughMet
int  sys_3d_get_debug_mode(void);
