#pragma once

#include <stdbool.h>

typedef struct camera2d camera2d_t;

camera2d_t *camera2d_create(float x, float y);
void camera2d_destroy(camera2d_t *cam);

void camera2d_set_position(camera2d_t *cam, float x, float y);
void camera2d_set_zoom(camera2d_t *cam, float zoom);
void camera2d_set_rotation(camera2d_t *cam, float rotation);
void camera2d_set_smoothing(camera2d_t *cam, float smoothing);

void camera2d_follow(camera2d_t *cam, float target_x, float target_y);
void camera2d_set_bounds(camera2d_t *cam, float min_x, float min_y, float max_x, float max_y);
void camera2d_clear_bounds(camera2d_t *cam);

float camera2d_get_x(camera2d_t *cam);
float camera2d_get_y(camera2d_t *cam);
float camera2d_get_zoom(camera2d_t *cam);

void camera2d_update(camera2d_t *cam, float delta_time);
void camera2d_apply(camera2d_t *cam);

camera2d_t *camera2d_get_active(void);
void camera2d_set_active(camera2d_t *cam);
