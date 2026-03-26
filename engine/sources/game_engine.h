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

typedef struct minic_ctx_s minic_ctx_t;
minic_ctx_t *game_engine_get_minic_ctx(void);
