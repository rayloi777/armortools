#include "runtime_api.h"
#include "component_api.h"
#include "entity_api.h"
#include "system_api.h"
#include "query_api.h"
#include "game_loop.h"
#include "ecs/ecs_world.h"
#include "ecs/ecs_components.h"
#include "ecs/ecs_dynamic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libs/minic.h>

static game_world_t *g_runtime_world = NULL;

void minic_register_builtins(void) {
}

void runtime_api_set_world(game_world_t *world) {
    g_runtime_world = world;
}

static uint64_t minic_component_register(const char *name, int size) {
    if (!g_runtime_world || !name) return 0;
    
    component_desc_t desc = {
        .name = name,
        .size = (size_t)size,
        .alignment = 4,
        .ctor = NULL,
        .dtor = NULL,
        .copy = NULL,
        .move = NULL
    };
    
    return component_register(g_runtime_world, &desc);
}

static int minic_component_add_field(uint64_t component_id, const char *name, int type, int offset) {
    return component_add_field(component_id, name, type, (size_t)offset);
}

static uint64_t minic_component_get_id(const char *name) {
    if (!g_runtime_world || !name) return 0;
    return component_get_id(g_runtime_world, name);
}

static uint64_t minic_entity_create(void) {
    if (!g_runtime_world) return 0;
    return entity_create(g_runtime_world);
}

static uint64_t minic_entity_create_named(const char *name) {
    if (!g_runtime_world || !name) return 0;
    return entity_create_with_name(g_runtime_world, name);
}

static void minic_entity_destroy(uint64_t entity) {
    if (!g_runtime_world || entity == 0) return;
    entity_destroy(g_runtime_world, entity);
}

static void minic_entity_add(uint64_t entity, uint64_t component_id) {
    if (!g_runtime_world || entity == 0 || component_id == 0) return;
    entity_add_component(g_runtime_world, entity, component_id);
}

static void minic_entity_remove(uint64_t entity, uint64_t component_id) {
    if (!g_runtime_world || entity == 0 || component_id == 0) return;
    entity_remove_component(g_runtime_world, entity, component_id);
}

static int minic_entity_has(uint64_t entity, uint64_t component_id) {
    if (!g_runtime_world || entity == 0 || component_id == 0) return 0;
    return entity_has_component(g_runtime_world, entity, component_id) ? 1 : 0;
}

static void *minic_entity_get(uint64_t entity, uint64_t component_id) {
    if (!g_runtime_world || entity == 0 || component_id == 0) return NULL;
    return entity_get_component_data(g_runtime_world, entity, component_id);
}

static void minic_entity_set_data(uint64_t entity, uint64_t component_id, void *data) {
    if (!g_runtime_world || entity == 0 || component_id == 0 || !data) return;
    entity_set_component_data(g_runtime_world, entity, component_id, data);
}

static void minic_comp_set_int(uint64_t comp_id, void *data, const char *field, int value) {
    component_set_field_int(comp_id, data, field, (int32_t)value);
}

static void minic_comp_set_float(uint64_t comp_id, void *data, const char *field, float value) {
    component_set_field_float(comp_id, data, field, value);
}

static void minic_comp_set_ptr(uint64_t comp_id, void *data, const char *field, void *value) {
    component_set_field_ptr(comp_id, data, field, value);
}

static int minic_comp_get_int(uint64_t comp_id, void *data, const char *field) {
    return component_get_field_int(comp_id, data, field);
}

static float minic_comp_get_float(uint64_t comp_id, void *data, const char *field) {
    return component_get_field_float(comp_id, data, field);
}

static void *minic_comp_get_ptr(uint64_t comp_id, void *data, const char *field) {
    return component_get_field_ptr(comp_id, data, field);
}

static float minic_sys_delta(void) {
    return game_loop_get_delta_time();
}

static float minic_sys_time(void) {
    return game_loop_get_time();
}

static uint64_t minic_sys_frame(void) {
    return game_loop_get_frame_count();
}

void runtime_api_register(void) {
    printf("Registering runtime APIs...\n");
    
    component_api_register();
    entity_api_register();
    system_api_register();
    query_api_register();
    
    minic_register("component_register", "i(p,i)", (minic_ext_fn_raw_t)minic_component_register);
    minic_register("component_add_field", "i(i,p,i,i)", (minic_ext_fn_raw_t)minic_component_add_field);
    minic_register("component_get_id", "i(p)", (minic_ext_fn_raw_t)minic_component_get_id);
    minic_register("component_get_name", "p(i)", (minic_ext_fn_raw_t)component_get_name);
    minic_register("component_get_size", "i(i)", (minic_ext_fn_raw_t)component_get_size);
    
    minic_register("entity_create", "i()", (minic_ext_fn_raw_t)minic_entity_create);
    minic_register("entity_create_named", "i(p)", (minic_ext_fn_raw_t)minic_entity_create_named);
    minic_register("entity_destroy", "v(i)", (minic_ext_fn_raw_t)minic_entity_destroy);
    minic_register("entity_add", "v(i,i)", (minic_ext_fn_raw_t)minic_entity_add);
    minic_register("entity_remove", "v(i,i)", (minic_ext_fn_raw_t)minic_entity_remove);
    minic_register("entity_has", "i(i,i)", (minic_ext_fn_raw_t)minic_entity_has);
    minic_register("entity_get", "p(i,i)", (minic_ext_fn_raw_t)minic_entity_get);
    minic_register("entity_set_data", "v(i,i,p)", (minic_ext_fn_raw_t)minic_entity_set_data);
    
    minic_register("comp_set_int", "v(i,p,p,i)", (minic_ext_fn_raw_t)minic_comp_set_int);
    minic_register("comp_set_float", "v(i,p,p,f)", (minic_ext_fn_raw_t)minic_comp_set_float);
    minic_register("comp_set_ptr", "v(i,p,p,p)", (minic_ext_fn_raw_t)minic_comp_set_ptr);
    minic_register("comp_get_int", "i(i,p,p)", (minic_ext_fn_raw_t)minic_comp_get_int);
    minic_register("comp_get_float", "f(i,p,p)", (minic_ext_fn_raw_t)minic_comp_get_float);
    minic_register("comp_get_ptr", "p(i,p,p)", (minic_ext_fn_raw_t)minic_comp_get_ptr);
    
    minic_register("sys_delta", "f()", (minic_ext_fn_raw_t)minic_sys_delta);
    minic_register("sys_time", "f()", (minic_ext_fn_raw_t)minic_sys_time);
    minic_register("sys_frame", "i()", (minic_ext_fn_raw_t)minic_sys_frame);
    
    minic_register("TYPE_INT", "i", NULL);
    minic_register("TYPE_FLOAT", "i", NULL);
    minic_register("TYPE_BOOL", "i", NULL);
    minic_register("TYPE_PTR", "i", NULL);
    minic_register("TYPE_STRING", "i", NULL);
    
    printf("Runtime APIs registered\n");
}
