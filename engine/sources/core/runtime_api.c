#include "runtime_api.h"
#include "component_api.h"
#include "entity_api.h"
#include "system_api.h"
#include "query_api.h"
#include "game_loop.h"
#include "ecs/ecs_world.h"
#include "ecs/ecs_components.h"
#include "ecs/ecs_dynamic.h"
#include "ecs/camera_bridge.h"
#include "ecs/sprite_bridge.h"

#include <minic.h>
#include <iron_input.h>
#include <time.h>
static game_world_t *g_runtime_world = NULL;

// Extract a 64-bit ID from a minic value — handles both pointer and float storage
static uint64_t extract_id(minic_val_t *arg) {
    if (arg->type == MINIC_T_PTR) return (uint64_t)(uintptr_t)arg->p;
    return (uint64_t)minic_val_to_d(*arg);
}

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
    uint64_t comp_id = extract_id(&args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    float value = (float)minic_val_to_d(args[3]);
    if (!data || !field) return minic_val_void();
    field_cache_entry_t *fc = ecs_dynamic_field_cache_lookup(comp_id, field);
    if (fc) {
        *(float *)((char *)data + fc->field_offset) = value;
    } else {
        component_set_field_float(comp_id, data, field, value);
    }
    return minic_val_void();
}

static minic_val_t minic_comp_get_float_native(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_float(0.0f);
    uint64_t comp_id = extract_id(&args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    if (!data || !field) return minic_val_float(0.0f);
    field_cache_entry_t *fc = ecs_dynamic_field_cache_lookup(comp_id, field);
    if (fc) {
        return minic_val_float(*(float *)((char *)data + fc->field_offset));
    }
    return minic_val_float(component_get_field_float(comp_id, data, field));
}

static minic_val_t minic_comp_add_float_native(minic_val_t *args, int argc) {
    if (argc < 4) return minic_val_void();
    uint64_t comp_id = extract_id(&args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    float delta = (float)minic_val_to_d(args[3]);
    if (!data || !field) return minic_val_void();
    field_cache_entry_t *fc = ecs_dynamic_field_cache_lookup(comp_id, field);
    if (fc) {
        *(float *)((char *)data + fc->field_offset) += delta;
    } else {
        float cur = component_get_field_float(comp_id, data, field);
        component_set_field_float(comp_id, data, field, cur + delta);
    }
    return minic_val_void();
}

#define MAX_BATCH_FIELDS 8

// Helper: parse comma-separated field names, look up field cache offsets.
// Returns number of fields parsed. Writes offsets to out_offsets[].
static int batch_lookup_fields(uint64_t comp_id, const char *fields, size_t *out_offsets, int max) {
    if (!fields) return 0;
    int count = 0;
    const char *p = fields;
    while (*p && count < max) {
        // Find end of current field name
        const char *start = p;
        while (*p && *p != ',') p++;
        int len = (int)(p - start);
        if (len == 0) break;
        // Copy to temp buffer for lookup
        char name[64];
        if (len >= (int)sizeof(name)) len = (int)sizeof(name) - 1;
        memcpy(name, start, len);
        name[len] = '\0';
        // Look up field cache
        field_cache_entry_t *fc = ecs_dynamic_field_cache_lookup(comp_id, name);
        if (fc) {
            out_offsets[count] = fc->field_offset;
        } else {
            // Fallback: use component API to get offset (slow, but correct)
            out_offsets[count] = (size_t)-1;
        }
        count++;
        if (*p == ',') p++;
    }
    return count;
}

static minic_val_t minic_comp_set_floats_native(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_void();
    uint64_t comp_id = extract_id(&args[0]);
    void *data = args[1].p;
    const char *fields = (const char *)args[2].p;
    if (!data || !fields) return minic_val_void();

    size_t offsets[MAX_BATCH_FIELDS];
    int count = batch_lookup_fields(comp_id, fields, offsets, MAX_BATCH_FIELDS);
    char *base = (char *)data;
    for (int i = 0; i < count && (3 + i) < argc; i++) {
        if (offsets[i] != (size_t)-1) {
            *(float *)(base + offsets[i]) = (float)minic_val_to_d(args[3 + i]);
        }
    }
    return minic_val_int(count);
}

static minic_val_t minic_comp_get_floats_native(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_float(0.0f);
    uint64_t comp_id = extract_id(&args[0]);
    void *data = args[1].p;
    const char *fields = (const char *)args[2].p;
    if (!data || !fields) return minic_val_float(0.0f);

    size_t offsets[MAX_BATCH_FIELDS];
    int count = batch_lookup_fields(comp_id, fields, offsets, MAX_BATCH_FIELDS);
    if (count < 1 || offsets[0] == (size_t)-1) return minic_val_float(0.0f);
    char *base = (char *)data;
    // Return the value of the first field for single-field lookup
    return minic_val_float(*(float *)(base + offsets[0]));
}

static minic_val_t minic_comp_add_floats_native(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_void();
    uint64_t comp_id = extract_id(&args[0]);
    void *data = args[1].p;
    const char *fields = (const char *)args[2].p;
    if (!data || !fields) return minic_val_void();

    size_t offsets[MAX_BATCH_FIELDS];
    int count = batch_lookup_fields(comp_id, fields, offsets, MAX_BATCH_FIELDS);
    char *base = (char *)data;
    for (int i = 0; i < count && (3 + i) < argc; i++) {
        if (offsets[i] != (size_t)-1) {
            *(float *)(base + offsets[i]) += (float)minic_val_to_d(args[3 + i]);
        }
    }
    return minic_val_int(count);
}

static minic_val_t minic_query_iter_comp_ptr_native(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_ptr(NULL);
    int query_id = (int)minic_val_to_d(args[0]);
    int entity_index = (int)minic_val_to_d(args[1]);
    int comp_index = (int)minic_val_to_d(args[2]);
    void *ptr = query_iter_comp_ptr(query_id, entity_index, comp_index);
    return minic_val_ptr(ptr);
}

static minic_val_t minic_query_iter_entity_native(minic_val_t *args, int argc) {
    if (argc < 2) return minic_val_float(0.0);
    int query_id = (int)minic_val_to_d(args[0]);
    int index = (int)minic_val_to_d(args[1]);
    uint64_t entity = query_iter_entity(query_id, index);
    return minic_val_float((double)entity);
}

static minic_val_t minic_query_foreach_native(minic_val_t *args, int argc) {
    if (argc < 2) return minic_val_int(0);
    int query_id = (int)minic_val_to_d(args[0]);
    void *callback = args[1].p;
    if (!callback) return minic_val_int(0);

    int total = 0;
    query_iter_begin(query_id);
    while (query_iter_next(query_id)) {
        int count = query_iter_count(query_id);
        for (int i = 0; i < count; i++) {
            uint64_t entity = query_iter_entity(query_id, i);
            void *comp_data = query_iter_comp_ptr(query_id, i, 0);
            minic_val_t cb_args[2];
            cb_args[0] = minic_val_float((double)entity);
            cb_args[1] = minic_val_ptr(comp_data);
            minic_call_fn(callback, cb_args, 2);
            total++;
        }
    }
    return minic_val_int(total);
}

static minic_val_t minic_query_foreach_batch_native(minic_val_t *args, int argc) {
    if (argc < 2) return minic_val_int(0);
    int query_id = (int)minic_val_to_d(args[0]);
    void *callback = args[1].p;
    if (!callback) return minic_val_int(0);

    int total = 0;
    query_iter_begin(query_id);
    while (query_iter_next(query_id)) {
        int count = query_iter_count(query_id);
        // Get pointer to first entity's component data — they're contiguous
        void *comp_data = query_iter_comp_ptr(query_id, 0, 0);
        minic_val_t cb_args[2];
        cb_args[0] = minic_val_int(count);
        cb_args[1] = minic_val_ptr(comp_data);
        minic_call_fn(callback, cb_args, 2);
        total += count;
    }
    return minic_val_int(total);
}

static minic_val_t minic_comp_set_int_native(minic_val_t *args, int argc) {
    if (argc < 4) return minic_val_void();
    uint64_t comp_id = extract_id(&args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    int32_t value = (int32_t)minic_val_to_d(args[3]);
    if (!data || !field) return minic_val_void();
    field_cache_entry_t *fc = ecs_dynamic_field_cache_lookup(comp_id, field);
    if (fc) {
        *(int32_t *)((char *)data + fc->field_offset) = value;
    } else {
        component_set_field_int(comp_id, data, field, value);
    }
    return minic_val_void();
}

static minic_val_t minic_comp_get_int_native(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_int(0);
    uint64_t comp_id = extract_id(&args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    if (!data || !field) return minic_val_int(0);
    field_cache_entry_t *fc = ecs_dynamic_field_cache_lookup(comp_id, field);
    if (fc) {
        return minic_val_int(*(int32_t *)((char *)data + fc->field_offset));
    }
    return minic_val_int(component_get_field_int(comp_id, data, field));
}

static minic_val_t minic_comp_set_ptr_native(minic_val_t *args, int argc) {
    if (argc < 4) return minic_val_void();
    uint64_t comp_id = extract_id(&args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    void *value = args[3].p;
    if (!data || !field) return minic_val_void();
    field_cache_entry_t *fc = ecs_dynamic_field_cache_lookup(comp_id, field);
    if (fc) {
        *(void **)((char *)data + fc->field_offset) = value;
    } else {
        component_set_field_ptr(comp_id, data, field, value);
    }
    return minic_val_void();
}

static minic_val_t minic_comp_get_ptr_native(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_ptr(NULL);
    uint64_t comp_id = extract_id(&args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    if (!data || !field) return minic_val_ptr(NULL);
    field_cache_entry_t *fc = ecs_dynamic_field_cache_lookup(comp_id, field);
    if (fc) {
        return minic_val_ptr(*(void **)((char *)data + fc->field_offset));
    }
    return minic_val_ptr(component_get_field_ptr(comp_id, data, field));
}

static minic_val_t minic_comp_set_bool_native(minic_val_t *args, int argc) {
    if (argc < 4) return minic_val_void();
    uint64_t comp_id = extract_id(&args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    bool value = (bool)(int)minic_val_to_d(args[3]);
    if (!data || !field) return minic_val_void();
    field_cache_entry_t *fc = ecs_dynamic_field_cache_lookup(comp_id, field);
    if (fc) {
        *(bool *)((char *)data + fc->field_offset) = value;
    } else {
        component_set_field_bool(comp_id, data, field, value);
    }
    return minic_val_void();
}

static minic_val_t minic_comp_get_bool_native(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_int(0);
    uint64_t comp_id = extract_id(&args[0]);
    void *data = args[1].p;
    const char *field = (const char *)args[2].p;
    if (!data || !field) return minic_val_int(0);
    field_cache_entry_t *fc = ecs_dynamic_field_cache_lookup(comp_id, field);
    if (fc) {
        return minic_val_int(*(bool *)((char *)data + fc->field_offset) ? 1 : 0);
    }
    return minic_val_int(component_get_field_bool(comp_id, data, field) ? 1 : 0);
}

static minic_val_t minic_component_add_field_native(minic_val_t *args, int argc) {
    if (argc < 4) return minic_val_int(0);
    uint64_t comp_id = extract_id(&args[0]);
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
    uint64_t comp_id = extract_id(&args[0]);
    int field_index = (int)minic_val_to_d(args[1]);
    char *name_out = (char *)args[2].p;
    int *type_out = (int *)args[3].p;
    size_t offset_out = 0;
    int result = component_get_field_info(comp_id, field_index, name_out, type_out, &offset_out);
    return minic_val_int(result);
}

static minic_type_t component_type_to_minic(int comp_type) {
    switch (comp_type) {
    case COMPONENT_TYPE_INT:   return MINIC_T_INT;
    case COMPONENT_TYPE_FLOAT: return MINIC_T_FLOAT;
    case COMPONENT_TYPE_PTR:   return MINIC_T_PTR;
    case COMPONENT_TYPE_BOOL:  return MINIC_T_BOOL;
    default:                   return MINIC_T_INT;
    }
}

static minic_val_t minic_component_finalize_native(minic_val_t *args, int argc) {
    if (argc < 2) return minic_val_int(-1);
    uint64_t comp_id = extract_id(&args[0]);
    const char *struct_name = (const char *)args[1].p;
    if (!struct_name) struct_name = component_get_name(comp_id);
    if (!struct_name) return minic_val_int(-1);

    int field_count = component_get_field_count(comp_id);
    if (field_count <= 0) return minic_val_int(-1);

    static const char *names[16];
    static char name_bufs[16][64];
    static int offsets[16];
    static minic_type_t types[16];
    static minic_type_t derefs[16];

    int actual = field_count < 16 ? field_count : 16;
    for (int i = 0; i < actual; i++) {
        int ftype = 0;
        size_t foffset = 0;
        component_get_field_info(comp_id, i, name_bufs[i], &ftype, &foffset);
        names[i] = name_bufs[i];
        offsets[i] = (int)foffset;
        types[i] = component_type_to_minic(ftype);
        derefs[i] = types[i];
    }

    minic_register_struct_native(struct_name, names, offsets, types, derefs, actual);
    return minic_val_int(0);
}

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
static minic_val_t minic_component_register_native(minic_val_t *args, int argc) {
    if (argc < 2 || !g_runtime_world) return minic_val_ptr(NULL);
    const char *name = (const char *)args[0].p;
    int size = (int)minic_val_to_d(args[1]);
    if (!name) return minic_val_ptr(NULL);

    component_desc_t desc = {
        .name = name,
        .size = (size_t)size,
        .alignment = 4,
        .ctor = NULL,
        .dtor = NULL,
        .copy = NULL,
        .move = NULL
    };

    uint64_t id = component_register(g_runtime_world, &desc);
    return minic_val_ptr((void *)(uintptr_t)id);
}
static uint64_t minic_component_get_id(const char *name) {
    if (!g_runtime_world || !name) return 0;
    return component_get_id(g_runtime_world, name);
}
static minic_val_t minic_component_get_id_native(minic_val_t *args, int argc) {
    if (argc < 1 || !g_runtime_world) return minic_val_ptr(NULL);
    const char *name = (const char *)args[0].p;
    if (!name) return minic_val_ptr(NULL);
    uint64_t id = component_get_id(g_runtime_world, name);
    return minic_val_ptr((void *)(uintptr_t)id);
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

// Native wrappers that preserve 64-bit entity IDs through minic_val_t via pointer storage
static minic_val_t minic_entity_create_native(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    if (!g_runtime_world) return minic_val_ptr(NULL);
    uint64_t id = entity_create(g_runtime_world);
    return minic_val_ptr((void *)(uintptr_t)id);
}
static minic_val_t minic_entity_create_named_native(minic_val_t *args, int argc) {
    if (argc < 1 || !g_runtime_world) return minic_val_ptr(NULL);
    const char *name = (const char *)args[0].p;
    if (!name) return minic_val_ptr(NULL);
    uint64_t id = entity_create_with_name(g_runtime_world, name);
    return minic_val_ptr((void *)(uintptr_t)id);
}
static minic_val_t minic_entity_destroy_native(minic_val_t *args, int argc) {
    if (argc < 1 || !g_runtime_world) return minic_val_void();
    uint64_t entity = extract_id(&args[0]);
    if (entity == 0) return minic_val_void();
    entity_destroy(g_runtime_world, entity);
    return minic_val_void();
}
static minic_val_t minic_entity_add_native(minic_val_t *args, int argc) {
    if (argc < 2 || !g_runtime_world) return minic_val_void();
    uint64_t entity = extract_id(&args[0]);
    uint64_t comp = extract_id(&args[1]);
    if (entity == 0 || comp == 0) return minic_val_void();
    entity_add_component(g_runtime_world, entity, comp);
    return minic_val_void();
}
static minic_val_t minic_entity_remove_native(minic_val_t *args, int argc) {
    if (argc < 2 || !g_runtime_world) return minic_val_void();
    uint64_t entity = extract_id(&args[0]);
    uint64_t comp = extract_id(&args[1]);
    if (entity == 0 || comp == 0) return minic_val_void();
    entity_remove_component(g_runtime_world, entity, comp);
    return minic_val_void();
}

static minic_val_t minic_entity_remove_component_native(minic_val_t *args, int argc) {
    if (argc < 2 || !g_runtime_world) return minic_val_void();
    uint64_t entity = extract_id(&args[0]);
    uint64_t comp = extract_id(&args[1]);
    if (entity == 0 || comp == 0) return minic_val_void();
    entity_remove_component(g_runtime_world, entity, comp);
    return minic_val_void();
}
static minic_val_t minic_entity_has_native(minic_val_t *args, int argc) {
    if (argc < 2 || !g_runtime_world) return minic_val_int(0);
    uint64_t entity = extract_id(&args[0]);
    uint64_t comp = extract_id(&args[1]);
    if (entity == 0 || comp == 0) return minic_val_int(0);
    return minic_val_int(entity_has_component(g_runtime_world, entity, comp) ? 1 : 0);
}
static minic_val_t minic_entity_exists_native(minic_val_t *args, int argc) {
    if (argc < 1 || !g_runtime_world) return minic_val_int(0);
    uint64_t entity = extract_id(&args[0]);
    if (entity == 0) return minic_val_int(0);
    return minic_val_int(entity_exists(g_runtime_world, entity) ? 1 : 0);
}
static minic_val_t minic_entity_is_valid_native(minic_val_t *args, int argc) {
    if (argc < 1 || !g_runtime_world) return minic_val_int(0);
    uint64_t entity = extract_id(&args[0]);
    if (entity == 0) return minic_val_int(0);
    return minic_val_int(entity_is_valid(g_runtime_world, entity) ? 1 : 0);
}
static minic_val_t minic_entity_get_name_native(minic_val_t *args, int argc) {
    if (argc < 1 || !g_runtime_world) return minic_val_ptr(NULL);
    uint64_t entity = extract_id(&args[0]);
    if (entity == 0) return minic_val_ptr(NULL);
    return minic_val_ptr((void *)entity_get_name(g_runtime_world, entity));
}
static minic_val_t minic_entity_find_by_name_native(minic_val_t *args, int argc) {
    if (argc < 1 || !g_runtime_world) return minic_val_ptr(NULL);
    const char *name = (const char *)args[0].p;
    if (!name) return minic_val_ptr(NULL);
    uint64_t id = entity_find_by_name(g_runtime_world, name);
    return minic_val_ptr((void *)(uintptr_t)id);
}
static minic_val_t minic_entity_get_parent_native(minic_val_t *args, int argc) {
    if (argc < 1 || !g_runtime_world) return minic_val_ptr(NULL);
    uint64_t entity = extract_id(&args[0]);
    if (entity == 0) return minic_val_ptr(NULL);
    uint64_t parent = entity_get_parent(g_runtime_world, entity);
    return minic_val_ptr((void *)(uintptr_t)parent);
}
static minic_val_t minic_entity_set_parent_native(minic_val_t *args, int argc) {
    if (argc < 2 || !g_runtime_world) return minic_val_void();
    uint64_t child = extract_id(&args[0]);
    uint64_t parent = extract_id(&args[1]);
    if (child == 0) return minic_val_void();
    entity_set_parent(g_runtime_world, child, parent);
    return minic_val_void();
}
static minic_val_t minic_entity_get_native(minic_val_t *args, int argc) {
    if (argc < 2 || !g_runtime_world) return minic_val_ptr(NULL);
    uint64_t entity = extract_id(&args[0]);
    uint64_t comp = extract_id(&args[1]);
    if (entity == 0 || comp == 0) return minic_val_ptr(NULL);
    return minic_val_ptr(entity_get_component_data(g_runtime_world, entity, comp));
}
static minic_val_t minic_entity_set_name_native(minic_val_t *args, int argc) {
    if (argc < 2) return minic_val_void();
    uint64_t entity = extract_id(&args[0]);
    const char *name = (const char *)args[1].p;
    if (!g_runtime_world || entity == 0 || !name) return minic_val_void();
    entity_set_name(g_runtime_world, entity, name);
    return minic_val_void();
}
static uint64_t minic_entity_find_by_name(const char *name) {
    if (!g_runtime_world || !name) return 0;
    return entity_find_by_name(g_runtime_world, name);
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
    uint64_t entity = extract_id(&args[0]);
    uint64_t component_id = extract_id(&args[1]);
    void *data = args[2].p;
    if (!g_runtime_world || entity == 0 || component_id == 0 || !data) return minic_val_void();
    entity_set_component_data(g_runtime_world, entity, component_id, data);
    return minic_val_void();
}

static minic_val_t minic_dbg_entity_has_comp(minic_val_t *args, int argc) {
    if (argc < 2) return minic_val_int(0);
    uint64_t entity = extract_id(&args[0]);
    uint64_t comp_id = extract_id(&args[1]);
    int result = entity_has_component(g_runtime_world, entity, comp_id) ? 1 : 0;
    printf("[dbg] entity_has_comp(%llu, %llu) = %d\n", (unsigned long long)entity, (unsigned long long)comp_id, result);
    return minic_val_int(result);
}

static float g_fps = 60.0f;
static int g_fps_frames = 0;
static float g_fps_time = 0.0f;

static void update_fps(float delta) {
    g_fps_frames++;
    g_fps_time += delta;
    if (g_fps_time >= 0.5f) {
        g_fps = (float)g_fps_frames / g_fps_time;
        g_fps_frames = 0;
        g_fps_time = 0.0f;
    }
}

static float minic_sys_delta(void) {
    float delta = game_loop_get_delta_time();
    update_fps(delta);
    return delta;
}
static float minic_sys_time(void) {
    return game_loop_get_time();
}
static uint64_t minic_sys_frame(void) {
    return game_loop_get_frame_count();
}
static float minic_sys_fps(void) {
    return g_fps;
}

static struct timespec g_bench_epoch = {0, 0};
static bool g_bench_epoch_set = false;

static minic_val_t minic_bench_time_native(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (!g_bench_epoch_set) {
        g_bench_epoch = ts;
        g_bench_epoch_set = true;
    }
    double elapsed = (double)(ts.tv_sec - g_bench_epoch.tv_sec) +
                     (double)(ts.tv_nsec - g_bench_epoch.tv_nsec) * 1e-9;
    return minic_val_float((float)elapsed);
}
static float minic_rand_float(void) {
    return (float)rand() / (float)RAND_MAX;
}
static minic_val_t minic_rand_range(minic_val_t *args, int argc) {
    if (argc < 2) return minic_val_float(0.0f);
    float min = (float)minic_val_to_d(args[0]);
    float max = (float)minic_val_to_d(args[1]);
    float result = min + (max - min) * ((float)rand() / (float)RAND_MAX);
    return minic_val_float(result);
}

static minic_val_t minic_sprite_load(minic_val_t *args, int argc) {
    if (argc < 1) return minic_val_void();
    const char *name = (const char*)args[0].p;
    if (!name) return minic_val_void();
    gpu_texture_t *tex = data_get_image((char*)name);
    return minic_val_ptr(tex);
}

static minic_val_t minic_sprite_draw(minic_val_t *args, int argc) {
    if (argc < 5) return minic_val_void();
    const char *path = (const char*)args[0].p;
    float x = (float)minic_val_to_d(args[1]);
    float y = (float)minic_val_to_d(args[2]);
    float w = (float)minic_val_to_d(args[3]);
    float h = (float)minic_val_to_d(args[4]);
    gpu_texture_t *tex = sprite_get_texture(path);
    if (tex) {
        draw_scaled_image(tex, x, y, w, h);
    }
    return minic_val_void();
}

static minic_val_t minic_sprite_create(minic_val_t *args, int argc) {
    if (argc < 1) return minic_val_void();
    uint64_t entity = extract_id(&args[0]);

    // Optional: set texture_path directly (bypasses comp_set_ptr VM issues)
    if (argc >= 2 && args[1].type == MINIC_T_PTR && args[1].p != NULL) {
        sprite_bridge_set_texture(entity, (const char *)args[1].p);
    }

    sprite_bridge_create_sprite(entity);
    return minic_val_void();
}

static minic_val_t minic_sys_width(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    return minic_val_float((float)iron_window_width());
}

static minic_val_t minic_sys_height(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    return minic_val_float((float)iron_window_height());
}

static minic_val_t minic_draw_begin(minic_val_t *args, int argc) {
    void *target = NULL;
    if (argc > 0 && args[0].type == MINIC_T_PTR) {
        target = args[0].p;
    }
    bool clear = (argc < 2) ? true : (bool)(int)minic_val_to_d(args[1]);
    int color = (argc < 3) ? 0 : (int)minic_val_to_d(args[2]);
    draw_begin(target, clear, color);
    return minic_val_void();
}

static minic_val_t minic_draw_end(minic_val_t *args, int argc) {
    (void)argc; (void)args;
    draw_end();
    return minic_val_void();
}

static minic_val_t minic_draw_set_color(minic_val_t *args, int argc) {
    if (argc < 1) {
        return minic_val_void();
    }
    int color = (int)minic_val_to_d(args[0]);
    draw_set_color((uint32_t)color);
    return minic_val_void();
}

static minic_val_t minic_draw_string(minic_val_t *args, int argc) {
    if (argc < 2 || draw_font == NULL) {
        return minic_val_void();
    }
    const char *text = (const char *)args[0].p;
    float x = (float)minic_val_to_d(args[1]);
    float y = (argc > 2) ? (float)minic_val_to_d(args[2]) : 0.0f;
    draw_string(text, x, y);
    return minic_val_void();
}

static minic_val_t minic_draw_set_font(minic_val_t *args, int argc) {
    if (argc < 2) {
        return minic_val_void();
    }
    const char *font_name = (const char *)args[0].p;
    int size = (int)minic_val_to_d(args[1]);
    draw_font_t *font = data_get_font((char*)font_name);
    if (font) {
        draw_font_init(font);
        draw_set_font(font, size);
    }
    return minic_val_void();
}

static minic_val_t minic_draw_fps(minic_val_t *args, int argc) {
    float x = (argc > 0) ? (float)minic_val_to_d(args[0]) : 10.0f;
    float y = (argc > 1) ? (float)minic_val_to_d(args[1]) : 50.0f;
    char buf[32];
    snprintf(buf, sizeof(buf), "FPS: %.1f", g_fps);
    draw_string(buf, x, y);
    return minic_val_void();
}

static minic_val_t minic_draw_line(minic_val_t *args, int argc) {
    if (argc < 4) {
        return minic_val_void();
    }
    float x0 = (float)minic_val_to_d(args[0]);
    float y0 = (float)minic_val_to_d(args[1]);
    float x1 = (float)minic_val_to_d(args[2]);
    float y1 = (float)minic_val_to_d(args[3]);
    float strength = (argc > 4) ? (float)minic_val_to_d(args[4]) : 1.0f;
    draw_line(x0, y0, x1, y1, strength);
    return minic_val_void();
}

static minic_val_t minic_draw_filled_rect(minic_val_t *args, int argc) {
    if (argc < 4) {
        return minic_val_void();
    }
    float x = (float)minic_val_to_d(args[0]);
    float y = (float)minic_val_to_d(args[1]);
    float w = (float)minic_val_to_d(args[2]);
    float h = (float)minic_val_to_d(args[3]);
    draw_filled_rect(x, y, w, h);
    return minic_val_void();
}

static minic_val_t minic_draw_rect(minic_val_t *args, int argc) {
    if (argc < 5) {
        return minic_val_void();
    }
    float x = (float)minic_val_to_d(args[0]);
    float y = (float)minic_val_to_d(args[1]);
    float w = (float)minic_val_to_d(args[2]);
    float h = (float)minic_val_to_d(args[3]);
    float strength = (float)minic_val_to_d(args[4]);
    draw_rect(x, y, w, h, strength);
    return minic_val_void();
}

static minic_val_t minic_draw_filled_circle(minic_val_t *args, int argc) {
    if (argc < 3) {
        return minic_val_void();
    }
    float cx = (float)minic_val_to_d(args[0]);
    float cy = (float)minic_val_to_d(args[1]);
    float radius = (float)minic_val_to_d(args[2]);
    int segments = (argc > 3) ? (int)minic_val_to_d(args[3]) : 32;
    draw_filled_circle(cx, cy, radius, segments);
    return minic_val_void();
}

static minic_val_t minic_draw_circle(minic_val_t *args, int argc) {
    if (argc < 4) {
        return minic_val_void();
    }
    float cx = (float)minic_val_to_d(args[0]);
    float cy = (float)minic_val_to_d(args[1]);
    float radius = (float)minic_val_to_d(args[2]);
    float strength = (float)minic_val_to_d(args[3]);
    int segments = (argc > 4) ? (int)minic_val_to_d(args[4]) : 32;
    draw_circle(cx, cy, radius, segments, strength);
    return minic_val_void();
}

static minic_val_t minic_camera_set_position(minic_val_t *args, int argc) {
    if (argc < 2) return minic_val_void();
    float x = (float)minic_val_to_d(args[0]);
    float y = (float)minic_val_to_d(args[1]);
    camera2d_set_position(camera_bridge_get_camera(), x, y);
    return minic_val_void();
}

static minic_val_t minic_camera_set_zoom(minic_val_t *args, int argc) {
    if (argc < 1) return minic_val_void();
    float zoom = (float)minic_val_to_d(args[0]);
    camera2d_set_zoom(camera_bridge_get_camera(), zoom);
    return minic_val_void();
}

static minic_val_t minic_camera_set_rotation(minic_val_t *args, int argc) {
    if (argc < 1) return minic_val_void();
    float rotation = (float)minic_val_to_d(args[0]);
    camera2d_set_rotation(camera_bridge_get_camera(), rotation);
    return minic_val_void();
}

static minic_val_t minic_camera_follow(minic_val_t *args, int argc) {
    if (argc < 2) return minic_val_void();
    float tx = (float)minic_val_to_d(args[0]);
    float ty = (float)minic_val_to_d(args[1]);
    camera2d_follow(camera_bridge_get_camera(), tx, ty);
    return minic_val_void();
}

static minic_val_t minic_camera_get_x(void) {
    return minic_val_float(camera2d_get_x(camera_bridge_get_camera()));
}

static minic_val_t minic_camera_get_y(void) {
    return minic_val_float(camera2d_get_y(camera_bridge_get_camera()));
}

static minic_val_t minic_camera_get_zoom(void) {
    return minic_val_float(camera2d_get_zoom(camera_bridge_get_camera()));
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

static minic_val_t minic_keyboard_repeat(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_PTR) {
        return minic_val_int(0);
    }
    const char *key = (const char *)args[0].p;
    if (!key) {
        return minic_val_int(0);
    }
    return minic_val_int(keyboard_repeat((char*)key) ? 1 : 0);
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

static minic_val_t minic_gamepad_down(minic_val_t *args, int argc) {
    if (argc < 2 || args[0].type != MINIC_T_INT || args[1].type != MINIC_T_PTR) {
        return minic_val_int(0);
    }
    int i = (int)minic_val_to_d(args[0]);
    const char *button = (const char *)args[1].p;
    if (!button) {
        return minic_val_int(0);
    }
    return minic_val_int((int)gamepad_down(i, (char*)button));
}

static minic_val_t minic_gamepad_started(minic_val_t *args, int argc) {
    if (argc < 2 || args[0].type != MINIC_T_INT || args[1].type != MINIC_T_PTR) {
        return minic_val_int(0);
    }
    int i = (int)minic_val_to_d(args[0]);
    const char *button = (const char *)args[1].p;
    if (!button) {
        return minic_val_int(0);
    }
    return minic_val_int(gamepad_started(i, (char*)button) ? 1 : 0);
}

static minic_val_t minic_gamepad_released(minic_val_t *args, int argc) {
    if (argc < 2 || args[0].type != MINIC_T_INT || args[1].type != MINIC_T_PTR) {
        return minic_val_int(0);
    }
    int i = (int)minic_val_to_d(args[0]);
    const char *button = (const char *)args[1].p;
    if (!button) {
        return minic_val_int(0);
    }
    return minic_val_int(gamepad_released(i, (char*)button) ? 1 : 0);
}

static minic_val_t minic_gamepad_stick_left_x(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_INT) {
        return minic_val_float(0.0f);
    }
    int i = (int)minic_val_to_d(args[0]);
    return minic_val_float(gamepad_stick_left_x(i));
}

static minic_val_t minic_gamepad_stick_left_y(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_INT) {
        return minic_val_float(0.0f);
    }
    int i = (int)minic_val_to_d(args[0]);
    return minic_val_float(gamepad_stick_left_y(i));
}

static minic_val_t minic_gamepad_stick_right_x(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_INT) {
        return minic_val_float(0.0f);
    }
    int i = (int)minic_val_to_d(args[0]);
    return minic_val_float(gamepad_stick_right_x(i));
}

static minic_val_t minic_gamepad_stick_right_y(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_INT) {
        return minic_val_float(0.0f);
    }
    int i = (int)minic_val_to_d(args[0]);
    return minic_val_float(gamepad_stick_right_y(i));
}

static minic_val_t minic_gamepad_stick_delta_x(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_INT) {
        return minic_val_float(0.0f);
    }
    int i = (int)minic_val_to_d(args[0]);
    return minic_val_float(gamepad_stick_delta_x(i));
}

static minic_val_t minic_gamepad_stick_delta_y(minic_val_t *args, int argc) {
    if (argc < 1 || args[0].type != MINIC_T_INT) {
        return minic_val_float(0.0f);
    }
    int i = (int)minic_val_to_d(args[0]);
    return minic_val_float(gamepad_stick_delta_y(i));
}

void runtime_api_register(void) {
    printf("Registering runtime APIs...\n");
    
    minic_register_native("printf", minic_printf);
    
    component_api_register();
    entity_api_register();
    system_api_set_world(g_runtime_world);
    system_api_register();
    query_api_set_world(g_runtime_world);
    query_api_register();
    
    minic_register_native("component_register", minic_component_register_native);
    minic_register_native("component_add_field", minic_component_add_field_native);
    minic_register_native("component_finalize", minic_component_finalize_native);
    minic_register_native("component_get_id", minic_component_get_id_native);
    minic_register_native("component_lookup", minic_component_get_id_native);
    minic_register("component_get_name", "p(i)", (minic_ext_fn_raw_t)component_get_name);
    minic_register("component_get_size", "i(i)", (minic_ext_fn_raw_t)component_get_size);
    minic_register("component_get_alignment", "i(i)", (minic_ext_fn_raw_t)minic_component_get_alignment);
    minic_register("component_get_field_count", "i(i)", (minic_ext_fn_raw_t)minic_component_get_field_count);
    minic_register_native("component_get_field_info", minic_component_get_field_info_native);
    minic_register("component_set_hooks", "i(p,p,p)", (minic_ext_fn_raw_t)minic_component_set_hooks);
    
    minic_register_native("entity_create", minic_entity_create_native);
    minic_register_native("entity_create_named", minic_entity_create_named_native);
    minic_register_native("entity_destroy", minic_entity_destroy_native);
    minic_register_native("entity_add", minic_entity_add_native);
    minic_register_native("entity_remove", minic_entity_remove_native);
    minic_register_native("entity_has", minic_entity_has_native);
    minic_register_native("entity_exists", minic_entity_exists_native);
    minic_register_native("entity_is_valid", minic_entity_is_valid_native);
    minic_register_native("entity_get_name", minic_entity_get_name_native);
    minic_register_native("entity_set_name", minic_entity_set_name_native);
    minic_register_native("entity_find_by_name", minic_entity_find_by_name_native);
    minic_register_native("entity_get_parent", minic_entity_get_parent_native);
    minic_register_native("entity_set_parent", minic_entity_set_parent_native);
    minic_register_native("entity_get", minic_entity_get_native);
    minic_register_native("entity_set_data", minic_entity_set_data_native);

    minic_register_native("entity_remove_component", minic_entity_remove_component_native);
    minic_register_native("comp_set_int", minic_comp_set_int_native);
    minic_register_native("comp_set_float", minic_comp_set_float_native);
    minic_register_native("comp_add_float", minic_comp_add_float_native);
    minic_register_native("comp_set_floats", minic_comp_set_floats_native);
    minic_register_native("comp_get_floats", minic_comp_get_floats_native);
    minic_register_native("comp_add_floats", minic_comp_add_floats_native);
    minic_register_native("comp_set_ptr", minic_comp_set_ptr_native);
    minic_register_native("comp_set_bool", minic_comp_set_bool_native);
    minic_register_native("comp_get_int", minic_comp_get_int_native);
    minic_register_native("comp_get_float", minic_comp_get_float_native);
    minic_register_native("comp_get_ptr", minic_comp_get_ptr_native);
    minic_register_native("comp_get_bool", minic_comp_get_bool_native);
    
    minic_register("sys_delta_time", "f()", (minic_ext_fn_raw_t)minic_sys_delta);
    minic_register("sys_time", "f()", (minic_ext_fn_raw_t)minic_sys_time);
    minic_register("sys_frame", "i()", (minic_ext_fn_raw_t)minic_sys_frame);
    minic_register("sys_fps", "f()", (minic_ext_fn_raw_t)minic_sys_fps);
    minic_register("sys_width", "f()", (minic_ext_fn_raw_t)minic_sys_width);
    minic_register("sys_height", "f()", (minic_ext_fn_raw_t)minic_sys_height);
    minic_register("rand_float", "f()", (minic_ext_fn_raw_t)minic_rand_float);
    minic_register("rand_range", "f(ff)", (minic_ext_fn_raw_t)minic_rand_range);
    minic_register_native("bench_time", minic_bench_time_native);

    minic_register_native("sprite_load", minic_sprite_load);
    minic_register_native("sprite_draw", minic_sprite_draw);
    minic_register_native("sprite_create", minic_sprite_create);
    minic_register_native("draw_begin", minic_draw_begin);
    minic_register_native("draw_end", minic_draw_end);
    minic_register_native("draw_set_color", minic_draw_set_color);
    minic_register_native("draw_string", minic_draw_string);
    minic_register_native("draw_set_font", minic_draw_set_font);
    minic_register_native("draw_line", minic_draw_line);
    minic_register_native("draw_filled_rect", minic_draw_filled_rect);
    minic_register_native("draw_rect", minic_draw_rect);
    minic_register_native("draw_filled_circle", minic_draw_filled_circle);
    minic_register_native("draw_circle", minic_draw_circle);
    minic_register_native("draw_fps", minic_draw_fps);

    minic_register_native("camera_set_position", minic_camera_set_position);
    minic_register_native("camera_set_zoom", minic_camera_set_zoom);
    minic_register_native("camera_set_rotation", minic_camera_set_rotation);
    minic_register_native("camera_follow", minic_camera_follow);
    minic_register_native("camera_get_x", minic_camera_get_x);
    minic_register_native("camera_get_y", minic_camera_get_y);
    minic_register_native("camera_get_zoom", minic_camera_get_zoom);
    
    minic_register_native("dbg_entity_has_comp", minic_dbg_entity_has_comp);
    
    minic_register("query_iter_begin", "v(i)", (minic_ext_fn_raw_t)query_iter_begin);
    minic_register("query_iter_next", "i(i)", (minic_ext_fn_raw_t)query_iter_next);
    minic_register("query_iter_count", "i(i)", (minic_ext_fn_raw_t)query_iter_count);
    minic_register("query_iter_entity", "i(i,i)", (minic_ext_fn_raw_t)query_iter_entity);
    minic_register_native("query_iter_entity_id", minic_query_iter_entity_native);
    minic_register_native("query_iter_comp_ptr", minic_query_iter_comp_ptr_native);
    minic_register_native("query_iter_comp_ptr", minic_query_iter_comp_ptr_native);
    minic_register_native("query_foreach", minic_query_foreach_native);
    minic_register_native("query_foreach_batch", minic_query_foreach_batch_native);
    
    minic_register("TYPE_INT", "i", NULL);
    minic_register("TYPE_FLOAT", "i", NULL);
    minic_register("TYPE_BOOL", "i", NULL);
    minic_register("TYPE_PTR", "i", NULL);
    minic_register("TYPE_STRING", "i", NULL);
    
    minic_register_native("keyboard_down", minic_keyboard_down);
    minic_register_native("keyboard_started", minic_keyboard_started);
    minic_register_native("keyboard_released", minic_keyboard_released);
    minic_register_native("keyboard_repeat", minic_keyboard_repeat);
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
    
    minic_register_native("gamepad_down", minic_gamepad_down);
    minic_register_native("gamepad_started", minic_gamepad_started);
    minic_register_native("gamepad_released", minic_gamepad_released);
    minic_register_native("gamepad_stick_left_x", minic_gamepad_stick_left_x);
    minic_register_native("gamepad_stick_left_y", minic_gamepad_stick_left_y);
    minic_register_native("gamepad_stick_right_x", minic_gamepad_stick_right_x);
    minic_register_native("gamepad_stick_right_y", minic_gamepad_stick_right_y);
    minic_register_native("gamepad_stick_delta_x", minic_gamepad_stick_delta_x);
    minic_register_native("gamepad_stick_delta_y", minic_gamepad_stick_delta_y);
    
    printf("Runtime APIs registered\n");
}
