#pragma once

#ifdef WITH_WREN

#include "libs/wren/include/wren.h"

void   wren_init();
void   wren_free();
float  wren_eval(const char *code);
void  *wren_make_call_handle(const char *signature);
char  *wren_call(void *fn_handle);
char  *wren_call_ptr(void *fn_handle, void *arg);
void  *wren_pcall_str(void *fn_handle, char *arg);
char  *wren_call_ptr_str(void *fn_handle, void *arg0, char *arg1);
void   wren_set_load_module_fn(WrenLoadModuleFn fn);

WrenVM *wren_get_vm();

#else

#define NULL ((void*)0)

void wren_init() {}
void wren_free() {}
float wren_eval(const char *code) { (void)code; return 0.0f; }
void *wren_make_call_handle(const char *signature) { (void)signature; return NULL; }
char *wren_call(void *fn_handle) { (void)fn_handle; return (char*)NULL; }
char *wren_call_ptr(void *fn_handle, void *arg) { (void)fn_handle; (void)arg; return (char*)NULL; }
void *wren_pcall_str(void *fn_handle, char *arg) { (void)fn_handle; (void)arg; return NULL; }
char *wren_call_ptr_str(void *fn_handle, void *arg0, char *arg1) { (void)fn_handle; (void)arg0; (void)arg1; return (char*)NULL; }
void wren_set_load_module_fn(void *fn) { (void)fn; }
void *wren_get_vm() { return NULL; }

#endif
