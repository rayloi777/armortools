#include "ecs_world.h"
#include "ecs_components.h"
#include "flecs/flecs.h"
#include <stdlib.h>
#include <stdio.h>

game_world_t *game_world_create(void) {
    game_world_t *gw = malloc(sizeof(game_world_t));
    if (!gw) return NULL;
    
    ecs_world_t *world = ecs_init();
    if (!world) {
        free(gw);
        return NULL;
    }
    
    ecs_register_components(world);
    
    gw->world = world;
    gw->delta_time = 0.0f;
    gw->time = 0.0f;
    gw->frame_count = 0;
    
    printf("Game World Created with Flecs ECS v4.0.0\n");
    return gw;
}

void game_world_destroy(game_world_t *world) {
    if (!world) return;
    if (world->world) {
        ecs_fini(world->world);
    }
    printf("Game World Destroyed\n");
    free(world);
}

void game_world_progress(game_world_t *world, float dt) {
    if (!world) return;
    ecs_progress(world->world, dt);
    world->delta_time = dt;
    world->time += dt;
    world->frame_count++;
}

void *game_world_get_ecs(game_world_t *world) {
    return world ? world->world : NULL;
}
