#include "lod_bridge.h"
#include "../ecs_world.h"
#include "../ecs_components.h"
#include "flecs.h"
#include <iron.h>
#include <stdio.h>
#include <math.h>

static game_world_t *g_lod_world = NULL;
static ecs_query_t *g_lod_query = NULL;

void lod_bridge_init(game_world_t *world) {
    g_lod_world = world;
    if (!world) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(world);

    ecs_query_desc_t desc = {0};
    desc.terms[0].id = ecs_component_comp_3d_lod();
    desc.terms[1].id = ecs_component_comp_3d_position();
    g_lod_query = ecs_query_init(ecs, &desc);

    printf("LOD Bridge initialized\n");
}

void lod_bridge_shutdown(void) {
    if (g_lod_query) { ecs_query_fini(g_lod_query); g_lod_query = NULL; }
    g_lod_world = NULL;
    printf("LOD Bridge shutdown\n");
}

void lod_bridge_update(void) {
    if (!g_lod_world || !g_lod_query) return;
    if (!scene_camera || !scene_camera->base || !scene_camera->base->transform) return;

    // Camera position
    vec4_t cam_pos = scene_camera->base->transform->loc;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_lod_world);

    ecs_iter_t it = ecs_query_iter(ecs, g_lod_query);
    while (ecs_query_next(&it)) {
        comp_3d_lod *lods = ecs_get_id(ecs, it.entities[0], ecs_component_comp_3d_lod());
        // Re-fetch for each entity
        for (int i = 0; i < it.count; i++) {
            const comp_3d_position *pos = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_position());
            comp_3d_lod *lod = (comp_3d_lod *)ecs_get_mut_id(ecs, it.entities[i], ecs_component_comp_3d_lod());
            if (!pos || !lod) continue;

            // Distance from camera to entity
            float dx = pos->x - cam_pos.x;
            float dy = pos->y - cam_pos.y;
            float dz = pos->z - cam_pos.z;
            float dist = sqrtf(dx * dx + dy * dy + dz * dz);

            // Select LOD level
            int new_lod = 0;
            if (dist > lod->dist2) new_lod = 2;
            else if (dist > lod->dist1) new_lod = 1;
            else new_lod = 0;

            if (new_lod != lod->active_lod) {
                lod->active_lod = new_lod;

                // Update the mesh_renderer's mesh path based on LOD level
                const comp_3d_mesh_renderer *mesh = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_mesh_renderer());
                if (!mesh) continue;

                // Note: actual mesh switching would require mesh_bridge_3d to
                // re-load the mesh object. For now, just update the active_lod
                // value so scripts and debug overlay can read it.
            }
        }
    }
}
