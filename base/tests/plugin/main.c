// Minic Plugin Test - Thin loader

#include <iron.h>
#include <stdint.h>
#include <stdio.h>

static void *g_plugin_fn_on_ui = NULL;

void console_info(char *s) {
    fprintf(stderr, "[INFO] %s\n", s);
}

void test_plugin_register_on_ui(void *fn) {
    g_plugin_fn_on_ui = fn;
}

void *test_plugin_create(void) {
    static int plugin_id = 0;
    plugin_id++;
    return (void *)(intptr_t)plugin_id;
}

int mouse_started(char *button) {
    return iron_input_mouse_started(0, button);
}

float mouse_x(void) {
    return iron_input_mouse_x();
}

float mouse_y(void) {
    return iron_input_mouse_y();
}

void test_plugin_load(const char *filename) {
    buffer_t *blob = iron_load_blob(filename);
    if (blob == NULL) {
        iron_log("Failed to load plugin: %s", filename);
        return;
    }

    minic_ctx_t *ctx = minic_eval_named(sys_buffer_to_string(blob), filename);
    if (ctx == NULL) {
        iron_log("Failed to eval plugin: %s", filename);
        return;
    }
}

void render(void) {
    draw_begin(NULL, true, 0xff1a1a2e);

    if (g_plugin_fn_on_ui != NULL) {
        minic_ctx_call_fn(NULL, g_plugin_fn_on_ui, NULL, 0);
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
    minic_register("console_info", "v(p)", (minic_ext_fn_raw_t)console_info);
    minic_register("gc_root", "v(p)", (minic_ext_fn_raw_t)gc_root);
    minic_register("gc_alloc", "p(i)", (minic_ext_fn_raw_t)gc_alloc);

    minic_register("mouse_started", "i(p)", (minic_ext_fn_raw_t)mouse_started);
    minic_register("mouse_x", "f()", (minic_ext_fn_raw_t)mouse_x);
    minic_register("mouse_y", "f()", (minic_ext_fn_raw_t)mouse_y);

    minic_register("draw_set_color", "v(i)", (minic_ext_fn_raw_t)draw_set_color);
    minic_register("draw_filled_rect", "v(f,f,f,f)", (minic_ext_fn_raw_t)draw_filled_rect);
    minic_register("draw_rect", "v(f,f,f,f,f)", (minic_ext_fn_raw_t)draw_rect);

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
