#include "iron_wren.h"
#include "iron_system.h"
#include "iron_input.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#ifdef WITH_WREN

static WrenVM *wren_vm = NULL;

WrenVM *wren_get_vm(void) {
    return wren_vm;
}

// Iron.print(text)
static void wren_iron_print(WrenVM *vm) {
    WrenType type = wrenGetSlotType(vm, 1);
    if (type == WREN_TYPE_STRING) {
        const char *text = wrenGetSlotString(vm, 1);
        iron_log("%s", text);
    } else if (type == WREN_TYPE_NUM) {
        double num = wrenGetSlotDouble(vm, 1);
        iron_log("%f", num);
    } else {
        iron_log("(unknown type)");
    }
}

// Iron.log(text)
static void wren_iron_log(WrenVM *vm) {
    const char *text = wrenGetSlotString(vm, 1);
    iron_log("%s", text);
}

// Math functions
static void wren_sin(WrenVM *vm) {
    double x = wrenGetSlotDouble(vm, 1);
    wrenSetSlotDouble(vm, 0, sin(x));
}

static void wren_cos(WrenVM *vm) {
    double x = wrenGetSlotDouble(vm, 1);
    wrenSetSlotDouble(vm, 0, cos(x));
}

static void wren_sqrt(WrenVM *vm) {
    double x = wrenGetSlotDouble(vm, 1);
    wrenSetSlotDouble(vm, 0, sqrt(x));
}

static void wren_abs(WrenVM *vm) {
    double x = wrenGetSlotDouble(vm, 1);
    wrenSetSlotDouble(vm, 0, fabs(x));
}

static void wren_floor(WrenVM *vm) {
    double x = wrenGetSlotDouble(vm, 1);
    wrenSetSlotDouble(vm, 0, floor(x));
}

static void wren_ceil(WrenVM *vm) {
    double x = wrenGetSlotDouble(vm, 1);
    wrenSetSlotDouble(vm, 0, ceil(x));
}

static void wren_round(WrenVM *vm) {
    double x = wrenGetSlotDouble(vm, 1);
    wrenSetSlotDouble(vm, 0, round(x));
}

static void wren_min(WrenVM *vm) {
    double a = wrenGetSlotDouble(vm, 1);
    double b = wrenGetSlotDouble(vm, 2);
    wrenSetSlotDouble(vm, 0, (a < b) ? a : b);
}

static void wren_max(WrenVM *vm) {
    double a = wrenGetSlotDouble(vm, 1);
    double b = wrenGetSlotDouble(vm, 2);
    wrenSetSlotDouble(vm, 0, (a > b) ? a : b);
}

static void wren_pow(WrenVM *vm) {
    double x = wrenGetSlotDouble(vm, 1);
    double y = wrenGetSlotDouble(vm, 2);
    wrenSetSlotDouble(vm, 0, pow(x, y));
}

static void wren_log_math(WrenVM *vm) {
    double x = wrenGetSlotDouble(vm, 1);
    wrenSetSlotDouble(vm, 0, log(x));
}

static void wren_exp(WrenVM *vm) {
    double x = wrenGetSlotDouble(vm, 1);
    wrenSetSlotDouble(vm, 0, exp(x));
}

static void wren_random(WrenVM *vm) {
    double r = (double)rand() / RAND_MAX;
    wrenSetSlotDouble(vm, 0, r);
}

// Mouse
static void wren_mouse_x(WrenVM *vm) {
    (void)vm;
    wrenSetSlotDouble(vm, 0, mouse_x);
}

static void wren_mouse_y(WrenVM *vm) {
    (void)vm;
    wrenSetSlotDouble(vm, 0, mouse_y);
}

static void wren_mouse_last_x(WrenVM *vm) {
    (void)vm;
    wrenSetSlotDouble(vm, 0, mouse_last_x);
}

static void wren_mouse_last_y(WrenVM *vm) {
    (void)vm;
    wrenSetSlotDouble(vm, 0, mouse_last_y);
}

static void wren_mouse_moved(WrenVM *vm) {
    (void)vm;
    wrenSetSlotBool(vm, 0, mouse_moved);
}

static void wren_mouse_movement_x(WrenVM *vm) {
    (void)vm;
    wrenSetSlotDouble(vm, 0, mouse_movement_x);
}

static void wren_mouse_movement_y(WrenVM *vm) {
    (void)vm;
    wrenSetSlotDouble(vm, 0, mouse_movement_y);
}

static void wren_mouse_wheel_delta(WrenVM *vm) {
    (void)vm;
    wrenSetSlotDouble(vm, 0, mouse_wheel_delta);
}

static void wren_mouse_down(WrenVM *vm) {
    const char *button = wrenGetSlotString(vm, 1);
    wrenSetSlotBool(vm, 0, mouse_down(button));
}

static void wren_mouse_started(WrenVM *vm) {
    const char *button = wrenGetSlotString(vm, 1);
    wrenSetSlotBool(vm, 0, mouse_started(button));
}

static void wren_mouse_released(WrenVM *vm) {
    const char *button = wrenGetSlotString(vm, 1);
    wrenSetSlotBool(vm, 0, mouse_released(button));
}

static void wren_mouse_down_any(WrenVM *vm) {
    (void)vm;
    wrenSetSlotBool(vm, 0, mouse_down_any());
}

static void wren_mouse_view_x(WrenVM *vm) {
    (void)vm;
    wrenSetSlotDouble(vm, 0, mouse_view_x());
}

static void wren_mouse_view_y(WrenVM *vm) {
    (void)vm;
    wrenSetSlotDouble(vm, 0, mouse_view_y());
}

// Keyboard
static void wren_keyboard_down(WrenVM *vm) {
    const char *key = wrenGetSlotString(vm, 1);
    wrenSetSlotBool(vm, 0, keyboard_down(key));
}

static void wren_keyboard_started(WrenVM *vm) {
    const char *key = wrenGetSlotString(vm, 1);
    wrenSetSlotBool(vm, 0, keyboard_started(key));
}

static void wren_keyboard_released(WrenVM *vm) {
    const char *key = wrenGetSlotString(vm, 1);
    wrenSetSlotBool(vm, 0, keyboard_released(key));
}

static void wren_keyboard_repeat(WrenVM *vm) {
    const char *key = wrenGetSlotString(vm, 1);
    wrenSetSlotBool(vm, 0, keyboard_repeat(key));
}

static void wren_keyboard_started_any(WrenVM *vm) {
    (void)vm;
    wrenSetSlotBool(vm, 0, keyboard_started_any());
}

// Foreign method binding
WrenForeignMethodFn wren_bind_foreign_method(WrenVM *vm, const char *module, const char *className, bool isStatic, const char *signature) {
    (void)vm;
    (void)module;
    
    if (strcmp(className, "Iron") == 0) {
        if (isStatic && strcmp(signature, "print(_)") == 0) return wren_iron_print;
        if (isStatic && strcmp(signature, "log(_)") == 0) return wren_iron_log;
    }
    
    if (strcmp(className, "Math") == 0) {
        if (isStatic && strcmp(signature, "sin(_)") == 0) return wren_sin;
        if (isStatic && strcmp(signature, "cos(_)") == 0) return wren_cos;
        if (isStatic && strcmp(signature, "sqrt(_)") == 0) return wren_sqrt;
        if (isStatic && strcmp(signature, "abs(_)") == 0) return wren_abs;
        if (isStatic && strcmp(signature, "floor(_)") == 0) return wren_floor;
        if (isStatic && strcmp(signature, "ceil(_)") == 0) return wren_ceil;
        if (isStatic && strcmp(signature, "round(_)") == 0) return wren_round;
        if (isStatic && strcmp(signature, "min(_,_)") == 0) return wren_min;
        if (isStatic && strcmp(signature, "max(_,_)") == 0) return wren_max;
        if (isStatic && strcmp(signature, "pow(_,_)") == 0) return wren_pow;
        if (isStatic && strcmp(signature, "log(_)") == 0) return wren_log_math;
        if (isStatic && strcmp(signature, "exp(_)") == 0) return wren_exp;
        if (isStatic && strcmp(signature, "random()") == 0) return wren_random;
    }

    if (strcmp(className, "Mouse") == 0) {
        if (isStatic && strcmp(signature, "x") == 0) return wren_mouse_x;
        if (isStatic && strcmp(signature, "y") == 0) return wren_mouse_y;
        if (isStatic && strcmp(signature, "last_x") == 0) return wren_mouse_last_x;
        if (isStatic && strcmp(signature, "last_y") == 0) return wren_mouse_last_y;
        if (isStatic && strcmp(signature, "moved") == 0) return wren_mouse_moved;
        if (isStatic && strcmp(signature, "movement_x") == 0) return wren_mouse_movement_x;
        if (isStatic && strcmp(signature, "movement_y") == 0) return wren_mouse_movement_y;
        if (isStatic && strcmp(signature, "wheel_delta") == 0) return wren_mouse_wheel_delta;
        if (isStatic && strcmp(signature, "down(_)") == 0) return wren_mouse_down;
        if (isStatic && strcmp(signature, "started(_)") == 0) return wren_mouse_started;
        if (isStatic && strcmp(signature, "released(_)") == 0) return wren_mouse_released;
        if (isStatic && strcmp(signature, "down_any()") == 0) return wren_mouse_down_any;
        if (isStatic && strcmp(signature, "view_x") == 0) return wren_mouse_view_x;
        if (isStatic && strcmp(signature, "view_y") == 0) return wren_mouse_view_y;
    }

    if (strcmp(className, "Keyboard") == 0) {
        if (isStatic && strcmp(signature, "down(_)") == 0) return wren_keyboard_down;
        if (isStatic && strcmp(signature, "started(_)") == 0) return wren_keyboard_started;
        if (isStatic && strcmp(signature, "released(_)") == 0) return wren_keyboard_released;
        if (isStatic && strcmp(signature, "repeat(_)") == 0) return wren_keyboard_repeat;
        if (isStatic && strcmp(signature, "started_any()") == 0) return wren_keyboard_started_any;
    }
    
    return NULL;
}

// Built-in Wren module for Iron classes
static const char *wren_iron_module = 
"class Iron {\n"
"  foreign static print(text)\n"
"  foreign static log(text)\n"
"}\n"
"\n"
"class Math {\n"
"  static PI { 3.141592653589793 }\n"
"  static E { 2.718281828459045 }\n"
"  foreign static sin(x)\n"
"  foreign static cos(x)\n"
"  foreign static sqrt(x)\n"
"  foreign static abs(x)\n"
"  foreign static floor(x)\n"
"  foreign static ceil(x)\n"
"  foreign static round(x)\n"
"  foreign static min(a, b)\n"
"  foreign static max(a, b)\n"
"  foreign static pow(x, y)\n"
"  foreign static log(x)\n"
"  foreign static exp(x)\n"
"  foreign static random()\n"
"}\n"
"\n"
"class Mouse {\n"
"  foreign static x\n"
"  foreign static y\n"
"  foreign static last_x\n"
"  foreign static last_y\n"
"  foreign static moved\n"
"  foreign static movement_x\n"
"  foreign static movement_y\n"
"  foreign static wheel_delta\n"
"  foreign static down(button)\n"
"  foreign static started(button)\n"
"  foreign static released(button)\n"
"  foreign static down_any()\n"
"  foreign static view_x\n"
"  foreign static view_y\n"
"}\n"
"\n"
"class Keyboard {\n"
"  foreign static down(key)\n"
"  foreign static started(key)\n"
"  foreign static released(key)\n"
"  foreign static repeat(key)\n"
"  foreign static started_any()\n"
"}\n";

// Initialize Wren VM
void wren_init(void) {
    if (wren_vm != NULL) {
        return;
    }

    WrenConfiguration config;
    wrenInitConfiguration(&config);
    config.writeFn = NULL;
    config.errorFn = NULL;
    config.bindForeignMethodFn = wren_bind_foreign_method;

    wren_vm = wrenNewVM(&config);

    WrenInterpretResult result = wrenInterpret(wren_vm, "main", wren_iron_module);
    if (result != WREN_RESULT_SUCCESS) {
        iron_log("[Wren] Failed to load Iron module");
    }

    iron_log("Wren VM initialized");
}

// Make call handle for repeated calls
void *wren_make_call_handle_impl(const char *signature) {
    WrenVM *vm = wren_get_vm();
    if (vm == NULL) {
        wren_init();
        vm = wren_get_vm();
    }
    return (void *)wrenMakeCallHandle(vm, signature);
}

// Call a pre-compiled function
char *wren_call_impl(void *fn_handle) {
    WrenVM *vm = wren_get_vm();
    if (vm == NULL || fn_handle == NULL) {
        return NULL;
    }

    WrenInterpretResult result = wrenCall(vm, (WrenHandle *)fn_handle);

    if (result == WREN_RESULT_COMPILE_ERROR) {
        iron_log("[Wren] Compile error in call");
        return NULL;
    } else if (result == WREN_RESULT_RUNTIME_ERROR) {
        iron_log("[Wren] Runtime error in call");
        return NULL;
    }

    return NULL;
}

// Free Wren VM
void wren_free(void) {
    if (wren_vm != NULL) {
        wrenFreeVM(wren_vm);
        wren_vm = NULL;
        iron_log("Wren VM freed");
    }
}

#endif
