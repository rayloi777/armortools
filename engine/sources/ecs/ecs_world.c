#include "ecs_world.h"
#include "ecs_components.h"
#include <stdlib.h>
#include <stdio.h>

game_world_t *game_world_create(void) {
    game_world_t *gw = malloc(sizeof(game_world_t));
    if (!gw) return NULL;
    
    gw->world = NULL;
    gw->delta_time = 0.0f;
    gw->time = 0.0f;
    gw->frame_count = 0;
    
    printf("Game World Created (stub ECS)\n");
    (void)ecs_register_components;
    return gw;
}

void game_world_destroy(game_world_t *world) {
    if (!world) return;
    printf("Game World Destroyed\n");
    free(world);
}

void game_world_progress(game_world_t *world, float dt) {
    if (!world) return;
    world->delta_time = dt;
    world->time += dt;
    world->frame_count++;
}

void *game_world_get_ecs(game_world_t *world) {
    (void)world;
    return NULL;
}
