#pragma once

#include <stdint.h>

typedef struct game_world_t {
    void *world;
    float delta_time;
    float time;
    uint64_t frame_count;
} game_world_t;

game_world_t *game_world_create(void);
void game_world_destroy(game_world_t *world);
void game_world_progress(game_world_t *world, float dt);

void *game_world_get_ecs(game_world_t *world);
