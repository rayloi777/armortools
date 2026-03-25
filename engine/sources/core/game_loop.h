#pragma once

#include <stdint.h>

struct game_world_t;

void game_loop_init(struct game_world_t *world);
void game_loop_shutdown(void);

void game_loop_update(void);

float game_loop_get_delta_time(void);
float game_loop_get_time(void);
uint64_t game_loop_get_frame_count(void);
