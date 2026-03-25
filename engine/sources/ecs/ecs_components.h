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

void ecs_register_components(void *world);
