#include "ecs_bridge.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "flecs.h"

static game_world_t *g_game_world = NULL;
static ecs_entity_t g_bridge_system = 0;

static void bridge_system(ecs_iter_t *it) {
    TransformPosition *pos = ecs_field(it, TransformPosition, 1);
    TransformRotation *rot = ecs_field(it, TransformRotation, 2);
    TransformScale *scale = ecs_field(it, TransformScale, 3);
    RenderObject *render_obj = ecs_field(it, RenderObject, 4);
    RenderMesh *mesh = ecs_field(it, RenderMesh, 5);

    for (int i = 0; i < it->count; i++) {
        if (render_obj[i].object == NULL && mesh[i].mesh_path != NULL) {
            mesh_data_t *mesh_data = data_get_mesh(mesh[i].mesh_path, mesh[i].mesh_path);
            material_data_t *mat_data = NULL;
            if (mesh[i].material_path != NULL) {
                mat_data = data_get_material(mesh[i].material_path, mesh[i].material_path);
            }
            
            if (mesh_data != NULL) {
                mesh_object_t *mesh_obj = mesh_object_create(mesh_data, mat_data);
                if (mesh_obj != NULL) {
                    render_obj[i].object = mesh_obj->base;
                    render_obj[i].transform = mesh_obj->base->transform;
                    render_obj[i].dirty = true;
                    
                    object_set_parent(mesh_obj->base, _scene_root);
                }
            }
        }
        
        if (render_obj[i].transform != NULL && render_obj[i].dirty) {
            transform_t *t = (transform_t *)render_obj[i].transform;
            
            t->loc.x = pos[i].x;
            t->loc.y = pos[i].y;
            t->loc.z = pos[i].z;
            
            t->rot.x = rot[i].x;
            t->rot.y = rot[i].y;
            t->rot.z = rot[i].z;
            t->rot.w = rot[i].w;
            
            t->scale.x = scale[i].x;
            t->scale.y = scale[i].y;
            t->scale.z = scale[i].z;
            
            t->dirty = true;
            transform_build_matrix(t);
            
            render_obj[i].dirty = false;
        }
    }
}

void ecs_bridge_init(void) {
    if (!g_game_world) {
        fprintf(stderr, "ECS Bridge: game world not set\n");
        return;
    }
    
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_game_world);
    if (!ecs) {
        fprintf(stderr, "Failed to get ECS world for bridge\n");
        return;
    }
    
    ecs_system_desc_t sys_desc = {0};
    sys_desc.entity = 0;
    sys_desc.query.terms[0].id = ecs_component_TransformPosition();
    sys_desc.query.terms[1].id = ecs_component_TransformRotation();
    sys_desc.query.terms[2].id = ecs_component_TransformScale();
    sys_desc.query.terms[3].id = ecs_component_RenderObject();
    sys_desc.query.terms[4].id = ecs_component_RenderMesh();
    sys_desc.callback = bridge_system;
    
    g_bridge_system = ecs_system_init(ecs, &sys_desc);
    
    if (g_bridge_system == 0) {
        fprintf(stderr, "Failed to create bridge system\n");
        return;
    }
    
    ecs_add_pair(ecs, g_bridge_system, EcsDependsOn, EcsPreStore);
    
    printf("ECS Bridge initialized\n");
}

void ecs_bridge_shutdown(void) {
    g_bridge_system = 0;
    g_game_world = NULL;
    printf("ECS Bridge shutdown\n");
}

void ecs_bridge_set_world(game_world_t *world) {
    g_game_world = world;
}

void ecs_bridge_sync_transform(uint64_t entity) {
    if (!g_game_world || !entity) return;
    
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_game_world);
    if (!ecs) return;
    
    ecs_entity_t e = (ecs_entity_t)entity;
    
    RenderObject *render_obj = (RenderObject *)ecs_get_id(ecs, e, ecs_component_RenderObject());
    if (!render_obj || !render_obj->transform) return;
    
    TransformPosition *pos = (TransformPosition *)ecs_get_id(ecs, e, ecs_component_TransformPosition());
    TransformRotation *rot = (TransformRotation *)ecs_get_id(ecs, e, ecs_component_TransformRotation());
    TransformScale *scale = (TransformScale *)ecs_get_id(ecs, e, ecs_component_TransformScale());
    
    if (!pos || !rot || !scale) return;
    
    transform_t *t = (transform_t *)render_obj->transform;
    
    t->loc.x = pos->x;
    t->loc.y = pos->y;
    t->loc.z = pos->z;
    
    t->rot.x = rot->x;
    t->rot.y = rot->y;
    t->rot.z = rot->z;
    t->rot.w = rot->w;
    
    t->scale.x = scale->x;
    t->scale.y = scale->y;
    t->scale.z = scale->z;
    
    t->dirty = true;
    transform_build_matrix(t);
}

void ecs_bridge_create_render_object(uint64_t entity) {
    if (!g_game_world || !entity) return;
    
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_game_world);
    if (!ecs) return;
    
    ecs_entity_t e = (ecs_entity_t)entity;
    
    RenderObject *render_obj = (RenderObject *)ecs_get_id(ecs, e, ecs_component_RenderObject());
    RenderMesh *mesh = (RenderMesh *)ecs_get_id(ecs, e, ecs_component_RenderMesh());
    
    if (!render_obj || !mesh || render_obj->object != NULL) return;
    
    if (mesh->mesh_path == NULL) return;
    
    mesh_data_t *mesh_data = data_get_mesh(mesh->mesh_path, mesh->mesh_path);
    material_data_t *mat_data = NULL;
    if (mesh->material_path != NULL) {
        mat_data = data_get_material(mesh->material_path, mesh->material_path);
    }
    
    if (mesh_data != NULL) {
        mesh_object_t *mesh_obj = mesh_object_create(mesh_data, mat_data);
        if (mesh_obj != NULL) {
            render_obj->object = mesh_obj->base;
            render_obj->transform = mesh_obj->base->transform;
            render_obj->dirty = true;
            
            object_set_parent(mesh_obj->base, _scene_root);
        }
    }
}

void ecs_bridge_destroy_render_object(uint64_t entity) {
    if (!g_game_world || !entity) return;
    
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_game_world);
    if (!ecs) return;
    
    ecs_entity_t e = (ecs_entity_t)entity;
    
    RenderObject *render_obj = (RenderObject *)ecs_get_id(ecs, e, ecs_component_RenderObject());
    
    if (render_obj && render_obj->object) {
        object_remove((object_t *)render_obj->object);
        render_obj->object = NULL;
        render_obj->transform = NULL;
        render_obj->dirty = false;
    }
}
