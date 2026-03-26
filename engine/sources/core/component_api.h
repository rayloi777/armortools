#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct game_world_t;

void component_api_register(void);

#define COMPONENT_TYPE_INT 0
#define COMPONENT_TYPE_FLOAT 1
#define COMPONENT_TYPE_BOOL 2
#define COMPONENT_TYPE_PTR 3
#define COMPONENT_TYPE_STRING 4

typedef struct {
    const char *name;
    size_t size;
    size_t alignment;
    void (*ctor)(void *ptr);
    void (*dtor)(void *ptr);
    void (*copy)(void *dst, const void *src);
    void (*move)(void *dst, void *src);
} component_desc_t;

uint64_t component_register(struct game_world_t *world, const component_desc_t *desc);
uint64_t component_get_id(struct game_world_t *world, const char *name);
bool component_exists(struct game_world_t *world, uint64_t component_id);
size_t component_get_size(uint64_t component_id);
size_t component_get_alignment(uint64_t component_id);

int component_add_field(uint64_t component_id, const char *name, int type, size_t offset);
const char *component_get_name(uint64_t component_id);
int component_get_field_count(uint64_t component_id);
int component_get_field_info(uint64_t component_id, int field_index, char *name_out, int *type_out, size_t *offset_out);

void component_set_field_int(uint64_t component_id, void *data, const char *field_name, int32_t value);
void component_set_field_float(uint64_t component_id, void *data, const char *field_name, float value);
void component_set_field_ptr(uint64_t component_id, void *data, const char *field_name, void *value);
int32_t component_get_field_int(uint64_t component_id, void *data, const char *field_name);
float component_get_field_float(uint64_t component_id, void *data, const char *field_name);
void *component_get_field_ptr(uint64_t component_id, void *data, const char *field_name);
