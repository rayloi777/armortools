#include "ecs_components.h"
#include "ecs_dynamic.h"
#include "flecs.h"
#include <stddef.h>
#include <string.h>

static ecs_entity_t comp_2d_position_entity = 0;
static ecs_entity_t comp_2d_rotation_entity = 0;
static ecs_entity_t comp_2d_scale_entity = 0;
static ecs_entity_t comp_EntityName_entity = 0;
static ecs_entity_t comp_EntityActive_entity = 0;
static ecs_entity_t comp_RenderObject_entity = 0;
static ecs_entity_t comp_RenderMesh_entity = 0;
static ecs_entity_t comp_EntityScript_entity = 0;
static ecs_entity_t comp_2d_sprite_entity = 0;
static ecs_entity_t comp_2d_camera_entity = 0;

ecs_entity_t ecs_component_comp_2d_position(void) { return comp_2d_position_entity; }
ecs_entity_t ecs_component_comp_2d_rotation(void) { return comp_2d_rotation_entity; }
ecs_entity_t ecs_component_comp_2d_scale(void) { return comp_2d_scale_entity; }
ecs_entity_t ecs_component_EntityName(void) { return comp_EntityName_entity; }
ecs_entity_t ecs_component_EntityActive(void) { return comp_EntityActive_entity; }
ecs_entity_t ecs_component_RenderObject(void) { return comp_RenderObject_entity; }
ecs_entity_t ecs_component_RenderMesh(void) { return comp_RenderMesh_entity; }
ecs_entity_t ecs_component_EntityScript(void) { return comp_EntityScript_entity; }
ecs_entity_t ecs_component_comp_2d_sprite(void) { return comp_2d_sprite_entity; }
ecs_entity_t ecs_component_comp_2d_camera(void) { return comp_2d_camera_entity; }

static ecs_entity_t register_component(ecs_world_t *ecs, const char *name, size_t size, size_t alignment) {
    ecs_component_desc_t desc = {0};
    desc.entity = 0;
    desc.type.name = name;
    desc.type.size = (ecs_size_t)size;
    desc.type.alignment = (ecs_size_t)alignment;
    uint64_t id = ecs_component_init(ecs, &desc);

    // No ctor — component data is preserved across archetype moves
    dynamic_component_t *dc = &g_components[g_component_count];
    memset(dc, 0, sizeof(dynamic_component_t));
    strncpy(dc->name, name, sizeof(dc->name) - 1);
    dc->name[sizeof(dc->name) - 1] = '\0';
    dc->flecs_id = id;
    dc->size = size;
    dc->alignment = alignment;
    g_component_count++;

    return id;
}

void ecs_register_components(void *world) {
    if (!world) return;
    ecs_world_t *ecs = (ecs_world_t *)world;

    comp_2d_position_entity = register_component(ecs, "comp_2d_position", sizeof(comp_2d_position), _Alignof(comp_2d_position));
    comp_2d_rotation_entity = register_component(ecs, "comp_2d_rotation", sizeof(comp_2d_rotation), _Alignof(comp_2d_rotation));
    comp_2d_scale_entity = register_component(ecs, "comp_2d_scale", sizeof(comp_2d_scale), _Alignof(comp_2d_scale));
    comp_EntityName_entity = register_component(ecs, "EntityName", sizeof(EntityName), _Alignof(EntityName));
    comp_EntityActive_entity = register_component(ecs, "EntityActive", sizeof(EntityActive), _Alignof(EntityActive));
    comp_RenderObject_entity = register_component(ecs, "RenderObject", sizeof(RenderObject), _Alignof(RenderObject));
    comp_RenderMesh_entity = register_component(ecs, "RenderMesh", sizeof(RenderMesh), _Alignof(RenderMesh));
    comp_EntityScript_entity = register_component(ecs, "EntityScript", sizeof(EntityScript), _Alignof(EntityScript));
    comp_2d_sprite_entity = register_component(ecs, "comp_2d_sprite", sizeof(comp_2d_sprite), _Alignof(comp_2d_sprite));
    comp_2d_camera_entity = register_component(ecs, "comp_2d_camera", sizeof(comp_2d_camera), _Alignof(comp_2d_camera));
}

void ecs_register_builtin_fields(void) {
    uint64_t id;

    id = comp_2d_position_entity;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_position, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_position, y));
    ecs_dynamic_component_add_field(id, "z", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_position, z));

    id = comp_2d_rotation_entity;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_rotation, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_rotation, y));
    ecs_dynamic_component_add_field(id, "z", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_rotation, z));
    ecs_dynamic_component_add_field(id, "w", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_rotation, w));

    id = comp_2d_scale_entity;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_scale, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_scale, y));
    ecs_dynamic_component_add_field(id, "z", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_scale, z));

    id = comp_EntityName_entity;
    ecs_dynamic_component_add_field(id, "value", DYNAMIC_TYPE_PTR, offsetof(EntityName, value));

    id = comp_EntityActive_entity;
    ecs_dynamic_component_add_field(id, "value", DYNAMIC_TYPE_BOOL, offsetof(EntityActive, value));

    id = comp_RenderObject_entity;
    ecs_dynamic_component_add_field(id, "object", DYNAMIC_TYPE_PTR, offsetof(RenderObject, object));
    ecs_dynamic_component_add_field(id, "transform", DYNAMIC_TYPE_PTR, offsetof(RenderObject, transform));
    ecs_dynamic_component_add_field(id, "dirty", DYNAMIC_TYPE_BOOL, offsetof(RenderObject, dirty));

    id = comp_RenderMesh_entity;
    ecs_dynamic_component_add_field(id, "mesh_path", DYNAMIC_TYPE_PTR, offsetof(RenderMesh, mesh_path));
    ecs_dynamic_component_add_field(id, "material_path", DYNAMIC_TYPE_PTR, offsetof(RenderMesh, material_path));
    ecs_dynamic_component_add_field(id, "cast_shadows", DYNAMIC_TYPE_BOOL, offsetof(RenderMesh, cast_shadows));
    ecs_dynamic_component_add_field(id, "receive_shadows", DYNAMIC_TYPE_BOOL, offsetof(RenderMesh, receive_shadows));

    id = comp_EntityScript_entity;
    ecs_dynamic_component_add_field(id, "script_path", DYNAMIC_TYPE_PTR, offsetof(EntityScript, script_path));
    ecs_dynamic_component_add_field(id, "script_ctx", DYNAMIC_TYPE_PTR, offsetof(EntityScript, script_ctx));
    ecs_dynamic_component_add_field(id, "update_fn", DYNAMIC_TYPE_PTR, offsetof(EntityScript, update_fn));
    ecs_dynamic_component_add_field(id, "initialized", DYNAMIC_TYPE_BOOL, offsetof(EntityScript, initialized));

    id = comp_2d_sprite_entity;
    ecs_dynamic_component_add_field(id, "texture_path", DYNAMIC_TYPE_PTR, offsetof(comp_2d_sprite, texture_path));
    ecs_dynamic_component_add_field(id, "region_x", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_sprite, region_x));
    ecs_dynamic_component_add_field(id, "region_y", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_sprite, region_y));
    ecs_dynamic_component_add_field(id, "src_width", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_sprite, src_width));
    ecs_dynamic_component_add_field(id, "src_height", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_sprite, src_height));
    ecs_dynamic_component_add_field(id, "pivot_x", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_sprite, pivot_x));
    ecs_dynamic_component_add_field(id, "pivot_y", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_sprite, pivot_y));
    ecs_dynamic_component_add_field(id, "scale_x", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_sprite, scale_x));
    ecs_dynamic_component_add_field(id, "scale_y", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_sprite, scale_y));
    ecs_dynamic_component_add_field(id, "flip_x", DYNAMIC_TYPE_BOOL, offsetof(comp_2d_sprite, flip_x));
    ecs_dynamic_component_add_field(id, "flip_y", DYNAMIC_TYPE_BOOL, offsetof(comp_2d_sprite, flip_y));
    ecs_dynamic_component_add_field(id, "layer", DYNAMIC_TYPE_INT, offsetof(comp_2d_sprite, layer));
    ecs_dynamic_component_add_field(id, "visible", DYNAMIC_TYPE_BOOL, offsetof(comp_2d_sprite, visible));
    ecs_dynamic_component_add_field(id, "width", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_sprite, width));
    ecs_dynamic_component_add_field(id, "height", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_sprite, height));
    ecs_dynamic_component_add_field(id, "render_object", DYNAMIC_TYPE_PTR, offsetof(comp_2d_sprite, render_object));

    id = comp_2d_camera_entity;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_camera, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_camera, y));
    ecs_dynamic_component_add_field(id, "zoom", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_camera, zoom));
    ecs_dynamic_component_add_field(id, "rotation", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_camera, rotation));
    ecs_dynamic_component_add_field(id, "smoothing", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_camera, smoothing));
    ecs_dynamic_component_add_field(id, "target_x", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_camera, target_x));
    ecs_dynamic_component_add_field(id, "target_y", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_camera, target_y));
    ecs_dynamic_component_add_field(id, "has_target", DYNAMIC_TYPE_BOOL, offsetof(comp_2d_camera, has_target));
    ecs_dynamic_component_add_field(id, "bounds_min_x", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_camera, bounds_min_x));
    ecs_dynamic_component_add_field(id, "bounds_min_y", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_camera, bounds_min_y));
    ecs_dynamic_component_add_field(id, "bounds_max_x", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_camera, bounds_max_x));
    ecs_dynamic_component_add_field(id, "bounds_max_y", DYNAMIC_TYPE_FLOAT, offsetof(comp_2d_camera, bounds_max_y));
    ecs_dynamic_component_add_field(id, "has_bounds", DYNAMIC_TYPE_BOOL, offsetof(comp_2d_camera, has_bounds));
}

uint64_t ecs_get_builtin_component(const char *name) {
    if (!name) return 0;
    if (strcmp(name, "comp_2d_position") == 0) return comp_2d_position_entity;
    if (strcmp(name, "comp_2d_rotation") == 0) return comp_2d_rotation_entity;
    if (strcmp(name, "comp_2d_scale") == 0) return comp_2d_scale_entity;
    if (strcmp(name, "EntityName") == 0) return comp_EntityName_entity;
    if (strcmp(name, "EntityActive") == 0) return comp_EntityActive_entity;
    if (strcmp(name, "RenderObject") == 0) return comp_RenderObject_entity;
    if (strcmp(name, "RenderMesh") == 0) return comp_RenderMesh_entity;
    if (strcmp(name, "EntityScript") == 0) return comp_EntityScript_entity;
    if (strcmp(name, "comp_2d_sprite") == 0) return comp_2d_sprite_entity;
    if (strcmp(name, "comp_2d_camera") == 0) return comp_2d_camera_entity;
    return 0;
}

const char *ecs_get_builtin_component_name(uint64_t component_id) {
    if (component_id == comp_2d_position_entity) return "comp_2d_position";
    if (component_id == comp_2d_rotation_entity) return "comp_2d_rotation";
    if (component_id == comp_2d_scale_entity) return "comp_2d_scale";
    if (component_id == comp_EntityName_entity) return "EntityName";
    if (component_id == comp_EntityActive_entity) return "EntityActive";
    if (component_id == comp_RenderObject_entity) return "RenderObject";
    if (component_id == comp_RenderMesh_entity) return "RenderMesh";
    if (component_id == comp_EntityScript_entity) return "EntityScript";
    if (component_id == comp_2d_sprite_entity) return "comp_2d_sprite";
    if (component_id == comp_2d_camera_entity) return "comp_2d_camera";
    return NULL;
}