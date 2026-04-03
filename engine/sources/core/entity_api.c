#include "entity_api.h"
#include "ecs/ecs_world.h"
#include "ecs/ecs_dynamic.h"
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
    ecs_entity_t e = ecs_new(ecs);
    ecs_set_name(ecs, e, name);
    return (uint64_t)e;
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
    if (!ecs_is_alive(ecs, (ecs_entity_t)entity)) return false;
    return ecs_has_id(ecs, (ecs_entity_t)entity, (ecs_id_t)component_id);
}

void entity_set_component_data(struct game_world_t *world, uint64_t entity, uint64_t component_id, const void *data) {
    if (!world || !world->world || entity == 0 || !data) return;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    
    dynamic_component_t *dc = ecs_dynamic_component_get(component_id);
    if (dc && dc->size > 0) {
        ecs_set_id(ecs, (ecs_entity_t)entity, (ecs_id_t)component_id, dc->size, data);
    }
}

void *entity_get_component_data(struct game_world_t *world, uint64_t entity, uint64_t component_id) {
    if (!world || !world->world || entity == 0) return NULL;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    if (!ecs_is_alive(ecs, (ecs_entity_t)entity)) return NULL;
    /* Uses ecs_get_id because Minic scripts read AND write through this pointer
       via comp_set_float/comp_get_float. The const cast is intentional.
       ecs_get_id is safe here — in this Flecs build it does NOT trigger
       archetype moves or OnSet hooks; it just looks up the component pointer. */
    return (void *)ecs_get_id(ecs, (ecs_entity_t)entity, (ecs_id_t)component_id);
}

bool entity_is_valid(struct game_world_t *world, uint64_t entity) {
    if (!world || !world->world || entity == 0) return false;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    return ecs_is_valid(ecs, (ecs_entity_t)entity);
}

bool entity_exists(struct game_world_t *world, uint64_t entity) {
    if (!world || !world->world || entity == 0) return false;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    return ecs_is_alive(ecs, (ecs_entity_t)entity);
}

const char *entity_get_name(struct game_world_t *world, uint64_t entity) {
    if (!world || !world->world || entity == 0) return NULL;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    return ecs_get_name(ecs, (ecs_entity_t)entity);
}

void entity_set_name(struct game_world_t *world, uint64_t entity, const char *name) {
    if (!world || !world->world || entity == 0 || !name) return;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    ecs_set_name(ecs, (ecs_entity_t)entity, name);
}

uint64_t entity_find_by_name(struct game_world_t *world, const char *name) {
    if (!world || !world->world || !name) return 0;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    ecs_entity_t e = ecs_lookup(ecs, name);
    return (uint64_t)e;
}

uint64_t entity_get_parent(struct game_world_t *world, uint64_t entity) {
    if (!world || !world->world || entity == 0) return 0;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    ecs_entity_t parent = ecs_get_target(ecs, (ecs_entity_t)entity, EcsChildOf, 0);
    return (uint64_t)parent;
}

void entity_set_parent(struct game_world_t *world, uint64_t child, uint64_t parent) {
    if (!world || !world->world || child == 0) return;
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    if (parent == 0) {
        ecs_entity_t current_parent = ecs_get_target(ecs, (ecs_entity_t)child, EcsChildOf, 0);
        if (current_parent != 0) {
            ecs_remove_pair(ecs, (ecs_entity_t)child, EcsChildOf, current_parent);
        }
    } else {
        ecs_add_pair(ecs, (ecs_entity_t)child, EcsChildOf, (ecs_entity_t)parent);
    }
}

void entity_api_register(void) {
    printf("Entity API registered\n");
}
