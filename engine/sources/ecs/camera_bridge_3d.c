#include "camera_bridge_3d.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "flecs.h"
#include <iron.h>
#include <stdio.h>
#include <string.h>

static game_world_t *g_world = NULL;
static camera_object_t *g_camera_3d = NULL;
static camera_data_t *g_camera_3d_data = NULL;
static ecs_query_t *g_camera_query = NULL;

void camera_bridge_3d_set_world(game_world_t *world) {
    g_world = world;
}

void camera_bridge_3d_init(void) {
    if (!g_world) {
        fprintf(stderr, "Camera Bridge 3D: game world not set\n");
        return;
    }

    // Create camera data with defaults
    g_camera_3d_data = GC_ALLOC_INIT(camera_data_t, {
        .name = "Camera3D",
        .near_plane = 0.1f,
        .far_plane = 100.0f,
        .fov = 60.0f,
        .frustum_culling = true
    });

    // Create camera object
    g_camera_3d = camera_object_create(g_camera_3d_data);
    if (!g_camera_3d) {
        fprintf(stderr, "Failed to create 3D camera object\n");
        return;
    }

    // Ensure scene root exists
    if (!_scene_root) {
        scene_t *empty_scene = GC_ALLOC_INIT(scene_t, {0});
        scene_create(empty_scene);
    }

    // Parent camera to scene root
    object_set_parent(g_camera_3d->base, _scene_root);

    // Build initial projection
    camera_object_build_proj(g_camera_3d, (f32)sys_width() / (f32)sys_height());
    camera_object_build_mat(g_camera_3d);

    // Cache query for per-frame camera lookup
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_world);
    g_camera_query = ecs_query_new(ecs, "[in] comp_3d_camera, [in] comp_3d_position, [in] comp_3d_rotation");

    printf("Camera Bridge 3D initialized\n");
}

void camera_bridge_3d_shutdown(void) {
    if (g_camera_query) {
        ecs_query_fini(g_camera_query);
        g_camera_query = NULL;
    }
    if (g_camera_3d) {
        camera_object_remove(g_camera_3d);
        g_camera_3d = NULL;
        g_camera_3d_data = NULL;
    }
    g_world = NULL;
    printf("Camera Bridge 3D shutdown\n");
}

void camera_bridge_3d_update(void) {
    if (!g_world || !g_camera_3d || !g_camera_query) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_world);
    if (!ecs) return;

    ecs_iter_t it = ecs_query_iter(ecs, g_camera_query);

    while (ecs_query_next(&it)) {
        comp_3d_camera *cam = ecs_field(&it, comp_3d_camera, 1);
        comp_3d_position *pos = ecs_field(&it, comp_3d_position, 2);
        comp_3d_rotation *rot = ecs_field(&it, comp_3d_rotation, 3);

        for (int i = 0; i < it.count; i++) {
            if (!cam[i].active) continue;

            // Sync position and rotation to Iron camera transform
            transform_t *t = g_camera_3d->base->transform;
            t->loc.x = pos[i].x;
            t->loc.y = pos[i].y;
            t->loc.z = pos[i].z;
            t->rot.x = rot[i].x;
            t->rot.y = rot[i].y;
            t->rot.z = rot[i].z;
            t->rot.w = rot[i].w;
            t->dirty = true;
            transform_build_matrix(t);

            // Update camera data from component
            g_camera_3d_data->fov = cam[i].fov;
            g_camera_3d_data->near_plane = cam[i].near_plane;
            g_camera_3d_data->far_plane = cam[i].far_plane;

            // Rebuild projection and view matrices
            camera_object_build_proj(g_camera_3d, (f32)sys_width() / (f32)sys_height());
            camera_object_build_mat(g_camera_3d);

            // Single active camera — stop after first match
            return;
        }
    }
}

camera_object_t *camera_bridge_3d_get_active(void) {
    return g_camera_3d;
}
