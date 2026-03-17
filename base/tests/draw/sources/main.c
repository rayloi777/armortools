// Draw string test
// ../../make --run

#include <stdio.h>
#include <iron.h>

draw_font_t *font;

void render(void *data) {
	draw_begin(NULL, true, 0xff202020);
	
	draw_set_color(0xffffffff);
	draw_set_font(font, 32);
	draw_string("Hello World!", 100, 100);
	draw_end();
}

void _kickstart() {
	iron_window_options_t *ops =
	    GC_ALLOC_INIT(iron_window_options_t, {.title     = "Draw Test",
	                                          .width     = 640,
	                                          .height    = 480,
	                                          .x         = -1,
	                                          .y         = -1,
	                                          .mode      = IRON_WINDOW_MODE_WINDOW,
	                                          .features  = IRON_WINDOW_FEATURES_RESIZABLE | IRON_WINDOW_FEATURES_MINIMIZABLE | IRON_WINDOW_FEATURES_MAXIMIZABLE,
	                                          .vsync     = true,
	                                          .frequency = 60,
	                                          .depth_bits = 32});
	sys_start(ops);

	font = data_get_font("font.ttf");
	draw_font_init(font);
	draw_set_font(font, 32);

	sys_notify_on_render(render, NULL);

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
