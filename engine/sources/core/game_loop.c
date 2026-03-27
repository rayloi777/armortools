#include "game_loop.h"
#include "ecs/ecs_world.h"
#include "minic_system.h"
#include <stdio.h>
#include <stdbool.h>

extern float sys_delta(void);

static game_world_t *g_loop_world = NULL;
static float g_delta_time = 0.0f;
static float g_time = 0.0f;
static uint64_t g_frame_count = 0;

void game_loop_init(game_world_t *world) {
    g_loop_world = world;
    g_delta_time = 0.0f;
    g_time = 0.0f;
    g_frame_count = 0;
}

void game_loop_shutdown(void) {
    g_loop_world = NULL;
}

void game_loop_update(void) {
    if (!g_loop_world) return;
    
    g_delta_time = sys_delta();
    g_time += g_delta_time;
    g_frame_count++;
    
    game_world_progress(g_loop_world, g_delta_time);
    
    minic_system_call_update();
}

float game_loop_get_delta_time(void) {
    return g_delta_time;
}

float game_loop_get_time(void) {
    return g_time;
}

uint64_t game_loop_get_frame_count(void) {
    return g_frame_count;
}
