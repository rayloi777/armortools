#pragma once

#include <minic.h>

#define MAX_MINIC_SYSTEMS 16

typedef struct {
    char name[64];
    minic_ctx_t *ctx;
    void *update_fn;
    void *init_fn;
} minic_system_t;

int minic_system_load(const char *name, const char *path);
void minic_system_unload_all(void);
void minic_system_call_update(void);
void minic_system_call_init(void);
minic_system_t *minic_system_get(int index);
int minic_system_count(void);
