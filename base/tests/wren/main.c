// Wren test
// ../../make --run

#include <iron.h>
#include <iron_wren.h>

void render() {
	_gpu_begin(NULL, NULL, NULL, GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH, 0xff1a1a2e, 1.0);
	gpu_end();
}

void _kickstart() {
	iron_window_options_t *ops =
	    GC_ALLOC_INIT(iron_window_options_t, {.title     = "Wren Test",
	                                          .width     = 640,
	                                          .height    = 480,
	                                          .x         = -1,
	                                          .y         = -1,
	                                          .mode      = IRON_WINDOW_MODE_WINDOW,
	                                          .features  = IRON_WINDOW_FEATURES_RESIZABLE | IRON_WINDOW_FEATURES_MINIMIZABLE | IRON_WINDOW_FEATURES_MAXIMIZABLE,
	                                          .vsync     = true,
	                                          .frequency = 60,
	                                          .depth_bits = 32});
	_iron_init(ops);

	// Test Wren
	wren_init();
	wren_eval("Iron.print(\"Hello from Wren!\")");
	wren_eval("Iron.print(\"Math.sin(0) = %(Math.sin(0))\")");
	wren_eval("Iron.print(\"Math.cos(0) = %(Math.cos(0))\")");
	wren_eval("Iron.print(\"Math.sqrt(16) = %(Math.sqrt(16))\")");
	wren_eval("Iron.print(\"Math.PI = %(Math.PI)\")");
	wren_free();

	_iron_set_update_callback(render);

	iron_start();
}

////

any_map_t *ui_children;
any_map_t *ui_nodes_custom_buttons;
i32        pipes_offset;
char      *strings_check_internet_connection() {
    return "";
}
void  console_error(char *s) {}
void  console_info(char *s) {}
char *tr(char *id, any_map_t *vars) {
	return id;
}
i32 pipes_get_constant_location(char *s) {
	return 0;
}
