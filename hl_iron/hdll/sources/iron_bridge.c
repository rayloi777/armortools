// Iron engine HDLL bridge layer.
// Compiled as a shared library (iron.hdll).
// HashLink loads this HDLL when Haxe code calls @:hlNative("iron") functions.
// Iron engine functions are resolved at load time from the host executable.

#include <hl.h>
#include <iron.h>

// Override HL_NAME to use "iron_" prefix for all bridge functions.
#undef HL_NAME
#define HL_NAME(n) hliron_##n

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

HL_PRIM void HL_NAME(system_exit)(void) {
	exit(0);
}
DEFINE_PRIM(_VOID, system_exit, _NO_ARG);

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
// Callback registration — called from Haxe to store closures in host
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
