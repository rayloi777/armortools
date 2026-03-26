#include "ecs_components.h"
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

ecs_entity_t ecs_component_TransformPosition(void) { return comp_TransformPosition; }
ecs_entity_t ecs_component_TransformRotation(void) { return comp_TransformRotation; }
ecs_entity_t ecs_component_TransformScale(void) { return comp_TransformScale; }
ecs_entity_t ecs_component_EntityName(void) { return comp_EntityName; }
ecs_entity_t ecs_component_EntityActive(void) { return comp_EntityActive; }
ecs_entity_t ecs_component_RenderObject(void) { return comp_RenderObject; }
ecs_entity_t ecs_component_RenderMesh(void) { return comp_RenderMesh; }
ecs_entity_t ecs_component_EntityScript(void) { return comp_EntityScript; }

static ecs_entity_t register_component(ecs_world_t *ecs, const char *name, size_t size, size_t alignment) {
    ecs_component_desc_t desc = {0};
    desc.entity = 0;
    desc.type.name = name;
    desc.type.size = (ecs_size_t)size;
    desc.type.alignment = (ecs_size_t)alignment;
    return ecs_component_init(ecs, &desc);
}

void ecs_register_components(void *world) {
    if (!world) return;
    ecs_world_t *ecs = (ecs_world_t *)world;
    
    comp_TransformPosition = register_component(ecs, "TransformPosition", sizeof(TransformPosition), ECS_ALIGNOF(TransformPosition));
    comp_TransformRotation = register_component(ecs, "TransformRotation", sizeof(TransformRotation), ECS_ALIGNOF(TransformRotation));
    comp_TransformScale = register_component(ecs, "TransformScale", sizeof(TransformScale), ECS_ALIGNOF(TransformScale));
    comp_EntityName = register_component(ecs, "EntityName", sizeof(EntityName), ECS_ALIGNOF(EntityName));
    comp_EntityActive = register_component(ecs, "EntityActive", sizeof(EntityActive), ECS_ALIGNOF(EntityActive));
    comp_RenderObject = register_component(ecs, "RenderObject", sizeof(RenderObject), ECS_ALIGNOF(RenderObject));
    comp_RenderMesh = register_component(ecs, "RenderMesh", sizeof(RenderMesh), ECS_ALIGNOF(RenderMesh));
    comp_EntityScript = register_component(ecs, "EntityScript", sizeof(EntityScript), ECS_ALIGNOF(EntityScript));
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
    
    return 0;
}
