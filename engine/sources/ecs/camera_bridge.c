#include "camera_bridge.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "ecs_bridge.h"
#include "flecs.h"
#include "../core/camera2d.h"
#include <stdio.h>
#include <string.h>

static ecs_entity_t g_camera_bridge_system = 0;
static camera2d_t *g_bridge_camera = NULL;

static void camera_bridge_system(ecs_iter_t *it) {
    Camera2D *cam = ecs_field(it, Camera2D, 0);
    
    for (int i = 0; i < it->count; i++) {
        if (g_bridge_camera == NULL) {
            g_bridge_camera = camera2d_create(cam[i].x, cam[i].y);
            camera2d_set_active(g_bridge_camera);
        }
        
        camera2d_set_position(g_bridge_camera, cam[i].x, cam[i].y);
        camera2d_set_zoom(g_bridge_camera, cam[i].zoom);
        camera2d_set_rotation(g_bridge_camera, cam[i].rotation);
        camera2d_set_smoothing(g_bridge_camera, cam[i].smoothing);
        
        if (cam[i].has_target) {
            camera2d_follow(g_bridge_camera, cam[i].target_x, cam[i].target_y);
        }
        
        if (cam[i].has_bounds) {
            camera2d_set_bounds(g_bridge_camera, 
                cam[i].bounds_min_x, cam[i].bounds_min_y,
                cam[i].bounds_max_x, cam[i].bounds_max_y);
        } else {
            camera2d_clear_bounds(g_bridge_camera);
        }
        
        camera2d_update(g_bridge_camera, 0.016f);
    }
}

void camera_bridge_set_world(game_world_t *world) {
    ecs_bridge_set_world(world);
}

void camera_bridge_init(void) {
    game_world_t *world = ecs_bridge_get_world();
    if (!world) {
        fprintf(stderr, "Camera Bridge: game world not set\n");
        return;
    }
    
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(world);
    if (!ecs) {
        fprintf(stderr, "Failed to get ECS world for camera bridge\n");
        return;
    }
    
    ecs_system_desc_t sys_desc = {0};
    sys_desc.entity = 0;
    sys_desc.query.terms[0].id = ecs_component_Camera2D();
    sys_desc.callback = camera_bridge_system;
    
    g_camera_bridge_system = ecs_system_init(ecs, &sys_desc);
    
    if (g_camera_bridge_system == 0) {
        fprintf(stderr, "Failed to create camera bridge system\n");
        return;
    }
    
    ecs_add_pair(ecs, g_camera_bridge_system, EcsDependsOn, EcsPreUpdate);
    
    printf("Camera Bridge initialized\n");
}

void camera_bridge_shutdown(void) {
    if (g_bridge_camera) {
        camera2d_destroy(g_bridge_camera);
        g_bridge_camera = NULL;
    }
    g_camera_bridge_system = 0;
    printf("Camera Bridge shutdown\n");
}

camera2d_t *camera_bridge_get_camera(void) {
    return g_bridge_camera;
}
