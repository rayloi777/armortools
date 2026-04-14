#include "transparent_bridge.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "flecs.h"
#include <iron.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static game_world_t *g_transparent_world = NULL;
static ecs_query_t *g_transparent_query = NULL;
static ecs_query_t *g_particle_query = NULL;

#define MAX_TRANSPARENT 128

typedef struct {
    ecs_entity_t entity;
    float        dist_sq;
} transparent_entry_t;

static transparent_entry_t g_sorted[MAX_TRANSPARENT];
static int g_sorted_count = 0;

void transparent_bridge_init(game_world_t *world) {
    g_transparent_world = world;
    if (!world) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(world);

    ecs_query_desc_t desc = {0};
    desc.terms[0].id = ecs_component_comp_3d_transparency();
    desc.terms[1].id = ecs_component_comp_3d_mesh_renderer();
    desc.terms[2].id = ecs_component_RenderObject3D();
    desc.terms[3].id = ecs_component_comp_3d_position();
    g_transparent_query = ecs_query_init(ecs, &desc);

    ecs_query_desc_t pdesc = {0};
    pdesc.terms[0].id = ecs_component_comp_3d_particle();
    pdesc.terms[1].id = ecs_component_comp_3d_mesh_renderer();
    pdesc.terms[2].id = ecs_component_RenderObject3D();
    pdesc.terms[3].id = ecs_component_comp_3d_position();
    g_particle_query = ecs_query_init(ecs, &pdesc);

    printf("Transparent Bridge initialized\n");
}

void transparent_bridge_shutdown(void) {
    if (g_transparent_query) { ecs_query_fini(g_transparent_query); g_transparent_query = NULL; }
    if (g_particle_query) { ecs_query_fini(g_particle_query); g_particle_query = NULL; }
    g_transparent_world = NULL;
    printf("Transparent Bridge shutdown\n");
}

// Set visible=false on all transparent mesh Iron objects so they're skipped
// during the G-buffer (opaque) pass.
void transparent_bridge_hide(void) {
    if (!g_transparent_world || !g_transparent_query) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_transparent_world);
    if (!ecs) return;

    ecs_iter_t it = ecs_query_iter(ecs, g_transparent_query);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            const RenderObject3D *robj = ecs_get_id(ecs, it.entities[i], ecs_component_RenderObject3D());
            if (!robj || !robj->iron_mesh_object) continue;
            object_t *base = (object_t *)robj->iron_mesh_object;
            base->visible = false;
        }
    }
}

// Restore visible=true on all transparent mesh Iron objects, then render them
// sorted back-to-front via render_path_submit_draw. Also hides opaque meshes
// so only transparent ones draw, then restores all visibility.
void transparent_bridge_render(void) {
    if (!g_transparent_world || !g_transparent_query) return;
    if (!scene_camera || !scene_camera->base || !scene_camera->base->transform) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_transparent_world);
    if (!ecs) return;

    vec4_t cam_pos = scene_camera->base->transform->loc;

    // Step 1: Show transparent meshes, collect & sort
    g_sorted_count = 0;
    ecs_iter_t it = ecs_query_iter(ecs, g_transparent_query);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count && g_sorted_count < MAX_TRANSPARENT; i++) {
            const comp_3d_position *pos = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_position());
            const RenderObject3D *robj = ecs_get_id(ecs, it.entities[i], ecs_component_RenderObject3D());
            if (!pos || !robj || !robj->iron_mesh_object) continue;

            const comp_3d_mesh_renderer *mr = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_mesh_renderer());
            if (mr && !mr->visible) continue;

            // Show this transparent mesh
            object_t *base = (object_t *)robj->iron_mesh_object;
            base->visible = true;

            float dx = pos->x - cam_pos.x;
            float dy = pos->y - cam_pos.y;
            float dz = pos->z - cam_pos.z;

            g_sorted[g_sorted_count].entity = it.entities[i];
            g_sorted[g_sorted_count].dist_sq = dx * dx + dy * dy + dz * dz;
            g_sorted_count++;
        }
    }

    if (g_sorted_count == 0) return;

    // Step 2: Hide opaque meshes so only transparent ones render
    for (int i = 0; i < (int)scene_meshes->length; i++) {
        mesh_object_t *m = (mesh_object_t *)scene_meshes->buffer[i];
        bool is_transparent = false;
        // Check if this mesh belongs to a transparent entity
        for (int j = 0; j < g_sorted_count; j++) {
            const RenderObject3D *robj = ecs_get_id(ecs, g_sorted[j].entity, ecs_component_RenderObject3D());
            if (robj && robj->iron_mesh_object == m->base) {
                is_transparent = true;
                break;
            }
        }
        if (!is_transparent) {
            m->base->visible = false;
        }
    }

    // Step 3: Render only transparent meshes
    render_path_submit_draw("mesh");

    // Step 4: Restore all meshes to visible
    for (int i = 0; i < (int)scene_meshes->length; i++) {
        mesh_object_t *m = (mesh_object_t *)scene_meshes->buffer[i];
        m->base->visible = true;
    }
}

void particle_bridge_update(float dt) {
    if (!g_transparent_world || !g_particle_query) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_transparent_world);
    if (!ecs) return;

    ecs_iter_t it = ecs_query_iter(ecs, g_particle_query);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            comp_3d_particle *p = (comp_3d_particle *)ecs_get_mut_id(ecs, it.entities[i], ecs_component_comp_3d_particle());
            if (!p || !p->alive) continue;

            p->lifetime -= dt;
            if (p->lifetime <= 0.0f) {
                p->alive = false;
                p->lifetime = 0.0f;
                continue;
            }

            comp_3d_position *pos = (comp_3d_position *)ecs_get_mut_id(ecs, it.entities[i], ecs_component_comp_3d_position());
            if (pos) {
                pos->x += p->velocity_x * dt;
                pos->y += p->velocity_y * dt;
                pos->z += p->velocity_z * dt;
            }
        }
    }
}
