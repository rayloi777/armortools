#include "camera2d.h"
#include "../global.h"
#include <stdlib.h>
#include <math.h>

static camera2d_t *g_active_camera = NULL;

typedef struct camera2d {
    float x, y;
    float zoom;
    float rotation;
    float smoothing;
    float target_x, target_y;
    bool has_target;
    float bounds_min_x, bounds_min_y;
    float bounds_max_x, bounds_max_y;
    bool has_bounds;
} camera2d_t;

camera2d_t *camera2d_create(float x, float y) {
    camera2d_t *cam = malloc(sizeof(camera2d_t));
    cam->x = x;
    cam->y = y;
    cam->zoom = 1.0f;
    cam->rotation = 0.0f;
    cam->smoothing = 0.1f;
    cam->target_x = 0;
    cam->target_y = 0;
    cam->has_target = false;
    cam->has_bounds = false;
    return cam;
}

void camera2d_destroy(camera2d_t *cam) {
    if (cam == g_active_camera) {
        g_active_camera = NULL;
    }
    free(cam);
}

void camera2d_set_position(camera2d_t *cam, float x, float y) {
    if (!cam) return;
    cam->x = x;
    cam->y = y;
    cam->has_target = false;
}

void camera2d_set_zoom(camera2d_t *cam, float zoom) {
    if (!cam) return;
    cam->zoom = zoom;
    if (cam->zoom < 0.01f) cam->zoom = 0.01f;
}

void camera2d_set_rotation(camera2d_t *cam, float rotation) {
    if (!cam) return;
    cam->rotation = rotation;
}

void camera2d_set_smoothing(camera2d_t *cam, float smoothing) {
    if (!cam) return;
    cam->smoothing = smoothing;
}

void camera2d_follow(camera2d_t *cam, float target_x, float target_y) {
    if (!cam) return;
    cam->target_x = target_x;
    cam->target_y = target_y;
    cam->has_target = true;
}

void camera2d_set_bounds(camera2d_t *cam, float min_x, float min_y, float max_x, float max_y) {
    if (!cam) return;
    cam->bounds_min_x = min_x;
    cam->bounds_min_y = min_y;
    cam->bounds_max_x = max_x;
    cam->bounds_max_y = max_y;
    cam->has_bounds = true;
}

void camera2d_clear_bounds(camera2d_t *cam) {
    if (!cam) return;
    cam->has_bounds = false;
}

float camera2d_get_x(camera2d_t *cam) {
    return cam ? cam->x : 0.0f;
}

float camera2d_get_y(camera2d_t *cam) {
    return cam ? cam->y : 0.0f;
}

float camera2d_get_zoom(camera2d_t *cam) {
    return cam ? cam->zoom : 1.0f;
}

void camera2d_update(camera2d_t *cam, float delta_time) {
    if (!cam || !cam->has_target) return;
    
    float dx = cam->target_x - cam->x;
    float dy = cam->target_y - cam->y;
    
    cam->x += dx * cam->smoothing;
    cam->y += dy * cam->smoothing;
    
    if (cam->has_bounds) {
        if (cam->x < cam->bounds_min_x) cam->x = cam->bounds_min_x;
        if (cam->x > cam->bounds_max_x) cam->x = cam->bounds_max_x;
        if (cam->y < cam->bounds_min_y) cam->y = cam->bounds_min_y;
        if (cam->y > cam->bounds_max_y) cam->y = cam->bounds_max_y;
    }
}

mat3_t camera2d_get_transform(camera2d_t *cam) {
    mat3_t t = mat3_identity();
    
    if (!cam) return t;
    
    t = mat3_multmat(t, mat3_translation(-cam->x, -cam->y));
    t = mat3_multmat(t, mat3_scale(mat3_identity(), vec4_create(cam->zoom, cam->zoom, 0, 0)));
    t = mat3_multmat(t, mat3_rotation(cam->rotation));
    
    return t;
}

void camera2d_apply(camera2d_t *cam) {
    if (!cam) return;
    mat3_t t = camera2d_get_transform(cam);
    draw_set_transform(t);
}

camera2d_t *camera2d_get_active(void) {
    return g_active_camera;
}

void camera2d_set_active(camera2d_t *cam) {
    g_active_camera = cam;
}
