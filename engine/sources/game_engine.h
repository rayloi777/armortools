#pragma once

#include <stdint.h>

struct game_world_t;

void game_engine_init(void);
void game_engine_shutdown(void);
void game_engine_start(void);

struct game_world_t *game_engine_get_world(void);
float game_engine_get_delta_time(void);
float game_engine_get_time(void);
uint64_t game_engine_get_frame_count(void);
