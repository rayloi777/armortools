
#include "iron_wren.h"
#include "iron_system.h"
#include <string.h>

#ifdef WITH_WREN

extern WrenVM *wren_get_vm(void);
extern void wren_init(void);
extern void wren_free(void);

float wren_eval(const char *code) {
    WrenVM *vm = wren_get_vm();
    if (vm == NULL) {
        wren_init();
        vm = wren_get_vm();
    }

    WrenInterpretResult result = wrenInterpret(vm, "main", code);

    if (result == WREN_RESULT_COMPILE_ERROR) {
        iron_log("[Wren] Compile error");
        return 0.0f;
    } else if (result == WREN_RESULT_RUNTIME_ERROR) {
        iron_log("[Wren] Runtime error");
        return 0.0f;
    }

    return 0.0f;
}

char *wren_call(void *fn_handle) {
    (void)fn_handle;
    return NULL;
}

char *wren_call_ptr(void *fn_handle, void *arg) {
    (void)fn_handle;
    (void)arg;
    return NULL;
}

void *wren_pcall_str(void *fn_handle, char *arg) {
    (void)fn_handle;
    (void)arg;
    return NULL;
}

char *wren_call_ptr_str(void *fn_handle, void *arg0, char *arg1) {
    (void)fn_handle;
    (void)arg0;
    (void)arg1;
    return NULL;
}

void wren_set_load_module_fn(WrenLoadModuleFn fn) {
    (void)fn;
}

#endif
