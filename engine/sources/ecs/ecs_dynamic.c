#include "ecs_dynamic.h"
#include "ecs_world.h"
#include "flecs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_DYNAMIC_COMPONENTS 256

static dynamic_component_t g_components[MAX_DYNAMIC_COMPONENTS];
static int g_component_count = 0;

void ecs_dynamic_init(void) {
    g_component_count = 0;
    memset(g_components, 0, sizeof(g_components));
}

void ecs_dynamic_shutdown(void) {
    for (int i = 0; i < g_component_count; i++) {
        if (g_components[i].type_info) {
            free(g_components[i].type_info);
            g_components[i].type_info = NULL;
        }
    }
    g_component_count = 0;
}

uint64_t ecs_dynamic_component_create(struct game_world_t *world, const char *name, size_t size, size_t alignment) {
    if (!world || !world->world || !name || g_component_count >= MAX_DYNAMIC_COMPONENTS) {
        return 0;
    }
    
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    
    uint64_t existing = ecs_lookup(ecs, name);
    if (existing != 0) {
        return existing;
    }
    
    void *type_info = NULL;
    if (size > 0) {
        type_info = calloc(1, size);
        if (!type_info) return 0;
    }
    
    ecs_component_desc_t desc = {0};
    desc.entity = 0;
    desc.type.name = name;
    desc.type.size = (ecs_size_t)size;
    desc.type.alignment = (ecs_size_t)(alignment > 0 ? alignment : 8);
    ecs_entity_t comp_id = ecs_component_init(ecs, &desc);
    
    if (comp_id == 0) {
        if (type_info) free(type_info);
        return 0;
    }
    
    dynamic_component_t *dc = &g_components[g_component_count];
    strncpy(dc->name, name, sizeof(dc->name) - 1);
    dc->name[sizeof(dc->name) - 1] = '\0';
    dc->flecs_id = comp_id;
    dc->size = size;
    dc->alignment = alignment;
    dc->field_count = 0;
    dc->type_info = type_info;
    
    g_component_count++;
    
    return comp_id;
}

int ecs_dynamic_component_add_field(uint64_t component_id, const char *name, int type, size_t offset) {
    for (int i = 0; i < g_component_count; i++) {
        if (g_components[i].flecs_id == component_id) {
            dynamic_component_t *dc = &g_components[i];
            if (dc->field_count >= 16) return -1;
            
            dynamic_field_t *field = &dc->fields[dc->field_count];
            strncpy(field->name, name, sizeof(field->name) - 1);
            field->name[sizeof(field->name) - 1] = '\0';
            field->type = type;
            field->offset = offset;
            field->count = 1;
            
            dc->field_count++;
            return 0;
        }
    }
    return -1;
}

dynamic_component_t *ecs_dynamic_component_get(uint64_t component_id) {
    for (int i = 0; i < g_component_count; i++) {
        if (g_components[i].flecs_id == component_id) {
            return &g_components[i];
        }
    }
    return NULL;
}

dynamic_component_t *ecs_dynamic_component_find(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < g_component_count; i++) {
        if (strcmp(g_components[i].name, name) == 0) {
            return &g_components[i];
        }
    }
    return NULL;
}

void ecs_dynamic_component_set_data(struct game_world_t *world, uint64_t entity, uint64_t component_id, const char *field_name, const void *value) {
    if (!world || !world->world || entity == 0 || !field_name || !value) return;
    
    dynamic_component_t *dc = ecs_dynamic_component_get(component_id);
    if (!dc) return;
    
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    void *comp_data = ecs_get_mut_id(ecs, (ecs_entity_t)entity, (ecs_id_t)component_id);
    if (!comp_data) return;
    
    for (int i = 0; i < dc->field_count; i++) {
        if (strcmp(dc->fields[i].name, field_name) == 0) {
            size_t field_size = 0;
            switch (dc->fields[i].type) {
                case DYNAMIC_TYPE_INT:
                case DYNAMIC_TYPE_FLOAT:
                case DYNAMIC_TYPE_BOOL:
                    field_size = 4;
                    break;
                case DYNAMIC_TYPE_PTR:
                case DYNAMIC_TYPE_STRING:
                    field_size = sizeof(void*);
                    break;
            }
            memcpy((char*)comp_data + dc->fields[i].offset, value, field_size);
            break;
        }
    }
}

void *ecs_dynamic_component_get_field(uint64_t component_id, const char *field_name, void *component_data) {
    if (!component_data || !field_name) return NULL;
    
    dynamic_component_t *dc = ecs_dynamic_component_get(component_id);
    if (!dc) return NULL;
    
    for (int i = 0; i < dc->field_count; i++) {
        if (strcmp(dc->fields[i].name, field_name) == 0) {
            return (char*)component_data + dc->fields[i].offset;
        }
    }
    return NULL;
}

int ecs_dynamic_component_count(void) {
    return g_component_count;
}

dynamic_component_t *ecs_dynamic_component_at(int index) {
    if (index < 0 || index >= g_component_count) return NULL;
    return &g_components[index];
}
