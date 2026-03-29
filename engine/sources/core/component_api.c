#include "component_api.h"
#include "ecs/ecs_world.h"
#include "ecs/ecs_components.h"
#include "ecs/ecs_dynamic.h"
#include "flecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint64_t component_register(struct game_world_t *world, const component_desc_t *desc) {
    if (!world || !world->world || !desc || !desc->name) {
        return 0;
    }
    
    uint64_t builtin = ecs_get_builtin_component(desc->name);
    if (builtin != 0) {
        return builtin;
    }
    
    dynamic_component_t *existing = ecs_dynamic_component_find(desc->name);
    if (existing) {
        return existing->flecs_id;
    }
    
    size_t alignment = desc->alignment > 0 ? desc->alignment : 8;
    uint64_t id = ecs_dynamic_component_create(world, desc->name, desc->size, alignment);
    
    if (id == 0) {
        printf("[component_api] Failed to register component: %s\n", desc->name);
        return 0;
    }
    
    printf("[component_api] Registered component: %s (id=%llu, size=%zu)\n", desc->name, (unsigned long long)id, desc->size);
    
    return id;
}

uint64_t component_get_id(struct game_world_t *world, const char *name) {
    if (!world || !name) return 0;
    
    uint64_t builtin = ecs_get_builtin_component(name);
    if (builtin != 0) {
        return builtin;
    }
    
    dynamic_component_t *dc = ecs_dynamic_component_find(name);
    if (dc) {
        return dc->flecs_id;
    }
    
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    ecs_entity_t found = ecs_lookup(ecs, name);
    return (uint64_t)found;
}

bool component_exists(struct game_world_t *world, uint64_t component_id) {
    if (!world || !world->world || component_id == 0) return false;
    
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    return ecs_exists(ecs, (ecs_entity_t)component_id);
}

size_t component_get_size(uint64_t component_id) {
    dynamic_component_t *dc = ecs_dynamic_component_get(component_id);
    if (dc) {
        return dc->size;
    }
    return 0;
}

size_t component_get_alignment(uint64_t component_id) {
    dynamic_component_t *dc = ecs_dynamic_component_get(component_id);
    if (dc) {
        return dc->alignment;
    }
    return 8;
}

int component_add_field(uint64_t component_id, const char *name, int type, size_t offset) {
    return ecs_dynamic_component_add_field(component_id, name, type, offset);
}

const char *component_get_name(uint64_t component_id) {
    dynamic_component_t *dc = ecs_dynamic_component_get(component_id);
    if (dc) {
        return dc->name;
    }
    return NULL;
}

int component_get_field_count(uint64_t component_id) {
    dynamic_component_t *dc = ecs_dynamic_component_get(component_id);
    if (dc) {
        return dc->field_count;
    }
    return 0;
}

int component_get_field_info(uint64_t component_id, int field_index, char *name_out, int *type_out, size_t *offset_out) {
    dynamic_component_t *dc = ecs_dynamic_component_get(component_id);
    if (!dc || field_index < 0 || field_index >= dc->field_count) {
        return -1;
    }
    
    dynamic_field_t *field = &dc->fields[field_index];
    if (name_out) strcpy(name_out, field->name);
    if (type_out) *type_out = field->type;
    if (offset_out) *offset_out = field->offset;
    
    return 0;
}

void component_set_field_int(uint64_t component_id, void *data, const char *field_name, int32_t value) {
    if (!data || !field_name) return;
    void *field = ecs_dynamic_component_get_field(component_id, field_name, data);
    if (field) {
        *(int32_t *)field = value;
    }
}

void component_set_field_float(uint64_t component_id, void *data, const char *field_name, float value) {
    if (!data || !field_name) return;
    void *field = ecs_dynamic_component_get_field(component_id, field_name, data);
    if (field) {
        *(float *)field = value;
    }
}

void component_set_field_ptr(uint64_t component_id, void *data, const char *field_name, void *value) {
    if (!data || !field_name) return;
    void *field = ecs_dynamic_component_get_field(component_id, field_name, data);
    if (field) {
        *(void **)field = value;
    }
}

void component_set_field_bool(uint64_t component_id, void *data, const char *field_name, bool value) {
    if (!data || !field_name) return;
    void *field = ecs_dynamic_component_get_field(component_id, field_name, data);
    if (field) {
        *(bool *)field = value;
    }
}

bool component_get_field_bool(uint64_t component_id, void *data, const char *field_name) {
    if (!data || !field_name) return false;
    void *field = ecs_dynamic_component_get_field(component_id, field_name, data);
    return field ? *(bool *)field : false;
}

int32_t component_get_field_int(uint64_t component_id, void *data, const char *field_name) {
    if (!data || !field_name) return 0;
    void *field = ecs_dynamic_component_get_field(component_id, field_name, data);
    return field ? *(int32_t *)field : 0;
}

float component_get_field_float(uint64_t component_id, void *data, const char *field_name) {
    if (!data || !field_name) return 0.0f;
    void *field = ecs_dynamic_component_get_field(component_id, field_name, data);
    return field ? *(float *)field : 0.0f;
}

void *component_get_field_ptr(uint64_t component_id, void *data, const char *field_name) {
    if (!data || !field_name) return NULL;
    void *field = ecs_dynamic_component_get_field(component_id, field_name, data);
    return field ? *(void **)field : NULL;
}

void component_api_register(void) {
    printf("[component_api] Component API initialized\n");
}
