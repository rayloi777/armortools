
#include "global.h"
#include "ui_menu.h"
#include "ui_menubar.h"

static char_ptr_array_t *combo_options;
static char *tab_names[DEMO_TAB_COUNT] = {
    "中文測試",
    "Buttons",
    "Inputs",
    "Layout",
    "Display",
    "Color"
};

void render_tab_buttons(void) {
    ui_text("Buttons", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();

    if (ui_button("Click Me", UI_ALIGN_CENTER, "")) {
    }
    ui_end_element();

    if (ui_button("Disabled Button", UI_ALIGN_CENTER, "")) {
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

    ui_handle_t *h_check2 = ui_handle(__ID__);
    h_check2->b = demo_ui->check_visible;
    demo_ui->check_visible = ui_check(h_check2, "Visible", "");
    ui_end_element();

    ui_separator(0, true);
    ui_end_element();

    ui_text("Radio Buttons", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();

    ui_handle_t *h_radio = ui_handle(__ID__);
    h_radio->i = demo_ui->radio_selection;
    if (ui_radio(h_radio, 0, "Option A", "")) {
        demo_ui->radio_selection = 0;
    }
    ui_end_element();
    if (ui_radio(h_radio, 1, "Option B", "")) {
        demo_ui->radio_selection = 1;
    }
    ui_end_element();
    if (ui_radio(h_radio, 2, "Option C", "")) {
        demo_ui->radio_selection = 2;
    }
    ui_end_element();

    ui_separator(0, true);
    ui_end_element();

    ui_text("Panel (Collapsible)", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();

    ui_handle_t *h_panel = ui_handle(__ID__);
    h_panel->b = demo_ui->panel_expanded;
    demo_ui->panel_expanded = ui_panel(h_panel, "Advanced Settings", false, true, false);
    ui_end_element();
    if (demo_ui->panel_expanded) {
        ui_indent();
        ui_text("Panel content goes here", UI_ALIGN_LEFT, 0x00000000);
        ui_end_element();
        ui_button("Panel Button", UI_ALIGN_CENTER, "");
        ui_end_element();
        ui_unindent();
    }
}

void render_tab_inputs(void) {
    ui_text("Slider", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();

    ui_handle_t *h_slider = ui_handle(__ID__);
    h_slider->f = demo_ui->slider_float;
    demo_ui->slider_float = ui_slider(h_slider, "Value", 0.0, 1.0, true, 100.0, true, UI_ALIGN_RIGHT, true);
    ui_end_element();

    ui_separator(0, true);
    ui_end_element();

    ui_text("Float Input", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();

    ui_handle_t *h_float = ui_handle(__ID__);
    demo_ui->float_value = ui_float_input(h_float, "Number:", UI_ALIGN_LEFT, 100.0);
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

    ui_text("Text Area", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();

    ui_text_area_line_numbers = false;
    ui_handle_t *h_area = ui_handle(__ID__);
    h_area->text = demo_ui->text_area_buffer;
    ui_text_area(h_area, UI_ALIGN_LEFT, true, "", false);
    ui_end_element();

    ui_separator(0, true);
    ui_end_element();

    ui_text("Combo Box", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();

    ui_handle_t *h_combo = ui_handle(__ID__);
    demo_ui->combo_selection = ui_combo(h_combo, combo_options, "Select:", true, UI_ALIGN_LEFT, false);
    ui_end_element();

    ui_separator(0, true);
    ui_end_element();

    ui_text("Inline Radio (Segmented)", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();

    ui_handle_t *h_inline = ui_handle(__ID__);
    h_inline->i = demo_ui->inline_radio_selection;
    demo_ui->inline_radio_selection = ui_inline_radio(h_inline, combo_options, UI_ALIGN_LEFT);
    ui_end_element();
}

void render_tab_layout(void) {
    ui_text("Row Layouts", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();

    ui_text("ui_row2() - Two columns:", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();
    ui_row2();
    ui_button("A", UI_ALIGN_CENTER, "");
    ui_end_element();
    ui_button("B", UI_ALIGN_CENTER, "");
    ui_end_element();

    ui_text("ui_row3() - Three columns:", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();
    ui_row3();
    ui_button("1", UI_ALIGN_CENTER, "");
    ui_end_element();
    ui_button("2", UI_ALIGN_CENTER, "");
    ui_end_element();
    ui_button("3", UI_ALIGN_CENTER, "");
    ui_end_element();

    ui_separator(0, true);
    ui_end_element();

    ui_text("Indent / Unindent", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();

    ui_button("No Indent", UI_ALIGN_CENTER, "");
    ui_end_element();
    ui_indent();
    ui_button("Indented 1", UI_ALIGN_CENTER, "");
    ui_end_element();
    ui_indent();
    ui_button("Indented 2", UI_ALIGN_CENTER, "");
    ui_end_element();
    ui_unindent();
    ui_button("Indented 1 (again)", UI_ALIGN_CENTER, "");
    ui_end_element();
    ui_unindent();
    ui_button("Back to normal", UI_ALIGN_CENTER, "");
    ui_end_element();

    ui_separator(0, true);
    ui_end_element();

    ui_text("Separator Styles", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();
    ui_text("Thin line:", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();
    ui_separator(1, false);
    ui_end_element();
    ui_text("Filled bar:", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();
    ui_separator(4, true);
    ui_end_element();
}

void render_tab_display(void) {
    ui_text("Image Display", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();

    ui_text("Noise Texture (original):", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();
    int state1 = ui_image(demo_ui->demo_image, 0xFFFFFFFF, 64);
    if (state1 == UI_STATE_HOVERED) {
        ui_tooltip("256x256 noise texture");
    }
    ui_end_element();

    ui_text("Scaled (128x64):", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();
    int state2 = ui_image(demo_ui->demo_image, 0xFFFFFFFF, 64);
    ui_end_element();

    ui_text("Color tinted:", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();
    int state3 = ui_image(demo_ui->demo_image, 0xFF4488FF, 64);
    if (state3 == UI_STATE_HOVERED) {
        ui_tooltip("Blue tinted!");
    }
    ui_end_element();

    ui_separator(0, true);
    ui_end_element();

    ui_text("Hover over elements to see tooltips", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();

    ui_handle_t *h_tip = ui_handle(__ID__);
    h_tip->b = demo_ui->check_bool;
    demo_ui->check_bool = ui_check(h_tip, "Hover for tooltip", "");
    ui_end_element();
}

void render_tab_color(void) {
    ui_text("Color Wheel", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();

    ui_handle_t *h_color = ui_handle(__ID__);
    h_color->color = demo_ui->selected_color;
    demo_ui->selected_color = ui_color_wheel(h_color, true, -1, -1, true, NULL, NULL);
    ui_end_element();

    ui_separator(0, true);
    ui_end_element();

    ui_text("Current Color", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();
    ui_fill(0, 0, 100, 30, demo_ui->selected_color);
    ui_end_element();

    ui_text("RGBA:", UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();
    char col_str[32];
    snprintf(col_str, sizeof(col_str), "R:%d G:%d B:%d A:%d",
             ui_color_r(demo_ui->selected_color),
             ui_color_g(demo_ui->selected_color),
             ui_color_b(demo_ui->selected_color),
             ui_color_a(demo_ui->selected_color));
    ui_text(col_str, UI_ALIGN_LEFT, 0x00000000);
    ui_end_element();
}

void render_tab_chinese(void) {
    ui_text("繁體中文測試", UI_ALIGN_LEFT, 0x00000000);
    //ui_end_element();
    ui_text("使用 NotoSansTC 字型", UI_ALIGN_LEFT, 0x00000000);
    //ui_end_element();

    ui_separator(0, true);
    //ui_end_element();

    ui_text("按鈕測試", UI_ALIGN_LEFT, 0x00000000);
    //ui_end_element();

    if (ui_button("點擊我", UI_ALIGN_CENTER, "")) {
    }
    //ui_end_element();

    if (ui_button("確認", UI_ALIGN_CENTER, "")) {
    }
    //ui_end_element();

    if (ui_button("取消", UI_ALIGN_CENTER, "")) {
    }
    //ui_end_element();

    ui_separator(0, true);
    //ui_end_element();

    ui_text("核取方塊", UI_ALIGN_LEFT, 0x00000000);
    //ui_end_element();

    ui_handle_t *h_check_zh = ui_handle(__ID__);
    h_check_zh->b = demo_ui->check_bool;
    demo_ui->check_bool = ui_check(h_check_zh, "啟用選項", "");
    //ui_end_element();

    ui_handle_t *h_check_zh2 = ui_handle(__ID__);
    h_check_zh2->b = demo_ui->check_visible;
    demo_ui->check_visible = ui_check(h_check_zh2, "顯示內容", "");
    //ui_end_element();

    ui_separator(0, true);
    //ui_end_element();

    ui_text("單選按鈕", UI_ALIGN_LEFT, 0x00000000);
    //ui_end_element();

    ui_handle_t *h_radio_zh = ui_handle(__ID__);
    h_radio_zh->i = demo_ui->radio_selection;
    if (ui_radio(h_radio_zh, 0, "選項甲", "")) {
        demo_ui->radio_selection = 0;
    }
    //ui_end_element();
    if (ui_radio(h_radio_zh, 1, "選項乙", "")) {
        demo_ui->radio_selection = 1;
    }
    //ui_end_element();
    if (ui_radio(h_radio_zh, 2, "選項丙", "")) {
        demo_ui->radio_selection = 2;
    }
    //ui_end_element();

    ui_separator(0, true);
    //ui_end_element();

    ui_text("滑桿", UI_ALIGN_LEFT, 0x00000000);
    //ui_end_element();

    ui_handle_t *h_slider_zh = ui_handle(__ID__);
    h_slider_zh->f = demo_ui->slider_float;
    demo_ui->slider_float = ui_slider(h_slider_zh, "數值", 0.0, 100.0, true, 1.0, true, UI_ALIGN_RIGHT, true);
    //ui_end_element();

    ui_separator(0, true);
    //ui_end_element();

    ui_text("文字輸入", UI_ALIGN_LEFT, 0x00000000);
    //ui_end_element();

    ui_handle_t *h_text_zh = ui_handle(__ID__);
    h_text_zh->text = demo_ui->text_input_buffer;
    ui_text_input(h_text_zh, "", UI_ALIGN_LEFT, true, false);
    //ui_end_element();

    ui_separator(0, true);
    //ui_end_element();

    ui_text("下拉選單", UI_ALIGN_LEFT, 0x00000000);
    //ui_end_element();

    ui_handle_t *h_combo_zh = ui_handle(__ID__);
    demo_ui->combo_selection = ui_combo(h_combo_zh, combo_options, "選擇：", true, UI_ALIGN_LEFT, false);
    //ui_end_element();

    ui_separator(0, true);
    //ui_end_element();

    ui_text("可摺疊面板", UI_ALIGN_LEFT, 0x00000000);
    //ui_end_element();

    ui_handle_t *h_panel_zh = ui_handle(__ID__);
    h_panel_zh->b = demo_ui->panel_expanded;
    demo_ui->panel_expanded = ui_panel(h_panel_zh, "進階設定", false, true, false);
    //ui_end_element();
    if (demo_ui->panel_expanded) {
        ui_indent();
        ui_text("面板內容放在這裡", UI_ALIGN_LEFT, 0x00000000);
        ui_end_element();
        ui_button("面板按鈕", UI_ALIGN_CENTER, "");
        ui_end_element();
        ui_unindent();
    }

    ui_separator(0, true);
    //ui_end_element();

    ui_text("顏色選擇器", UI_ALIGN_LEFT, 0x00000000);
    //ui_end_element();

    ui_handle_t *h_color_zh = ui_handle(__ID__);
    h_color_zh->color = demo_ui->selected_color;
    demo_ui->selected_color = ui_color_wheel(h_color_zh, true, -1, -1, true, NULL, NULL);
    //ui_end_element();

    ui_text("目前顏色:", UI_ALIGN_LEFT, 0x00000000);
    //ui_end_element();
    ui_fill(0, 0, 100, 30, demo_ui->selected_color);
    //ui_end_element();
}

void demo_ui_update(void *_) {
}

void demo_ui_render(void *_) {
    draw_begin(NULL, true, 0xff1a1a2e);
    draw_end();
    
    ui_begin(ui);

    ui_menubar_render();

    if (menubar_get_exit_requested()) {
        menubar_reset_requests();
        iron_stop();
    }

    if (menubar_get_theme_requested()) {
        menubar_reset_requests();
        demo_ui->theme = (demo_ui->theme == THEME_DARK) ? THEME_LIGHT : THEME_DARK;
    }

    if (menubar_get_about_requested()) {
        menubar_reset_requests();
    }

    i32 menubar_height = math_floor(ui_menu_menubar_h(ui));
    i32 window_y_offset = menubar_height + 5;

    if (ui_window(demo_ui->windows[DEMO_WINDOW_MAIN], 10, window_y_offset, 450, 700, true)) {

        ui_text("UI Demo", UI_ALIGN_LEFT, 0x00000000);
        ui_text("繁體中文測試", UI_ALIGN_LEFT, 0x00000000);

        ui_handle_t *h_tab = ui_handle(__ID__);
        for (i32 i = 0; i < DEMO_TAB_COUNT; i++) {
            if (ui_tab(h_tab, tab_names[i], false, -1, false)) {
                demo_ui->active_tab = i;
            }
        }

        switch (demo_ui->active_tab) {
            case DEMO_TAB_BUTTONS: render_tab_buttons(); break;
            case DEMO_TAB_INPUTS: render_tab_inputs(); break;
            case DEMO_TAB_LAYOUT: render_tab_layout(); break;
            case DEMO_TAB_DISPLAY: render_tab_display(); break;
            case DEMO_TAB_COLOR: render_tab_color(); break;
            case DEMO_TAB_CHINESE: render_tab_chinese(); break;
        }
    }

    if (ui_window(demo_ui->windows[DEMO_WINDOW_SECOND], 470, window_y_offset, 800, 800, true)) {
       
        ui_text("Hello from Window2", UI_ALIGN_LEFT, 0x00000000);

        ui_text("Hello from Window2", UI_ALIGN_LEFT, 0x00000000);

        ui_text("Hello from Window2", UI_ALIGN_LEFT, 0x00000000);

    }

    ui_end();
}

void demo_ui_init(void) {
    ui_menubar_init();
    combo_options = char_ptr_array_create(8);
    gc_root(combo_options);
    char_ptr_array_push(combo_options, "選項一");
    char_ptr_array_push(combo_options, "選項二");
    char_ptr_array_push(combo_options, "選項三");
    char_ptr_array_push(combo_options, "選項四");
    char_ptr_array_push(combo_options, "選項五");
}
