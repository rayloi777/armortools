#pragma once

#include <stdint.h>
#include <stddef.h>

struct game_world_t;

#define DYNAMIC_TYPE_INT 0
#define DYNAMIC_TYPE_FLOAT 1
#define DYNAMIC_TYPE_BOOL 2
#define DYNAMIC_TYPE_PTR 3
#define DYNAMIC_TYPE_STRING 4

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
} dynamic_component_t;

void ecs_dynamic_init(void);
void ecs_dynamic_shutdown(void);

uint64_t ecs_dynamic_component_create(struct game_world_t *world, const char *name, size_t size, size_t alignment);
int ecs_dynamic_component_add_field(uint64_t component_id, const char *name, int type, size_t offset);
dynamic_component_t *ecs_dynamic_component_get(uint64_t component_id);
dynamic_component_t *ecs_dynamic_component_find(const char *name);

void ecs_dynamic_component_set_data(struct game_world_t *world, uint64_t entity, uint64_t component_id, const char *field_name, const void *value);
void *ecs_dynamic_component_get_field(uint64_t component_id, const char *field_name, void *component_data);

int ecs_dynamic_component_count(void);
dynamic_component_t *ecs_dynamic_component_at(int index);
