
#include "global.h"

static char_ptr_array_t combo_options;

void demo_ui_update(void *_) {
}

void demo_ui_render(void *_) {
    ui_begin(ui);

    if (ui_window(demo_ui->windows[DEMO_WINDOW_MAIN], 10, 10, 400, 700, true)) {
        ui->_y += 5;

        ui_text("Iron Engine UI Demo", UI_ALIGN_LEFT, 0x00000000);
        ui_end_element();
        ui_separator(0, true);
        ui_end_element();

        ui_text("Buttons", UI_ALIGN_LEFT, 0x00000000);
        ui_end_element();

        if (ui_button("Button 1", UI_ALIGN_CENTER, "")) {
        }
        ui_end_element();

        if (ui_button("Button 2", UI_ALIGN_CENTER, "")) {
        }
        ui_end_element();

        ui_separator(0, true);
        ui_end_element();

        ui_text("Checkbox", UI_ALIGN_LEFT, 0x00000000);
        ui_end_element();

        ui_handle_t *h_check = ui_handle(__ID__);
        h_check->b = demo_ui->check_bool;
        demo_ui->check_bool = ui_check(h_check, "Enable Option", "");
        ui_end_element();

        ui_separator(0, true);
        ui_end_element();

        ui_text("Slider", UI_ALIGN_LEFT, 0x00000000);
        ui_end_element();

        ui_handle_t *h_slider = ui_handle(__ID__);
        h_slider->f = demo_ui->slider_float;
        demo_ui->slider_float = ui_slider(h_slider, "Value", 0.0, 1.0, true, 100.0, true, UI_ALIGN_RIGHT, true);
        ui_end_element();

        ui_separator(0, true);
        ui_end_element();

        ui_text("Text Input", UI_ALIGN_LEFT, 0x00000000);
        ui_end_element();

        ui_handle_t *h_text = ui_handle(__ID__);
        h_text->text = demo_ui->text_input_buffer;
        ui_text_input(h_text, "", UI_ALIGN_LEFT, true, false);
        ui_end_element();

        ui_separator(0, true);
        ui_end_element();

        ui_text("Combo Box", UI_ALIGN_LEFT, 0x00000000);
        ui_end_element();

        ui_handle_t *h_combo = ui_handle(__ID__);
        demo_ui->combo_selection = ui_combo(h_combo, &combo_options, "Select", true, UI_ALIGN_LEFT, false);
        ui_end_element();
    }

    ui_end();
}

void demo_ui_init(void) {
    char_ptr_array_push(&combo_options, "Option 1");
    char_ptr_array_push(&combo_options, "Option 2");
    char_ptr_array_push(&combo_options, "Option 3");
}
