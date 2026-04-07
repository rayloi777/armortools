#pragma once

#include <stdint.h>

struct game_world_t;

void mesh_bridge_3d_set_world(struct game_world_t *world);
void mesh_bridge_3d_init(void);
void mesh_bridge_3d_shutdown(void);
void mesh_bridge_3d_sync_transforms(void);
void mesh_bridge_3d_create_mesh(uint64_t entity);
void mesh_bridge_3d_destroy_mesh(uint64_t entity);
