#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct game_world_t;

void component_api_register(void);

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