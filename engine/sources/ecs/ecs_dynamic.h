#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct game_world_t;

#define DYNAMIC_TYPE_INT 0
#define DYNAMIC_TYPE_FLOAT 1
#define DYNAMIC_TYPE_BOOL 2
#define DYNAMIC_TYPE_PTR 3
#define DYNAMIC_TYPE_STRING 4

#define MAX_DYNAMIC_COMPONENTS 256

typedef struct {
    char name[32];
    int type;
    size_t offset;
    size_t count;
} dynamic_field_t;

typedef struct {
    char name[64];
    uint64_t flecs_id;
    size_t size;
    size_t alignment;
    int field_count;
    dynamic_field_t fields[16];
    void *type_info;
    char ctor_name[64];
    char dtor_name[64];
} dynamic_component_t;

extern dynamic_component_t g_components[MAX_DYNAMIC_COMPONENTS];
extern int g_component_count;

void ecs_dynamic_init(void);
void ecs_dynamic_shutdown(void);

uint64_t ecs_dynamic_component_create(struct game_world_t *world, const char *name, size_t size, size_t alignment);
int ecs_dynamic_component_add_field(uint64_t component_id, const char *name, int type, size_t offset);
int ecs_dynamic_component_set_hooks(struct game_world_t *world, uint64_t component_id, const char *ctor_name, const char *dtor_name);
dynamic_component_t *ecs_dynamic_component_get(uint64_t component_id);
dynamic_component_t *ecs_dynamic_component_find(const char *name);

void ecs_dynamic_component_set_data(struct game_world_t *world, uint64_t entity, uint64_t component_id, const char *field_name, const void *value);
void *ecs_dynamic_component_get_field(uint64_t component_id, const char *field_name, void *component_data);

int ecs_dynamic_component_count(void);
dynamic_component_t *ecs_dynamic_component_at(int index);

typedef struct {
    uint64_t component_id;
    uint32_t field_name_hash;
    size_t field_offset;
    int field_type;
} field_cache_entry_t;

#define FIELD_CACHE_BUCKETS 256
#define FIELD_CACHE_BUCKET_CAP 16
#define MAX_FIELD_CACHE (FIELD_CACHE_BUCKETS * FIELD_CACHE_BUCKET_CAP)

void ecs_dynamic_field_cache_build(void);
field_cache_entry_t *ecs_dynamic_field_cache_lookup(uint64_t component_id, const char *field_name);
field_cache_entry_t *ecs_dynamic_field_cache_lookup_hashed(uint64_t component_id, uint32_t field_name_hash);

// Component ID hash table — O(1) lookup by flecs_id
#define COMP_ID_BUCKETS 64
void ecs_dynamic_comp_id_cache_build(void);
dynamic_component_t *ecs_dynamic_component_get_fast(uint64_t component_id);
