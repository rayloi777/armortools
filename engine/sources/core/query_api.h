#pragma once

#include <stdint.h>
#include <stdbool.h>

struct game_world_t;

#define MAX_QUERIES 64

typedef struct {
    uint64_t flecs_query;
    char filter[256];
    bool valid;
    int last_count;
    uint64_t last_entities[256];
} runtime_query_t;

void query_api_register(void);
void query_api_set_world(struct game_world_t *world);

int query_create(const char *filter_expr);
void query_destroy(int query_id);
bool query_next(int query_id);
int query_count(int query_id);
uint64_t query_get(int query_id, int index);
