#include "asset_loader.h"
#include "ecs/ecs_world.h"
#include "ecs/ecs_components.h"
#include <iron.h>
#include <stdio.h>
#include <string.h>

static game_world_t *g_asset_loader_world = NULL;

void asset_loader_set_world(game_world_t *world) {
    g_asset_loader_world = world;
}

uint64_t asset_loader_load_scene(const char *path) {
    if (!g_asset_loader_world || !path) return 0;

    // Parse .arm file
    scene_t *scene_raw = data_get_scene_raw(path);
    if (!scene_raw) {
        fprintf(stderr, "Asset Loader: failed to load scene '%s'\n", path);
        return 0;
    }

    // Create Iron scene graph if not already initialized
    if (!_scene_root) {
        scene_create(scene_raw);
    }
    else {
        // Scene already initialized — add objects individually
        if (scene_raw->objects != NULL) {
            for (int i = 0; i < scene_raw->objects->length; i++) {
                obj_t *o = (obj_t *)scene_raw->objects->buffer[i];
                scene_create_object(o, scene_raw, _scene_root);
            }
        }
    }

    if (!_scene_root) {
        fprintf(stderr, "Asset Loader: scene_create failed for '%s'\n", path);
        return 0;
    }

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_asset_loader_world);
    if (!ecs) return 0;

    uint64_t first_entity = 0;

    // Sync mesh objects to ECS entities
    if (scene_meshes != NULL) {
        for (int i = 0; i < scene_meshes->length; i++) {
            mesh_object_t *mesh_obj = (mesh_object_t *)scene_meshes->buffer[i];
            if (!mesh_obj || !mesh_obj->base) continue;

            ecs_entity_t e = ecs_new(ecs);
            if (first_entity == 0) first_entity = (uint64_t)e;

            // Read transform from Iron object
            transform_t *t = mesh_obj->base->transform;

            // Set 3D position
            comp_3d_position pos = { t->loc.x, t->loc.y, t->loc.z };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_position(), sizeof(comp_3d_position), &pos);

            // Set 3D rotation
            comp_3d_rotation rot = { t->rot.x, t->rot.y, t->rot.z, t->rot.w };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_rotation(), sizeof(comp_3d_rotation), &rot);

            // Set 3D scale
            comp_3d_scale scl = { t->scale.x, t->scale.y, t->scale.z };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_scale(), sizeof(comp_3d_scale), &scl);

            // Set mesh renderer
            comp_3d_mesh_renderer mr = { .visible = true };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_mesh_renderer(), sizeof(comp_3d_mesh_renderer), &mr);

            // Set RenderObject3D
            RenderObject3D robj = {
                .iron_mesh_object = mesh_obj->base,
                .iron_transform = mesh_obj->base->transform,
                .dirty = true
            };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_RenderObject3D(), sizeof(RenderObject3D), &robj);
        }
    }

    // Sync cameras to ECS entities
    if (scene_cameras != NULL) {
        bool first_cam = true;
        for (int i = 0; i < scene_cameras->length; i++) {
            camera_object_t *cam_obj = (camera_object_t *)scene_cameras->buffer[i];
            if (!cam_obj || !cam_obj->base) continue;

            ecs_entity_t e = ecs_new(ecs);
            if (first_entity == 0) first_entity = (uint64_t)e;

            transform_t *t = cam_obj->base->transform;

            comp_3d_position pos = { t->loc.x, t->loc.y, t->loc.z };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_position(), sizeof(comp_3d_position), &pos);

            comp_3d_rotation rot = { t->rot.x, t->rot.y, t->rot.z, t->rot.w };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_rotation(), sizeof(comp_3d_rotation), &rot);

            comp_3d_scale scl = { t->scale.x, t->scale.y, t->scale.z };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_scale(), sizeof(comp_3d_scale), &scl);

            comp_3d_camera cam = {
                .fov = cam_obj->data->fov,
                .near_plane = cam_obj->data->near_plane,
                .far_plane = cam_obj->data->far_plane,
                .perspective = true,
                .active = first_cam
            };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_camera(), sizeof(comp_3d_camera), &cam);

            first_cam = false;
        }
    }

    printf("Asset Loader: scene '%s' loaded (%d meshes, %d cameras)\n",
        path,
        scene_meshes ? scene_meshes->length : 0,
        scene_cameras ? scene_cameras->length : 0);

    return first_entity;
}

int asset_loader_load_mesh(uint64_t entity, const char *mesh_path, const char *material_path) {
    if (!g_asset_loader_world || !entity || !mesh_path) return -1;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_asset_loader_world);
    if (!ecs) return -1;

    ecs_entity_t e = (ecs_entity_t)entity;
    if (!ecs_is_alive(ecs, e)) return -1;

    // Ensure scene root exists
    if (!_scene_root) {
        scene_t *empty = GC_ALLOC_INIT(scene_t, {0});
        scene_create(empty);
    }

    mesh_data_t *mesh_data = data_get_mesh(mesh_path, mesh_path);
    if (!mesh_data) {
        fprintf(stderr, "Asset Loader: failed to load mesh '%s'\n", mesh_path);
        return -1;
    }

    material_data_t *mat_data = NULL;
    if (material_path != NULL) {
        mat_data = data_get_material(material_path, material_path);
    }

    mesh_object_t *mesh_obj = mesh_object_create(mesh_data, mat_data);
    if (!mesh_obj) {
        fprintf(stderr, "Asset Loader: failed to create mesh object\n");
        return -1;
    }

    object_set_parent(mesh_obj->base, _scene_root);

    // Update or add RenderObject3D
    RenderObject3D robj = {
        .iron_mesh_object = mesh_obj->base,
        .iron_transform = mesh_obj->base->transform,
        .dirty = true
    };
    ecs_set_id(ecs, e, (ecs_id_t)ecs_component_RenderObject3D(), sizeof(RenderObject3D), &robj);

    // Update mesh renderer
    comp_3d_mesh_renderer mr = { .visible = true };
    ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_mesh_renderer(), sizeof(comp_3d_mesh_renderer), &mr);

    return 0;
}
