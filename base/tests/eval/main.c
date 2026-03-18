// Eval test - QuickJS
// ../../make --run

#include <stdio.h>
#include <iron.h>
#include <iron_eval.h>

char *js_code = NULL;

void console_trace(char *s) {
	fprintf(stderr, "%s\n", s);
}

void render() {
	_gpu_begin(NULL, NULL, NULL, GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH, 0xff1a1a2e, 1.0);

	float result = js_eval(js_code);
	iron_log("JS result: %f", result);

	gpu_end();
}

void _kickstart() {
	iron_window_options_t *ops =
	    GC_ALLOC_INIT(iron_window_options_t, {.title     = "Eval Test",
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

	iron_file_reader_t reader;
	if (iron_file_reader_open(&reader, "data/a.js", IRON_FILE_TYPE_ASSET)) {
		size_t size = iron_file_reader_size(&reader);
		js_code = malloc(size + 1);
		iron_file_reader_read(&reader, js_code, size);
		js_code[size] = '\0';
		iron_file_reader_close(&reader);
		iron_log("Loaded JS: %s", js_code);
	} else {
		iron_log("Failed to load data/a.js");
	}

	js_init();

	_iron_set_update_callback(render);

	iron_start();
}

//

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
