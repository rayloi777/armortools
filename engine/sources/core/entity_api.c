#include "entity_api.h"
#include "ecs/ecs_world.h"
#include "flecs.h"
#include <stdio.h>
#include <string.h>

uint64_t entity_create(struct game_world_t *world) {
    if (!world || !world->world) return 0;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    return (uint64_t)ecs_new(ecs);
}

uint64_t entity_create_with_name(struct game_world_t *world, const char *name) {
    if (!world || !world->world || !name) return 0;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    (void)name;
    return (uint64_t)ecs_new(ecs);
}

void entity_destroy(struct game_world_t *world, uint64_t entity) {
    if (!world || !world->world || entity == 0) return;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    ecs_delete(ecs, (ecs_entity_t)entity);
}

void entity_add_component(struct game_world_t *world, uint64_t entity, uint64_t component_id) {
    if (!world || !world->world || entity == 0) return;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    ecs_add_id(ecs, (ecs_entity_t)entity, (ecs_id_t)component_id);
}

void entity_remove_component(struct game_world_t *world, uint64_t entity, uint64_t component_id) {
    if (!world || !world->world || entity == 0) return;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    ecs_remove_id(ecs, (ecs_entity_t)entity, (ecs_id_t)component_id);
}

bool entity_has_component(struct game_world_t *world, uint64_t entity, uint64_t component_id) {
    if (!world || !world->world || entity == 0) return false;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    return ecs_has_id(ecs, (ecs_entity_t)entity, (ecs_id_t)component_id);
}

void entity_set_component_data(struct game_world_t *world, uint64_t entity, uint64_t component_id, const void *data) {
    if (!world || !world->world || entity == 0 || !data) return;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    ecs_set_id(ecs, (ecs_entity_t)entity, (ecs_id_t)component_id, sizeof(void*), data);
}

void *entity_get_component_data(struct game_world_t *world, uint64_t entity, uint64_t component_id) {
    if (!world || !world->world || entity == 0) return NULL;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    return ecs_get_id(ecs, (ecs_entity_t)entity, (ecs_id_t)component_id);
}

bool entity_is_valid(struct game_world_t *world, uint64_t entity) {
    if (!world || !world->world || entity == 0) return false;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    return ecs_is_valid(ecs, (ecs_entity_t)entity);
}

bool entity_exists(struct game_world_t *world, uint64_t entity) {
    if (!world || !world->world || entity == 0) return false;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    return ecs_exists(ecs, (ecs_entity_t)entity);
}

void entity_api_register(void) {
    printf("Entity API registered\n");
}