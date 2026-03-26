#include "query_api.h"
#include "ecs/ecs_world.h"
#include "flecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libs/minic.h>

static runtime_query_t g_queries[MAX_QUERIES];
static bool g_query_api_initialized = false;
static game_world_t *g_query_world = NULL;

void query_api_set_world(game_world_t *world) {
    g_query_world = world;
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
    if (!g_query_world || !g_query_world->world || !filter_expr) return -1;
    
    runtime_query_t *q = find_free_query();
    if (!q) {
        printf("ERROR: Max queries reached\n");
        return -1;
    }
    
    ecs_world_t *ecs = (ecs_world_t *)g_query_world->world;
    
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
    q->last_count = 0;
    memset(q->last_entities, 0, sizeof(q->last_entities));
    
    printf("Query created: id=%d, filter=%s\n", (int)(q - g_queries), filter_expr);
    return (int)(q - g_queries);
}

void query_destroy(int query_id) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q) return;
    
    if (q->flecs_query) {
        ecs_query_fini((ecs_query_t *)q->flecs_query);
    }
    
    q->valid = false;
    q->flecs_query = 0;
    q->last_count = 0;
    memset(q->last_entities, 0, sizeof(q->last_entities));
}

bool query_next(int query_id) {
    runtime_query_t *q = get_query_by_id(query_id);
    if (!q || !q->flecs_query) return false;
    
    ecs_world_t *ecs = (ecs_world_t *)g_query_world->world;
    ecs_query_t *query = (ecs_query_t *)q->flecs_query;
    
    ecs_iter_t it = ecs_query_iter(ecs, query);
    
    if (!ecs_query_next(&it)) {
        q->last_count = 0;
        memset(q->last_entities, 0, sizeof(q->last_entities));
        return false;
    }
    
    q->last_count = it.count;
    for (int i = 0; i < it.count && i < 256; i++) {
        q->last_entities[i] = (uint64_t)it.entities[i];
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
    if (!q || index < 0 || index >= q->last_count) return 0;
    return q->last_entities[index];
}

void query_api_register(void) {
    query_api_init();
    
    minic_register("query_create", "i(p)", (minic_ext_fn_raw_t)query_create);
    minic_register("query_destroy", "v(i)", (minic_ext_fn_raw_t)query_destroy);
    minic_register("query_next", "i(i)", (minic_ext_fn_raw_t)query_next);
    minic_register("query_count", "i(i)", (minic_ext_fn_raw_t)query_count);
    minic_register("query_get", "i(i,i)", (minic_ext_fn_raw_t)query_get);
    
    printf("Query API registered\n");
}
