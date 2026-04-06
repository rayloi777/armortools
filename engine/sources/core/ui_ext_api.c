#include "ui_ext_api.h"
#include <iron.h>
#include <iron_ui.h>
#include <iron_input.h>
#include <iron_draw.h>
#include "../../base/sources/libs/minic.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// ── UI Context ──────────────────────────────────────────────

static ui_t         g_ui;
static ui_options_t g_ui_ops;
static ui_theme_t   g_ui_theme;
static bool         g_ui_initialized = false;

// ── Handle Pool ─────────────────────────────────────────────

#define MAX_UI_HANDLES 256
static ui_handle_t *g_handles[MAX_UI_HANDLES];
static int          g_handle_count = 0;

// ── Init / Shutdown ─────────────────────────────────────────

// Fake window handle for horizontal layout
static ui_handle_t g_hlayout_handle;

void ui_ext_api_init(void) {
    if (g_ui_initialized) return;

    ui_theme_default(&g_ui_theme);

    draw_font_t *font = data_get_font("font.ttf");
    if (font) draw_font_init(font);

    memset(&g_ui_ops, 0, sizeof(g_ui_ops));
    g_ui_ops.font         = font;
    g_ui_ops.theme        = &g_ui_theme;
    g_ui_ops.scale_factor = 1.0f;

    ui_init(&g_ui, &g_ui_ops);
    g_ui_initialized = true;

    // Initialize horizontal layout handle — texture.height must be non-zero
    // so ui_is_visible() doesn't skip elements when this is set as current_window
    memset(&g_hlayout_handle, 0, sizeof(g_hlayout_handle));
    g_hlayout_handle.layout = UI_LAYOUT_HORIZONTAL;
    g_hlayout_handle.texture.height = 10000;

    memset(g_handles, 0, sizeof(g_handles));
    g_handle_count = 0;
}

void ui_ext_api_shutdown(void) {
    g_handle_count = 0;
    g_ui_initialized = false;
}

// ── Per-frame begin / end ───────────────────────────────────

void ui_ext_api_begin(void) {
    if (!g_ui_initialized) return;
    ui_begin(&g_ui);
}

void ui_ext_api_end(void) {
    if (!g_ui_initialized) return;
    ui_end();
    ui_end_frame();
}

// ── Input sync ──────────────────────────────────────────────

void ui_ext_api_update_input(void) {
    if (!g_ui_initialized) return;
    ui_mouse_move(&g_ui, (int)mouse_x, (int)mouse_y, (int)(mouse_moved ? mouse_movement_x : 0), (int)(mouse_moved ? mouse_movement_y : 0));
    if (mouse_started_any())    ui_mouse_down(&g_ui, 0, (int)mouse_x, (int)mouse_y);
    if (mouse_released("left")) ui_mouse_up(&g_ui, 0, (int)mouse_x, (int)mouse_y);
    if (mouse_wheel_delta != 0.0f) ui_mouse_wheel(&g_ui, mouse_wheel_delta);
}

// ── Helpers ─────────────────────────────────────────────────

static ui_handle_t *get_handle(minic_val_t *arg) {
    if (arg->type == MINIC_T_PTR) return (ui_handle_t *)arg->p;
    return NULL;
}

// ── Minic Wrappers ──────────────────────────────────────────

static minic_val_t minic_ui_ext_handle(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    if (g_handle_count >= MAX_UI_HANDLES) return minic_val_ptr(NULL);
    ui_handle_t *h = ui_handle_create();
    if (!h) return minic_val_ptr(NULL);
    g_handles[g_handle_count++] = h;
    return minic_val_ptr(h);
}

static minic_val_t minic_ui_ext_window(minic_val_t *args, int argc) {
    if (argc < 6) return minic_val_int(0);
    ui_handle_t *h = get_handle(&args[0]);
    int x      = (int)minic_val_to_d(args[1]);
    int y      = (int)minic_val_to_d(args[2]);
    int w      = (int)minic_val_to_d(args[3]);
    int hh     = (int)minic_val_to_d(args[4]);
    bool drag  = (bool)(int)minic_val_to_d(args[5]);
    bool open  = ui_window(h, x, y, w, hh, drag);
    return minic_val_int(open ? 1 : 0);
}

static minic_val_t minic_ui_ext_end_window(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    ui_end_window();
    return minic_val_void();
}

static minic_val_t minic_ui_ext_button(minic_val_t *args, int argc) {
    if (argc < 1) return minic_val_int(0);
    char *text  = (char *)args[0].p;
    int align   = (argc > 1) ? (int)minic_val_to_d(args[1]) : UI_ALIGN_LEFT;
    char *label = (argc > 2 && args[2].type == MINIC_T_PTR) ? (char *)args[2].p : "";
    bool pressed = ui_button(text, align, label);
    return minic_val_int(pressed ? 1 : 0);
}

static minic_val_t minic_ui_ext_text(minic_val_t *args, int argc) {
    if (argc < 1) return minic_val_int(0);
    char *text = (char *)args[0].p;
    int align  = (argc > 1) ? (int)minic_val_to_d(args[1]) : UI_ALIGN_LEFT;
    int bg     = (argc > 2) ? (int)minic_val_to_d(args[2]) : 0;
    int result = ui_text(text, align, bg);
    return minic_val_int(result);
}

static minic_val_t minic_ui_ext_panel(minic_val_t *args, int argc) {
    if (argc < 5) return minic_val_int(0);
    ui_handle_t *h = get_handle(&args[0]);
    char *text     = (char *)args[1].p;
    bool is_tree   = (bool)(int)minic_val_to_d(args[2]);
    bool filled    = (bool)(int)minic_val_to_d(args[3]);
    bool align_r   = (bool)(int)minic_val_to_d(args[4]);
    bool open      = ui_panel(h, text, is_tree, filled, align_r);
    return minic_val_int(open ? 1 : 0);
}

static minic_val_t minic_ui_ext_tab(minic_val_t *args, int argc) {
    if (argc < 5) return minic_val_int(0);
    ui_handle_t *h = get_handle(&args[0]);
    char *text     = (char *)args[1].p;
    bool vertical  = (bool)(int)minic_val_to_d(args[2]);
    uint32_t color = (uint32_t)(int)minic_val_to_d(args[3]);
    bool align_r   = (bool)(int)minic_val_to_d(args[4]);
    bool active    = ui_tab(h, text, vertical, color, align_r);
    return minic_val_int(active ? 1 : 0);
}

static minic_val_t minic_ui_ext_begin_menu(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    ui_begin_menu();
    return minic_val_void();
}

static minic_val_t minic_ui_ext_end_menu(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    ui_end_menu();
    return minic_val_void();
}

static minic_val_t minic_ui_ext_menubar_button(minic_val_t *args, int argc) {
    if (argc < 1) return minic_val_int(0);
    char *text = (char *)args[0].p;
    bool pressed = ui_menubar_button(text);
    return minic_val_int(pressed ? 1 : 0);
}

static minic_val_t minic_ui_ext_check(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_int(0);
    ui_handle_t *h = get_handle(&args[0]);
    char *text     = (char *)args[1].p;
    char *label    = (char *)args[2].p;
    bool checked   = ui_check(h, text, label);
    return minic_val_int(checked ? 1 : 0);
}

static minic_val_t minic_ui_ext_slider(minic_val_t *args, int argc) {
    if (argc < 9) return minic_val_float(0.0f);
    ui_handle_t *h   = get_handle(&args[0]);
    char *text        = (char *)args[1].p;
    float from        = (float)minic_val_to_d(args[2]);
    float to          = (float)minic_val_to_d(args[3]);
    bool filled       = (bool)(int)minic_val_to_d(args[4]);
    float precision   = (float)minic_val_to_d(args[5]);
    bool display_val  = (bool)(int)minic_val_to_d(args[6]);
    int align         = (int)minic_val_to_d(args[7]);
    bool text_edit    = (bool)(int)minic_val_to_d(args[8]);
    float value = ui_slider(h, text, from, to, filled, precision, display_val, align, text_edit);
    return minic_val_float(value);
}

static minic_val_t minic_ui_ext_separator(minic_val_t *args, int argc) {
    int h    = (argc > 0) ? (int)minic_val_to_d(args[0]) : 4;
    bool fill = (argc > 1) ? (bool)(int)minic_val_to_d(args[1]) : true;
    ui_separator(h, fill);
    return minic_val_void();
}

static minic_val_t minic_ui_ext_indent(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    ui_indent();
    return minic_val_void();
}

static minic_val_t minic_ui_ext_unindent(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    ui_unindent();
    return minic_val_void();
}

static minic_val_t minic_ui_ext_row2(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    ui_row2();
    return minic_val_void();
}

static minic_val_t minic_ui_ext_row3(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    ui_row3();
    return minic_val_void();
}

static minic_val_t minic_ui_ext_row4(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    ui_row4();
    return minic_val_void();
}

static minic_val_t minic_ui_ext_begin_region(minic_val_t *args, int argc) {
    if (argc < 3) return minic_val_void();
    int x = (int)minic_val_to_d(args[0]);
    int y = (int)minic_val_to_d(args[1]);
    int w = (int)minic_val_to_d(args[2]);
    ui_begin_region(&g_ui, x, y, w);
    return minic_val_void();
}

static minic_val_t minic_ui_ext_end_region(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    ui_end_region();
    return minic_val_void();
}

static minic_val_t minic_ui_ext_menubar_h(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    if (!g_ui_initialized) return minic_val_float(0.0f);
    // UI_MENUBAR_H() = BUTTON_H * 1.1 + 2 + button_offset_y
    float bh = g_ui.ops->theme->BUTTON_H * (g_ui.ops->scale_factor > 0 ? g_ui.ops->scale_factor : 1.0f);
    float eh = g_ui.ops->theme->ELEMENT_H * (g_ui.ops->scale_factor > 0 ? g_ui.ops->scale_factor : 1.0f);
    float offset_y = (eh - bh) / 2.0f;
    return minic_val_float(bh * 1.1f + 2.0f + offset_y);
}

static minic_val_t minic_ui_ext_window_w(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    if (!g_ui_initialized) return minic_val_float(0.0f);
    return minic_val_float((float)g_ui._window_w);
}

static minic_val_t minic_ui_ext_input_started(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    if (!g_ui_initialized) return minic_val_int(0);
    return minic_val_int(g_ui.input_started ? 1 : 0);
}

static minic_val_t minic_ui_ext_input_released(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    if (!g_ui_initialized) return minic_val_int(0);
    return minic_val_int(g_ui.input_released ? 1 : 0);
}

static minic_val_t minic_ui_ext_input_in_rect(minic_val_t *args, int argc) {
    if (argc < 4) return minic_val_int(0);
    float x = (float)minic_val_to_d(args[0]);
    float y = (float)minic_val_to_d(args[1]);
    float w = (float)minic_val_to_d(args[2]);
    float h = (float)minic_val_to_d(args[3]);
    return minic_val_int(ui_input_in_rect(x, y, w, h) ? 1 : 0);
}

static minic_val_t minic_ui_ext_begin_hlayout(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    if (!g_ui_initialized) return minic_val_void();
    g_ui.current_window = &g_hlayout_handle;
    return minic_val_void();
}

static minic_val_t minic_ui_ext_end_hlayout(minic_val_t *args, int argc) {
    (void)args; (void)argc;
    if (!g_ui_initialized) return minic_val_void();
    g_ui.current_window = NULL;
    return minic_val_void();
}

// ── Registration ────────────────────────────────────────────

void ui_ext_api_register(void) {
    minic_register_native("ui_ext_handle", minic_ui_ext_handle);
    minic_register_native("ui_ext_window", minic_ui_ext_window);
    minic_register_native("ui_ext_end_window", minic_ui_ext_end_window);
    minic_register_native("ui_ext_button", minic_ui_ext_button);
    minic_register_native("ui_ext_text", minic_ui_ext_text);
    minic_register_native("ui_ext_panel", minic_ui_ext_panel);
    minic_register_native("ui_ext_tab", minic_ui_ext_tab);
    minic_register_native("ui_ext_begin_menu", minic_ui_ext_begin_menu);
    minic_register_native("ui_ext_end_menu", minic_ui_ext_end_menu);
    minic_register_native("ui_ext_menubar_button", minic_ui_ext_menubar_button);
    minic_register_native("ui_ext_check", minic_ui_ext_check);
    minic_register_native("ui_ext_slider", minic_ui_ext_slider);
    minic_register_native("ui_ext_separator", minic_ui_ext_separator);
    minic_register_native("ui_ext_indent", minic_ui_ext_indent);
    minic_register_native("ui_ext_unindent", minic_ui_ext_unindent);
    minic_register_native("ui_ext_row2", minic_ui_ext_row2);
    minic_register_native("ui_ext_row3", minic_ui_ext_row3);
    minic_register_native("ui_ext_row4", minic_ui_ext_row4);
    minic_register_native("ui_ext_begin_region", minic_ui_ext_begin_region);
    minic_register_native("ui_ext_end_region", minic_ui_ext_end_region);
    minic_register_native("ui_ext_menubar_h", minic_ui_ext_menubar_h);
    minic_register_native("ui_ext_window_w", minic_ui_ext_window_w);
    minic_register_native("ui_ext_input_started", minic_ui_ext_input_started);
    minic_register_native("ui_ext_input_released", minic_ui_ext_input_released);
    minic_register_native("ui_ext_input_in_rect", minic_ui_ext_input_in_rect);
    minic_register_native("ui_ext_begin_hlayout", minic_ui_ext_begin_hlayout);
    minic_register_native("ui_ext_end_hlayout", minic_ui_ext_end_hlayout);
}
