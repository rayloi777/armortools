#include "asset_loader.h"
#include "ecs/ecs_world.h"
#include "ecs/ecs_components.h"
#include <iron.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

static game_world_t *g_asset_loader_world = NULL;

void asset_loader_set_world(game_world_t *world) {
    g_asset_loader_world = world;
}

// Ensure scene_t has required data for scene_create().
// .arm files from ArmorPaint often lack camera, material, shader, and world data.
void scene_ensure_defaults(scene_t *scene) {
    // Add camera object if scene has no camera_object
    bool has_camera = false;
    if (scene->objects != NULL) {
        for (int i = 0; i < scene->objects->length; i++) {
            obj_t *o = (obj_t *)scene->objects->buffer[i];
            if (o->type && strcmp(o->type, "camera_object") == 0) {
                has_camera = true;
                break;
            }
        }
    }
    if (!has_camera && scene->objects != NULL) {
        any_array_push(scene->objects,
            GC_ALLOC_INIT(obj_t, {
                .name = "Camera",
                .type = "camera_object",
                .data_ref = "DefaultCamera",
                .visible = true,
                .spawn = true
            }));
    }

    // Set material_ref on mesh objects that lack it
    if (scene->objects != NULL) {
        for (int i = 0; i < scene->objects->length; i++) {
            obj_t *o = (obj_t *)scene->objects->buffer[i];
            if (o->type && strcmp(o->type, "mesh_object") == 0 && o->material_ref == NULL) {
                o->material_ref = "DefaultMaterial";
            }
        }
    }

    // Ensure mesh_datas have a "nor" vertex array using short4norm format
    // for full 3D normals. Converts existing short2norm normals and computes
    // face normals from geometry for meshes missing normals entirely.
    if (scene->mesh_datas != NULL) {
        for (int m = 0; m < scene->mesh_datas->length; m++) {
            mesh_data_t *md = (mesh_data_t *)scene->mesh_datas->buffer[m];
            if (!md->vertex_arrays) continue;

            int pos_vert_count = 0;
            vertex_array_t *pos_va = NULL;
            vertex_array_t *nor_va = NULL;

            for (int v = 0; v < md->vertex_arrays->length; v++) {
                vertex_array_t *va = (vertex_array_t *)md->vertex_arrays->buffer[v];
                if (va->attrib && strcmp(va->attrib, "pos") == 0 && va->values) {
                    pos_va = va;
                    pos_vert_count = va->values->length / 4;
                }
                if (va->attrib && strcmp(va->attrib, "nor") == 0) {
                    nor_va = va;
                }
            }

            if (pos_vert_count == 0) continue;

            // Generate face normals from geometry for meshes missing nor
            if (nor_va == NULL && pos_va != NULL && md->index_array != NULL) {
                int num_verts = pos_vert_count;
                // Accumulate per-vertex normals (float, unnormalized)
                float *accum = (float *)calloc(num_verts * 3, sizeof(float));

                // Decode packed i16 positions to float [-1, 1]
                float *positions = (float *)malloc(num_verts * 3 * sizeof(float));
                for (int j = 0; j < num_verts; j++) {
                    positions[j * 3 + 0] = (float)pos_va->values->buffer[j * 4 + 0] / 32767.0f;
                    positions[j * 3 + 1] = (float)pos_va->values->buffer[j * 4 + 1] / 32767.0f;
                    positions[j * 3 + 2] = (float)pos_va->values->buffer[j * 4 + 2] / 32767.0f;
                }

                // Compute face normals from triangles
                int num_tri = md->index_array->length / 3;
                for (int t = 0; t < num_tri; t++) {
                    int i0 = md->index_array->buffer[t * 3 + 0];
                    int i1 = md->index_array->buffer[t * 3 + 1];
                    int i2 = md->index_array->buffer[t * 3 + 2];
                    float *p0 = &positions[i0 * 3];
                    float *p1 = &positions[i1 * 3];
                    float *p2 = &positions[i2 * 3];
                    // Cross product (p1-p0) x (p2-p0)
                    float e1x = p1[0] - p0[0], e1y = p1[1] - p0[1], e1z = p1[2] - p0[2];
                    float e2x = p2[0] - p0[0], e2y = p2[1] - p0[1], e2z = p2[2] - p0[2];
                    float nx = e1y * e2z - e1z * e2y;
                    float ny = e1z * e2x - e1x * e2z;
                    float nz = e1x * e2y - e1y * e2x;
                    accum[i0 * 3 + 0] += nx; accum[i0 * 3 + 1] += ny; accum[i0 * 3 + 2] += nz;
                    accum[i1 * 3 + 0] += nx; accum[i1 * 3 + 1] += ny; accum[i1 * 3 + 2] += nz;
                    accum[i2 * 3 + 0] += nx; accum[i2 * 3 + 1] += ny; accum[i2 * 3 + 2] += nz;
                }

                // Normalize and pack to short2norm (nx, ny only; shader reconstructs nz)
                i16_array_t *nor_vals = i16_array_create(num_verts * 2);
                nor_vals->length = num_verts * 2;
                for (int j = 0; j < num_verts; j++) {
                    float nx = accum[j * 3 + 0];
                    float ny = accum[j * 3 + 1];
                    float nz = accum[j * 3 + 2];
                    float len = sqrtf(nx * nx + ny * ny + nz * nz);
                    if (len > 0.0001f) { nx /= len; ny /= len; nz /= len; }
                    else { nx = 0; ny = 1; nz = 0; } // fallback: up
                    nor_vals->buffer[j * 2 + 0] = (int16_t)(nx * 32767.0f);
                    nor_vals->buffer[j * 2 + 1] = (int16_t)(ny * 32767.0f);
                }

                vertex_array_t *new_nor = GC_ALLOC_INIT(vertex_array_t, {
                    .attrib = "nor",
                    .data = "short2norm",
                    .values = nor_vals,
                });
                any_array_push(md->vertex_arrays, new_nor);
                free(positions);
                free(accum);
                printf("[asset_loader] Generated face normals for mesh '%s' (%d verts, %d tris)\n",
                    md->name ? md->name : "?", num_verts, num_tri);
            }
        }
    }

    // Camera data
    if (scene->camera_datas == NULL || scene->camera_datas->length == 0) {
        scene->camera_datas = any_array_create_from_raw(
            (void *[]){
                GC_ALLOC_INIT(camera_data_t, {
                    .name = "DefaultCamera",
                    .near_plane = 0.01,
                    .far_plane = 100.0,
                    .fov = 0.85
                }),
            },
            1);
    }
    if (scene->camera_ref == NULL) {
        scene->camera_ref = "Camera";
    }

    // World data
    if (scene->world_datas == NULL || scene->world_datas->length == 0) {
        scene->world_datas = any_array_create_from_raw(
            (void *[]){
                GC_ALLOC_INIT(world_data_t, {.name = "DefaultWorld", .color = 0xff1a1a2e}),
            },
            1);
    }
    if (scene->world_ref == NULL) {
        scene->world_ref = "DefaultWorld";
    }

    // Material data — single material with "mesh" context
    if (scene->material_datas == NULL || scene->material_datas->length == 0) {
        scene->material_datas = any_array_create_from_raw(
            (void *[]){
                GC_ALLOC_INIT(material_data_t,
                    {.name     = "DefaultMaterial",
                     .shader   = "DefaultShader",
                     .contexts = any_array_create_from_raw(
                         (void *[]){
                             GC_ALLOC_INIT(material_context_t,
                                 {.name          = "mesh",
                                  .bind_constants = any_array_create_from_raw(
                                      (void *[]){
                                          GC_ALLOC_INIT(bind_const_t,
                                              {.name = "metallic",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f}, 1)}),
                                          GC_ALLOC_INIT(bind_const_t,
                                              {.name = "roughness",
                                               .vec = f32_array_create_from_raw((f32[]){0.5f}, 1)}),
                                          GC_ALLOC_INIT(bind_const_t,
                                              {.name = "albedo_rgb",
                                               .vec = f32_array_create_from_raw((f32[]){0.8f, 0.8f, 0.8f}, 3)}),
                                          GC_ALLOC_INIT(bind_const_t,
                                              {.name = "ao",
                                               .vec = f32_array_create_from_raw((f32[]){1.0f}, 1)}),
                                          GC_ALLOC_INIT(bind_const_t,
                                              {.name = "light_dir",
                                               .vec = f32_array_create_from_raw((f32[]){-0.5f, -0.7f, -0.5f}, 3)}),
                                          GC_ALLOC_INIT(bind_const_t,
                                              {.name = "light_color",
                                               .vec = f32_array_create_from_raw((f32[]){1.0f, 1.0f, 1.0f}, 3)}),
                                          GC_ALLOC_INIT(bind_const_t,
                                              {.name = "light_strength",
                                               .vec = f32_array_create_from_raw((f32[]){3.0f}, 1)}),
                                          GC_ALLOC_INIT(bind_const_t,
                                              {.name = "ambient_strength",
                                               .vec = f32_array_create_from_raw((f32[]){0.03f}, 1)}),
                                          GC_ALLOC_INIT(bind_const_t,
                                              {.name = "cam_pos",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f, 2.0f, 5.0f}, 3)}),
                                          GC_ALLOC_INIT(bind_const_t,
                                              {.name = "shadow_enabled",
                                               .vec = f32_array_create_from_raw((f32[]){1.0f}, 1)}),
                                          GC_ALLOC_INIT(bind_const_t,
                                              {.name = "ground_y",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_pos0",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f, 0.0f, 0.0f}, 3)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_pos1",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f, 0.0f, 0.0f}, 3)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_pos2",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f, 0.0f, 0.0f}, 3)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_pos3",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f, 0.0f, 0.0f}, 3)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_color0",
                                               .vec = f32_array_create_from_raw((f32[]){1.0f, 1.0f, 1.0f}, 3)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_color1",
                                               .vec = f32_array_create_from_raw((f32[]){1.0f, 1.0f, 1.0f}, 3)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_color2",
                                               .vec = f32_array_create_from_raw((f32[]){1.0f, 1.0f, 1.0f}, 3)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_color3",
                                               .vec = f32_array_create_from_raw((f32[]){1.0f, 1.0f, 1.0f}, 3)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_strength0",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_strength1",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_strength2",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_strength3",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_radius0",
                                               .vec = f32_array_create_from_raw((f32[]){10.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_radius1",
                                               .vec = f32_array_create_from_raw((f32[]){10.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_radius2",
                                               .vec = f32_array_create_from_raw((f32[]){10.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "point_light_radius3",
                                               .vec = f32_array_create_from_raw((f32[]){10.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "num_point_lights",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "light_vp_r0",
                                               .vec = f32_array_create(4)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "light_vp_r1",
                                               .vec = f32_array_create(4)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "light_vp_r2",
                                               .vec = f32_array_create(4)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "light_vp_r3",
                                               .vec = f32_array_create(4)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "shadow_pass",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "opacity",
                                               .vec = f32_array_create_from_raw((f32[]){1.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "use_albedo_tex",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "use_metallic_tex",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f}, 1)}),
                                         GC_ALLOC_INIT(bind_const_t,
                                              {.name = "use_roughness_tex",
                                               .vec = f32_array_create_from_raw((f32[]){0.0f}, 1)}),
                                      },
                                      37),
                                  .bind_textures = any_array_create_from_raw(
                                      (void *[]){
                                          GC_ALLOC_INIT(bind_tex_t,
                                              {.name = "_shadow_map", .file = "_shadow_map"}),
                                          GC_ALLOC_INIT(bind_tex_t,
                                              {.name = "tex_albedo", .file = ""}),
                                          GC_ALLOC_INIT(bind_tex_t,
                                              {.name = "tex_metallic", .file = ""}),
                                          GC_ALLOC_INIT(bind_tex_t,
                                              {.name = "tex_roughness", .file = ""}),
                                      },
                                      4)}),
                         },
                         1)}),
            },
            1);
    }

    // Shader data — deferred G-buffer pipeline
    if (scene->shader_datas == NULL || scene->shader_datas->length == 0) {
        scene->shader_datas = any_array_create_from_raw(
            (void *[]){
                GC_ALLOC_INIT(shader_data_t,
                    {.name     = "DefaultShader",
                     .contexts = any_array_create_from_raw(
                         (void *[]){
                             GC_ALLOC_INIT(shader_context_t,
                                 {.name            = "mesh",
                                  .vertex_shader   = "mesh_gbuffer.vert",
                                  .fragment_shader = "mesh_gbuffer.frag",
                                  .compare_mode    = "less",
                                  .cull_mode       = "none",
                                  .depth_write     = true,
                                  .color_attachments = any_array_create_from_raw(
                                      (void *[]){
                                          (void *)"RGBA64",
                                      },
                                      1),
                                  .vertex_elements = any_array_create_from_raw(
                                      (void *[]){
                                          GC_ALLOC_INIT(vertex_element_t, {.name = "pos", .data = "short4norm"}),
                                          GC_ALLOC_INIT(vertex_element_t, {.name = "nor", .data = "short2norm"}),
                                          GC_ALLOC_INIT(vertex_element_t, {.name = "tex", .data = "short2norm"}),
                                      },
                                      3),
                                  .constants = any_array_create_from_raw(
                                      (void *[]){
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "WVP", .type = "mat4", .link = "_world_view_proj_matrix"}),
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "World", .type = "mat4", .link = "_world_matrix"}),
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "metallic", .type = "float", .link = NULL}),
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "roughness", .type = "float", .link = NULL}),
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "albedo_rgb", .type = "vec3", .link = NULL}),
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "ao", .type = "float", .link = NULL}),
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "light_dir", .type = "vec3", .link = NULL}),
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "light_color", .type = "vec3", .link = NULL}),
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "light_strength", .type = "float", .link = NULL}),
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "ambient_strength", .type = "float", .link = NULL}),
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "cam_pos", .type = "vec3", .link = NULL}),
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "shadow_enabled", .type = "float", .link = NULL}),
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "shadow_pass", .type = "float", .link = NULL}),
                                          GC_ALLOC_INIT(shader_const_t,
                                              {.name = "ground_y", .type = "float", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_pos0", .type = "vec3", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_pos1", .type = "vec3", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_pos2", .type = "vec3", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_pos3", .type = "vec3", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_color0", .type = "vec3", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_color1", .type = "vec3", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_color2", .type = "vec3", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_color3", .type = "vec3", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_strength0", .type = "float", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_strength1", .type = "float", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_strength2", .type = "float", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_strength3", .type = "float", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_radius0", .type = "float", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_radius1", .type = "float", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_radius2", .type = "float", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "point_light_radius3", .type = "float", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "num_point_lights", .type = "float", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "light_vp_r0", .type = "vec4", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "light_vp_r1", .type = "vec4", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "light_vp_r2", .type = "vec4", .link = NULL}),
                                         GC_ALLOC_INIT(shader_const_t,
                                              {.name = "light_vp_r3", .type = "vec4", .link = NULL}),
                                      },
                                      35),
                                  .texture_units = any_array_create_from_raw(
                                      (void *[]){
                                          GC_ALLOC_INIT(tex_unit_t,
                                              {.name = "_shadow_map", .link = "_shadow_map"}),
                                      },
                                      1),
                                  .depth_attachment = "D32"}),
                         },
                         1)}),
            },
            1);
    }

    // Patch shader contexts that lack depth_attachment — the 3D render path
    // always uses a depth buffer, so missing depth_attachment causes a Metal
    // validation error (pipeline MTLPixelFormatInvalid vs Depth32Float).
    if (scene->shader_datas != NULL) {
        for (int i = 0; i < scene->shader_datas->length; i++) {
            shader_data_t *sd = (shader_data_t *)scene->shader_datas->buffer[i];
            if (!sd || !sd->contexts) continue;
            for (int j = 0; j < sd->contexts->length; j++) {
                shader_context_t *ctx = (shader_context_t *)sd->contexts->buffer[j];
                if (ctx && ctx->depth_attachment == NULL) {
                    ctx->depth_attachment = "D32";
                }
            }
        }
    }
}

uint64_t asset_loader_load_scene(const char *path) {
    if (!g_asset_loader_world || !path) return 0;

    // Parse .arm file
    scene_t *scene_raw = data_get_scene_raw(path);
    if (!scene_raw) {
        fprintf(stderr, "Asset Loader: failed to load scene '%s'\n", path);
        return 0;
    }

    // Auto-generate missing camera/material/shader/world data
    scene_ensure_defaults(scene_raw);

    // Cache patched scene under its own name so scene_create can find it
    // (scene_create -> scene_add_scene -> data_get_scene_raw(scene_raw->name))
    if (scene_raw->name != NULL) {
        any_map_set(data_cached_scene_raws, scene_raw->name, scene_raw);
    }

    // Remove existing scene if present, then create fresh
    if (_scene_root != NULL) {
        scene_remove();
    }
    scene_create(scene_raw);

    if (!_scene_root) {
        fprintf(stderr, "Asset Loader: scene_create failed for '%s'\n", path);
        return 0;
    }

    // Initialize viewport dimensions to prevent division-by-zero
    render_path_current_w = sys_w();
    render_path_current_h = sys_h();

    // Position the camera behind origin
    if (scene_camera != NULL && scene_camera->base != NULL) {
        transform_t *t = scene_camera->base->transform;
        t->loc = vec4_create(0, 2, 5, 1);
        t->rot = quat_create(0, 0, 0, 1);
        transform_build_matrix(t);
        camera_object_build_proj(scene_camera, (f32)sys_w() / (f32)sys_h());
        camera_object_build_mat(scene_camera);
    }

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_asset_loader_world);
    if (!ecs) return 0;

    uint64_t first_entity = 0;

    // Sync mesh objects to ECS entities — read from Iron transform_t
    // (which was initialized from .arm mat4 data by scene_create)
    if (scene_meshes != NULL) {
        for (int i = 0; i < scene_meshes->length; i++) {
            mesh_object_t *mesh_obj = (mesh_object_t *)scene_meshes->buffer[i];
            if (!mesh_obj || !mesh_obj->base) continue;

            ecs_entity_t e = ecs_new(ecs);
            if (first_entity == 0) first_entity = (uint64_t)e;

            transform_t *t = mesh_obj->base->transform;

            comp_3d_position pos = { t->loc.x, t->loc.y, t->loc.z };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_position(), sizeof(comp_3d_position), &pos);

            comp_3d_rotation rot = { t->rot.x, t->rot.y, t->rot.z, t->rot.w };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_rotation(), sizeof(comp_3d_rotation), &rot);

            comp_3d_scale scl = { t->scale.x, t->scale.y, t->scale.z };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_scale(), sizeof(comp_3d_scale), &scl);

            comp_3d_mesh_renderer mr = { .visible = true };
            ecs_set_id(ecs, e, (ecs_id_t)ecs_component_comp_3d_mesh_renderer(), sizeof(comp_3d_mesh_renderer), &mr);

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
    if (!g_asset_loader_world || !entity || !mesh_path || mesh_path[0] == '\0') return -1;

    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(g_asset_loader_world);
    if (!ecs) return -1;

    ecs_entity_t e = (ecs_entity_t)entity;
    if (!ecs_is_alive(ecs, e)) return -1;

    // Ensure scene root exists (manual init, not via scene_create which needs valid file)
    if (!_scene_root) {
        scene_meshes = any_array_create(0);
        gc_root(scene_meshes);
        scene_cameras = any_array_create(0);
        gc_root(scene_cameras);
        scene_empties = any_array_create(0);
        gc_root(scene_empties);
        scene_embedded = any_map_create();
        gc_root(scene_embedded);
        _scene_root = object_create(true);
        _scene_root->name = "Root";
    }

    mesh_data_t *mesh_data = data_get_mesh(mesh_path, "");
    if (!mesh_data) {
        fprintf(stderr, "Asset Loader: failed to load mesh '%s'\n", mesh_path);
        return -1;
    }

    material_data_t *mat_data = NULL;
    if (material_path != NULL && material_path[0] != '\0') {
        mat_data = data_get_material(material_path, "");
    }
    else {
        // Try loading material from the same .arm file, if it has any
        scene_t *scene_raw = data_get_scene_raw(mesh_path);
        if (scene_raw && scene_raw->material_datas && scene_raw->material_datas->length > 0) {
            mat_data = data_get_material(mesh_path, "");
        }
    }

    // If no material found, create a default one with "mesh" context
    if (!mat_data) {
        shader_data_t *shader_data = GC_ALLOC_INIT(shader_data_t, {
            .name = "DefaultShader",
            .contexts = any_array_create_from_raw(
                (void *[]){
                    GC_ALLOC_INIT(shader_context_t, {
                        .name = "mesh",
                        .vertex_shader = "mesh.vert",
                        .fragment_shader = "mesh.frag",
                        .compare_mode = "less",
                        .cull_mode = "none",
                        .depth_write = true,
                        .vertex_elements = any_array_create_from_raw(
                            (void *[]){
                                GC_ALLOC_INIT(vertex_element_t, {.name = "pos", .data = "short4norm"}),
                            },
                            1),
                        .constants = any_array_create_from_raw(
                            (void *[]){
                                GC_ALLOC_INIT(shader_const_t, {.name = "WVP", .type = "mat4", .link = "_world_view_proj_matrix"}),
                            },
                            1),
                        .color_attachments = any_array_create_from_raw(
                            (void *[]){
                                "RGBA32",
                            },
                            1),
                        .depth_attachment = "D32"
                    }),
                },
                1)
        });
        shader_data = shader_data_create(shader_data);

        mat_data = GC_ALLOC_INIT(material_data_t, {
            .name = "DefaultMaterial",
            .shader = "DefaultShader",
            .contexts = any_array_create_from_raw(
                (void *[]){
                    GC_ALLOC_INIT(material_context_t, {.name = "mesh"}),
                },
                1)
        });
        mat_data->_ = gc_alloc(sizeof(material_data_runtime_t));
        mat_data->_->uid = 1.0f;
        mat_data->_->shader = shader_data;
        // Load material contexts
        for (int i = 0; i < (int)mat_data->contexts->length; i++) {
            material_context_t *c = (material_context_t *)mat_data->contexts->buffer[i];
            material_context_load(c);
        }
    }

    mesh_object_t *mesh_obj = mesh_object_create(mesh_data, mat_data);
    if (!mesh_obj) {
        fprintf(stderr, "Asset Loader: failed to create mesh object\n");
        return -1;
    }

    object_set_parent(mesh_obj->base, _scene_root);

    // Root transform to prevent GC collection during frame rendering
    if (mesh_obj->base->transform) {
        gc_root(mesh_obj->base->transform);
    }

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
