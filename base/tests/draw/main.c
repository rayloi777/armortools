// Draw shapes test
// ../../make --run

#include <iron.h>

f32 rotation = 0.0f;
draw_font_t *font;
gpu_texture_t *image;

void render() {
    rotation += 0.01f;

    draw_begin(NULL, true, 0xff1a1a2e);

    f32 w = (f32)iron_window_width();
    f32 h = (f32)iron_window_height();

    draw_set_color(0xffe94560);
    draw_filled_rect(50, 50, 150, 100);

    draw_set_color(0xff0f3460);
    draw_rect(220, 50, 150, 100, 3.0f);

    draw_set_color(0xff16c79a);
    draw_line(50, 200, 200, 200, 2.0f);

    draw_set_color(0xffffe194);
    draw_line_aa(220, 200, 370, 200, 2.0f);

    draw_set_color(0xffea5455);
    draw_filled_rect(50, 250, 2.0f, 100);

    draw_set_color(0xff00d9ff);
    draw_filled_circle(120, 400, 50, 32);

    draw_set_color(0xffff6b6b);
    draw_circle(250, 400, 50, 32, 3.0f);

    draw_set_color(0xff4ecca3);
    draw_line_aa(350, 300, 450, 450, 4.0f);
    draw_line_aa(450, 450, 550, 350, 4.0f);
    draw_line_aa(550, 350, 350, 300, 4.0f);

    f32 cx = w / 2.0f + cosf(rotation) * 100.0f;
    f32 cy = h / 2.0f + sinf(rotation) * 50.0f;
    draw_set_color(0xfffff01f);
    draw_filled_circle(cx, cy, 20, 16);

    draw_set_font(font, 32);
    draw_set_color(0xffffffff);
    draw_string("Hello Draw!", 400, 50);

    draw_set_font(font, 16);
    draw_set_color(0xffaaaaaa);
    draw_string("Shapes: rect, line, circle", 400, 90);

    draw_set_color(0xffffffff);
    draw_string("draw_image:", 50, 480);
    if (image != NULL) draw_image(image, 50, 500);

    draw_string("draw_scaled_image:", 200, 480);
    if (image != NULL) draw_scaled_image(image, 200, 500, 64, 64);

    draw_string("draw_sub_image:", 300, 480);
    if (image != NULL) draw_sub_image(image, 0, 0, 128, 128, 300, 500);

    draw_string("draw_scaled_sub_image:", 450, 480);
    if (image != NULL) draw_scaled_sub_image(image, 0, 0, 64, 64, 450, 500, 128, 128);

    draw_end();
}

void _kickstart() {
    iron_window_options_t *ops =
        GC_ALLOC_INIT(iron_window_options_t, {.title     = "Draw Test",
                                              .width     = 640,
                                              .height    = 680,
                                              .x         = -1,
                                              .y         = -1,
                                              .mode      = IRON_WINDOW_MODE_WINDOW,
                                              .features  = IRON_WINDOW_FEATURES_RESIZABLE | IRON_WINDOW_FEATURES_MINIMIZABLE | IRON_WINDOW_FEATURES_MAXIMIZABLE,
                                              .vsync     = true,
                                              .frequency = 60,
                                              .depth_bits = 0});

    sys_start(ops);

    font = data_get_font("font.ttf");
    draw_font_init(font);

    image = data_get_image("color_wheel.k");
    if (image == NULL) {
        iron_log("Failed to load color_wheel image");
    }

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
