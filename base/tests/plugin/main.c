// Minic Plugin Test with Draw
// Uses draw_* functions instead of ui_* for simplicity

#include <iron.h>
#include <stdint.h>
#include <stdio.h>

static void *g_plugin_fn_on_ui  = NULL;
static void *g_plugin_fn_on_update = NULL;
static minic_ctx_t *g_plugin_ctx = NULL;

static float g_button_x = 100.0f;
static float g_button_y = 100.0f;
static float g_button_w = 120.0f;
static float g_button_h = 40.0f;
static int g_button_hovered = 0;
static int g_button_pressed = 0;

void console_info(char *s) {
    fprintf(stderr, "[INFO] %s\n", s);
}
void console_error(char *s) {
    fprintf(stderr, "[ERROR] %s\n", s);
}
void console_log(char *s) {
    fprintf(stderr, "[LOG] %s\n", s);
}

void test_plugin_register_on_ui(void *fn) {
    g_plugin_fn_on_ui = fn;
}

void test_plugin_register_on_update(void *fn) {
    g_plugin_fn_on_update = fn;
}

void *test_plugin_create(void) {
    static int plugin_id = 0;
    plugin_id++;
    return (void *)(intptr_t)plugin_id;
}

void test_plugin_load(const char *filename) {
    buffer_t *blob = iron_load_blob(filename);
    if (blob == NULL) {
        iron_log("Failed to load plugin: %s", filename);
        return;
    }

    g_plugin_ctx = minic_eval_named(sys_buffer_to_string(blob), filename);
    if (g_plugin_ctx == NULL) {
        iron_log("Failed to eval plugin: %s", filename);
        return;
    }
}

int hit_test(float mx, float my) {
    return mx >= g_button_x && mx <= g_button_x + g_button_w &&
           my >= g_button_y && my <= g_button_y + g_button_h;
}

void render(void) {
    draw_begin(NULL, true, 0xff1a1a2e);

    // Draw button background
    if (g_button_pressed) {
        draw_set_color(0xff205d9c);
    } else if (g_button_hovered) {
        draw_set_color(0xff383838);
    } else {
        draw_set_color(0xff323232);
    }
    draw_filled_rect(g_button_x, g_button_y, g_button_w, g_button_h);

    // Draw button border
    draw_set_color(0xff555555);
    draw_rect(g_button_x, g_button_y, g_button_w, g_button_h, 1.0f);

    // Draw button text placeholder (just a rect for now)
    draw_set_color(0xffffffff);
    draw_string("Click Me", g_button_x + 20, g_button_y + 12);

    // Call plugin UI
    if (g_plugin_fn_on_ui != NULL) {
        minic_ctx_call_fn(g_plugin_ctx, g_plugin_fn_on_ui, NULL, 0);
    }

    draw_end();
}

void _kickstart(void) {
    iron_window_options_t *ops =
        GC_ALLOC_INIT(iron_window_options_t, {.title     = "Minic Plugin Test",
                                              .width     = 800,
                                              .height    = 600,
                                              .x         = -1,
                                              .y         = -1,
                                              .mode      = IRON_WINDOW_MODE_WINDOW,
                                              .features  = IRON_WINDOW_FEATURES_RESIZABLE | IRON_WINDOW_FEATURES_MINIMIZABLE | IRON_WINDOW_FEATURES_MAXIMIZABLE,
                                              .vsync     = true,
                                              .frequency = 60,
                                              .depth_bits = 0});

    sys_start(ops);

    minic_register_builtins();

    minic_register("test_plugin_create", "p()", (minic_ext_fn_raw_t)test_plugin_create);
    minic_register("test_plugin_register_on_ui", "v(p)", (minic_ext_fn_raw_t)test_plugin_register_on_ui);
    minic_register("test_plugin_register_on_update", "v(p)", (minic_ext_fn_raw_t)test_plugin_register_on_update);
    minic_register("console_info", "v(p)", (minic_ext_fn_raw_t)console_info);
    minic_register("console_error", "v(p)", (minic_ext_fn_raw_t)console_error);
    minic_register("console_log", "v(p)", (minic_ext_fn_raw_t)console_log);
    minic_register("gc_root", "v(p)", (minic_ext_fn_raw_t)gc_root);
    minic_register("gc_alloc", "p(i)", (minic_ext_fn_raw_t)gc_alloc);

    test_plugin_load("data/test_plugin.c");

    _iron_set_update_callback(render);

    iron_start();
}

// Stubs
void project_save(bool save_and_quit) {}
char *project_filepath = "";
char *project_filepath_get(void) { return project_filepath; }
void ui_box_show_message(char *title, char *text, bool copyable) {}
void ui_files_show2(char *filters, bool is_save, bool open_multiple, void *files_done) {}
void ui_files_show(char *filter, bool open_multiple, void *files_done) {}
void context_set_viewport_shader(void *shader) {}
void node_shader_write_frag(void *raw, char *s) {}
void plugin_register_texture(char *ext, void *fn) {}
void plugin_unregister_texture(char *ext) {}
void plugin_register_mesh(char *ext, void *fn) {}
void plugin_unregister_mesh(char *ext) {}
void plugin_make_raw_mesh(void) {}
void plugin_material_category_add(void *cat, void *nodes) {}
void plugin_brush_category_add(void *cat, void *nodes) {}
void plugin_material_category_remove(void *cat) {}
void plugin_brush_category_remove(void *cat) {}
void plugin_material_custom_nodes_set(void *type, void *fn) {}
void plugin_brush_custom_nodes_set(void *type, void *fn) {}
void plugin_material_custom_nodes_remove(void *type) {}
void plugin_brush_custom_nodes_remove(void *type) {}
void *plugin_material_kong_get(void) { return NULL; }
char *parser_material_parse_value_input(void *inp, bool vector_as_grayscale) { return ""; }
void plugin_notify_on_ui(void *p, void *f) {}
void plugin_notify_on_update(void *p, void *f) {}
void plugin_notify_on_delete(void *p, void *f) {}
void *plugin_create(void) { return NULL; }
void *parser_material_kong = NULL;

any_map_t *ui_children;
any_map_t *ui_nodes_custom_buttons;
i32 pipes_offset;
char *strings_check_internet_connection(void) { return ""; }
char *tr(char *id, any_map_t *vars) { return id; }
i32 pipes_get_constant_location(char *s) { return 0; }
