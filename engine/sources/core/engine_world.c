#include "engine_world.h"

static game_world_t *g_engine_world = NULL;

void engine_world_set(game_world_t *world) {
    g_engine_world = world;
}

game_world_t *engine_world_get(void) {
    return g_engine_world;
}
