#include "runtime_api.h"
#include "component_api.h"
#include "entity_api.h"
#include "system_api.h"
#include "query_api.h"
#include "game_loop.h"
#include "ecs/ecs_world.h"
#include "ecs/ecs_components.h"
#include "ecs/ecs_dynamic.h"

#include <minic.h>
#include <iron_input.h>

static game_world_t *g_runtime_world = NULL;

static minic_val_t minic_printf(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_PTR) {
        return minic_val_int(0);
    }
    const char *fmt = (const char *)args[0].p;
    if (!fmt) {
        return minic_val_int(0);
    }
    
    int arg_idx = 1;
    char output[1024];
    int out_pos = 0;
    
    while (*fmt && out_pos < 1020) {
        if (*fmt != '%') {
            output[out_pos++] = *fmt++;
            continue;
        }
        
        fmt++;
        char spec = *fmt++;
        char tmp[128];
        
        if (spec == 'd' || spec == 'i') {
            int iv = (arg_idx < argc) ? (int)minic_val_to_d(args[arg_idx++]) : 0;
            snprintf(tmp, sizeof(tmp), "%d", iv);
        }
        else if (spec == 'u') {
            unsigned uv = (arg_idx < argc) ? (unsigned)(int)minic_val_to_d(args[arg_idx++]) : 0;
            snprintf(tmp, sizeof(tmp), "%u", uv);
        }
        else if (spec == 'f' || spec == 'g') {
            double dv = (arg_idx < argc) ? minic_val_to_d(args[arg_idx++]) : 0.0;
            snprintf(tmp, sizeof(tmp), "%g", dv);
        }
        else if (spec == 's') {
            const char *sv = (arg_idx < argc && args[arg_idx].type == MINIC_T_PTR) ? (const char *)args[arg_idx++].p : "(null)";
            snprintf(tmp, sizeof(tmp), "%s", sv);
        }
        else if (spec == 'p') {
            void *pv = (arg_idx < argc && args[arg_idx].type == MINIC_T_PTR) ? args[arg_idx++].p : NULL;
            snprintf(tmp, sizeof(tmp), "%p", pv);
        }
        else if (spec == '%') {
            output[out_pos++] = '%';
            continue;
        }
        else {
            snprintf(tmp, sizeof(tmp), "%%%c", spec);
        }
        
        int len = (int)strlen(tmp);
        if (out_pos + len < 1020) {
            memcpy(output + out_pos, tmp, len);
            out_pos += len;
        }
    }
    
    output[out_pos++] = '\n';
    output[out_pos] = '\0';
    fprintf(stderr, "%s", output);
    
    return minic_val_int(out_pos);
}

static minic_val_t minic_comp_set_float_native(minic_val_t *args, int argc) {
    if (argc < 4) return minic_val_void();
    uint64_t comp_id = (uint64_t)(int)minic_val_to_d(args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    float value = (float)minic_val_to_d(args[3]);
    component_set_field_float(comp_id, data, field, value);
    return minic_val_void();
}

static minic_val_t minic_comp_get_float_native(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_float(0.0f);
    uint64_t comp_id = (uint64_t)(int)minic_val_to_d(args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    float result = component_get_field_float(comp_id, data, field);
    return minic_val_float(result);
}

static minic_val_t minic_comp_set_int_native(minic_val_t *args, int argc) {
    if (argc < 4) return minic_val_void();
    uint64_t comp_id = (uint64_t)(int)minic_val_to_d(args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    int32_t value = (int32_t)minic_val_to_d(args[3]);
    component_set_field_int(comp_id, data, field, value);
    return minic_val_void();
}

static minic_val_t minic_comp_get_int_native(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_int(0);
    uint64_t comp_id = (uint64_t)(int)minic_val_to_d(args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    int32_t result = component_get_field_int(comp_id, data, field);
    return minic_val_int(result);
}

static minic_val_t minic_comp_set_ptr_native(minic_val_t *args, int argc) {
    if (argc < 4) return minic_val_void();
    uint64_t comp_id = (uint64_t)(int)minic_val_to_d(args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    void *value = args[3].p;
    component_set_field_ptr(comp_id, data, field, value);
    return minic_val_void();
}

static minic_val_t minic_comp_get_ptr_native(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_ptr(NULL);
    uint64_t comp_id = (uint64_t)(int)minic_val_to_d(args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    void *result = component_get_field_ptr(comp_id, data, field);
    return minic_val_ptr(result);
}

static minic_val_t minic_component_add_field_native(minic_val_t *args, int argc) {
    if (argc < 4) return minic_val_int(0);
    uint64_t comp_id = (uint64_t)(int)minic_val_to_d(args[0]);
    const char *name = (const char *)args[1].p;
    int type = (int)minic_val_to_d(args[2]);
    int offset = (int)minic_val_to_d(args[3]);
    int result = component_add_field(comp_id, name, type, (size_t)offset);
    return minic_val_int(result);
}

static int minic_component_get_alignment(uint64_t comp_id) {
    return (int)component_get_alignment(comp_id);
}

static int minic_component_get_field_count(uint64_t comp_id) {
    return component_get_field_count(comp_id);
}

static minic_val_t minic_component_get_field_info_native(minic_val_t *args, int argc) {
    if (argc < 4) return minic_val_int(-1);
    uint64_t comp_id = (uint64_t)(int)minic_val_to_d(args[0]);
    int field_index = (int)minic_val_to_d(args[1]);
    char *name_out = (char *)args[2].p;
    int *type_out = (int *)args[3].p;
    size_t offset_out = 0;
    int result = component_get_field_info(comp_id, field_index, name_out, type_out, &offset_out);
    return minic_val_int(result);
}

void minic_register_builtins(void) {
    minic_register_native("printf", minic_printf);
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
static uint64_t minic_component_get_id(const char *name) {
    if (!g_runtime_world || !name) return 0;
    return component_get_id(g_runtime_world, name);
}
static int minic_component_set_hooks(const char *name, const char *ctor_name, const char *dtor_name) {
    if (!g_runtime_world || !name) return -1;
    uint64_t comp_id = component_get_id(g_runtime_world, name);
    if (comp_id == 0) return -1;
    return ecs_dynamic_component_set_hooks(g_runtime_world, comp_id, ctor_name, dtor_name);
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
static int minic_entity_exists(uint64_t entity) {
    if (!g_runtime_world || entity == 0) return 0;
    return entity_exists(g_runtime_world, entity) ? 1 : 0;
}
static int minic_entity_is_valid(uint64_t entity) {
    if (!g_runtime_world || entity == 0) return 0;
    return entity_is_valid(g_runtime_world, entity) ? 1 : 0;
}
static const char *minic_entity_get_name(uint64_t entity) {
    if (!g_runtime_world || entity == 0) return NULL;
    return entity_get_name(g_runtime_world, entity);
}
static minic_val_t minic_entity_set_name_native(minic_val_t *args, int argc) {
    if (argc < 2) return minic_val_void();
    uint64_t entity = (uint64_t)(int)minic_val_to_d(args[0]);
    const char *name = (const char *)args[1].p;
    if (!g_runtime_world || entity == 0 || !name) return minic_val_void();
    entity_set_name(g_runtime_world, entity, name);
    return minic_val_void();
}
static uint64_t minic_entity_get_parent(uint64_t entity) {
    if (!g_runtime_world || entity == 0) return 0;
    return entity_get_parent(g_runtime_world, entity);
}
static void minic_entity_set_parent(uint64_t child, uint64_t parent) {
    if (!g_runtime_world || child == 0) return;
    entity_set_parent(g_runtime_world, child, parent);
}
static void *minic_entity_get(uint64_t entity, uint64_t component_id) {
    if (!g_runtime_world || entity == 0 || component_id == 0) return NULL;
    return entity_get_component_data(g_runtime_world, entity, component_id);
}
static minic_val_t minic_entity_set_data_native(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_void();
    uint64_t entity = (uint64_t)(int)minic_val_to_d(args[0]);
    uint64_t component_id = (uint64_t)(int)minic_val_to_d(args[1]);
    void *data = args[2].p;
    if (!g_runtime_world || entity == 0 || component_id == 0 || !data) return minic_val_void();
    entity_set_component_data(g_runtime_world, entity, component_id, data);
    return minic_val_void();
}
static minic_val_t minic_dbg_entity_has_comp(minic_val_t *args, int argc) {
    if (argc < 2) return minic_val_int(0);
    uint64_t entity = (uint64_t)(int)minic_val_to_d(args[0]);
    uint64_t comp_id = (uint64_t)(int)minic_val_to_d(args[1]);
    int result = entity_has_component(g_runtime_world, entity, comp_id) ? 1 : 0;
    printf("[dbg] entity_has_comp(%llu, %llu) = %d\n", (unsigned long long)entity, (unsigned long long)comp_id, result);
    return minic_val_int(result);
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

static minic_val_t minic_keyboard_down(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_PTR) {
        return minic_val_int(0);
    }
    const char *key = (const char *)args[0].p;
    if (!key) {
        return minic_val_int(0);
    }
    return minic_val_int(keyboard_down((char*)key) ? 1 : 0);
}

static minic_val_t minic_keyboard_started(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_PTR) {
        return minic_val_int(0);
    }
    const char *key = (const char *)args[0].p;
    if (!key) {
        return minic_val_int(0);
    }
    return minic_val_int(keyboard_started((char*)key) ? 1 : 0);
}

static minic_val_t minic_keyboard_released(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_PTR) {
        return minic_val_int(0);
    }
    const char *key = (const char *)args[0].p;
    if (!key) {
        return minic_val_int(0);
    }
    return minic_val_int(keyboard_released((char*)key) ? 1 : 0);
}

static minic_val_t minic_mouse_view_x(void) {
    return minic_val_float(mouse_view_x());
}

static minic_val_t minic_mouse_view_y(void) {
    return minic_val_float(mouse_view_y());
}

static minic_val_t minic_mouse_down(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_PTR) {
        return minic_val_int(0);
    }
    const char *button = (const char *)args[0].p;
    if (!button) {
        return minic_val_int(0);
    }
    return minic_val_int(mouse_down((char*)button) ? 1 : 0);
}

static minic_val_t minic_mouse_started(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_PTR) {
        return minic_val_int(0);
    }
    const char *button = (const char *)args[0].p;
    if (!button) {
        return minic_val_int(0);
    }
    return minic_val_int(mouse_started((char*)button) ? 1 : 0);
}

static minic_val_t minic_mouse_released(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_PTR) {
        return minic_val_int(0);
    }
    const char *button = (const char *)args[0].p;
    if (!button) {
        return minic_val_int(0);
    }
    return minic_val_int(mouse_released((char*)button) ? 1 : 0);
}

static minic_val_t minic_mouse_x(void) {
    return minic_val_float(mouse_x);
}

static minic_val_t minic_mouse_y(void) {
    return minic_val_float(mouse_y);
}

static minic_val_t minic_mouse_dx(void) {
    return minic_val_float(mouse_movement_x);
}

static minic_val_t minic_mouse_dy(void) {
    return minic_val_float(mouse_movement_y);
}

static minic_val_t minic_mouse_wheel_delta(void) {
    return minic_val_float(mouse_wheel_delta);
}

void runtime_api_register(void) {
    printf("Registering runtime APIs...\n");
    
    component_api_register();
    entity_api_register();
    system_api_set_world(g_runtime_world);
    system_api_register();
    query_api_set_world(g_runtime_world);
    query_api_register();
    
    minic_register("component_register", "i(p,i)", (minic_ext_fn_raw_t)minic_component_register);
    minic_register_native("component_add_field", minic_component_add_field_native);
    minic_register("component_get_id", "i(p)", (minic_ext_fn_raw_t)minic_component_get_id);
    minic_register("component_lookup", "i(p)", (minic_ext_fn_raw_t)minic_component_get_id);
    minic_register("component_get_name", "p(i)", (minic_ext_fn_raw_t)component_get_name);
    minic_register("component_get_size", "i(i)", (minic_ext_fn_raw_t)component_get_size);
    minic_register("component_get_alignment", "i(i)", (minic_ext_fn_raw_t)minic_component_get_alignment);
    minic_register("component_get_field_count", "i(i)", (minic_ext_fn_raw_t)minic_component_get_field_count);
    minic_register_native("component_get_field_info", minic_component_get_field_info_native);
    minic_register("component_set_hooks", "i(p,p,p)", (minic_ext_fn_raw_t)minic_component_set_hooks);
    
    minic_register("entity_create", "i()", (minic_ext_fn_raw_t)minic_entity_create);
    minic_register("entity_create_named", "i(p)", (minic_ext_fn_raw_t)minic_entity_create_named);
    minic_register("entity_destroy", "v(i)", (minic_ext_fn_raw_t)minic_entity_destroy);
    minic_register("entity_add", "v(i,i)", (minic_ext_fn_raw_t)minic_entity_add);
    minic_register("entity_remove", "v(i,i)", (minic_ext_fn_raw_t)minic_entity_remove);
    minic_register("entity_has", "i(i,i)", (minic_ext_fn_raw_t)minic_entity_has);
    minic_register("entity_exists", "i(i)", (minic_ext_fn_raw_t)minic_entity_exists);
    minic_register("entity_is_valid", "i(i)", (minic_ext_fn_raw_t)minic_entity_is_valid);
    minic_register("entity_get_name", "p(i)", (minic_ext_fn_raw_t)minic_entity_get_name);
    minic_register_native("entity_set_name", minic_entity_set_name_native);
    minic_register("entity_get_parent", "i(i)", (minic_ext_fn_raw_t)minic_entity_get_parent);
    minic_register("entity_set_parent", "v(i,i)", (minic_ext_fn_raw_t)minic_entity_set_parent);
    minic_register("entity_get", "p(i,i)", (minic_ext_fn_raw_t)minic_entity_get);
    minic_register_native("entity_set_data", minic_entity_set_data_native);
    
    minic_register_native("comp_set_int", minic_comp_set_int_native);
    minic_register_native("comp_set_float", minic_comp_set_float_native);
    minic_register_native("comp_set_ptr", minic_comp_set_ptr_native);
    minic_register_native("comp_get_int", minic_comp_get_int_native);
    minic_register_native("comp_get_float", minic_comp_get_float_native);
    minic_register_native("comp_get_ptr", minic_comp_get_ptr_native);
    
    minic_register("sys_delta", "f()", (minic_ext_fn_raw_t)minic_sys_delta);
    minic_register("sys_time", "f()", (minic_ext_fn_raw_t)minic_sys_time);
    minic_register("sys_frame", "i()", (minic_ext_fn_raw_t)minic_sys_frame);
    
    minic_register_native("dbg_entity_has_comp", minic_dbg_entity_has_comp);
    
    minic_register("TYPE_INT", "i", NULL);
    minic_register("TYPE_FLOAT", "i", NULL);
    minic_register("TYPE_BOOL", "i", NULL);
    minic_register("TYPE_PTR", "i", NULL);
    minic_register("TYPE_STRING", "i", NULL);
    
    minic_register_native("keyboard_down", minic_keyboard_down);
    minic_register_native("keyboard_started", minic_keyboard_started);
    minic_register_native("keyboard_released", minic_keyboard_released);
    minic_register_native("mouse_view_x", minic_mouse_view_x);
    minic_register_native("mouse_view_y", minic_mouse_view_y);
    minic_register_native("mouse_down", minic_mouse_down);
    minic_register_native("mouse_started", minic_mouse_started);
    minic_register_native("mouse_released", minic_mouse_released);
    minic_register_native("mouse_x", minic_mouse_x);
    minic_register_native("mouse_y", minic_mouse_y);
    minic_register_native("mouse_dx", minic_mouse_dx);
    minic_register_native("mouse_dy", minic_mouse_dy);
    minic_register_native("mouse_wheel_delta", minic_mouse_wheel_delta);
    
    printf("Runtime APIs registered\n");
}
