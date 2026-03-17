#include "iron_wren.h"
#include "iron_system.h"
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
    const char *text = wrenGetSlotString(vm, 1);
    iron_log("%s", text);
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

// Free Wren VM
void wren_free(void) {
    if (wren_vm != NULL) {
        wrenFreeVM(wren_vm);
        wren_vm = NULL;
        iron_log("Wren VM freed");
    }
}

#endif
