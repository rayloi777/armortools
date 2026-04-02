#include "minic_system.h"
#include "game_loop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../base/sources/libs/minic.h"
#include "../../base/sources/iron_file.h"

static minic_system_t g_minic_systems[MAX_MINIC_SYSTEMS];
static int g_minic_system_count = 0;

static minic_system_t *find_system(const char *name) {
    for (int i = 0; i < g_minic_system_count; i++) {
        if (strcmp(g_minic_systems[i].name, name) == 0) {
            return &g_minic_systems[i];
        }
    }
    return NULL;
}

int minic_system_load(const char *name, const char *path) {
    if (!name || !path) {
        fprintf(stderr, "[minic_system] Error: name or path is NULL\n");
        return -1;
    }
    
    if (g_minic_system_count >= MAX_MINIC_SYSTEMS) {
        fprintf(stderr, "[minic_system] Error: max systems reached (%d)\n", MAX_MINIC_SYSTEMS);
        return -1;
    }
    
    if (find_system(name)) {
        fprintf(stderr, "[minic_system] Warning: system '%s' already loaded\n", name);
        return -1;
    }
    
    iron_file_reader_t reader;
    if (!iron_file_reader_open(&reader, path, IRON_FILE_TYPE_ASSET)) {
        fprintf(stderr, "[minic_system] Error: failed to open '%s'\n", path);
        return -1;
    }
    
    size_t size = iron_file_reader_size(&reader);
    char *source = malloc(size + 1);
    if (!source) {
        fprintf(stderr, "[minic_system] Error: failed to allocate memory\n");
        iron_file_reader_close(&reader);
        return -1;
    }
    
    iron_file_reader_read(&reader, source, size);
    source[size] = '\0';
    iron_file_reader_close(&reader);
    
    minic_ctx_t *ctx = minic_ctx_create(source);
    free(source);
    
    if (!ctx) {
        fprintf(stderr, "[minic_system] Error: failed to create minic context for '%s'\n", name);
        return -1;
    }
    
    minic_ctx_run(ctx);
    
    void *step_fn = minic_ctx_get_fn(ctx, "step");
    void *init_fn = minic_ctx_get_fn(ctx, "init");
    void *draw_fn = minic_ctx_get_fn(ctx, "draw");
    
    minic_system_t *sys = &g_minic_systems[g_minic_system_count];
    strncpy(sys->name, name, sizeof(sys->name) - 1);
    sys->name[sizeof(sys->name) - 1] = '\0';
    sys->ctx = ctx;
    sys->step_fn = step_fn;
    sys->init_fn = init_fn;
    sys->draw_fn = draw_fn;
    
    g_minic_system_count++;

    printf("[minic_system] Loaded '%s' from '%s' (step=%p, init=%p, draw=%p)\n",
           name, path, step_fn, init_fn, draw_fn);

    return 0;
}

void minic_system_unload_all(void) {
    for (int i = 0; i < g_minic_system_count; i++) {
        if (g_minic_systems[i].ctx) {
            minic_ctx_free(g_minic_systems[i].ctx);
            g_minic_systems[i].ctx = NULL;
        }
        g_minic_systems[i].step_fn = NULL;
        g_minic_systems[i].init_fn = NULL;
        g_minic_systems[i].draw_fn = NULL;
    }
    g_minic_system_count = 0;
}

void minic_system_call_step(void) {
    for (int i = 0; i < g_minic_system_count; i++) {
        if (g_minic_systems[i].step_fn) {
            minic_call_fn(g_minic_systems[i].step_fn, NULL, 0);
        }
    }
}

void minic_system_call_init(void) {
    for (int i = 0; i < g_minic_system_count; i++) {
        if (g_minic_systems[i].init_fn) {
            minic_call_fn(g_minic_systems[i].init_fn, NULL, 0);
        }
    }
}

void minic_system_call_draw(void) {
    for (int i = 0; i < g_minic_system_count; i++) {
        if (g_minic_systems[i].draw_fn) {
            minic_call_fn(g_minic_systems[i].draw_fn, NULL, 0);
        }
    }
}

minic_system_t *minic_system_get(int index) {
    if (index < 0 || index >= g_minic_system_count) {
        return NULL;
    }
    return &g_minic_systems[index];
}

int minic_system_count(void) {
    return g_minic_system_count;
}
