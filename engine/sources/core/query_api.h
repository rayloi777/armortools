#pragma once

#include <stdint.h>
#include <stdbool.h>

struct game_world_t;

#define MAX_QUERIES 64
#define MAX_QUERY_COMPONENTS 16

typedef struct {
    uint64_t flecs_query;
    char filter[256];
    bool valid;
    bool iter_started;
    bool truncated;
    int last_count;
    int total_count;
    uint64_t last_entities[1024];
    uint64_t *all_entities;
    int all_entities_capacity;
    void *cached_it;
    uint64_t components[MAX_QUERY_COMPONENTS];
    int component_count;
    void *cached_comp_data[MAX_QUERY_COMPONENTS];
    size_t cached_comp_sizes[MAX_QUERY_COMPONENTS];
    int cached_comp_count;
} runtime_query_t;

void query_api_register(void);
void query_api_set_world(struct game_world_t *world);

int query_create(const char *filter_expr);
void query_destroy(int query_id);
bool query_next(int query_id);
int query_count(int query_id);
uint64_t query_get(int query_id, int index);

int query_new(void);
int query_with(int query_id, uint64_t component_id);
int query_find(int query_id);
int query_entities(int query_id, uint64_t *entities, int max);
void query_free(int query_id);

int query_total_count(int query_id);
bool query_was_truncated(int query_id);
int query_get_all(int query_id, uint64_t *entities, int max);

void query_iter_begin(int query_id);
bool query_iter_next(int query_id);
int query_iter_count(int query_id);
uint64_t query_iter_entity(int query_id, int index);
void *query_iter_comp_ptr(int query_id, int entity_index, int comp_index);
uint64_t query_get_component_id(int query_id, int comp_index);
