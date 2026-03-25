
#include "global.h"
#include "ui_menu.h"

bool   ui_menu_show       = false;
bool   ui_menu_show_first = false;
bool   ui_menu_keep_open  = false;
bool   ui_menu_hide_flag = false;
bool   ui_menu_nested     = false;
i32    ui_menu_x          = 0;
i32    ui_menu_y          = 0;
i32    ui_menu_h          = 0;
i32    ui_menu_sub_x      = 0;
i32    ui_menu_sub_y      = 0;
void (*ui_menu_commands)(void) = NULL;

static f32 menu_default_element_w = 120;

void ui_menu_init(void) {
}

f32 ui_menu_menubar_h(ui_t *ui) {
    f32 button_offset_y = (ui->ops->theme->ELEMENT_H * UI_SCALE() - ui->ops->theme->BUTTON_H * UI_SCALE()) / (float)2;
    return ui->ops->theme->BUTTON_H * UI_SCALE() * 1.1 + 2 + button_offset_y;
}

void ui_menu_render(void) {
    if (!ui_menu_show || ui_menu_commands == NULL) {
        return;
    }

    i32 menu_w = math_floor(menu_default_element_w * UI_SCALE() * 2.0);

    i32 _FILL_BUTTON_BG            = ui->ops->theme->FILL_BUTTON_BG;
    ui->ops->theme->FILL_BUTTON_BG = false;
    i32 _ELEMENT_OFFSET            = ui->ops->theme->ELEMENT_OFFSET;
    ui->ops->theme->ELEMENT_OFFSET = 0;
    i32 _ELEMENT_H                 = ui->ops->theme->ELEMENT_H;
    ui->ops->theme->ELEMENT_H      = 28;

    if (ui_menu_nested) {
        ui_menu_show_first = true;
        ui_menu_nested     = false;
    }

    if (ui_menu_show_first) {
        ui_menu_x -= iron_window_width() * 2;
        ui_menu_y -= iron_window_height() * 2;
    }

    draw_begin(NULL, false, 0);
    ui_begin_region(ui, ui_menu_x, ui_menu_y, menu_w);
    ui->input_enabled = true;
    ui_menu_begin();

    ui_menu_commands();

    ui_menu_hide_flag = !ui_menu_keep_open && !ui_menu_show_first &&
                         (ui->changed || ui->input_released || ui->input_released_r || ui->is_escape_down);
    ui_menu_keep_open = false;

    ui->ops->theme->FILL_BUTTON_BG = _FILL_BUTTON_BG;
    ui->ops->theme->ELEMENT_OFFSET = _ELEMENT_OFFSET;
    ui->ops->theme->ELEMENT_H      = _ELEMENT_H;
    ui_menu_end();
    ui_end_region();
    draw_end();
    ui->input_enabled = true;

    if (ui_menu_show_first) {
        ui_menu_show_first = false;
        ui_menu_keep_open  = true;
        ui_menu_h          = math_floor(ui->_y - ui_menu_y);
        ui_menu_x += iron_window_width() * 2;
        ui_menu_y += iron_window_height() * 2;
        ui_menu_fit_to_screen();
        ui_menu_render();
    }

    if (ui_menu_hide_flag) {
        ui_menu_hide();
        ui_menu_show_first = true;
        ui_menu_commands   = NULL;
    }
}

void ui_menu_hide(void) {
    ui_menu_show = false;
}

void ui_menu_draw(void (*commands)(void), i32 x, i32 y) {
    if (ui_menu_show) {
        ui_menu_nested    = true;
        ui_menu_keep_open = true;
    }
    ui_menu_show = true;
    ui_menu_commands = commands;
    ui_menu_x = x > -1 ? x : math_floor(mouse_x + 1);
    ui_menu_y = y > -1 ? y : math_floor(mouse_y + 1);
    ui_menu_h = 0;
}

void ui_menu_fit_to_screen(void) {
    f32 menu_w = menu_default_element_w * UI_SCALE() * 2.0;
    if (ui_menu_x + menu_w > iron_window_width()) {
        if (ui_menu_x - menu_w > 0) {
            ui_menu_x = math_floor(ui_menu_x - menu_w);
        }
        else {
            ui_menu_x = math_floor(iron_window_width() - menu_w);
        }
    }
    if (ui_menu_y + ui_menu_h > iron_window_height()) {
        if (ui_menu_y - ui_menu_h > 0) {
            ui_menu_y = math_floor(ui_menu_y - ui_menu_h);
        }
        else {
            ui_menu_y = iron_window_height() - ui_menu_h;
        }
        ui_menu_x += 1;
    }
}

void ui_menu_separator(void) {
    ui->_y++;
    ui_fill(8, 0, ui->_w / (float)UI_SCALE() - 16, 1, ui->ops->theme->BUTTON_COL);
}

bool ui_menu_button(char *text, char *label) {
    bool result = ui_button(string("  %s", text), UI_ALIGN_LEFT, label);
    if (string_equals(label, ">") && result) {
        ui_menu_keep_open = true;
    }
    return result;
}

void ui_menu_begin(void) {
    draw_set_color(ui->ops->theme->SEPARATOR_COL);
    ui_draw_rect(true, ui->_x, ui->_y, ui->_w, ui_menu_h > 0 ? ui_menu_h : 200);
    draw_set_color(0xffffffff);
}

void ui_menu_end(void) {
}
