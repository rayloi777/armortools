#pragma once

#include <stdint.h>

void ecs_bridge_init(void);
void ecs_bridge_shutdown(void);

void ecs_bridge_sync_transform(uint64_t entity);
void ecs_bridge_create_render_object(uint64_t entity);
void ecs_bridge_destroy_render_object(uint64_t entity);
