
#pragma once

#include <iron.h>
#include "enums.h"
#include "types.h"

demo_ui_t *demo_ui;
ui_t      *ui;

extern draw_font_t *demo_font;
extern ui_theme_t *demo_theme;
extern gpu_texture_t *demo_color_wheel;
extern gpu_texture_t *demo_color_wheel_gradient;

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

extern bool   ui_menu_show;
extern bool   ui_menu_show_first;
extern bool   ui_menu_keep_open;
extern bool   ui_menu_hide_flag;
extern bool   ui_menu_nested;
extern i32    ui_menu_x;
extern i32    ui_menu_y;
extern i32    ui_menu_h;
extern i32    ui_menu_sub_x;
extern i32    ui_menu_sub_y;
extern void (*ui_menu_commands)(void);