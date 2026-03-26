#include "system_api.h"
#include "ecs/ecs_world.h"
#include "flecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libs/minic.h>

#define MAX_SYSTEMS 64

static registered_system_t g_systems[MAX_SYSTEMS];
static int g_system_count = 0;
static bool g_initialized = false;

void system_api_init(void) {
    if (g_initialized) return;
    memset(g_systems, 0, sizeof(g_systems));
    g_initialized = true;
}

static ecs_entity_t phase_to_flecs(system_phase_t phase) {
    switch (phase) {
        case SYSTEM_PHASE_PRE_UPDATE: return EcsPreUpdate;
        case SYSTEM_PHASE_UPDATE: return EcsOnUpdate;
        case SYSTEM_PHASE_POST_UPDATE: return EcsPostUpdate;
        case SYSTEM_PHASE_PRE_FRAME: return EcsPreStore;
        case SYSTEM_PHASE_FRAME: return EcsOnValidate;
        case SYSTEM_PHASE_POST_FRAME: return EcsPostFrame;
        case SYSTEM_PHASE_INIT: return EcsOnStart;
        case SYSTEM_PHASE_SHUTDOWN: return EcsPreStore;
        default: return EcsOnUpdate;
    }
}

static registered_system_t *find_free_system(void) {
    for (int i = 0; i < MAX_SYSTEMS; i++) {
        if (g_systems[i].flecs_id == 0) {
            return &g_systems[i];
        }
    }
    return NULL;
}

registered_system_t *system_get_by_id(uint64_t system_id) {
    if (system_id == 0) return NULL;
    for (int i = 0; i < MAX_SYSTEMS; i++) {
        if (g_systems[i].flecs_id == system_id) {
            return &g_systems[i];
        }
    }
    return NULL;
}

registered_system_t *system_get_by_name(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < MAX_SYSTEMS; i++) {
        if (g_systems[i].flecs_id != 0 && strcmp(g_systems[i].name, name) == 0) {
            return &g_systems[i];
        }
    }
    return NULL;
}

static void system_trampoline(ecs_iter_t *it) {
    registered_system_t *sys = NULL;
    for (int i = 0; i < MAX_SYSTEMS; i++) {
        if (g_systems[i].flecs_id == it->system) {
            sys = &g_systems[i];
            break;
        }
    }
    
    if (!sys || !sys->enabled) return;
    
    if (sys->minic_callback) {
        minic_val_t args[2];
        args[0].type = MINIC_T_PTR;
        args[0].p = it;
        args[1].type = MINIC_T_PTR;
        args[1].p = sys->user_context;
        minic_call_fn(sys->minic_callback, args, 2);
    }
}

uint64_t system_create_with_components(
    struct game_world_t *world,
    const char *name,
    system_phase_t phase,
    const uint64_t *component_ids,
    int component_count,
    void *minic_callback
) {
    if (!world || !world->world || !name) return 0;
    
    registered_system_t *existing = system_get_by_name(name);
    if (existing) {
        return existing->flecs_id;
    }
    
    registered_system_t *sys = find_free_system();
    if (!sys) {
        printf("ERROR: Max systems reached\n");
        return 0;
    }
    
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    
    ecs_system_desc_t desc = {0};
    desc.entity = 0;
    desc.callback = system_trampoline;
    desc.ctx = sys;
    
    if (component_ids && component_count > 0) {
        for (int i = 0; i < component_count && i < FLECS_TERM_COUNT_MAX; i++) {
            desc.query.terms[i].id = (ecs_id_t)component_ids[i];
            desc.query.terms[i].inout = EcsInOutDefault;
        }
    }
    
    ecs_entity_t system_id = ecs_system_init(ecs, &desc);
    if (system_id == 0) {
        printf("ERROR: Failed to create system: %s\n", name);
        return 0;
    }
    
    ecs_set_name(ecs, system_id, name);
    ecs_entity_t phase_entity = phase_to_flecs(phase);
    ecs_add_pair(ecs, system_id, EcsDependsOn, phase_entity);
    
    strncpy(sys->name, name, sizeof(sys->name) - 1);
    sys->name[sizeof(sys->name) - 1] = '\0';
    sys->flecs_id = system_id;
    sys->phase = phase;
    sys->minic_callback = minic_callback;
    sys->user_context = NULL;
    sys->enabled = true;
    
    g_system_count++;
    
    printf("System created: %s (id=%llu)\n", name, (unsigned long long)system_id);
    return system_id;
}

uint64_t system_create(
    struct game_world_t *world,
    const char *name,
    system_phase_t phase,
    const char *query_expr,
    system_callback_t callback
) {
    return system_create_with_components(world, name, phase, NULL, 0, (void *)callback);
}

void system_destroy(struct game_world_t *world, uint64_t system_id) {
    if (!world || !world->world || system_id == 0) return;
    
    registered_system_t *sys = system_get_by_id(system_id);
    if (!sys) return;
    
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    ecs_delete(ecs, (ecs_entity_t)system_id);
    
    sys->flecs_id = 00;
    sys->name[0] = '\0';
    sys->minic_callback = NULL;
    sys->user_context = NULL;
    sys->enabled = false;
    
    g_system_count--;
}

void system_enable(struct game_world_t *world, uint64_t system_id, bool enabled) {
    if (!world || !world->world || system_id == 0) return;
    
    registered_system_t *sys = system_get_by_id(system_id);
    if (!sys) return;
    
    sys->enabled = enabled;
    
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    ecs_enable(ecs, (ecs_entity_t)system_id, enabled);
}

bool system_is_enabled(struct game_world_t *world, uint64_t system_id) {
    if (!world || !world->world || system_id == 0) return false;
    
    registered_system_t *sys = system_get_by_id(system_id);
    if (!sys) return false;
    return sys->enabled;
}

void system_set_context(struct game_world_t *world, uint64_t system_id, void *ctx) {
    if (!world || !world->world || system_id == 0) return;
    
    registered_system_t *sys = system_get_by_id(system_id);
    if (!sys) return;
    
    sys->user_context = ctx;
}

void *system_get_context(struct game_world_t *world, uint64_t system_id) {
    if (!world || !world->world || system_id == 0) return NULL;
    
    registered_system_t *sys = system_get_by_id(system_id);
    if (!sys) return NULL;
    return sys->user_context;
}

static int minic_system_create(
    game_world_t *world,
    const char *name,
    system_phase_t phase,
    const uint64_t *component_ids,
    int component_count
) {
    return (int)system_create_with_components(world, name, phase, component_ids, component_count, NULL);
}

static int minic_system_destroy(game_world_t *world, const char *name) {
    registered_system_t *sys = system_get_by_name(name);
    if (!sys) return -1;
    system_destroy(world, sys->flecs_id);
    return 0;
}

static void minic_system_enable(game_world_t *world, const char *name, bool enabled) {
    registered_system_t *sys = system_get_by_name(name);
    if (!sys) return;
    system_enable(world, sys->flecs_id, enabled);
}

static bool minic_system_is_enabled(game_world_t *world, const char *name) {
    registered_system_t *sys = system_get_by_name(name);
    if (!sys) return true;
    return system_is_enabled(world, sys->flecs_id);
}

static void minic_system_set_context(game_world_t *world, const char *name, void *ctx) {
    registered_system_t *sys = system_get_by_name(name);
    if (!sys) return;
    system_set_context(world, sys->flecs_id, ctx);
}

static void *minic_system_get_context(game_world_t *world, const char *name) {
    registered_system_t *sys = system_get_by_name(name);
    if (!sys) return NULL;
    return system_get_context(world, sys->flecs_id);
}

static int minic_system_get_entity_count(game_world_t *world, const char *name) {
    (void)world;
    (void)name;
    return g_system_count;
}

static uint64_t minic_system_get_entity(game_world_t *world, const char *name, int index) {
    (void)world;
    (void)name;
    (void)index;
    return 0;
}

void system_api_register(void) {
    system_api_init();
    
    minic_register("system_create", "i(p,i,p,i)", (minic_ext_fn_raw_t)minic_system_create);
    minic_register("system_destroy", "i(p,p)", (minic_ext_fn_raw_t)minic_system_destroy);
    minic_register("system_enable", "v(p,p,i)", (minic_ext_fn_raw_t)minic_system_enable);
    minic_register("system_is_enabled", "i(p,p)", (minic_ext_fn_raw_t)minic_system_is_enabled);
    minic_register("system_set_context", "v(p,p,p)", (minic_ext_fn_raw_t)minic_system_set_context);
    minic_register("system_get_context", "p(p,p)", (minic_ext_fn_raw_t)minic_system_get_context);
    minic_register("system_get_entity_count", "i(p,p)", (minic_ext_fn_raw_t)minic_system_get_entity_count);
    minic_register("system_get_entity", "i(p,p,i)", (minic_ext_fn_raw_t)minic_system_get_entity);
    
    minic_register("PHASE_PRE_UPDATE", "i", NULL);
    minic_register("PHASE_UPDATE", "i", NULL);
    minic_register("PHASE_POST_UPDATE", "i", NULL);
    minic_register("PHASE_PRE_FRAME", "i", NULL);
    minic_register("PHASE_FRAME", "i", NULL);
    minic_register("PHASE_POST_FRAME", "i", NULL);
    minic_register("PHASE_INIT", "i", NULL);
    minic_register("PHASE_SHUTDOWN", "i", NULL);
    
    printf("System API registered\n");
}