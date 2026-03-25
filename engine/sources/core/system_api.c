#include "system_api.h"
#include "ecs/ecs_world.h"
#include "flecs.h"
#include <stdio.h>
#include <stdlib.h>

static ecs_entity_t phase_to_flecs_phase(system_phase_t phase) {
    switch (phase) {
        case SYSTEM_PHASE_PRE_UPDATE: return EcsPreUpdate;
        case SYSTEM_PHASE_UPDATE: return EcsOnUpdate;
        case SYSTEM_PHASE_POST_UPDATE: return EcsPostUpdate;
        case SYSTEM_PHASE_INIT: return EcsOnStart;
        default: return EcsOnUpdate;
    }
}

uint64_t system_create(
    struct game_world_t *world,
    const char *name,
    system_phase_t phase,
    const char *query,
    system_callback_t callback
) {
    if (!world || !world->world || !name || !callback) return 0;
    (void)phase;
    (void)query;
    printf("System created: %s\n", name);
    return 0;
}

void system_destroy(struct game_world_t *world, uint64_t system_id) {
    (void)world;
    (void)system_id;
}

void system_enable(struct game_world_t *world, uint64_t system_id, bool enabled) {
    (void)world;
    (void)system_id;
    (void)enabled;
}

bool system_is_enabled(struct game_world_t *world, uint64_t system_id) {
    (void)world;
    (void)system_id;
    return true;
}

void system_set_context(struct game_world_t *world, uint64_t system_id, void *ctx) {
    (void)world;
    (void)system_id;
    (void)ctx;
}

void *system_get_context(struct game_world_t *world, uint64_t system_id) {
    (void)world;
    (void)system_id;
    return NULL;
}

void system_api_register(void) {
    printf("System API registered\n");
}