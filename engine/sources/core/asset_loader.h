#pragma once

#include <stdint.h>

struct game_world_t;

void asset_loader_set_world(struct game_world_t *world);
uint64_t asset_loader_load_scene(const char *path);
int asset_loader_load_mesh(uint64_t entity, const char *mesh_path, const char *material_path);
