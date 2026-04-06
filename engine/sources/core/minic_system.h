#pragma once

#include <minic.h>

#define MAX_MINIC_SYSTEMS 16

typedef struct {
    char name[64];
    minic_ctx_t *ctx;
    void *step_fn;
    void *init_fn;
    void *draw_fn;
    void *draw_ui_fn;
    void *draw_ui_ext_fn;
} minic_system_t;

int minic_system_load(const char *name, const char *path);
int minic_system_load_manifest(const char *manifest_path);
void minic_system_unload_all(void);
void minic_system_call_step(void);
void minic_system_call_init(void);
void minic_system_call_draw(void);
void minic_system_call_draw_ui(void);
void minic_system_call_draw_ui_ext(void);
minic_system_t *minic_system_get(int index);
int minic_system_count(void);
