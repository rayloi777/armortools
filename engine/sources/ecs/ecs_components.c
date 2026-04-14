#include "ecs_components.h"
#include "ecs_dynamic.h"
#include "../components/transform.h"
#include "../components/camera.h"
#include "../components/mesh_renderer.h"
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
static ecs_entity_t comp_3d_position_entity = 0;
static ecs_entity_t comp_3d_rotation_entity = 0;
static ecs_entity_t comp_3d_scale_entity = 0;
static ecs_entity_t comp_3d_camera_entity = 0;
static ecs_entity_t comp_3d_mesh_renderer_entity = 0;
static ecs_entity_t comp_RenderObject3D_entity = 0;
static ecs_entity_t comp_directional_light_entity = 0;
static ecs_entity_t comp_3d_material_entity = 0;
static ecs_entity_t comp_3d_transparency_entity = 0;
static ecs_entity_t comp_3d_particle_entity = 0;

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
ecs_entity_t ecs_component_comp_3d_position(void) { return comp_3d_position_entity; }
ecs_entity_t ecs_component_comp_3d_rotation(void) { return comp_3d_rotation_entity; }
ecs_entity_t ecs_component_comp_3d_scale(void) { return comp_3d_scale_entity; }
ecs_entity_t ecs_component_comp_3d_camera(void) { return comp_3d_camera_entity; }
ecs_entity_t ecs_component_comp_3d_mesh_renderer(void) { return comp_3d_mesh_renderer_entity; }
ecs_entity_t ecs_component_RenderObject3D(void) { return comp_RenderObject3D_entity; }
ecs_entity_t ecs_component_comp_directional_light(void) { return comp_directional_light_entity; }
ecs_entity_t ecs_component_comp_3d_material(void) { return comp_3d_material_entity; }
ecs_entity_t ecs_component_comp_3d_transparency(void) { return comp_3d_transparency_entity; }
ecs_entity_t ecs_component_comp_3d_particle(void) { return comp_3d_particle_entity; }

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
    comp_3d_position_entity = register_component(ecs, "comp_3d_position", sizeof(comp_3d_position), _Alignof(comp_3d_position));
    comp_3d_rotation_entity = register_component(ecs, "comp_3d_rotation", sizeof(comp_3d_rotation), _Alignof(comp_3d_rotation));
    comp_3d_scale_entity = register_component(ecs, "comp_3d_scale", sizeof(comp_3d_scale), _Alignof(comp_3d_scale));
    comp_3d_camera_entity = register_component(ecs, "comp_3d_camera", sizeof(comp_3d_camera), _Alignof(comp_3d_camera));
    comp_3d_mesh_renderer_entity = register_component(ecs, "comp_3d_mesh_renderer", sizeof(comp_3d_mesh_renderer), _Alignof(comp_3d_mesh_renderer));
    comp_RenderObject3D_entity = register_component(ecs, "RenderObject3D", sizeof(RenderObject3D), _Alignof(RenderObject3D));
    comp_directional_light_entity = register_component(ecs, "comp_directional_light", sizeof(comp_directional_light), _Alignof(comp_directional_light));
    comp_3d_material_entity = register_component(ecs, "comp_3d_material", sizeof(comp_3d_material), _Alignof(comp_3d_material));
    comp_3d_transparency_entity = register_component(ecs, "comp_3d_transparency", sizeof(comp_3d_transparency), _Alignof(comp_3d_transparency));
    comp_3d_particle_entity = register_component(ecs, "comp_3d_particle", sizeof(comp_3d_particle), _Alignof(comp_3d_particle));
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

    id = comp_3d_position_entity;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_position, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_position, y));
    ecs_dynamic_component_add_field(id, "z", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_position, z));

    id = comp_3d_rotation_entity;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_rotation, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_rotation, y));
    ecs_dynamic_component_add_field(id, "z", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_rotation, z));
    ecs_dynamic_component_add_field(id, "w", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_rotation, w));

    id = comp_3d_scale_entity;
    ecs_dynamic_component_add_field(id, "x", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_scale, x));
    ecs_dynamic_component_add_field(id, "y", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_scale, y));
    ecs_dynamic_component_add_field(id, "z", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_scale, z));

    id = comp_3d_camera_entity;
    ecs_dynamic_component_add_field(id, "fov", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_camera, fov));
    ecs_dynamic_component_add_field(id, "near_plane", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_camera, near_plane));
    ecs_dynamic_component_add_field(id, "far_plane", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_camera, far_plane));
    ecs_dynamic_component_add_field(id, "perspective", DYNAMIC_TYPE_BOOL, offsetof(comp_3d_camera, perspective));
    ecs_dynamic_component_add_field(id, "active", DYNAMIC_TYPE_BOOL, offsetof(comp_3d_camera, active));
    ecs_dynamic_component_add_field(id, "ortho_left", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_camera, ortho_left));
    ecs_dynamic_component_add_field(id, "ortho_right", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_camera, ortho_right));
    ecs_dynamic_component_add_field(id, "ortho_bottom", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_camera, ortho_bottom));
    ecs_dynamic_component_add_field(id, "ortho_top", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_camera, ortho_top));

    id = comp_3d_mesh_renderer_entity;
    ecs_dynamic_component_add_field(id, "mesh_path", DYNAMIC_TYPE_PTR, offsetof(comp_3d_mesh_renderer, mesh_path));
    ecs_dynamic_component_add_field(id, "material_path", DYNAMIC_TYPE_PTR, offsetof(comp_3d_mesh_renderer, material_path));
    ecs_dynamic_component_add_field(id, "visible", DYNAMIC_TYPE_BOOL, offsetof(comp_3d_mesh_renderer, visible));

    id = comp_RenderObject3D_entity;
    ecs_dynamic_component_add_field(id, "iron_mesh_object", DYNAMIC_TYPE_PTR, offsetof(RenderObject3D, iron_mesh_object));
    ecs_dynamic_component_add_field(id, "iron_transform", DYNAMIC_TYPE_PTR, offsetof(RenderObject3D, iron_transform));
    ecs_dynamic_component_add_field(id, "dirty", DYNAMIC_TYPE_BOOL, offsetof(RenderObject3D, dirty));

    id = comp_directional_light_entity;
    ecs_dynamic_component_add_field(id, "dir_x", DYNAMIC_TYPE_FLOAT, offsetof(comp_directional_light, dir_x));
    ecs_dynamic_component_add_field(id, "dir_y", DYNAMIC_TYPE_FLOAT, offsetof(comp_directional_light, dir_y));
    ecs_dynamic_component_add_field(id, "dir_z", DYNAMIC_TYPE_FLOAT, offsetof(comp_directional_light, dir_z));
    ecs_dynamic_component_add_field(id, "color_r", DYNAMIC_TYPE_FLOAT, offsetof(comp_directional_light, color_r));
    ecs_dynamic_component_add_field(id, "color_g", DYNAMIC_TYPE_FLOAT, offsetof(comp_directional_light, color_g));
    ecs_dynamic_component_add_field(id, "color_b", DYNAMIC_TYPE_FLOAT, offsetof(comp_directional_light, color_b));
    ecs_dynamic_component_add_field(id, "strength", DYNAMIC_TYPE_FLOAT, offsetof(comp_directional_light, strength));
    ecs_dynamic_component_add_field(id, "enabled", DYNAMIC_TYPE_BOOL, offsetof(comp_directional_light, enabled));

    id = comp_3d_material_entity;
    ecs_dynamic_component_add_field(id, "metallic", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, metallic));
    ecs_dynamic_component_add_field(id, "roughness", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, roughness));
    ecs_dynamic_component_add_field(id, "albedo_r", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, albedo_r));
    ecs_dynamic_component_add_field(id, "albedo_g", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, albedo_g));
    ecs_dynamic_component_add_field(id, "albedo_b", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, albedo_b));
    ecs_dynamic_component_add_field(id, "emissive_r", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, emissive_r));
    ecs_dynamic_component_add_field(id, "emissive_g", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, emissive_g));
    ecs_dynamic_component_add_field(id, "emissive_b", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, emissive_b));
    ecs_dynamic_component_add_field(id, "ao", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_material, ao));

    id = comp_3d_transparency_entity;
    ecs_dynamic_component_add_field(id, "opacity", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_transparency, opacity));
    ecs_dynamic_component_add_field(id, "blend_mode", DYNAMIC_TYPE_INT, offsetof(comp_3d_transparency, blend_mode));
    ecs_dynamic_component_add_field(id, "two_sided", DYNAMIC_TYPE_BOOL, offsetof(comp_3d_transparency, two_sided));

    id = comp_3d_particle_entity;
    ecs_dynamic_component_add_field(id, "lifetime", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_particle, lifetime));
    ecs_dynamic_component_add_field(id, "max_lifetime", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_particle, max_lifetime));
    ecs_dynamic_component_add_field(id, "velocity_x", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_particle, velocity_x));
    ecs_dynamic_component_add_field(id, "velocity_y", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_particle, velocity_y));
    ecs_dynamic_component_add_field(id, "velocity_z", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_particle, velocity_z));
    ecs_dynamic_component_add_field(id, "size", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_particle, size));
    ecs_dynamic_component_add_field(id, "color_r", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_particle, color_r));
    ecs_dynamic_component_add_field(id, "color_g", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_particle, color_g));
    ecs_dynamic_component_add_field(id, "color_b", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_particle, color_b));
    ecs_dynamic_component_add_field(id, "color_a", DYNAMIC_TYPE_FLOAT, offsetof(comp_3d_particle, color_a));
    ecs_dynamic_component_add_field(id, "alive", DYNAMIC_TYPE_BOOL, offsetof(comp_3d_particle, alive));
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
    if (strcmp(name, "comp_3d_position") == 0) return comp_3d_position_entity;
    if (strcmp(name, "comp_3d_rotation") == 0) return comp_3d_rotation_entity;
    if (strcmp(name, "comp_3d_scale") == 0) return comp_3d_scale_entity;
    if (strcmp(name, "comp_3d_camera") == 0) return comp_3d_camera_entity;
    if (strcmp(name, "comp_3d_mesh_renderer") == 0) return comp_3d_mesh_renderer_entity;
    if (strcmp(name, "RenderObject3D") == 0) return comp_RenderObject3D_entity;
    if (strcmp(name, "comp_directional_light") == 0) return comp_directional_light_entity;
    if (strcmp(name, "comp_3d_material") == 0) return comp_3d_material_entity;
    if (strcmp(name, "comp_3d_transparency") == 0) return comp_3d_transparency_entity;
    if (strcmp(name, "comp_3d_particle") == 0) return comp_3d_particle_entity;
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
    if (component_id == comp_3d_position_entity) return "comp_3d_position";
    if (component_id == comp_3d_rotation_entity) return "comp_3d_rotation";
    if (component_id == comp_3d_scale_entity) return "comp_3d_scale";
    if (component_id == comp_3d_camera_entity) return "comp_3d_camera";
    if (component_id == comp_3d_mesh_renderer_entity) return "comp_3d_mesh_renderer";
    if (component_id == comp_RenderObject3D_entity) return "RenderObject3D";
    if (component_id == comp_directional_light_entity) return "comp_directional_light";
    if (component_id == comp_3d_material_entity) return "comp_3d_material";
    if (component_id == comp_3d_transparency_entity) return "comp_3d_transparency";
    if (component_id == comp_3d_particle_entity) return "comp_3d_particle";
    return NULL;
}