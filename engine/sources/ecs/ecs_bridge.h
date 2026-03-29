#pragma once

#include <stdint.h>

struct game_world_t;

void ecs_bridge_set_world(struct game_world_t *world);
void ecs_bridge_init(void);
void ecs_bridge_shutdown(void);
struct game_world_t *ecs_bridge_get_world(void);

void ecs_bridge_sync_transform(uint64_t entity);
void ecs_bridge_create_render_object(uint64_t entity);
void ecs_bridge_destroy_render_object(uint64_t entity);
