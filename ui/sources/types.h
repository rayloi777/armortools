
#pragma once

#include "enums.h"

typedef struct {
    ui_handle_t *windows[DEMO_WINDOW_COUNT];
    demo_tab_t active_tab;
    demo_theme_t theme;

    bool check_enabled;
    bool check_visible;
    bool check_bool;
    i32 radio_selection;
    bool panel_expanded;

    f32 slider_float;
    f32 float_value;
    char text_input_buffer[256];
    char text_area_buffer[512];
    i32 combo_selection;
    i32 inline_radio_selection;

    f32 color_hue;
    f32 color_sat;
    f32 color_val;
    uint32_t selected_color;

    gpu_texture_t *demo_image;
} demo_ui_t;
