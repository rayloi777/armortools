
#include "global.h"
#include "ui_menu.h"
#include "ui_menubar.h"

static menu_category_t ui_menubar_category = MENU_CATEGORY_FILE;
static ui_handle_t    *ui_menubar_handle   = NULL;
static char           *menu_labels[MENU_CATEGORY_COUNT] = {
    "File",
    "Edit",
    "View",
    "Help",
};

static bool menubar_exit_requested   = false;
static bool menubar_theme_requested  = false;
static bool menubar_about_requested  = false;

bool menubar_get_exit_requested(void) {
    return menubar_exit_requested;
}

bool menubar_get_theme_requested(void) {
    return menubar_theme_requested;
}

bool menubar_get_about_requested(void) {
    return menubar_about_requested;
}

void menubar_reset_requests(void) {
    menubar_exit_requested  = false;
    menubar_theme_requested = false;
    menubar_about_requested = false;
}

void ui_menubar_init(void) {
    ui_menubar_handle = ui_handle_create();
    ui_menubar_handle->layout = UI_LAYOUT_HORIZONTAL;
    ui_menu_init();
}

void ui_menubar_render_menu(void) {
    if (ui_menubar_category == MENU_CATEGORY_FILE) {
        if (ui_menu_button("New", "")) {
        }
        if (ui_menu_button("Open...", "")) {
        }
        if (ui_menu_button("Save", "")) {
        }
        ui_menu_separator();
        if (ui_menu_button("Exit", "")) {
            menubar_exit_requested = true;
        }
    }
    else if (ui_menubar_category == MENU_CATEGORY_EDIT) {
        if (ui_menu_button("Undo", "")) {
        }
        if (ui_menu_button("Redo", "")) {
        }
        ui_menu_separator();
        if (ui_menu_button("Toggle Theme", "")) {
            menubar_theme_requested = true;
        }
    }
    else if (ui_menubar_category == MENU_CATEGORY_VIEW) {
        if (ui_menu_button("Zoom In", "")) {
        }
        if (ui_menu_button("Zoom Out", "")) {
        }
        if (ui_menu_button("Reset View", "")) {
        }
        ui_menu_separator();
        if (ui_menu_button("Toggle Fullscreen", "")) {
        }
    }
    else if (ui_menubar_category == MENU_CATEGORY_HELP) {
        if (ui_menu_button("Documentation", "")) {
        }
        if (ui_menu_button("About...", "")) {
            menubar_about_requested = true;
        }
    }
}

void ui_menubar_show_menu(menu_category_t category) {
    if (ui_menu_show && ui_menubar_category == category) {
        return;
    }

    ui_menu_show       = true;
    ui_menu_show_first = true;
    ui_menu_commands   = ui_menubar_render_menu;
    ui_menubar_category = category;

    ui_menu_x = math_floor(ui->_x - ui->_w);
    ui_menu_y = math_floor(ui_menu_menubar_h(ui) + 2);
}

void ui_menubar_render(void) {
    f32 menubar_h = ui_menu_menubar_h(ui);
    i32 window_w  = iron_window_width();

    if (ui_window(ui_menubar_handle, 0, 0, window_w, menubar_h, false)) {

        ui_begin_menu();

        for (i32 i = 0; i < MENU_CATEGORY_COUNT; ++i) {
            if (ui_menubar_button(menu_labels[i]) ||
                (ui_menu_show && ui_menu_commands == ui_menubar_render_menu && ui->is_hovered)) {
                ui_menubar_show_menu(i);
            }
        }

        ui_end_menu();
    }

    ui_menu_render();
}
