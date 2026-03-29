#include "ecs_components.h"
#include "ecs_dynamic.h"
#include "flecs.h"
#include <string.h>

static ecs_entity_t comp_TransformPosition = 0;
static ecs_entity_t comp_TransformRotation = 0;
static ecs_entity_t comp_TransformScale = 0;
static ecs_entity_t comp_EntityName = 0;
static ecs_entity_t comp_EntityActive = 0;
static ecs_entity_t comp_RenderObject = 0;
static ecs_entity_t comp_RenderMesh = 0;
static ecs_entity_t comp_EntityScript = 0;
static ecs_entity_t comp_RenderSprite = 0;
static ecs_entity_t comp_Camera2D = 0;

ecs_entity_t ecs_component_TransformPosition(void) { return comp_TransformPosition; }
ecs_entity_t ecs_component_TransformRotation(void) { return comp_TransformRotation; }
ecs_entity_t ecs_component_TransformScale(void) { return comp_TransformScale; }
ecs_entity_t ecs_component_EntityName(void) { return comp_EntityName; }
ecs_entity_t ecs_component_EntityActive(void) { return comp_EntityActive; }
ecs_entity_t ecs_component_RenderObject(void) { return comp_RenderObject; }
ecs_entity_t ecs_component_RenderMesh(void) { return comp_RenderMesh; }
ecs_entity_t ecs_component_EntityScript(void) { return comp_EntityScript; }
ecs_entity_t ecs_component_RenderSprite(void) { return comp_RenderSprite; }
ecs_entity_t ecs_component_Camera2D(void) { return comp_Camera2D; }

static ecs_entity_t register_component(ecs_world_t *ecs, const char *name, size_t size, size_t alignment) {
    ecs_component_desc_t desc = {0};
    desc.entity = 0;
    desc.type.name = name;
    desc.type.size = (ecs_size_t)size;
    desc.type.alignment = (ecs_size_t)alignment;
    return ecs_component_init(ecs, &desc);
}

static void add_builtin_to_dynamic(const char *name, ecs_entity_t flecs_id, size_t size, size_t alignment) {
    if (g_component_count >= MAX_DYNAMIC_COMPONENTS) return;
    
    dynamic_component_t *dc = &g_components[g_component_count];
    strncpy(dc->name, name, sizeof(dc->name) - 1);
    dc->name[sizeof(dc->name) - 1] = '\0';
    dc->flecs_id = flecs_id;
    dc->size = size;
    dc->alignment = alignment;
    dc->field_count = 0;
    dc->type_info = NULL;
    dc->ctor_name[0] = '\0';
    dc->dtor_name[0] = '\0';
    
    g_component_count++;
}

void ecs_register_components(void *world) {
    if (!world) return;
    ecs_world_t *ecs = (ecs_world_t *)world;
    
    comp_TransformPosition = register_component(ecs, "TransformPosition", sizeof(TransformPosition), ECS_ALIGNOF(TransformPosition));
    add_builtin_to_dynamic("TransformPosition", comp_TransformPosition, sizeof(TransformPosition), ECS_ALIGNOF(TransformPosition));
    
    comp_TransformRotation = register_component(ecs, "TransformRotation", sizeof(TransformRotation), ECS_ALIGNOF(TransformRotation));
    add_builtin_to_dynamic("TransformRotation", comp_TransformRotation, sizeof(TransformRotation), ECS_ALIGNOF(TransformRotation));
    
    comp_TransformScale = register_component(ecs, "TransformScale", sizeof(TransformScale), ECS_ALIGNOF(TransformScale));
    add_builtin_to_dynamic("TransformScale", comp_TransformScale, sizeof(TransformScale), ECS_ALIGNOF(TransformScale));
    
    comp_EntityName = register_component(ecs, "EntityName", sizeof(EntityName), ECS_ALIGNOF(EntityName));
    add_builtin_to_dynamic("EntityName", comp_EntityName, sizeof(EntityName), ECS_ALIGNOF(EntityName));
    
    comp_EntityActive = register_component(ecs, "EntityActive", sizeof(EntityActive), ECS_ALIGNOF(EntityActive));
    add_builtin_to_dynamic("EntityActive", comp_EntityActive, sizeof(EntityActive), ECS_ALIGNOF(EntityActive));
    
    comp_RenderObject = register_component(ecs, "RenderObject", sizeof(RenderObject), ECS_ALIGNOF(RenderObject));
    add_builtin_to_dynamic("RenderObject", comp_RenderObject, sizeof(RenderObject), ECS_ALIGNOF(RenderObject));
    
    comp_RenderMesh = register_component(ecs, "RenderMesh", sizeof(RenderMesh), ECS_ALIGNOF(RenderMesh));
    add_builtin_to_dynamic("RenderMesh", comp_RenderMesh, sizeof(RenderMesh), ECS_ALIGNOF(RenderMesh));
    
    comp_EntityScript = register_component(ecs, "EntityScript", sizeof(EntityScript), ECS_ALIGNOF(EntityScript));
    add_builtin_to_dynamic("EntityScript", comp_EntityScript, sizeof(EntityScript), ECS_ALIGNOF(EntityScript));
    
    comp_RenderSprite = register_component(ecs, "RenderSprite", sizeof(RenderSprite), ECS_ALIGNOF(RenderSprite));
    add_builtin_to_dynamic("RenderSprite", comp_RenderSprite, sizeof(RenderSprite), ECS_ALIGNOF(RenderSprite));
    
    comp_Camera2D = register_component(ecs, "Camera2D", sizeof(Camera2D), ECS_ALIGNOF(Camera2D));
    add_builtin_to_dynamic("Camera2D", comp_Camera2D, sizeof(Camera2D), ECS_ALIGNOF(Camera2D));
    
    ecs_register_builtin_fields();
}

void ecs_register_builtin_fields(void) {
    ecs_dynamic_component_add_field(comp_TransformPosition, "x", DYNAMIC_TYPE_FLOAT, 0);
    ecs_dynamic_component_add_field(comp_TransformPosition, "y", DYNAMIC_TYPE_FLOAT, 4);
    ecs_dynamic_component_add_field(comp_TransformPosition, "z", DYNAMIC_TYPE_FLOAT, 8);
    
    ecs_dynamic_component_add_field(comp_RenderSprite, "texture_path", DYNAMIC_TYPE_PTR, 0);
    ecs_dynamic_component_add_field(comp_RenderSprite, "region_x", DYNAMIC_TYPE_FLOAT, 8);
    ecs_dynamic_component_add_field(comp_RenderSprite, "region_y", DYNAMIC_TYPE_FLOAT, 12);
    ecs_dynamic_component_add_field(comp_RenderSprite, "src_width", DYNAMIC_TYPE_FLOAT, 16);
    ecs_dynamic_component_add_field(comp_RenderSprite, "src_height", DYNAMIC_TYPE_FLOAT, 20);
    ecs_dynamic_component_add_field(comp_RenderSprite, "pivot_x", DYNAMIC_TYPE_FLOAT, 24);
    ecs_dynamic_component_add_field(comp_RenderSprite, "pivot_y", DYNAMIC_TYPE_FLOAT, 28);
    ecs_dynamic_component_add_field(comp_RenderSprite, "scale_x", DYNAMIC_TYPE_FLOAT, 32);
    ecs_dynamic_component_add_field(comp_RenderSprite, "scale_y", DYNAMIC_TYPE_FLOAT, 36);
    ecs_dynamic_component_add_field(comp_RenderSprite, "flip_x", DYNAMIC_TYPE_BOOL, 40);
    ecs_dynamic_component_add_field(comp_RenderSprite, "flip_y", DYNAMIC_TYPE_BOOL, 41);
    ecs_dynamic_component_add_field(comp_RenderSprite, "layer", DYNAMIC_TYPE_INT, 44);
    ecs_dynamic_component_add_field(comp_RenderSprite, "visible", DYNAMIC_TYPE_BOOL, 48);
    ecs_dynamic_component_add_field(comp_RenderSprite, "width", DYNAMIC_TYPE_FLOAT, 52);
    ecs_dynamic_component_add_field(comp_RenderSprite, "height", DYNAMIC_TYPE_FLOAT, 56);
    ecs_dynamic_component_add_field(comp_RenderSprite, "render_object", DYNAMIC_TYPE_PTR, 64);
    
    ecs_dynamic_component_add_field(comp_Camera2D, "x", DYNAMIC_TYPE_FLOAT, 0);
    ecs_dynamic_component_add_field(comp_Camera2D, "y", DYNAMIC_TYPE_FLOAT, 4);
    ecs_dynamic_component_add_field(comp_Camera2D, "zoom", DYNAMIC_TYPE_FLOAT, 8);
    ecs_dynamic_component_add_field(comp_Camera2D, "rotation", DYNAMIC_TYPE_FLOAT, 12);
    ecs_dynamic_component_add_field(comp_Camera2D, "smoothing", DYNAMIC_TYPE_FLOAT, 16);
    ecs_dynamic_component_add_field(comp_Camera2D, "target_x", DYNAMIC_TYPE_FLOAT, 20);
    ecs_dynamic_component_add_field(comp_Camera2D, "target_y", DYNAMIC_TYPE_FLOAT, 24);
    ecs_dynamic_component_add_field(comp_Camera2D, "has_target", DYNAMIC_TYPE_BOOL, 28);
    ecs_dynamic_component_add_field(comp_Camera2D, "bounds_min_x", DYNAMIC_TYPE_FLOAT, 32);
    ecs_dynamic_component_add_field(comp_Camera2D, "bounds_min_y", DYNAMIC_TYPE_FLOAT, 36);
    ecs_dynamic_component_add_field(comp_Camera2D, "bounds_max_x", DYNAMIC_TYPE_FLOAT, 40);
    ecs_dynamic_component_add_field(comp_Camera2D, "bounds_max_y", DYNAMIC_TYPE_FLOAT, 44);
    ecs_dynamic_component_add_field(comp_Camera2D, "has_bounds", DYNAMIC_TYPE_BOOL, 48);
}

uint64_t ecs_get_builtin_component(const char *name) {
    if (!name) return 0;
    
    if (strcmp(name, "TransformPosition") == 0) return comp_TransformPosition;
    if (strcmp(name, "TransformRotation") == 0) return comp_TransformRotation;
    if (strcmp(name, "TransformScale") == 0) return comp_TransformScale;
    if (strcmp(name, "EntityName") == 0) return comp_EntityName;
    if (strcmp(name, "EntityActive") == 0) return comp_EntityActive;
    if (strcmp(name, "RenderObject") == 0) return comp_RenderObject;
    if (strcmp(name, "RenderMesh") == 0) return comp_RenderMesh;
    if (strcmp(name, "EntityScript") == 0) return comp_EntityScript;
    if (strcmp(name, "RenderSprite") == 0) return comp_RenderSprite;
    if (strcmp(name, "Camera2D") == 0) return comp_Camera2D;
    
    return 0;
}

const char *ecs_get_builtin_component_name(uint64_t component_id) {
    if (component_id == comp_TransformPosition) return "TransformPosition";
    if (component_id == comp_TransformRotation) return "TransformRotation";
    if (component_id == comp_TransformScale) return "TransformScale";
    if (component_id == comp_EntityName) return "EntityName";
    if (component_id == comp_EntityActive) return "EntityActive";
    if (component_id == comp_RenderObject) return "RenderObject";
    if (component_id == comp_RenderMesh) return "RenderMesh";
    if (component_id == comp_EntityScript) return "EntityScript";
    if (component_id == comp_RenderSprite) return "RenderSprite";
    if (component_id == comp_Camera2D) return "Camera2D";
    
    return NULL;
}
