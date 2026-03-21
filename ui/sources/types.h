
#pragma once

#include "enums.h"

typedef struct {
    ui_handle_t *windows[DEMO_WINDOW_COUNT];
    demo_tab_t active_tab;
    demo_theme_t theme;
    f32 slider_float;
    i32 slider_int;
    bool check_bool;
    i32 radio_selection;
    char text_input_buffer[256];
    i32 combo_selection;
    i32 selected_button;
    bool button_clicked;
    bool menu_open;
    f32 color_hue;
    f32 color_sat;
    f32 color_val;
} demo_ui_t;
