# 2D Game Engine Extension Plan

## Table of Contents
1. [ECS Architecture Overview](#ecs-architecture-overview)
2. [ECS Component Reference](#ecs-component-reference)
3. [ECS Bridge Pattern](#ecs-bridge-pattern)
4. [Dynamic ECS & Minic](#dynamic-ecs--minic)
5. [2D Feature Plans](#2d-feature-plans)
6. [Implementation Order](#implementation-order)

---

# ECS Architecture Overview

## Layer Structure

```
┌─────────────────────────────────────────────────────────────┐
│                    Game Engine                               │
│         (game_loop.c, game_engine.c)                        │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              High-Level APIs                                 │
│   entity_api.c  │  system_api.c  │  query_api.c            │
│   component_api.c  │  runtime_api.c                          │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│           ECS Bridge Layer (ecs_bridge.c)                    │
│   - Syncs ECS entities to Iron Engine scene graph           │
│   - Creates bridge_system for transform propagation          │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│           ECS World Layer                                   │
│   ecs_world.c  │  ecs_components.c  │  ecs_dynamic.c       │
│   - game_world_t wrapper                                   │
│   - Builtin component registration                          │
│   - Dynamic component management                            │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              Flecs Core (flecs.c/h)                         │
│   - ecs_init() / ecs_fini() / ecs_progress()              │
│   - ecs_component_init() / ecs_system_init()                │
│   - ecs_query_init() / ecs_field()                         │
└─────────────────────────────────────────────────────────────┘
```

---

# ECS Component Reference

## Static Components (ecs_components.h)

```c
// Position in world space
typedef struct { float x, y, z; } TransformPosition;

// Quaternion rotation
typedef struct { float x, y, z, w; } TransformRotation;

// Scale
typedef struct { float x, y, z; } TransformScale;

// Entity name string
typedef struct { char *value; } EntityName;

// Active flag (for enable/disable)
typedef struct { bool value; } EntityActive;

// Bridge component - holds Iron Engine object references
typedef struct {
    void *object;      // object_t* - Iron scene graph node
    void *transform;   // transform_t* - Iron transform node
    bool dirty;        // Sync flag - needs update?
} RenderObject;

// 3D mesh rendering data
typedef struct {
    char *mesh_path;
    char *material_path;
    bool cast_shadows;
    bool receive_shadows;
} RenderMesh;

// Script component for minic integration
typedef struct {
    char *script_path;
    void *script_ctx;
    void *update_fn;
    bool initialized;
} EntityScript;
```

---

# ECS Bridge Pattern

## Architecture

```
ECS Entity                              Iron Engine Scene Graph
──────────                              ──────────────────────
TransformPosition ──────────────────►  transform_t (loc, rot, scale)
TransformRotation ──────────────────►
TransformScale ─────────────────────►
RenderObject ───────────────────────►  object_t (scene graph node)
    ├── object: NULL ───────────────►      mesh_object_t ──► mesh_data, material
    └── dirty: true ───────────────►      transform_build_matrix()
RenderMesh ────────────────────────►  asset loading (mesh, material)
```

## bridge_system() Implementation

```c
// ecs_bridge.c
static void bridge_system(ecs_iter_t *it) {
    TransformPosition *pos = ecs_field(it, TransformPosition, 1);
    TransformRotation *rot = ecs_field(it, TransformRotation, 2);
    TransformScale *scale = ecs_field(it, TransformScale, 3);
    RenderObject *render_obj = ecs_field(it, RenderObject, 4);
    RenderMesh *mesh = ecs_field(it, RenderMesh, 5);

    for (int i = 0; i < it->count; i++) {
        // Create Iron object if needed
        if (render_obj[i].object == NULL && mesh[i].mesh_path != NULL) {
            mesh_data_t *mesh_data = data_get_mesh(mesh[i].mesh_path, mesh[i].mesh_path);
            // ... create mesh_object_t ...
        }
        
        // Sync transform if dirty
        if (render_obj[i].transform != NULL && render_obj[i].dirty) {
            transform_t *t = (transform_t *)render_obj[i].transform;
            t->loc.x = pos[i].x; t->loc.y = pos[i].y; t->loc.z = pos[i].z;
            // ... copy rotation, scale ...
            t->dirty = true;
            transform_build_matrix(t);
            render_obj[i].dirty = false;
        }
    }
}
```

## System Registration

```c
void ecs_bridge_init(void) {
    ecs_system_desc_t sys_desc = {0};
    sys_desc.query.terms[0].id = ecs_component_TransformPosition();
    sys_desc.query.terms[1].id = ecs_component_TransformRotation();
    sys_desc.query.terms[2].id = ecs_component_TransformScale();
    sys_desc.query.terms[3].id = ecs_component_RenderObject();
    sys_desc.query.terms[4].id = ecs_component_RenderMesh();
    sys_desc.callback = bridge_system;
    
    g_bridge_system = ecs_system_init(ecs, &sys_desc);
    ecs_add_pair(ecs, g_bridge_system, EcsDependsOn, EcsPreStore);
}
```

---

# Dynamic ECS & Minic

## Overview

| Aspect | Static ECS | Dynamic ECS |
|--------|------------|-------------|
| Components | Compile-time C structs | Runtime created |
| Definition | ecs_components.h | ecs_dynamic.c |
| Flexibility | Fixed types | Can add new types at runtime |
| Scripting | Via EntityScript | Constructor/destructor hooks |

## Minic System Integration

All 2D systems are **Minic Systems** that run every frame automatically:
- Trigger: Every frame via `ecs_progress()`
- Control: Script-based (camera position, sprite animation, etc.)

---

# 2D Feature Plans

## Part 1: Sprite System

### Design

```c
// RenderSprite (ecs_components.h)
typedef struct {
    char *texture_path;           // Path to texture asset
    
    // Atlas region (for sprite sheet)
    float region_x, region_y;     // Top-left of frame in atlas
    float src_width, src_height; // Original frame size
    
    // Transform
    float pivot_x, pivot_y;      // Anchor point (0-1)
    float scale_x, scale_y;       // Scale multiplier
    
    // Flip
    bool flip_x, flip_y;
    
    // Render order
    int layer;                    // Z-order (0=背景, 100=前景, 200=UI)
    
    // State
    bool visible;
    
    // Cached (rendered size after scale applied)
    float width;                  // src_width * scale_x
    float height;                 // src_height * scale_y
    
    void *render_object;          // Internal: bridge reference
} RenderSprite;
```

### Layer Values

| Layer | Purpose |
|-------|---------|
| 0-9 | Background |
| 10-50 | Background decoration |
| 51-100 | Game objects |
| 101-150 | Foreground decoration |
| 151-200 | UI/HUD |

### Bridge System (sprite_bridge.c)

```c
static void sprite_bridge_system(ecs_iter_t *it) {
    TransformPosition *pos = ecs_field(it, TransformPosition, 1);
    RenderSprite *sprite = ecs_field(it, RenderSprite, 2);
    
    for (int i = 0; i < it->count; i++) {
        // Create sprite if needed
        if (sprite[i].render_object == NULL && sprite[i].texture_path != NULL) {
            sprite[i].render_object = sprite_object_create(...);
        }
        
        // Sync position and properties
        if (sprite[i].render_object != NULL) {
            sprite_renderer_set_position(sprite[i].render_object, pos[i].x, pos[i].y);
            sprite_renderer_set_scale(sprite[i].render_object, sprite[i].scale_x, sprite[i].scale_y);
            sprite_renderer_set_flip(sprite[i].render_object, sprite[i].flip_x, sprite[i].flip_y);
            sprite_renderer_set_layer(sprite[i].render_object, sprite[i].layer);
            
            // Update cached dimensions
            sprite[i].width = sprite[i].src_width * sprite[i].scale_x;
            sprite[i].height = sprite[i].src_height * sprite[i].scale_y;
        }
    }
}
```

### sprite_api.c Enhancement

```c
// sprite_api.h (enhanced)
typedef struct sprite_renderer sprite_renderer_t;

sprite_renderer_t *sprite_renderer_create(void);
void sprite_renderer_destroy(sprite_renderer_t *sr);
void sprite_renderer_draw(sprite_renderer_t *sr);
void sprite_renderer_set_texture(sprite_renderer_t *sr, const char *texture_name);
void sprite_renderer_set_scale(sprite_renderer_t *sr, float scale_x, float scale_y);
void sprite_renderer_set_position(sprite_renderer_t *sr, float x, float y);
void sprite_renderer_set_flip(sprite_renderer_t *sr, bool flip_x, bool flip_y);
void sprite_renderer_set_layer(sprite_renderer_t *sr, int layer);
void sprite_renderer_set_region(sprite_renderer_t *sr, float x, float y, float w, float h);
void sprite_renderer_set_visible(sprite_renderer_t *sr, bool visible);

// Sorting
void sprite_renderer_sort_by_layer(void);
```

---

## Part 2: 2D Camera System

### Design

```c
// Camera2D (ecs_components.h)
typedef struct {
    float x, y;              // Camera position in world space
    float zoom;             // Zoom level (1.0 = default)
    float rotation;          // Rotation in radians
    float smoothing;         // Follow smoothing (0-1)
    
    // Target (for follow)
    float target_x, target_y;
    bool has_target;
    
    // Bounds (optional)
    float bounds_min_x, bounds_min_y;
    float bounds_max_x, bounds_max_y;
    bool has_bounds;
    
    // Aspect ratio
    float aspect_ratio;
} Camera2D;
```

### camera2d Functions

```c
// camera2d.h
typedef struct camera2d camera2d_t;

camera2d_t *camera2d_create(float x, float y);
void camera2d_destroy(camera2d_t *cam);

void camera2d_set_position(camera2d_t *cam, float x, float y);
void camera2d_set_zoom(camera2d_t *cam, float zoom);
void camera2d_set_rotation(camera2d_t *cam, float rotation);
void camera2d_set_smoothing(camera2d_t *cam, float smoothing);

void camera2d_follow(camera2d_t *cam, float target_x, float target_y);
void camera2d_set_bounds(camera2d_t *cam, float min_x, float min_y, float max_x, float max_y);

mat3_t camera2d_get_transform(camera2d_t *cam);
void camera2d_apply(camera2d_t *cam);  // Calls draw_set_transform()
```

### Camera Transform

```c
mat3_t camera2d_get_transform(camera2d_t *cam) {
    mat3_t t = mat3_identity();
    
    // Translate to negative position (world moves opposite to camera)
    t = mat3_multmat(t, mat3_translation(-cam->x, -cam->y));
    
    // Apply zoom (scale from origin)
    t = mat3_multmat(t, mat3_scale(mat3_identity(), vec4_create(cam->zoom, cam->zoom, 0, 0)));
    
    // Apply rotation
    t = mat3_multmat(t, mat3_rotation(cam->rotation));
    
    return t;
}
```

---

## Part 3: Tilemap System

### Design

```c
// TilemapRenderer (ecs_components.h)
typedef struct {
    char *tilemap_path;      // Path to tilemap data
    int active_layer;        // Which layer to render
    bool visible;
} TilemapRenderer;

// Tilemap data (runtime)
typedef struct {
    char *name;
    int width, height;       // Map size in tiles
    int tile_width, tile_height;
    any_array_t *tilesets;  // tileset_t[]
    any_array_t *layers;    // tilemap_layer_t[]
} tilemap_t;

typedef struct {
    char *name;
    int *tiles;             // Tile indices (width * height)
    bool visible;
    float opacity;
} tilemap_layer_t;

typedef struct {
    char *name;
    int first_gid;
    int tile_width, tile_height;
    int columns, rows;
    gpu_texture_t *image;
} tileset_t;
```

### tilemap Functions

```c
// tilemap.h
tilemap_t *tilemap_load(const char *path);
void tilemap_destroy(tilemap_t *map);

void tilemap_set_layer(tilemap_t *map, int layer_index);
int tilemap_get_tile(tilemap_t *map, int layer, int x, int y);
void tilemap_set_tile(tilemap_t *map, int layer, int x, int y, int tile_id);

void tilemap_render(tilemap_t *map, int layer_index, mat3_t camera_transform);
void tilemap_render_all(tilemap_t *map, mat3_t camera_transform);
```

---

## Part 4: 2D Physics System

### Design

```c
// Physics2DBody (ecs_components.h)
typedef struct {
    body2d_type_t type;     // STATIC, DYNAMIC, KINEMATIC
    vec2_t velocity;
    vec2_t force;
    float mass;
    float restitution;       // Bounciness
    float friction;
    aabb2d_t aabb;           // Axis-aligned bounding box
    bool enabled;
    bool awake;
} Physics2DBody;

typedef enum {
    BODY2D_STATIC = 0,
    BODY2D_DYNAMIC = 1,
    BODY2D_KINEMATIC = 2
} body2d_type_t;

typedef struct { vec2_t min; vec2_t max; } aabb2d_t;
```

### physics2d Functions

```c
// physics2d.h
typedef struct physics2d_world physics2d_world_t;

physics2d_world_t *physics2d_world_create(void);
void physics2d_world_destroy(physics2d_world_t *world);

body2d_t *physics2d_world_add_body(physics2d_world_t *world, body2d_t *body);
void physics2d_world_remove_body(physics2d_world_t *world, body2d_t *body);

void physics2d_world_update(physics2d_world_t *world, float dt);

typedef void (*collision_callback_t)(uint64_t entity_a, uint64_t entity_b, 
                                      vec2_t normal, float depth);
void physics2d_world_set_collision_callback(physics2d_world_t *world, 
                                              collision_callback_t cb);

// Collision detection
bool aabb2d_intersect(aabb2d_t a, aabb2d_t b);
bool aabb2d_contains_point(aabb2d_t a, vec2_t p);
```

---

# Implementation Order

## Phase 1: Sprite System
- [ ] Add `RenderSprite` to ecs_components.h
- [ ] Register in ecs_components.c
- [ ] Enhance sprite_api.c (region, flip, layer, width/height cache)
- [ ] Create sprite_bridge.c
- [ ] Add sprite rendering to game loop
- [ ] Sort sprites by layer before rendering

## Phase 2: 2D Camera
- [ ] Add `Camera2D` to ecs_components.h
- [ ] Create camera2d.c/h
- [ ] Implement follow, zoom, bounds
- [ ] Integrate with draw_set_transform()
- [ ] Add camera update to game loop

## Phase 3: Tilemap
- [ ] Create tilemap.h/c
- [ ] Implement JSON/TMX loader
- [ ] Add `TilemapRenderer` component
- [ ] Render via iron_draw with camera culling

## Phase 4: 2D Physics
- [ ] Add `Physics2DBody` to ecs_components.h
- [ ] Implement aabb2d collision
- [ ] Create physics2d.c/h
- [ ] Add collision callbacks

---

# File Structure

```
engine/sources/
├── core/
│   ├── sprite_api.c/h          (enhance)
│   ├── camera2d.c/h            (NEW)
│   └── tilemap.c/h             (NEW)
├── ecs/
│   ├── ecs_components.h        (enhance: add RenderSprite, Camera2D, TilemapRenderer, Physics2DBody)
│   ├── ecs_components.c        (enhance: register new components)
│   ├── ecs_bridge.c            (reference)
│   ├── sprite_bridge.c         (NEW)
│   ├── camera2d_bridge.c       (NEW)
│   ├── tilemap_bridge.c        (NEW)
│   ├── physics2d.c             (NEW)
│   └── ecs_dynamic.c/h         (reference)
└── game_loop.c                 (enhance: sprite render, camera apply)
```

---

# Reference Files

| File | Purpose |
|------|---------|
| `ecs_bridge.c` | Example of ECS→scene graph bridge |
| `ecs_components.h/c` | Component definitions |
| `ecs_dynamic.c/h` | Dynamic ECS + minic hooks |
| `entity_api.c/h` | High-level entity API |
| `system_api.c/h` | System management |
| `query_api.c/h` | Query functionality |
| `sprite_api.c/h` | Existing sprite implementation |
| `flecs/flecs.h` | Flecs library API |
| `iron_draw.h` | 2D rendering API |

---

# Key Flecs API

## World Management
```c
ecs_world_t *ecs_init(void);
void ecs_fini(ecs_world_t *world);
void ecs_progress(ecs_world_t *world, float delta_time);
```

## Component Management
```c
ecs_entity_t ecs_component_init(ecs_world_t *world, const ecs_component_desc_t *desc);
ecs_id_t ecs_get_id(ecs_world_t *world, ecs_entity_t entity, ecs_id_t component);
```

## System Management
```c
ecs_entity_t ecs_system_init(ecs_world_t *world, const ecs_system_desc_t *desc);
void ecs_add_pair(ecs_world_t *world, ecs_entity_t entity, ecs_entity_t relationship, ecs_entity_t target);
```

## Iteration
```c
void *ecs_field(ecs_iter_t *it, size_t size, int32_t index);
bool ecs_query_next(ecs_iter_t *it);
```
