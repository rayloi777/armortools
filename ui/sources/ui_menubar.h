#pragma once

#include "global.h"

void  ui_menubar_init(void);
void  ui_menubar_render(void);
void  ui_menubar_render_menu(void);
void  ui_menubar_show_menu(menu_category_t category);
bool  menubar_get_exit_requested(void);
bool  menubar_get_theme_requested(void);
bool  menubar_get_about_requested(void);
void  menubar_reset_requests(void);
