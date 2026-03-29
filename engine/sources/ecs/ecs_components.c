#include "ecs_components.h"
#include "ecs_dynamic.h"
#include "flecs.h"
#include <stddef.h>
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
static ecs_entity_t comp_RenderRect = 0;
static ecs_entity_t comp_RenderCircle = 0;
static ecs_entity_t comp_RenderLine = 0;
static ecs_entity_t comp_RenderText = 0;

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
ecs_entity_t ecs_component_RenderRect(void) { return comp_RenderRect; }
ecs_entity_t ecs_component_RenderCircle(void) { return comp_RenderCircle; }
ecs_entity_t ecs_component_RenderLine(void) { return comp_RenderLine; }
ecs_entity_t ecs_component_RenderText(void) { return comp_RenderText; }

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

    comp_TransformPosition = register_component(ecs, "TransformPosition", sizeof(TransformPosition), _Alignof(TransformPosition));
    comp_TransformRotation = register_component(ecs, "TransformRotation", sizeof(TransformRotation), _Alignof(TransformRotation));
    comp_TransformScale = register_component(ecs, "TransformScale", sizeof(TransformScale), _Alignof(TransformScale));
    comp_EntityName = register_component(ecs, "EntityName", sizeof(EntityName), _Alignof(EntityName));
    comp_EntityActive = register_component(ecs, "EntityActive", sizeof(EntityActive), _Alignof(EntityActive));
    comp_RenderObject = register_component(ecs, "RenderObject", sizeof(RenderObject), _Alignof(RenderObject));
    comp_RenderMesh = register_component(ecs, "RenderMesh", sizeof(RenderMesh), _Alignof(RenderMesh));
    comp_EntityScript = register_component(ecs, "EntityScript", sizeof(EntityScript), _Alignof(EntityScript));
    comp_RenderSprite = register_component(ecs, "RenderSprite", sizeof(RenderSprite), _Alignof(RenderSprite));
    comp_Camera2D = register_component(ecs, "Camera2D", sizeof(Camera2D), _Alignof(Camera2D));
    comp_RenderRect = register_component(ecs, "RenderRect", sizeof(RenderRect), _Alignof(RenderRect));
    comp_RenderCircle = register_component(ecs, "RenderCircle", sizeof(RenderCircle), _Alignof(RenderCircle));
    comp_RenderLine = register_component(ecs, "RenderLine", sizeof(RenderLine), _Alignof(RenderLine));
    comp_RenderText = register_component(ecs, "RenderText", sizeof(RenderText), _Alignof(RenderText));
}

void ecs_register_builtin_fields(void) {
    uint64_t id;

    id = comp_TransformPosition;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(TransformPosition, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(TransformPosition, y));
    ecs_dynamic_component_add_field(id, "z", DYNAMIC_TYPE_FLOAT, offsetof(TransformPosition, z));

    id = comp_TransformRotation;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(TransformRotation, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(TransformRotation, y));
    ecs_dynamic_component_add_field(id, "z", DYNAMIC_TYPE_FLOAT, offsetof(TransformRotation, z));
    ecs_dynamic_component_add_field(id, "w", DYNAMIC_TYPE_FLOAT, offsetof(TransformRotation, w));

    id = comp_TransformScale;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(TransformScale, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(TransformScale, y));
    ecs_dynamic_component_add_field(id, "z", DYNAMIC_TYPE_FLOAT, offsetof(TransformScale, z));

    id = comp_EntityName;
    ecs_dynamic_component_add_field(id, "value", DYNAMIC_TYPE_PTR, offsetof(EntityName, value));

    id = comp_EntityActive;
    ecs_dynamic_component_add_field(id, "value", DYNAMIC_TYPE_BOOL, offsetof(EntityActive, value));

    id = comp_RenderObject;
    ecs_dynamic_component_add_field(id, "object", DYNAMIC_TYPE_PTR, offsetof(RenderObject, object));
    ecs_dynamic_component_add_field(id, "transform", DYNAMIC_TYPE_PTR, offsetof(RenderObject, transform));
    ecs_dynamic_component_add_field(id, "dirty", DYNAMIC_TYPE_BOOL, offsetof(RenderObject, dirty));

    id = comp_RenderMesh;
    ecs_dynamic_component_add_field(id, "mesh_path", DYNAMIC_TYPE_PTR, offsetof(RenderMesh, mesh_path));
    ecs_dynamic_component_add_field(id, "material_path", DYNAMIC_TYPE_PTR, offsetof(RenderMesh, material_path));
    ecs_dynamic_component_add_field(id, "cast_shadows", DYNAMIC_TYPE_BOOL, offsetof(RenderMesh, cast_shadows));
    ecs_dynamic_component_add_field(id, "receive_shadows", DYNAMIC_TYPE_BOOL, offsetof(RenderMesh, receive_shadows));

    id = comp_EntityScript;
    ecs_dynamic_component_add_field(id, "script_path", DYNAMIC_TYPE_PTR, offsetof(EntityScript, script_path));
    ecs_dynamic_component_add_field(id, "script_ctx", DYNAMIC_TYPE_PTR, offsetof(EntityScript, script_ctx));
    ecs_dynamic_component_add_field(id, "update_fn", DYNAMIC_TYPE_PTR, offsetof(EntityScript, update_fn));
    ecs_dynamic_component_add_field(id, "initialized", DYNAMIC_TYPE_BOOL, offsetof(EntityScript, initialized));

    id = comp_RenderSprite;
    ecs_dynamic_component_add_field(id, "texture_path", DYNAMIC_TYPE_PTR, offsetof(RenderSprite, texture_path));
    ecs_dynamic_component_add_field(id, "region_x", DYNAMIC_TYPE_FLOAT, offsetof(RenderSprite, region_x));
    ecs_dynamic_component_add_field(id, "region_y", DYNAMIC_TYPE_FLOAT, offsetof(RenderSprite, region_y));
    ecs_dynamic_component_add_field(id, "src_width", DYNAMIC_TYPE_FLOAT, offsetof(RenderSprite, src_width));
    ecs_dynamic_component_add_field(id, "src_height", DYNAMIC_TYPE_FLOAT, offsetof(RenderSprite, src_height));
    ecs_dynamic_component_add_field(id, "pivot_x", DYNAMIC_TYPE_FLOAT, offsetof(RenderSprite, pivot_x));
    ecs_dynamic_component_add_field(id, "pivot_y", DYNAMIC_TYPE_FLOAT, offsetof(RenderSprite, pivot_y));
    ecs_dynamic_component_add_field(id, "scale_x", DYNAMIC_TYPE_FLOAT, offsetof(RenderSprite, scale_x));
    ecs_dynamic_component_add_field(id, "scale_y", DYNAMIC_TYPE_FLOAT, offsetof(RenderSprite, scale_y));
    ecs_dynamic_component_add_field(id, "flip_x", DYNAMIC_TYPE_BOOL, offsetof(RenderSprite, flip_x));
    ecs_dynamic_component_add_field(id, "flip_y", DYNAMIC_TYPE_BOOL, offsetof(RenderSprite, flip_y));
    ecs_dynamic_component_add_field(id, "layer", DYNAMIC_TYPE_INT, offsetof(RenderSprite, layer));
    ecs_dynamic_component_add_field(id, "visible", DYNAMIC_TYPE_BOOL, offsetof(RenderSprite, visible));
    ecs_dynamic_component_add_field(id, "width", DYNAMIC_TYPE_FLOAT, offsetof(RenderSprite, width));
    ecs_dynamic_component_add_field(id, "height", DYNAMIC_TYPE_FLOAT, offsetof(RenderSprite, height));
    ecs_dynamic_component_add_field(id, "render_object", DYNAMIC_TYPE_PTR, offsetof(RenderSprite, render_object));

    id = comp_Camera2D;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(Camera2D, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(Camera2D, y));
    ecs_dynamic_component_add_field(id, "zoom", DYNAMIC_TYPE_FLOAT, offsetof(Camera2D, zoom));
    ecs_dynamic_component_add_field(id, "rotation", DYNAMIC_TYPE_FLOAT, offsetof(Camera2D, rotation));
    ecs_dynamic_component_add_field(id, "smoothing", DYNAMIC_TYPE_FLOAT, offsetof(Camera2D, smoothing));
    ecs_dynamic_component_add_field(id, "target_x", DYNAMIC_TYPE_FLOAT, offsetof(Camera2D, target_x));
    ecs_dynamic_component_add_field(id, "target_y", DYNAMIC_TYPE_FLOAT, offsetof(Camera2D, target_y));
    ecs_dynamic_component_add_field(id, "has_target", DYNAMIC_TYPE_BOOL, offsetof(Camera2D, has_target));
    ecs_dynamic_component_add_field(id, "bounds_min_x", DYNAMIC_TYPE_FLOAT, offsetof(Camera2D, bounds_min_x));
    ecs_dynamic_component_add_field(id, "bounds_min_y", DYNAMIC_TYPE_FLOAT, offsetof(Camera2D, bounds_min_y));
    ecs_dynamic_component_add_field(id, "bounds_max_x", DYNAMIC_TYPE_FLOAT, offsetof(Camera2D, bounds_max_x));
    ecs_dynamic_component_add_field(id, "bounds_max_y", DYNAMIC_TYPE_FLOAT, offsetof(Camera2D, bounds_max_y));
    ecs_dynamic_component_add_field(id, "has_bounds", DYNAMIC_TYPE_BOOL, offsetof(Camera2D, has_bounds));

    id = comp_RenderRect;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(RenderRect, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(RenderRect, y));
    ecs_dynamic_component_add_field(id, "width", DYNAMIC_TYPE_FLOAT, offsetof(RenderRect, width));
    ecs_dynamic_component_add_field(id, "height", DYNAMIC_TYPE_FLOAT, offsetof(RenderRect, height));
    ecs_dynamic_component_add_field(id, "color", DYNAMIC_TYPE_INT, offsetof(RenderRect, color));
    ecs_dynamic_component_add_field(id, "layer", DYNAMIC_TYPE_INT, offsetof(RenderRect, layer));
    ecs_dynamic_component_add_field(id, "filled", DYNAMIC_TYPE_BOOL, offsetof(RenderRect, filled));
    ecs_dynamic_component_add_field(id, "strength", DYNAMIC_TYPE_FLOAT, offsetof(RenderRect, strength));
    ecs_dynamic_component_add_field(id, "visible", DYNAMIC_TYPE_BOOL, offsetof(RenderRect, visible));

    id = comp_RenderCircle;
    ecs_dynamic_component_add_field(id, "cx", DYNAMIC_TYPE_FLOAT, offsetof(RenderCircle, cx));
    ecs_dynamic_component_add_field(id, "cy", DYNAMIC_TYPE_FLOAT, offsetof(RenderCircle, cy));
    ecs_dynamic_component_add_field(id, "radius", DYNAMIC_TYPE_FLOAT, offsetof(RenderCircle, radius));
    ecs_dynamic_component_add_field(id, "segments", DYNAMIC_TYPE_INT, offsetof(RenderCircle, segments));
    ecs_dynamic_component_add_field(id, "color", DYNAMIC_TYPE_INT, offsetof(RenderCircle, color));
    ecs_dynamic_component_add_field(id, "layer", DYNAMIC_TYPE_INT, offsetof(RenderCircle, layer));
    ecs_dynamic_component_add_field(id, "filled", DYNAMIC_TYPE_BOOL, offsetof(RenderCircle, filled));
    ecs_dynamic_component_add_field(id, "strength", DYNAMIC_TYPE_FLOAT, offsetof(RenderCircle, strength));
    ecs_dynamic_component_add_field(id, "visible", DYNAMIC_TYPE_BOOL, offsetof(RenderCircle, visible));

    id = comp_RenderLine;
    ecs_dynamic_component_add_field(id, "x0", DYNAMIC_TYPE_FLOAT, offsetof(RenderLine, x0));
    ecs_dynamic_component_add_field(id, "y0", DYNAMIC_TYPE_FLOAT, offsetof(RenderLine, y0));
    ecs_dynamic_component_add_field(id, "x1", DYNAMIC_TYPE_FLOAT, offsetof(RenderLine, x1));
    ecs_dynamic_component_add_field(id, "y1", DYNAMIC_TYPE_FLOAT, offsetof(RenderLine, y1));
    ecs_dynamic_component_add_field(id, "strength", DYNAMIC_TYPE_FLOAT, offsetof(RenderLine, strength));
    ecs_dynamic_component_add_field(id, "color", DYNAMIC_TYPE_INT, offsetof(RenderLine, color));
    ecs_dynamic_component_add_field(id, "layer", DYNAMIC_TYPE_INT, offsetof(RenderLine, layer));
    ecs_dynamic_component_add_field(id, "visible", DYNAMIC_TYPE_BOOL, offsetof(RenderLine, visible));

    id = comp_RenderText;
    ecs_dynamic_component_add_field(id, "text", DYNAMIC_TYPE_PTR, offsetof(RenderText, text));
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(RenderText, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(RenderText, y));
    ecs_dynamic_component_add_field(id, "font_path", DYNAMIC_TYPE_PTR, offsetof(RenderText, font_path));
    ecs_dynamic_component_add_field(id, "font_size", DYNAMIC_TYPE_INT, offsetof(RenderText, font_size));
    ecs_dynamic_component_add_field(id, "color", DYNAMIC_TYPE_INT, offsetof(RenderText, color));
    ecs_dynamic_component_add_field(id, "layer", DYNAMIC_TYPE_INT, offsetof(RenderText, layer));
    ecs_dynamic_component_add_field(id, "visible", DYNAMIC_TYPE_BOOL, offsetof(RenderText, visible));
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
    if (strcmp(name, "RenderRect") == 0) return comp_RenderRect;
    if (strcmp(name, "RenderCircle") == 0) return comp_RenderCircle;
    if (strcmp(name, "RenderLine") == 0) return comp_RenderLine;
    if (strcmp(name, "RenderText") == 0) return comp_RenderText;
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
    if (component_id == comp_RenderRect) return "RenderRect";
    if (component_id == comp_RenderCircle) return "RenderCircle";
    if (component_id == comp_RenderLine) return "RenderLine";
    if (component_id == comp_RenderText) return "RenderText";
    return NULL;
}

