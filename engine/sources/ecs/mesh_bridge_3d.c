#include "mesh_bridge_3d.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "flecs.h"
#include <iron.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Compute a tight bounding sphere radius from mesh vertex positions using Ritter's algorithm.
// The vertex buffer stores INT16-packed positions with stride 4 (x,y,z,w).
// To get world-space coordinates: divide by 32767 and multiply by scale_pos.
// Guarded for unity build: scene_3d_api.c defines the same function later.
#ifndef HAVE_BOUNDING_SPHERE_RADIUS
#define HAVE_BOUNDING_SPHERE_RADIUS
static float calculate_bounding_sphere_radius(mesh_data_t *mesh_data) {
    vertex_array_t *positions = mesh_data_get_vertex_array(mesh_data, "pos");
    if (!positions || !positions->values || positions->values->length < 4) return 1.0f;

    float scale = mesh_data->scale_pos / 32767.0f;
    float *buf = positions->values->buffer;
    int count = positions->values->length;

    float x0 = buf[0] * scale, y0 = buf[1] * scale, z0 = buf[2] * scale;
    float max_dist_sq = 0.0f;
    float p1x = x0, p1y = y0, p1z = z0;
    for (int i = 4; i < count; i += 4) {
        float x = buf[i] * scale, y = buf[i+1] * scale, z = buf[i+2] * scale;
        float dx = x - x0, dy = y - y0, dz = z - z0;
        float d2 = dx*dx + dy*dy + dz*dz;
        if (d2 > max_dist_sq) { max_dist_sq = d2; p1x = x; p1y = y; p1z = z; }
    }

    max_dist_sq = 0.0f;
    float p2x = p1x, p2y = p1y, p2z = p1z;
    for (int i = 0; i < count; i += 4) {
        float x = buf[i] * scale, y = buf[i+1] * scale, z = buf[i+2] * scale;
        float dx = x - p1x, dy = y - p1y, dz = z - p1z;
        float d2 = dx*dx + dy*dy + dz*dz;
        if (d2 > max_dist_sq) { max_dist_sq = d2; p2x = x; p2y = y; p2z = z; }
    }

    float cx = (p1x + p2x) * 0.5f;
    float cy = (p1y + p2y) * 0.5f;
    float cz = (p1z + p2z) * 0.5f;
    float dx = p2x - p1x, dy = p2y - p1y, dz = p2z - p1z;
    float radius = sqrtf(dx*dx + dy*dy + dz*dz) * 0.5f;

    for (int i = 0; i < count; i += 4) {
        float x = buf[i] * scale, y = buf[i+1] * scale, z = buf[i+2] * scale;
        float ex = x - cx, ey = y - cy, ez = z - cz;
        float dist = sqrtf(ex*ex + ey*ey + ez*ez);
        if (dist > radius) {
            float new_r = (radius + dist) * 0.5f;
            float ratio = (new_r - radius) / dist;
            cx += ex * ratio;
            cy += ey * ratio;
            cz += ez * ratio;
            radius = new_r;
        }
    }

    return radius > 0.001f ? radius : 1.0f;
}
#endif // HAVE_BOUNDING_SPHERE_RADIUS

static game_world_t *g_mesh_3d_world = NULL;
static ecs_query_t *g_sync_query = NULL;
static ecs_query_t *g_cleanup_query = NULL;

void mesh_bridge_3d_set_world(game_world_t *world) {
    g_mesh_3d_world = world;
}

void mesh_bridge_3d_init(void) {
    if (!g_mesh_3d_world) {
        fprintf(stderr, "Mesh Bridge 3D: game world not set\n");
        return;
    }

    // Ensure scene root exists
    if (!_scene_root) {
        scene_t *empty_scene = GC_ALLOC_INIT(scene_t, {0});
        scene_create(empty_scene);
    }

    // Cache queries using ecs_query_desc_t pattern (matches render2d_bridge.c)
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_mesh_3d_world);

    ecs_query_desc_t sync_desc = {0};
    sync_desc.terms[0].id = ecs_component_comp_3d_position();
    sync_desc.terms[1].id = ecs_component_comp_3d_rotation();
    sync_desc.terms[2].id = ecs_component_comp_3d_scale();
    sync_desc.terms[3].id = ecs_component_comp_3d_mesh_renderer();
    sync_desc.terms[4].id = ecs_component_RenderObject3D();
    g_sync_query = ecs_query_init(ecs, &sync_desc);

    ecs_query_desc_t cleanup_desc = {0};
    cleanup_desc.terms[0].id = ecs_component_RenderObject3D();
    g_cleanup_query = ecs_query_init(ecs, &cleanup_desc);

    printf("Mesh Bridge 3D initialized\n");
}

void mesh_bridge_3d_shutdown(void) {
    if (g_mesh_3d_world) {
        ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_mesh_3d_world);
        if (ecs && g_cleanup_query) {
            ecs_iter_t it = ecs_query_iter(ecs, g_cleanup_query);
            while (ecs_query_next(&it)) {
                RenderObject3D *robj = ecs_field(&it, RenderObject3D, 1);
                for (int i = 0; i < it.count; i++) {
                    if (robj[i].iron_mesh_object) {
                        object_remove((object_t *)robj[i].iron_mesh_object);
                        robj[i].iron_mesh_object = NULL;
                        robj[i].iron_transform = NULL;
                    }
                }
            }
        }
    }
    if (g_sync_query) { ecs_query_fini(g_sync_query); g_sync_query = NULL; }
    if (g_cleanup_query) { ecs_query_fini(g_cleanup_query); g_cleanup_query = NULL; }
    g_mesh_3d_world = NULL;
    printf("Mesh Bridge 3D shutdown\n");
}

void mesh_bridge_3d_sync_transforms(void) {
    if (!g_mesh_3d_world || !g_sync_query) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_mesh_3d_world);
    if (!ecs) return;

    ecs_iter_t it = ecs_query_iter(ecs, g_sync_query);

    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            // Read RenderObject3D to get the Iron mesh object pointer
            const RenderObject3D *robj = ecs_get_id(ecs, it.entities[i], ecs_component_RenderObject3D());
            if (!robj || !robj->iron_mesh_object) continue;

            object_t *base = (object_t *)robj->iron_mesh_object;
            if (!base->transform) continue;

            // Read fresh component data via ecs_get_id (bypasses stale iterator
            // after Flecs archetype moves triggered by ecs_set_id)
            const comp_3d_position *pos = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_position());
            const comp_3d_rotation *rot = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_rotation());
            const comp_3d_scale *scale = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_scale());
            if (!pos || !rot || !scale) continue;

            transform_t *t = base->transform;
            t->loc.x = pos->x;
            t->loc.y = pos->y;
            t->loc.z = pos->z;
            t->rot.x = rot->x;
            t->rot.y = rot->y;
            t->rot.z = rot->z;
            t->rot.w = rot->w;
            t->scale.x = scale->x;
            t->scale.y = scale->y;
            t->scale.z = scale->z;
            t->dirty = true;
            transform_build_matrix(t);
        }
    }
}

void mesh_bridge_3d_create_mesh(uint64_t entity) {
    if (!g_mesh_3d_world || !entity) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_mesh_3d_world);
    if (!ecs) return;

    ecs_entity_t e = (ecs_entity_t)entity;

    RenderObject3D *robj = (RenderObject3D *)ecs_get_id(ecs, e, ecs_component_RenderObject3D());
    comp_3d_mesh_renderer *mr = (comp_3d_mesh_renderer *)ecs_get_id(ecs, e, ecs_component_comp_3d_mesh_renderer());

    if (!robj || !mr || robj->iron_mesh_object != NULL) return;
    if (mr->mesh_path == NULL) return;

    mesh_data_t *mesh_data = data_get_mesh(mr->mesh_path, mr->mesh_path);
    material_data_t *mat_data = NULL;
    if (mr->material_path != NULL) {
        mat_data = data_get_material(mr->material_path, mr->material_path);
    }

    if (mesh_data != NULL) {
        mesh_object_t *mesh_obj = mesh_object_create(mesh_data, mat_data);
        if (mesh_obj != NULL) {
            // Set correct bounding sphere dimensions so frustum culling uses accurate bounds.
            if (mesh_obj->base->raw == NULL) {
                float radius = calculate_bounding_sphere_radius(mesh_data);
                float dim_val = radius / sqrtf(3.0f);
                obj_t *o_raw = GC_ALLOC_INIT(obj_t, {
                    .name = "",
                    .dimensions = GC_ALLOC_INIT(f32_array_t, {
                        .buffer = gc_alloc(sizeof(float) * 3),
                        .length = 3,
                        .capacity = 3
                    })
                });
                o_raw->dimensions->buffer[0] = dim_val;
                o_raw->dimensions->buffer[1] = dim_val;
                o_raw->dimensions->buffer[2] = dim_val;
                mesh_obj->base->raw = o_raw;
                mesh_obj->base->transform->dirty = true;
                transform_build_matrix(mesh_obj->base->transform);
            }

            robj->iron_mesh_object = mesh_obj->base;
            robj->iron_transform = mesh_obj->base->transform;
            robj->dirty = true;
            object_set_parent(mesh_obj->base, _scene_root);
        }
    }
}

void mesh_bridge_3d_destroy_mesh(uint64_t entity) {
    if (!g_mesh_3d_world || !entity) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_mesh_3d_world);
    if (!ecs) return;

    ecs_entity_t e = (ecs_entity_t)entity;

    RenderObject3D *robj = (RenderObject3D *)ecs_get_id(ecs, e, ecs_component_RenderObject3D());
    if (robj && robj->iron_mesh_object) {
        object_remove((object_t *)robj->iron_mesh_object);
        robj->iron_mesh_object = NULL;
        robj->iron_transform = NULL;
        robj->dirty = false;
    }
}
