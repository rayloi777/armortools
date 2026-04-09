#include "scene_3d_api.h"
#include "ecs/ecs_world.h"
#include "ecs/ecs_components.h"
#include "asset_loader.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <minic.h>

#ifndef DEG_TO_RAD
#define DEG_TO_RAD (3.14159265358979323846f / 180.0f)
#endif

// Access g_runtime_world from runtime_api.c
extern game_world_t *g_runtime_world;

// Helper: extract 64-bit entity ID from minic value
static uint64_t extract_entity_id3d(minic_val_t *arg) {
    if (arg->type == MINIC_T_ID) return arg->u64;
    if (arg->type == MINIC_T_PTR) return (uint64_t)(uintptr_t)arg->p;
    return (uint64_t)minic_val_to_d(*arg);
}

// camera_3d_create(fov, near, far) -> entity
static minic_val_t minic_camera_3d_create(minic_val_t *args, int argc) {
    if (!g_runtime_world) return minic_val_id(0);

    float fov = (argc > 0) ? (float)minic_val_to_d(args[0]) * DEG_TO_RAD : 60.0f * DEG_TO_RAD;
    float near_plane = (argc > 1) ? (float)minic_val_to_d(args[1]) : 0.1f;
    float far_plane = (argc > 2) ? (float)minic_val_to_d(args[2]) : 100.0f;

    uint64_t entity = entity_create(g_runtime_world);
    if (entity == 0) return minic_val_id(0);

    uint64_t pos_id = ecs_component_comp_3d_position();
    uint64_t rot_id = ecs_component_comp_3d_rotation();
    uint64_t cam_id = ecs_component_comp_3d_camera();

    entity_add_component(g_runtime_world, entity, pos_id);
    entity_add_component(g_runtime_world, entity, rot_id);
    entity_add_component(g_runtime_world, entity, cam_id);

    // Default position behind origin
    comp_3d_position pos = {0.0f, 0.0f, -5.0f};
    entity_set_component_data(g_runtime_world, entity, pos_id, &pos);

    // Identity rotation
    comp_3d_rotation rot = {0.0f, 0.0f, 0.0f, 1.0f};
    entity_set_component_data(g_runtime_world, entity, rot_id, &rot);

    // Camera params
    comp_3d_camera cam = {
        .fov = fov,
        .near_plane = near_plane,
        .far_plane = far_plane,
        .perspective = true,
        .active = true
    };
    entity_set_component_data(g_runtime_world, entity, cam_id, &cam);

    return minic_val_id(entity);
}

// camera_3d_set_fov(entity, fov)
static minic_val_t minic_camera_3d_set_fov(minic_val_t *args, int argc) {
    if (argc < 2 || !g_runtime_world) return minic_val_void();
    uint64_t entity = extract_entity_id3d(&args[0]);
    float fov = (float)minic_val_to_d(args[1]) * DEG_TO_RAD;

    uint64_t cam_id = ecs_component_comp_3d_camera();
    comp_3d_camera *cam = (comp_3d_camera *)entity_get_component_data(g_runtime_world, entity, cam_id);
    if (cam) cam->fov = fov;
    return minic_val_void();
}

// camera_3d_set_position(entity, x, y, z)
static minic_val_t minic_camera_3d_set_position(minic_val_t *args, int argc) {
    if (argc < 4 || !g_runtime_world) return minic_val_void();
    uint64_t entity = extract_entity_id3d(&args[0]);
    float x = (float)minic_val_to_d(args[1]);
    float y = (float)minic_val_to_d(args[2]);
    float z = (float)minic_val_to_d(args[3]);

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_runtime_world);
    if (!ecs) return minic_val_void();

    uint64_t pos_id = ecs_component_comp_3d_position();
    comp_3d_position pos = {x, y, z};
    ecs_set_id(ecs, (ecs_entity_t)entity, (ecs_id_t)pos_id, sizeof(comp_3d_position), &pos);

    return minic_val_void();
}

// camera_3d_look_at(entity, tx, ty, tz)
static minic_val_t minic_camera_3d_look_at(minic_val_t *args, int argc) {
    if (argc < 4 || !g_runtime_world) return minic_val_void();
    uint64_t entity = extract_entity_id3d(&args[0]);
    float tx = (float)minic_val_to_d(args[1]);
    float ty = (float)minic_val_to_d(args[2]);
    float tz = (float)minic_val_to_d(args[3]);

    uint64_t pos_id = ecs_component_comp_3d_position();
    uint64_t rot_id = ecs_component_comp_3d_rotation();

    comp_3d_position *pos = (comp_3d_position *)entity_get_component_data(g_runtime_world, entity, pos_id);
    if (!pos) return minic_val_void();

    // Direction from camera to target
    float dx = tx - pos->x;
    float dy = ty - pos->y;
    float dz = tz - pos->z;
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (len < 0.0001f) return minic_val_void();
    dx /= len; dy /= len; dz /= len;

    // Forward is -Z direction
    float fx = -dx, fy = -dy, fz = -dz;
    float ux = 0.0f, uy = 1.0f, uz = 0.0f;

    // Right = cross(forward, up)
    float rx = fy * uz - fz * uy;
    float ry = fz * ux - fx * uz;
    float rz = fx * uy - fy * ux;
    len = sqrtf(rx * rx + ry * ry + rz * rz);
    if (len > 0.0001f) { rx /= len; ry /= len; rz /= len; }

    // Up = cross(right, forward)
    ux = ry * fz - rz * fy;
    uy = rz * fx - rx * fz;
    uz = rx * fy - ry * fx;

    // Rotation matrix to quaternion
    float m00 = rx, m10 = ry, m20 = rz;
    float m01 = ux, m11 = uy, m21 = uz;
    float m02 = fx, m12 = fy, m22 = fz;

    float trace = m00 + m11 + m22;
    comp_3d_rotation rot = {0};
    if (trace > 0.0f) {
        float s = 0.5f / sqrtf(trace + 1.0f);
        rot.w = 0.25f / s;
        rot.x = (m21 - m12) * s;
        rot.y = (m02 - m20) * s;
        rot.z = (m10 - m01) * s;
    } else if (m00 > m11 && m00 > m22) {
        float s = 2.0f * sqrtf(1.0f + m00 - m11 - m22);
        rot.w = (m21 - m12) / s;
        rot.x = 0.25f * s;
        rot.y = (m01 + m10) / s;
        rot.z = (m02 + m20) / s;
    } else if (m11 > m22) {
        float s = 2.0f * sqrtf(1.0f + m11 - m00 - m22);
        rot.w = (m02 - m20) / s;
        rot.x = (m01 + m10) / s;
        rot.y = 0.25f * s;
        rot.z = (m12 + m21) / s;
    } else {
        float s = 2.0f * sqrtf(1.0f + m22 - m00 - m11);
        rot.w = (m10 - m01) / s;
        rot.x = (m02 + m20) / s;
        rot.y = (m12 + m21) / s;
        rot.z = 0.25f * s;
    }

    entity_set_component_data(g_runtime_world, entity, rot_id, &rot);

    return minic_val_void();
}

// Check if string ends with suffix
static bool path_ends_with(const char *s, const char *suffix) {
    if (!s || !suffix) return false;
    size_t slen = strlen(s);
    size_t suflen = strlen(suffix);
    if (suflen > slen) return false;
    return strcmp(s + slen - suflen, suffix) == 0;
}

// mesh_create(mesh_path, material_path) -> entity
static minic_val_t minic_mesh_create(minic_val_t *args, int argc) {
    if (!g_runtime_world) return minic_val_id(0);

    const char *mesh_path = (argc > 0 && args[0].type == MINIC_T_PTR) ? (const char *)args[0].p : NULL;
    const char *mat_path = (argc > 1 && args[1].type == MINIC_T_PTR) ? (const char *)args[1].p : NULL;

    // .arm files are scene files — route to scene-based loader
    if (mesh_path && path_ends_with(mesh_path, ".arm")) {
        uint64_t entity = asset_loader_load_scene(mesh_path);
        return minic_val_id(entity);
    }

    uint64_t entity = entity_create(g_runtime_world);
    if (entity == 0) return minic_val_id(0);

    // Add transform components
    uint64_t pos_id = ecs_component_comp_3d_position();
    uint64_t rot_id = ecs_component_comp_3d_rotation();
    uint64_t scale_id = ecs_component_comp_3d_scale();
    uint64_t mr_id = ecs_component_comp_3d_mesh_renderer();

    entity_add_component(g_runtime_world, entity, pos_id);
    entity_add_component(g_runtime_world, entity, rot_id);
    entity_add_component(g_runtime_world, entity, scale_id);
    entity_add_component(g_runtime_world, entity, mr_id);

    // Default transform
    comp_3d_position pos = {0.0f, 0.0f, 0.0f};
    comp_3d_rotation rot = {0.0f, 0.0f, 0.0f, 1.0f};
    comp_3d_scale scale = {1.0f, 1.0f, 1.0f};

    entity_set_component_data(g_runtime_world, entity, pos_id, &pos);
    entity_set_component_data(g_runtime_world, entity, rot_id, &rot);
    entity_set_component_data(g_runtime_world, entity, scale_id, &scale);

    // Load mesh if path provided
    if (mesh_path) {
        asset_loader_load_mesh(entity, mesh_path, mat_path);
    }

    return minic_val_id(entity);
}

// entity_set_position(entity, x, y, z)
static minic_val_t minic_entity_set_position_3d(minic_val_t *args, int argc) {
    if (argc < 4 || !g_runtime_world) return minic_val_void();
    uint64_t entity = extract_entity_id3d(&args[0]);
    float x = (float)minic_val_to_d(args[1]);
    float y = (float)minic_val_to_d(args[2]);
    float z = (float)minic_val_to_d(args[3]);

    uint64_t pos_id = ecs_component_comp_3d_position();
    comp_3d_position *pos = (comp_3d_position *)entity_get_component_data(g_runtime_world, entity, pos_id);
    if (pos) {
        pos->x = x;
        pos->y = y;
        pos->z = z;
    }
    return minic_val_void();
}

// entity_set_rotation(entity, x, y, z, w)
static minic_val_t minic_entity_set_rotation_3d(minic_val_t *args, int argc) {
    if (argc < 5 || !g_runtime_world) return minic_val_void();
    uint64_t entity = extract_entity_id3d(&args[0]);
    float x = (float)minic_val_to_d(args[1]);
    float y = (float)minic_val_to_d(args[2]);
    float z = (float)minic_val_to_d(args[3]);
    float w = (float)minic_val_to_d(args[4]);

    uint64_t rot_id = ecs_component_comp_3d_rotation();
    comp_3d_rotation *rot = (comp_3d_rotation *)entity_get_component_data(g_runtime_world, entity, rot_id);
    if (rot) {
        rot->x = x;
        rot->y = y;
        rot->z = z;
        rot->w = w;
    }
    return minic_val_void();
}

// entity_set_scale(entity, x, y, z)
static minic_val_t minic_entity_set_scale_3d(minic_val_t *args, int argc) {
    if (argc < 4 || !g_runtime_world) return minic_val_void();
    uint64_t entity = extract_entity_id3d(&args[0]);
    float x = (float)minic_val_to_d(args[1]);
    float y = (float)minic_val_to_d(args[2]);
    float z = (float)minic_val_to_d(args[3]);

    uint64_t scale_id = ecs_component_comp_3d_scale();
    comp_3d_scale *scl = (comp_3d_scale *)entity_get_component_data(g_runtime_world, entity, scale_id);
    if (scl) {
        scl->x = x;
        scl->y = y;
        scl->z = z;
    }
    return minic_val_void();
}

// entity_get_x(entity) -> float
static minic_val_t minic_entity_get_x(minic_val_t *args, int argc) {
    if (argc < 1 || !g_runtime_world) return minic_val_float(0.0f);
    uint64_t entity = extract_entity_id3d(&args[0]);
    uint64_t pos_id = ecs_component_comp_3d_position();
    comp_3d_position *pos = (comp_3d_position *)entity_get_component_data(g_runtime_world, entity, pos_id);
    return minic_val_float(pos ? pos->x : 0.0f);
}

// entity_get_y(entity) -> float
static minic_val_t minic_entity_get_y(minic_val_t *args, int argc) {
    if (argc < 1 || !g_runtime_world) return minic_val_float(0.0f);
    uint64_t entity = extract_entity_id3d(&args[0]);
    uint64_t pos_id = ecs_component_comp_3d_position();
    comp_3d_position *pos = (comp_3d_position *)entity_get_component_data(g_runtime_world, entity, pos_id);
    return minic_val_float(pos ? pos->y : 0.0f);
}

// entity_get_z(entity) -> float
static minic_val_t minic_entity_get_z(minic_val_t *args, int argc) {
    if (argc < 1 || !g_runtime_world) return minic_val_float(0.0f);
    uint64_t entity = extract_entity_id3d(&args[0]);
    uint64_t pos_id = ecs_component_comp_3d_position();
    comp_3d_position *pos = (comp_3d_position *)entity_get_component_data(g_runtime_world, entity, pos_id);
    return minic_val_float(pos ? pos->z : 0.0f);
}

// light_directional(dx, dy, dz, r, g, b, strength) -> entity
static minic_val_t minic_light_directional(minic_val_t *args, int argc) {
    if (!g_runtime_world) return minic_val_id(0);

    float dx = (argc > 0) ? (float)minic_val_to_d(args[0]) : 0.0f;
    float dy = (argc > 1) ? (float)minic_val_to_d(args[1]) : -1.0f;
    float dz = (argc > 2) ? (float)minic_val_to_d(args[2]) : 0.0f;
    float r  = (argc > 3) ? (float)minic_val_to_d(args[3]) : 1.0f;
    float g  = (argc > 4) ? (float)minic_val_to_d(args[4]) : 1.0f;
    float b  = (argc > 5) ? (float)minic_val_to_d(args[5]) : 1.0f;
    float str = (argc > 6) ? (float)minic_val_to_d(args[6]) : 1.0f;

    uint64_t entity = entity_create(g_runtime_world);
    if (entity == 0) return minic_val_id(0);

    uint64_t light_id = ecs_component_comp_directional_light();
    entity_add_component(g_runtime_world, entity, light_id);

    comp_directional_light light = {
        .dir_x = dx, .dir_y = dy, .dir_z = dz,
        .color_r = r, .color_g = g, .color_b = b,
        .strength = str,
        .enabled = true
    };
    entity_set_component_data(g_runtime_world, entity, light_id, &light);

    return minic_val_id(entity);
}

// camera_3d_perspective(entity, fov, near, far)
static minic_val_t minic_camera_3d_perspective(minic_val_t *args, int argc) {
    if (argc < 4 || !g_runtime_world) return minic_val_void();
    uint64_t entity = extract_entity_id3d(&args[0]);
    float fov = (float)minic_val_to_d(args[1]) * DEG_TO_RAD;
    float near_plane = (float)minic_val_to_d(args[2]);
    float far_plane = (float)minic_val_to_d(args[3]);

    uint64_t cam_id = ecs_component_comp_3d_camera();
    comp_3d_camera *cam = (comp_3d_camera *)entity_get_component_data(g_runtime_world, entity, cam_id);
    if (cam) {
        cam->fov = fov;
        cam->near_plane = near_plane;
        cam->far_plane = far_plane;
        cam->perspective = true;
    }
    return minic_val_void();
}

// camera_3d_orthographic(entity, left, right, bottom, top, near, far)
static minic_val_t minic_camera_3d_orthographic(minic_val_t *args, int argc) {
    if (argc < 7 || !g_runtime_world) return minic_val_void();
    uint64_t entity = extract_entity_id3d(&args[0]);
    float left = (float)minic_val_to_d(args[1]);
    float right = (float)minic_val_to_d(args[2]);
    float bottom = (float)minic_val_to_d(args[3]);
    float top = (float)minic_val_to_d(args[4]);
    float near_plane = (float)minic_val_to_d(args[5]);
    float far_plane = (float)minic_val_to_d(args[6]);

    uint64_t cam_id = ecs_component_comp_3d_camera();
    comp_3d_camera *cam = (comp_3d_camera *)entity_get_component_data(g_runtime_world, entity, cam_id);
    if (cam) {
        cam->ortho_left = left;
        cam->ortho_right = right;
        cam->ortho_bottom = bottom;
        cam->ortho_top = top;
        cam->near_plane = near_plane;
        cam->far_plane = far_plane;
        cam->perspective = false;
    }
    return minic_val_void();
}

// mesh_load_arm(path) -> ptr — loads .arm scene into Iron objects without creating ECS entities
static minic_val_t minic_mesh_load_arm(minic_val_t *args, int argc) {
    if (!g_runtime_world) return minic_val_ptr(NULL);

    const char *mesh_path = (argc > 0 && args[0].type == MINIC_T_PTR) ? (const char *)args[0].p : NULL;
    if (!mesh_path) return minic_val_ptr(NULL);

    // Parse .arm file
    scene_t *scene_raw = data_get_scene_raw(mesh_path);
    if (!scene_raw) {
        fprintf(stderr, "[mesh_load] Failed to load scene '%s'\n", mesh_path);
        return minic_val_ptr(NULL);
    }

    // Auto-generate missing camera/material/shader/world data
    scene_ensure_defaults(scene_raw);

    // Cache patched scene under its own name so scene_create can find it
    if (scene_raw->name != NULL) {
        any_map_set(data_cached_scene_raws, scene_raw->name, scene_raw);
    }

    // Remove existing scene if present, then create fresh
    if (_scene_root != NULL) {
        scene_remove();
    }
    scene_create(scene_raw);

    if (!_scene_root) {
        fprintf(stderr, "[mesh_load] scene_create failed for '%s'\n", mesh_path);
        return minic_val_ptr(NULL);
    }

    // Initialize viewport dimensions to prevent division-by-zero
    render_path_current_w = sys_w();
    render_path_current_h = sys_h();

    // Position the auto-created camera behind origin
    if (scene_camera != NULL && scene_camera->base != NULL) {
        transform_t *t = scene_camera->base->transform;
        t->loc = vec4_create(0, 2, 5, 1);
        t->rot = quat_create(0, 0, 0, 1);
        transform_build_matrix(t);
        camera_object_build_proj(scene_camera, (f32)sys_w() / (f32)sys_h());
        camera_object_build_mat(scene_camera);
    }

    // Return the first mesh object pointer (or NULL)
    if (scene_meshes != NULL && scene_meshes->length > 0) {
        mesh_object_t *mesh_obj = (mesh_object_t *)scene_meshes->buffer[0];
        if (mesh_obj && mesh_obj->base) {
            printf("[mesh_load] Loaded '%s', returning mesh object\n", mesh_path);
            return minic_val_ptr(mesh_obj->base);
        }
    }

    printf("[mesh_load] Loaded '%s', no mesh objects found\n", mesh_path);
    return minic_val_ptr(NULL);
}

void scene_3d_api_register(void) {
    minic_register_native("camera_3d_create", minic_camera_3d_create);
    minic_register_native("camera_3d_set_fov", minic_camera_3d_set_fov);
    minic_register_native("camera_3d_perspective", minic_camera_3d_perspective);
    minic_register_native("camera_3d_orthographic", minic_camera_3d_orthographic);
    minic_register_native("camera_3d_set_position", minic_camera_3d_set_position);
    minic_register_native("camera_3d_look_at", minic_camera_3d_look_at);

    minic_register_native("mesh_create", minic_mesh_create);

    minic_register_native("entity_set_position", minic_entity_set_position_3d);
    minic_register_native("entity_set_rotation", minic_entity_set_rotation_3d);
    minic_register_native("entity_set_scale", minic_entity_set_scale_3d);

    minic_register_native("entity_get_x", minic_entity_get_x);
    minic_register_native("entity_get_y", minic_entity_get_y);
    minic_register_native("entity_get_z", minic_entity_get_z);

    minic_register_native("light_directional", minic_light_directional);
    minic_register_native("mesh_load_arm", minic_mesh_load_arm);

    printf("3D Scene API registered\n");
}
