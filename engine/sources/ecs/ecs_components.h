#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct { float x, y, z; } TransformPosition;
typedef struct { float x, y, z, w; } TransformRotation;
typedef struct { float x, y, z; } TransformScale;
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
} RenderSprite;

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
} Camera2D;

typedef struct {
    float x, y;
    float width, height;
    uint32_t color;
    int layer;
    bool filled;
    float strength;
    bool visible;
} RenderRect;

typedef struct {
    float cx, cy;
    float radius;
    int segments;
    uint32_t color;
    int layer;
    bool filled;
    float strength;
    bool visible;
} RenderCircle;

typedef struct {
    float x0, y0;
    float x1, y1;
    float strength;
    uint32_t color;
    int layer;
    bool visible;
} RenderLine;

typedef struct {
    char *text;
    float x, y;
    char *font_path;
    int font_size;
    uint32_t color;
    int layer;
    bool visible;
} RenderText;

void ecs_register_components(void *world);
void ecs_register_builtin_fields(void);

uint64_t ecs_get_builtin_component(const char *name);
const char *ecs_get_builtin_component_name(uint64_t component_id);

uint64_t ecs_component_TransformPosition(void);
uint64_t ecs_component_TransformRotation(void);
uint64_t ecs_component_TransformScale(void);
uint64_t ecs_component_EntityName(void);
uint64_t ecs_component_EntityActive(void);
uint64_t ecs_component_RenderObject(void);
uint64_t ecs_component_RenderMesh(void);
uint64_t ecs_component_EntityScript(void);
uint64_t ecs_component_RenderSprite(void);
uint64_t ecs_component_Camera2D(void);
uint64_t ecs_component_RenderRect(void);
uint64_t ecs_component_RenderCircle(void);
uint64_t ecs_component_RenderLine(void);
uint64_t ecs_component_RenderText(void);
