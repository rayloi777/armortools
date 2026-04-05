#include "query_api.h"
#include "ecs/ecs_world.h"
#include "ecs/ecs_components.h"
#include "ecs/ecs_dynamic.h"
#include "flecs.h"
#include "engine_world.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libs/minic.h>

#define ECS_ITER_SIZE sizeof(ecs_iter_t)

static runtime_query_t g_queries[MAX_QUERIES];
static bool g_query_api_initialized = false;

void query_api_set_world(game_world_t *world) {
    (void)world;
}

void query_api_init(void) {
    if (g_query_api_initialized) return;
    memset(g_queries, 0, sizeof(g_queries));
    g_query_api_initialized = true;
}

static runtime_query_t *find_free_query(void) {
    for (int i = 0; i < MAX_QUERIES; i++) {
        if (!g_queries[i].valid) {
            return &g_queries[i];
        }
    }
    return NULL;
}

static runtime_query_t *get_query_by_id(int query_id) {
    if (query_id < 0 || query_id >= MAX_QUERIES) return NULL;
    if (!g_queries[query_id].valid) return NULL;
    return &g_queries[query_id];
}

int query_create(const char *filter_expr) {
    if (!engine_world_get() || !engine_world_get()->world || !filter_expr) return -1;
    
    runtime_query_t *q = find_free_query();
    if (!q) {
        printf("ERROR: Max queries reached\n");
        return -1;
    }
    
    ecs_world_t *ecs = (ecs_world_t *)engine_world_get()->world;
    
    ecs_query_desc_t desc = {0};
    desc.expr = filter_expr;
    
    ecs_query_t *query = ecs_query_init(ecs, &desc);
    if (!query) {
        printf("ERROR: Failed to create query: %s\n", filter_expr);
        return -1;
    }
    
    q->flecs_query = (uint64_t)query;
    strncpy(q->filter, filter_expr, sizeof(q->filter) - 1);
    q->filter[sizeof(q->filter) - 1] = '\0';
    q->valid = true;
    q->iter_started = false;
    q->last_count = 0;
    memset(q->last_entities, 0, sizeof(q->last_entities));
    memset(q->components, 0, sizeof(q->components));
    q->component_count = 0;

    // Parse component names from filter expression and look up their IDs
    {
        char expr_copy[256];
        strncpy(expr_copy, filter_expr, sizeof(expr_copy) - 1);
        expr_copy[sizeof(expr_copy) - 1] = '\0';
        char *p = expr_copy;
        while (*p && q->component_count < MAX_QUERY_COMPONENTS) {
            // Skip whitespace and commas
            while (*p == ' ' || *p == ',' || *p == '\t') p++;
            if (!*p) break;
            char *start = p;
            while (*p && *p != ',' && *p != ' ' && *p != '\t') p++;
            char saved = *p;
            *p = '\0';
            ecs_entity_t comp_id = ecs_lookup(ecs, start);
            if (comp_id != 0) {
                q->components[q->component_count++] = comp_id;
            }
            *p = saved;
        }
    }
    q->cached_it = malloc(ECS_ITER_SIZE);
    if (!q->cached_it) {
        ecs_query_fini(query);
        return -1;
    }
    
    return (int)(q - g_queries);
}

int query_new(void) {
    if (!engine_world_get() || !engine_world_get()->world) return -1;
    
    runtime_query_t *q = find_free_query();
    if (!q) {
        return -1;
    }
    
    memset(q, 0, sizeof(runtime_query_t));
    q->valid = true;
    q->cached_it = malloc(ECS_ITER_SIZE);
    if (!q->cached_it) {
        return -1;
    }
    
    return (int)(q - g_queries);
}

int query_with(int query_id, uint64_t component_id) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q || q->component_count >= MAX_QUERY_COMPONENTS) return -1;
    
    q->components[q->component_count++] = component_id;
    return 0;
}

static bool build_filter_and_run(runtime_query_t *q) {
    if (!engine_world_get() || !engine_world_get()->world) return false;
    if (q->component_count == 0) return false;
    
    if (q->flecs_query) {
        ecs_query_fini((ecs_query_t *)q->flecs_query);
        q->flecs_query = 0;
    }
    
    char filter[256] = {0};
    for (int i = 0; i < q->component_count; i++) {
        if (i > 0) {
            strcat(filter, ", ");
        }
        
        const char *name = NULL;
        
        dynamic_component_t *dc = ecs_dynamic_component_get(q->components[i]);
        if (dc) {
            name = dc->name;
        } else {
            name = ecs_get_builtin_component_name(q->components[i]);
        }
        
        if (name) {
            strcat(filter, name);
        } else {
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%llu", (unsigned long long)q->components[i]);
            strcat(filter, tmp);
        }
    }
    
    ecs_world_t *ecs = (ecs_world_t *)engine_world_get()->world;
    ecs_query_desc_t desc = {0};
    desc.expr = filter;
    
    ecs_query_t *query = ecs_query_init(ecs, &desc);
    if (!query) {
        return false;
    }
    
    q->flecs_query = (uint64_t)query;
    strncpy(q->filter, filter, sizeof(q->filter) - 1);
    
    *(ecs_iter_t*)q->cached_it = ecs_query_iter(ecs, query);
    q->iter_started = true;
    
    if (!ecs_query_next((ecs_iter_t*)q->cached_it)) {
        q->last_count = 0;
        q->total_count = 0;
        q->truncated = false;
        memset(q->last_entities, 0, sizeof(q->last_entities));
        q->iter_started = false;
        return false;
    }
    
    ecs_iter_t *it = (ecs_iter_t*)q->cached_it;
    q->total_count = it->count;
    q->last_count = it->count < 1024 ? it->count : 1024;
    q->truncated = it->count > 1024;
    
    for (int i = 0; i < q->last_count; i++) {
        q->last_entities[i] = (uint64_t)it->entities[i];
    }
    
    if (q->truncated) {
        if (!q->all_entities || q->all_entities_capacity < it->count) {
            free(q->all_entities);
            q->all_entities = malloc(sizeof(uint64_t) * it->count);
            q->all_entities_capacity = q->all_entities ? it->count : 0;
        }
        if (q->all_entities) {
            for (int i = 0; i < it->count; i++) {
                q->all_entities[i] = (uint64_t)it->entities[i];
            }
        }
    }
    
    return true;
}

int query_find(int query_id) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q) return 0;

    if (build_filter_and_run(q)) {
        return q->total_count;
    }
    return 0;
}

int query_count_cached(int query_id) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q) return 0;
    return q->last_count;
}

int query_entities(int query_id, uint64_t *entities, int max) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q || !entities || max <= 0) return 0;
    
    int count = q->last_count < max ? q->last_count : max;
    for (int i = 0; i < count; i++) {
        entities[i] = q->last_entities[i];
    }
    return count;
}

static minic_val_t minic_query_entities_native(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_int(0);
    int query_id = (int)minic_val_to_d(args[0]);
    uint64_t *entities = (uint64_t *)args[1].p;
    int max = (int)minic_val_to_d(args[2]);
    int result = query_entities(query_id, entities, max);
    return minic_val_int(result);
}

void query_free(int query_id) {
    query_destroy(query_id);
}

void query_destroy(int query_id) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q) return;
    
    if (q->cached_it) {
        free(q->cached_it);
        q->cached_it = NULL;
    }
    
    if (q->flecs_query) {
        ecs_query_fini((ecs_query_t *)q->flecs_query);
    }
    
    if (q->all_entities) {
        free(q->all_entities);
        q->all_entities = NULL;
    }
    
    q->valid = false;
    q->flecs_query = 0;
    q->iter_started = false;
    q->truncated = false;
    q->last_count = 0;
    q->total_count = 0;
    memset(q->last_entities, 0, sizeof(q->last_entities));
}

bool query_next(int query_id) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q || !q->flecs_query) return false;
    
    ecs_world_t *ecs = (ecs_world_t *)engine_world_get()->world;
    ecs_query_t *query = (ecs_query_t *)q->flecs_query;
    
    if (!q->iter_started) {
        *(ecs_iter_t*)q->cached_it = ecs_query_iter(ecs, query);
        q->iter_started = true;
    }
    
    if (!ecs_query_next((ecs_iter_t*)q->cached_it)) {
        q->last_count = 0;
        q->total_count = 0;
        q->truncated = false;
        memset(q->last_entities, 0, sizeof(q->last_entities));
        q->iter_started = false;
        return false;
    }
    
    ecs_iter_t *it = (ecs_iter_t*)q->cached_it;
    q->total_count = it->count;
    q->last_count = it->count < 1024 ? it->count : 1024;
    q->truncated = it->count > 1024;
    
    for (int i = 0; i < q->last_count; i++) {
        q->last_entities[i] = (uint64_t)it->entities[i];
    }
    
    if (q->truncated) {
        if (!q->all_entities || q->all_entities_capacity < it->count) {
            free(q->all_entities);
            q->all_entities = malloc(sizeof(uint64_t) * it->count);
            q->all_entities_capacity = q->all_entities ? it->count : 0;
        }
        if (q->all_entities) {
            for (int i = 0; i < it->count; i++) {
                q->all_entities[i] = (uint64_t)it->entities[i];
            }
        }
    }

    return true;
}

int query_count(int query_id) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q) return 0;
    return q->last_count;
}

uint64_t query_get(int query_id, int index) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q || index < 0) return 0;
    
    uint64_t entity;
    
    if (index < 1024) {
        entity = q->last_entities[index];
    } else if (q->all_entities && index < q->total_count) {
        entity = q->all_entities[index];
    } else {
        return 0;
    }

    return entity;
}

int query_total_count(int query_id) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q) return 0;
    return q->total_count;
}

uint64_t query_get_component_id(int query_id, int comp_index) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q || comp_index < 0 || comp_index >= q->component_count) return 0;
    return q->components[comp_index];
}

bool query_was_truncated(int query_id) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q) return false;
    return q->truncated;
}

int query_get_all(int query_id, uint64_t *entities, int max) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q || !entities || max <= 0) return 0;
    
    int count = q->total_count < max ? q->total_count : max;
    
    if (q->all_entities) {
        for (int i = 0; i < count; i++) {
            entities[i] = q->all_entities[i];
        }
    } else {
        for (int i = 0; i < count; i++) {
            entities[i] = q->last_entities[i];
        }
    }
    
    return count;
}

void query_iter_begin(int query_id) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q || !q->flecs_query || !engine_world_get()) return;

    ecs_world_t *ecs = (ecs_world_t *)engine_world_get()->world;
    if (!q->cached_it) {
        q->cached_it = malloc(ECS_ITER_SIZE);
    }
    *(ecs_iter_t *)q->cached_it = ecs_query_iter(ecs, (ecs_query_t *)q->flecs_query);
    q->iter_started = true;
    q->last_count = 0;
    q->total_count = 0;
}

bool query_iter_next(int query_id) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q || !q->cached_it || !q->iter_started) return false;

    if (!ecs_query_next((ecs_iter_t *)q->cached_it)) {
        q->last_count = 0;
        q->iter_started = false;
        return false;
    }

    ecs_iter_t *it = (ecs_iter_t *)q->cached_it;
    q->total_count = it->count;
    q->last_count = it->count < 1024 ? it->count : 1024;
    q->truncated = it->count > 1024;

    for (int i = 0; i < q->last_count; i++) {
        q->last_entities[i] = (uint64_t)it->entities[i];
    }

    // Cache component data arrays for direct access
    q->cached_comp_count = 0;
    int term_count = it->field_count;
    if (term_count == 0) term_count = q->component_count > 0 ? q->component_count : 1;
    if (term_count > MAX_QUERY_COMPONENTS) term_count = MAX_QUERY_COMPONENTS;
    for (int t = 0; t < term_count; t++) {
        size_t sz = ecs_field_size(it, t + 1);
        if (sz == 0 && t < q->component_count) {
            // Use stored component ID to look up size from dynamic registry
            dynamic_component_t *dc = ecs_dynamic_component_get(q->components[t]);
            if (dc && dc->size > 0) sz = dc->size;
        }
        void *data = ecs_field_w_size(it, sz, t + 1);
        q->cached_comp_data[t] = data;
        q->cached_comp_sizes[t] = sz;
        q->cached_comp_count = t + 1;
    }

    return true;
}

int query_iter_count(int query_id) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q) return 0;
    return q->total_count;
}

uint64_t query_iter_entity(int query_id, int index) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q || index < 0 || index >= q->last_count) return 0;
    return q->last_entities[index];
}

void *query_iter_comp_ptr(int query_id, int entity_index, int comp_index) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q || entity_index < 0 || entity_index >= q->last_count) return NULL;
    if (comp_index < 0 || comp_index >= q->cached_comp_count) return NULL;
    if (!q->cached_comp_data[comp_index] || q->cached_comp_sizes[comp_index] == 0) return NULL;

    char *base = (char *)q->cached_comp_data[comp_index];
    size_t size = q->cached_comp_sizes[comp_index];
    // Pad stride to 8-byte alignment (Flecs aligns component storage)
    size_t stride = (size + 7) & ~(size_t)7;
    if (stride == 0) stride = size;
    return base + entity_index * stride;
}

void query_api_register(void) {
    query_api_init();

    minic_register("query_create", "i(p)", (minic_ext_fn_raw_t)query_create);
    minic_register("query_destroy", "v(i)", (minic_ext_fn_raw_t)query_destroy);
    minic_register("query_next", "i(i)", (minic_ext_fn_raw_t)query_next);
    minic_register("query_count", "i(i)", (minic_ext_fn_raw_t)query_count);
    minic_register("query_get", "i(i,i)", (minic_ext_fn_raw_t)query_get);
    
    minic_register("query_new", "i()", (minic_ext_fn_raw_t)query_new);
    minic_register("query_with", "i(i,I)", (minic_ext_fn_raw_t)query_with);
    minic_register("query_find", "i(i)", (minic_ext_fn_raw_t)query_find);
    minic_register_native("query_entities", minic_query_entities_native);
    minic_register("query_count_cached", "i(i)", (minic_ext_fn_raw_t)query_count_cached);
    minic_register("query_free", "v(i)", (minic_ext_fn_raw_t)query_free);
    
    minic_register("query_total_count", "i(i)", (minic_ext_fn_raw_t)query_total_count);
    minic_register("query_was_truncated", "i(i)", (minic_ext_fn_raw_t)query_was_truncated);
    
    printf("Query API registered\n");
}
