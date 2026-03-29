#include "camera_bridge.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "ecs_bridge.h"
#include "flecs.h"
#include "../core/camera2d.h"
#include <stdio.h>
#include <string.h>

static camera2d_t *g_bridge_camera = NULL;

void camera_bridge_set_world(game_world_t *world) {
    ecs_bridge_set_world(world);
}

void camera_bridge_init(void) {
    g_bridge_camera = camera2d_create(640.0, 360.0);
    camera2d_set_active(g_bridge_camera);
    printf("Camera Bridge initialized\n");
}
void camera_bridge_shutdown(void) {
    if (g_bridge_camera) {
        camera2d_destroy(g_bridge_camera);
        g_bridge_camera = NULL;
    }
    printf("Camera Bridge shutdown\n");
}
camera2d_t *camera_bridge_get_camera(void) {
    return g_bridge_camera;
}
