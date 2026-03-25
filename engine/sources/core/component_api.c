#include "component_api.h"
#include "ecs/ecs_world.h"
#include "flecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint64_t component_register(struct game_world_t *world, const component_desc_t *desc) {
    if (!world || !world->world || !desc || !desc->name) return 0;
    (void)desc;
    printf("Component registered: %s\n", desc->name);
    return 0;
}

uint64_t component_get_id(struct game_world_t *world, const char *name) {
    if (!world || !world->world || !name) return 0;
    (void)name;
    return 0;
}

bool component_exists(struct game_world_t *world, uint64_t component_id) {
    if (!world || !world->world || component_id == 0) return false;
    (void)component_id;
    return false;
}

size_t component_get_size(uint64_t component_id) {
    (void)component_id;
    return 0;
}

size_t component_get_alignment(uint64_t component_id) {
    (void)component_id;
    return 0;
}

void component_api_register(void) {
    printf("Component API registered\n");
}