#pragma once

#include "global.h"

void  ui_menu_init(void);
void  ui_menu_render(void);
void  ui_menu_draw(void (*commands)(void), i32 x, i32 y);
void  ui_menu_hide(void);
void  ui_menu_fit_to_screen(void);
void  ui_menu_separator(void);
bool  ui_menu_button(char *text, char *label);
void  ui_menu_begin(void);
void  ui_menu_end(void);
f32   ui_menu_menubar_h(ui_t *ui);
