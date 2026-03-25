// Eval Test - QuickJS
// ../../make --run

#include <iron.h>
#include <iron_eval.h>

extern JSContext *js_ctx;

static JSValue fn_init;
static JSValue fn_update;
static JSValue fn_render;
static f32     last_time;
static bool    js_initialized = false;

char *js_code = NULL;

void parse_render_result(JSValue result, f32 *out_x, f32 *out_y, f32 *out_size, i32 *out_color) {
    JSValue x_val = JS_GetPropertyStr(js_ctx, result, "x");
    JSValue y_val = JS_GetPropertyStr(js_ctx, result, "y");
    JSValue size_val = JS_GetPropertyStr(js_ctx, result, "size");
    JSValue color_val = JS_GetPropertyStr(js_ctx, result, "color");

    double x, y, size;
    i32 color;
    JS_ToFloat64(js_ctx, &x, x_val);
    JS_ToFloat64(js_ctx, &y, y_val);
    JS_ToFloat64(js_ctx, &size, size_val);
    JS_ToInt32(js_ctx, &color, color_val);

    *out_x = (f32)x;
    *out_y = (f32)y;
    *out_size = (f32)size;
    *out_color = color;

    JS_FreeValue(js_ctx, x_val);
    JS_FreeValue(js_ctx, y_val);
    JS_FreeValue(js_ctx, size_val);
    JS_FreeValue(js_ctx, color_val);
}

void render() {
    f32 now = (f32)iron_time();
    f32 dt = js_initialized ? (now - last_time) : (1.0f / 60.0f);
    last_time = now;
    dt = dt > 0.1f ? (1.0f / 60.0f) : dt;

    draw_begin(NULL, true, 0xff1a1a2e);

    if (js_initialized) {
        JSValue argv_dt[1];
        argv_dt[0] = JS_NewFloat64(js_ctx, (double)dt);

        js_call_arg(&fn_update, 1, argv_dt);
        JS_FreeValue(js_ctx, argv_dt[0]);

        argv_dt[0] = JS_NewFloat64(js_ctx, (double)dt);
        JSValue result = js_call_arg(&fn_render, 1, argv_dt);
        JS_FreeValue(js_ctx, argv_dt[0]);

        if (JS_IsObject(result)) {
            f32 x, y, size;
            i32 color;
            parse_render_result(result, &x, &y, &size, &color);

            draw_set_color(color);
            draw_filled_circle(x, y, size, 32);
        }
        JS_FreeValue(js_ctx, result);
    }

    draw_end();
}

void _kickstart() {
    iron_window_options_t *ops =
        GC_ALLOC_INIT(iron_window_options_t, {.title     = "Eval Test",
                                              .width     = 640,
                                              .height    = 480,
                                              .x         = -1,
                                              .y         = -1,
                                              .mode      = IRON_WINDOW_MODE_WINDOW,
                                              .features  = IRON_WINDOW_FEATURES_RESIZABLE | IRON_WINDOW_FEATURES_MINIMIZABLE | IRON_WINDOW_FEATURES_MAXIMIZABLE,
                                              .vsync     = true,
                                              .frequency = 60,
                                              .depth_bits = 0});

    sys_start(ops);

    js_init();

    iron_file_reader_t reader;
    if (iron_file_reader_open(&reader, "data/test.js", IRON_FILE_TYPE_ASSET)) {
        size_t size = iron_file_reader_size(&reader);
        js_code = malloc(size + 1);
        iron_file_reader_read(&reader, js_code, size);
        js_code[size] = '\0';
        iron_file_reader_close(&reader);
        iron_log("Loaded JS: %s", js_code);
    } else {
        iron_log("Failed to load data/test.js");
        _iron_set_update_callback(render);
        iron_start();
        return;
    }

    js_eval(js_code);
    free(js_code);
    js_code = NULL;

    JSValue global_obj = JS_GetGlobalObject(js_ctx);

    fn_init = JS_GetPropertyStr(js_ctx, global_obj, "init");
    fn_update = JS_GetPropertyStr(js_ctx, global_obj, "update");
    fn_render = JS_GetPropertyStr(js_ctx, global_obj, "render");

    fn_init = JS_DupValue(js_ctx, fn_init);
    fn_update = JS_DupValue(js_ctx, fn_update);
    fn_render = JS_DupValue(js_ctx, fn_render);

    JS_FreeValue(js_ctx, global_obj);

    js_call_arg(&fn_init, 0, NULL);
    js_initialized = true;
    last_time = (f32)iron_time();

    _iron_set_update_callback(render);

    iron_start();
}

//

any_map_t *ui_children;
any_map_t *ui_nodes_custom_buttons;
i32        pipes_offset;
char      *strings_check_internet_connection() {
    return "";
}
void console_trace(char *s) {
    fprintf(stderr, "%s\n", s);
}
void  console_error(char *s) {}
void  console_info(char *s) {}
char *tr(char *id, any_map_t *vars) {
    return id;
}
i32 pipes_get_constant_location(char *s) {
    return 0;
}