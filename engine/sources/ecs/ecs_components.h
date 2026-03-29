#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct { float x, y, z; } comp_2d_position;
typedef struct { float x, y, z, w; } comp_2d_rotation;
typedef struct { float x, y, z; } comp_2d_scale;
typedef struct { char *value; } EntityName;
typedef struct { bool value; } EntityActive;

typedef struct {
    void *object;
    void *transform;
    bool dirty;
} RenderObject;

typedef struct {
    char *mesh_path;
    char *material_path;
    bool cast_shadows;
    bool receive_shadows;
} RenderMesh;

typedef struct {
    char *script_path;
    void *script_ctx;
    void *update_fn;
    bool initialized;
} EntityScript;

typedef struct {
    char *texture_path;
    float region_x, region_y;
    float src_width, src_height;
    float pivot_x, pivot_y;
    float scale_x, scale_y;
    bool flip_x, flip_y;
    int layer;
    bool visible;
    float width, height;
    void *render_object;
} comp_2d_sprite;

typedef struct {
    float x, y;
    float zoom;
    float rotation;
    float smoothing;
    float target_x, target_y;
    bool has_target;
    float bounds_min_x, bounds_min_y;
    float bounds_max_x, bounds_max_y;
    bool has_bounds;
} comp_2d_camera;

typedef struct {
    float x, y;
    float width, height;
    uint32_t color;
    int layer;
    bool filled;
    float strength;
    bool visible;
} comp_2d_rect;

typedef struct {
    float cx, cy;
    float radius;
    int segments;
    uint32_t color;
    int layer;
    bool filled;
    float strength;
    bool visible;
} comp_2d_circle;

typedef struct {
    float x0, y0;
    float x1, y1;
    float strength;
    uint32_t color;
    int layer;
    bool visible;
} comp_2d_line;

typedef struct {
    char *text;
    float x, y;
    char *font_path;
    int font_size;
    uint32_t color;
    int layer;
    bool visible;
} comp_2d_text;

void ecs_register_components(void *world);
void ecs_register_builtin_fields(void);

uint64_t ecs_get_builtin_component(const char *name);
const char *ecs_get_builtin_component_name(uint64_t component_id);

uint64_t ecs_component_comp_2d_position(void);
uint64_t ecs_component_comp_2d_rotation(void);
uint64_t ecs_component_comp_2d_scale(void);
uint64_t ecs_component_EntityName(void);
uint64_t ecs_component_EntityActive(void);
uint64_t ecs_component_RenderObject(void);
uint64_t ecs_component_RenderMesh(void);
uint64_t ecs_component_EntityScript(void);
uint64_t ecs_component_comp_2d_sprite(void);
uint64_t ecs_component_comp_2d_camera(void);
uint64_t ecs_component_comp_2d_rect(void);
uint64_t ecs_component_comp_2d_circle(void);
uint64_t ecs_component_comp_2d_line(void);
uint64_t ecs_component_comp_2d_text(void);
