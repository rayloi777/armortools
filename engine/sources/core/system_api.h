#pragma once

#include <stdint.h>
#include <stdbool.h>

struct game_world_t;
typedef uint64_t ecs_entity_t;

typedef void (*system_callback_t)(void *it);

typedef enum {
    SYSTEM_PHASE_PRE_UPDATE,
    SYSTEM_PHASE_UPDATE,
    SYSTEM_PHASE_POST_UPDATE,
    SYSTEM_PHASE_PRE_FRAME,
    SYSTEM_PHASE_FRAME,
    SYSTEM_PHASE_POST_FRAME,
    SYSTEM_PHASE_INIT,
    SYSTEM_PHASE_SHUTDOWN
} system_phase_t;

typedef struct {
    char name[64];
    uint64_t flecs_id;
    system_phase_t phase;
    void *minic_callback;
    void *user_context;
    bool enabled;
    ecs_entity_t last_entities[1024];
    int last_count;
} registered_system_t;

uint64_t system_create(
    struct game_world_t *world,
    const char *name,
    system_phase_t phase,
    const char *query_expr,
    system_callback_t callback
);

uint64_t system_create_with_components(
    struct game_world_t *world,
    const char *name,
    system_phase_t phase,
    const uint64_t *component_ids,
    int component_count,
    void *minic_callback
);

void system_destroy(struct game_world_t *world, uint64_t system_id);
void system_enable(struct game_world_t *world, uint64_t system_id, bool enabled);
bool system_is_enabled(struct game_world_t *world, uint64_t system_id);
void system_set_context(struct game_world_t *world, uint64_t system_id, void *ctx);
void *system_get_context(struct game_world_t *world, uint64_t system_id);

registered_system_t *system_get_by_id(uint64_t system_id);
registered_system_t *system_get_by_name(const char *name);

void system_api_init(void);
void system_api_set_world(struct game_world_t *world);
void system_api_register(void);
