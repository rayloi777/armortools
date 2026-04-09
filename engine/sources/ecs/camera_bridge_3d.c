#include "camera_bridge_3d.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "flecs.h"
#include <iron.h>
#include <iron_system.h>
#include <stdio.h>
#include <string.h>

#ifndef DEG_TO_RAD
#define DEG_TO_RAD (3.14159265358979323846f / 180.0f)
#endif

static game_world_t *g_camera_3d_world = NULL;
static camera_object_t *g_camera_3d = NULL;
static camera_data_t *g_camera_3d_data = NULL;
static ecs_query_t *g_camera_3d_query = NULL;

void camera_bridge_3d_set_world(game_world_t *world) {
    g_camera_3d_world = world;
}

void camera_bridge_3d_init(void) {
    if (!g_camera_3d_world) {
        fprintf(stderr, "Camera Bridge 3D: game world not set\n");
        return;
    }

    // Initialize minimal Iron scene infrastructure if not already done.
    // We can't call scene_create() with an empty scene because it tries to
    // load scene files and access scene_cameras->buffer[0] on an empty array.
    if (!_scene_root) {
        scene_meshes = any_array_create(0);
        gc_root(scene_meshes);
        scene_cameras = any_array_create(0);
        gc_root(scene_cameras);
        scene_empties = any_array_create(0);
        gc_root(scene_empties);
        scene_embedded = any_map_create();
        gc_root(scene_embedded);
        _scene_root = object_create(true);
        _scene_root->name = "Root";
    }

    // Create camera data with defaults
    g_camera_3d_data = GC_ALLOC_INIT(camera_data_t, {
        .name = "Camera3D",
        .near_plane = 0.1f,
        .far_plane = 100.0f,
        .fov = 60.0f * DEG_TO_RAD,
        .frustum_culling = true
    });

    // Create camera object
    g_camera_3d = camera_object_create(g_camera_3d_data);
    if (!g_camera_3d) {
        fprintf(stderr, "Failed to create 3D camera object\n");
        return;
    }

    // Parent camera to scene root
    object_set_parent(g_camera_3d->base, _scene_root);

    // Set as active scene camera so Iron's rendering uses it
    scene_camera = g_camera_3d;

    // Build initial projection
    f32 aspect = (f32)iron_window_width() / (f32)iron_window_height();
    camera_object_build_proj(g_camera_3d, aspect);
    camera_object_build_mat(g_camera_3d);

    // Cache query for per-frame camera lookup
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_camera_3d_world);
    ecs_query_desc_t qdesc = {0};
    qdesc.terms[0].id = ecs_component_comp_3d_camera();
    qdesc.terms[1].id = ecs_component_comp_3d_position();
    qdesc.terms[2].id = ecs_component_comp_3d_rotation();
    g_camera_3d_query = ecs_query_init(ecs, &qdesc);

    printf("Camera Bridge 3D initialized\n");
}

void camera_bridge_3d_shutdown(void) {
    if (g_camera_3d_query) {
        ecs_query_fini(g_camera_3d_query);
        g_camera_3d_query = NULL;
    }
    if (g_camera_3d) {
        camera_object_remove(g_camera_3d);
        g_camera_3d = NULL;
        g_camera_3d_data = NULL;
    }
    g_camera_3d_world = NULL;
    printf("Camera Bridge 3D shutdown\n");
}

void camera_bridge_3d_update(void) {
    if (!g_camera_3d_world || !g_camera_3d || !g_camera_3d_query) return;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_camera_3d_world);
    if (!ecs) return;

    ecs_iter_t it = ecs_query_iter(ecs, g_camera_3d_query);

    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            // Read fresh component data via ecs_get_id (bypasses stale iterator
            // after Flecs archetype moves triggered by ecs_set_id)
            const comp_3d_position *pos = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_position());
            const comp_3d_rotation *rot = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_rotation());
            const comp_3d_camera *cam_fresh = ecs_get_id(ecs, it.entities[i], ecs_component_comp_3d_camera());
            if (!pos || !rot || !cam_fresh) continue;

            // Sync position and rotation to Iron camera transform
            transform_t *t = g_camera_3d->base->transform;
            t->loc.x = pos->x;
            t->loc.y = pos->y;
            t->loc.z = pos->z;
            t->rot.x = rot->x;
            t->rot.y = rot->y;
            t->rot.z = rot->z;
            t->rot.w = rot->w;
            t->dirty = true;
            transform_build_matrix(t);

            // Update camera data from component
            g_camera_3d_data->fov = cam_fresh->fov;
            g_camera_3d_data->near_plane = cam_fresh->near_plane;
            g_camera_3d_data->far_plane = cam_fresh->far_plane;

            // Sync perspective/ortho mode
            if (cam_fresh->perspective) {
                g_camera_3d_data->ortho = NULL;
            } else {
                if (g_camera_3d_data->ortho == NULL || g_camera_3d_data->ortho->length < 4) {
                    g_camera_3d_data->ortho = gc_alloc(sizeof(f32_array_t));
                    g_camera_3d_data->ortho->buffer = gc_alloc(sizeof(float) * 4);
                    g_camera_3d_data->ortho->length = 4;
                    g_camera_3d_data->ortho->capacity = 4;
                }
                g_camera_3d_data->ortho->buffer[0] = cam_fresh->ortho_left;
                g_camera_3d_data->ortho->buffer[1] = cam_fresh->ortho_right;
                g_camera_3d_data->ortho->buffer[2] = cam_fresh->ortho_bottom;
                g_camera_3d_data->ortho->buffer[3] = cam_fresh->ortho_top;
            }

            // Rebuild projection and view matrices
            f32 aspect = (f32)iron_window_width() / (f32)iron_window_height();
            camera_object_build_proj(g_camera_3d, aspect);
            camera_object_build_mat(g_camera_3d);

            // Single active camera — stop after first match
            return;
        }
    }
}

void *camera_bridge_3d_get_active(void) {
    return (void *)g_camera_3d;
}
