#include "scene_api.h"
#include "asset_loader.h"
#include "ecs/ecs_world.h"
#include <stdio.h>
#include <stdint.h>

// Minic value types
#include <minic.h>

static game_world_t *g_scene_api_world = NULL;

void scene_api_set_world(game_world_t *world) {
    g_scene_api_world = world;
}

static minic_val_t minic_scene_load(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_PTR) {
        fprintf(stderr, "scene_load: expected string path argument\n");
        return minic_val_int(0);
    }
    const char *path = (const char *)args[0].p;
    uint64_t root = asset_loader_load_scene(path);
    return minic_val_id(root);
}

static minic_val_t minic_mesh_load(minic_val_t *args, int argc) {
    if (argc < 3) {
        fprintf(stderr, "mesh_load: expected 3 arguments (entity, mesh_path, material_path)\n");
        return minic_val_int(-1);
    }

    uint64_t entity = 0;
    if (args[0].type == MINIC_T_ID) {
        entity = args[0].u64;
    }
    else {
        entity = (uint64_t)minic_val_to_d(args[0]);
    }

    const char *mesh_path = (args[1].type == MINIC_T_PTR) ? (const char *)args[1].p : NULL;
    const char *mat_path = (args[2].type == MINIC_T_PTR) ? (const char *)args[2].p : NULL;

    int result = asset_loader_load_mesh(entity, mesh_path, mat_path);
    return minic_val_int(result);
}

void scene_api_register(void) {
    minic_register_native("scene_load", minic_scene_load);
    minic_register_native("mesh_load", minic_mesh_load);
}
