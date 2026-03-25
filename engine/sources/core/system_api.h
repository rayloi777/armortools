#pragma once

#include <stdint.h>
#include <stdbool.h>

struct game_world_t;

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

uint64_t system_create(
    struct game_world_t *world,
    const char *name,
    system_phase_t phase,
    const char *query,
    system_callback_t callback
);

void system_destroy(struct game_world_t *world, uint64_t system_id);
void system_enable(struct game_world_t *world, uint64_t system_id, bool enabled);
bool system_is_enabled(struct game_world_t *world, uint64_t system_id);
void system_set_context(struct game_world_t *world, uint64_t system_id, void *ctx);
void *system_get_context(struct game_world_t *world, uint64_t system_id);

void system_api_register(void);