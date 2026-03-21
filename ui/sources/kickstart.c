
#include "global.h"

draw_font_t *demo_font;
ui_theme_t *demo_theme;
gpu_texture_t *demo_color_wheel;
gpu_texture_t *demo_color_wheel_gradient;

void _kickstart() {
    iron_window_options_t *ops = GC_ALLOC_INIT(iron_window_options_t, {
        .title     = "Iron UI Demo",
        .width     = 1024,
        .height    = 768,
        .x         = -1,
        .y         = -1,
        .mode      = IRON_WINDOW_MODE_WINDOW,
        .features  = IRON_WINDOW_FEATURES_RESIZABLE | IRON_WINDOW_FEATURES_MINIMIZABLE | IRON_WINDOW_FEATURES_MAXIMIZABLE,
        .vsync     = true,
        .frequency = 60,
        .depth_bits = 0
    });

    sys_start(ops);

    ui_children = any_map_create();
    gc_root(ui_children);

    demo_font = data_get_font("font.ttf");
    draw_font_init(demo_font);

    demo_color_wheel = data_get_image("color_wheel.k");
    demo_color_wheel_gradient = data_get_image("color_wheel_gradient.k");

    demo_theme = ui_theme_create();

    ui_options_t *ui_ops = GC_ALLOC_INIT(ui_options_t, {
        .theme = demo_theme,
        .font = demo_font,
        .scale_factor = 1.0f,
        .color_wheel = demo_color_wheel,
        .black_white_gradient = demo_color_wheel_gradient
    });
    ui = ui_create(ui_ops);
    gc_root(ui);

    demo_ui = GC_ALLOC_INIT(demo_ui_t, {0});
    gc_root(demo_ui);

    for (i32 i = 0; i < DEMO_WINDOW_COUNT; ++i) {
        demo_ui->windows[i] = ui_handle_create();
        gc_root(demo_ui->windows[i]);
    }

    demo_ui->slider_float = 0.5f;
    demo_ui->slider_int = 50;
    demo_ui->check_bool = true;
    demo_ui->radio_selection = 0;
    demo_ui->combo_selection = 0;
    demo_ui->active_tab = DEMO_TAB_WIDGETS;
    demo_ui->theme = THEME_DARK;

    strcpy(demo_ui->text_input_buffer, "Hello, UI!");

    demo_ui_init();

    sys_notify_on_update(demo_ui_update, NULL);
    sys_notify_on_render(demo_ui_render, NULL);

    iron_start();
}
