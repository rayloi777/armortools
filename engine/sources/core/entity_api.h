#pragma once

#include <stdint.h>
#include <stdbool.h>

struct game_world_t;

uint64_t entity_create(struct game_world_t *world);
uint64_t entity_create_with_name(struct game_world_t *world, const char *name);
void entity_destroy(struct game_world_t *world, uint64_t entity);

void entity_add_component(struct game_world_t *world, uint64_t entity, uint64_t component_id);
void entity_remove_component(struct game_world_t *world, uint64_t entity, uint64_t component_id);
bool entity_has_component(struct game_world_t *world, uint64_t entity, uint64_t component_id);

void entity_set_component_data(struct game_world_t *world, uint64_t entity, uint64_t component_id, const void *data);
void *entity_get_component_data(struct game_world_t *world, uint64_t entity, uint64_t component_id);

bool entity_is_valid(struct game_world_t *world, uint64_t entity);
bool entity_exists(struct game_world_t *world, uint64_t entity);

const char *entity_get_name(struct game_world_t *world, uint64_t entity);
void entity_set_name(struct game_world_t *world, uint64_t entity, const char *name);
uint64_t entity_get_parent(struct game_world_t *world, uint64_t entity);
void entity_set_parent(struct game_world_t *world, uint64_t child, uint64_t parent);

void entity_api_register(void);