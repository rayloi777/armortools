// Iron engine HDLL bridge layer.
// Uses DEFINE_PRIM macros — can be compiled into host (static) or as standalone .hdll.
//
// For static registration: host calls iron_register_all_prims() before hl_module_load.
// For HDLL: compile this file as a shared library named iron.hdll.

#include <hl.h>
#include <iron.h>

// Override HL_NAME to use "iron_" prefix for all bridge functions.
#undef HL_NAME
#define HL_NAME(n) iron_##n

// ============================================================
// System functions
// ============================================================

HL_PRIM i32 HL_NAME(system_get_width)(void) {
	return sys_w();
}
DEFINE_PRIM(_I32, system_get_width, _NO_ARG);

HL_PRIM i32 HL_NAME(system_get_height)(void) {
	return sys_h();
}
DEFINE_PRIM(_I32, system_get_height, _NO_ARG);

HL_PRIM double HL_NAME(system_get_time)(void) {
	return iron_time();
}
DEFINE_PRIM(_F64, system_get_time, _NO_ARG);

// ============================================================
// GPU functions
// ============================================================

// Simplified gpu_begin — clears to a solid color with depth clear.
// Wraps _gpu_begin(NULL, NULL, NULL, flags, color, depth) for MVP.
HL_PRIM void HL_NAME(gpu_begin_default)(i32 color, f32 depth) {
	_gpu_begin(NULL, NULL, NULL, GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH,
	    (unsigned)color, depth);
}
DEFINE_PRIM(_VOID, gpu_begin_default, _I32 _F32);

HL_PRIM void HL_NAME(gpu_end)(void) {
	gpu_end();
}
DEFINE_PRIM(_VOID, gpu_end, _NO_ARG);

// ============================================================
// Input functions
// ============================================================

HL_PRIM bool HL_NAME(keyboard_down)(vbyte *key) {
	return keyboard_down((char *)key);
}
DEFINE_PRIM(_BOOL, keyboard_down, _BYTES);

HL_PRIM bool HL_NAME(keyboard_started)(vbyte *key) {
	return keyboard_started((char *)key);
}
DEFINE_PRIM(_BOOL, keyboard_started, _BYTES);

HL_PRIM bool HL_NAME(keyboard_released)(vbyte *key) {
	return keyboard_released((char *)key);
}
DEFINE_PRIM(_BOOL, keyboard_released, _BYTES);

HL_PRIM bool HL_NAME(mouse_down)(vbyte *button) {
	return mouse_down((char *)button);
}
DEFINE_PRIM(_BOOL, mouse_down, _BYTES);

HL_PRIM bool HL_NAME(mouse_started)(vbyte *button) {
	return mouse_started((char *)button);
}
DEFINE_PRIM(_BOOL, mouse_started, _BYTES);

HL_PRIM f32 HL_NAME(mouse_get_x)(void) {
	return mouse_x;
}
DEFINE_PRIM(_F32, mouse_get_x, _NO_ARG);

HL_PRIM f32 HL_NAME(mouse_get_y)(void) {
	return mouse_y;
}
DEFINE_PRIM(_F32, mouse_get_y, _NO_ARG);

// ============================================================
// Callback registration — called from Haxe to register closures
// ============================================================

// Forward declarations (implemented in host/main.c)
extern void iron_bridge_set_render_closure(vclosure *cb);
extern void iron_bridge_set_update_closure(vclosure *cb);

HL_PRIM void HL_NAME(set_render_callback)(vclosure *cb) {
	iron_bridge_set_render_closure(cb);
}
DEFINE_PRIM(_VOID, set_render_callback, _FUN(_VOID, _NO_ARG));

HL_PRIM void HL_NAME(set_update_callback)(vclosure *cb) {
	iron_bridge_set_update_closure(cb);
}
DEFINE_PRIM(_VOID, set_update_callback, _FUN(_VOID, _NO_ARG));

// ============================================================
// Static registration — called by host before hl_module_load
// ============================================================

extern void *iron_hlp_gpu_begin_default();
extern void *iron_hlp_gpu_end();
extern void *iron_hlp_system_get_width();
extern void *iron_hlp_system_get_height();
extern void *iron_hlp_system_get_time();
extern void *iron_hlp_keyboard_down();
extern void *iron_hlp_keyboard_started();
extern void *iron_hlp_keyboard_released();
extern void *iron_hlp_mouse_down();
extern void *iron_hlp_mouse_started();
extern void *iron_hlp_mouse_get_x();
extern void *iron_hlp_mouse_get_y();
extern void *iron_hlp_set_render_callback();
extern void *iron_hlp_set_update_callback();

void iron_register_all_prims(void) {
	// GPU
	hl_register_prim("gpu_begin_default@2",        (void *)iron_hlp_gpu_begin_default);
	hl_register_prim("gpu_end@0",                   (void *)iron_hlp_gpu_end);
	// System
	hl_register_prim("system_get_width@0",          (void *)iron_hlp_system_get_width);
	hl_register_prim("system_get_height@0",         (void *)iron_hlp_system_get_height);
	hl_register_prim("system_get_time@0",           (void *)iron_hlp_system_get_time);
	// Input
	hl_register_prim("keyboard_down@1",             (void *)iron_hlp_keyboard_down);
	hl_register_prim("keyboard_started@1",          (void *)iron_hlp_keyboard_started);
	hl_register_prim("keyboard_released@1",         (void *)iron_hlp_keyboard_released);
	hl_register_prim("mouse_down@1",                (void *)iron_hlp_mouse_down);
	hl_register_prim("mouse_started@1",             (void *)iron_hlp_mouse_started);
	hl_register_prim("mouse_get_x@0",              (void *)iron_hlp_mouse_get_x);
	hl_register_prim("mouse_get_y@0",              (void *)iron_hlp_mouse_get_y);
	// Callbacks
	hl_register_prim("set_render_callback@1",       (void *)iron_hlp_set_render_callback);
	hl_register_prim("set_update_callback@1",       (void *)iron_hlp_set_update_callback);
}
