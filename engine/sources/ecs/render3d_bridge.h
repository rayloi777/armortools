#pragma once

#include <stdbool.h>

struct game_world_t;

typedef struct {
    int mesh_total;
    int mesh_rendered;
    int mesh_culled;
    int mesh_shadow;
    int pass_count;
} render3d_stats_t;

void sys_3d_set_world(struct game_world_t *world);
void sys_3d_init(void);
void sys_3d_shutdown(void);
void sys_3d_draw(void);
bool sys_3d_was_rendered(void);
void sys_3d_reset_frame(void);
void sys_3d_set_debug_mode(int mode);  // 0=Normal, 1=Depth, 2=Albedo, 3=RoughMet
int  sys_3d_get_debug_mode(void);
render3d_stats_t sys_3d_get_stats(void);
const char *sys_3d_stat_string(void);
