#include "game_engine.h"
#include "core/engine_world.h"
#include "core/game_loop.h"
#include "core/runtime_api.h"
#include "core/system_api.h"
#include "core/minic_system.h"
#include "core/sprite_api.h"
#include "ecs/ecs_world.h"
#include "ecs/ecs_components.h"
#include "ecs/ecs_dynamic.h"
#include "ecs/ecs_bridge.h"
#include "ecs/sprite_bridge.h"
#include "ecs/camera_bridge.h"
#include "ecs/render2d_bridge.h"
#include <iron_input.h>
#include <iron.h>
#include <stdio.h>

static game_world_t *g_world = NULL;
static bool g_initialized = false;
static minic_ctx_t *g_script_ctx = NULL;

static void engine_keyboard_down_callback(i32 code) {
    keyboard_down_listener(code);
}

static void engine_keyboard_up_callback(i32 code) {
    keyboard_up_listener(code);
}

static void engine_mouse_down_callback(i32 button, i32 x, i32 y) {
    mouse_down_listener(button, x, y);
}

static void engine_mouse_up_callback(i32 button, i32 x, i32 y) {
    mouse_up_listener(button, x, y);
}

static void engine_mouse_move_callback(i32 x, i32 y, i32 dx, i32 dy) {
    mouse_move_listener(x, y, dx, dy);
}

minic_ctx_t *game_engine_get_minic_ctx(void) {
    return g_script_ctx;
}

static void load_and_run_script(const char *path) {
    iron_file_reader_t reader;
    if (iron_file_reader_open(&reader, path, IRON_FILE_TYPE_ASSET)) {
        size_t size = iron_file_reader_size(&reader);
        char *script = malloc(size + 1);
        if (script) {
            iron_file_reader_read(&reader, script, size);
            script[size] = '\0';
            if (g_script_ctx) {
                minic_ctx_free(g_script_ctx);
            }
            g_script_ctx = minic_ctx_create(script);
            minic_ctx_run(g_script_ctx);
            free(script);
        }
        iron_file_reader_close(&reader);
    } else {
        fprintf(stderr, "Failed to open script: %s\n", path);
    }
}

void console_trace(char *s) {
    fprintf(stderr, "%s\n", s);
}

void console_error(char *s) {
    fprintf(stderr, "ERROR: %s\n", s);
}

void console_info(char *s) {
    fprintf(stderr, "INFO: %s\n", s);
}

char *strings_check_internet_connection(void) {
    return NULL;
}

void ui_children(void) {}
void ui_nodes_custom_buttons(void) {}

char *tr(char *s) {
    return s;
}

void pipes_offset(int val) {
    (void)val;
}

void pipes_get_constant_location(int *loc) {
    (void)loc;
}

void console_log(char *s) {
    fprintf(stderr, "LOG: %s\n", s);
}

void *config_raw = NULL;
void *context_raw = NULL;
void *context_main_object = NULL;

void context_set_viewport_shader(void) {}

void import_mesh_importers(void) {}
void import_texture_importers(void) {}

void game_engine_init(void) {
    system_api_init();

    g_world = game_world_create();
    engine_world_set(g_world);

    ecs_bridge_set_world(g_world);
    ecs_bridge_init();

    sprite_bridge_set_world(g_world);
    sprite_bridge_init();

    camera_bridge_set_world(g_world);
    camera_bridge_init();

    sys_2d_set_world(g_world);
    sys_2d_init();

    ecs_dynamic_field_cache_build();
    ecs_dynamic_comp_id_cache_build();

    runtime_api_set_world(g_world);
    runtime_api_register();

    input_register();
    gamepad_reset();

    iron_set_keyboard_down_callback(engine_keyboard_down_callback);
    iron_set_keyboard_up_callback(engine_keyboard_up_callback);
    iron_set_mouse_down_callback(engine_mouse_down_callback);
    iron_set_mouse_up_callback(engine_mouse_up_callback);
    iron_set_mouse_move_callback(engine_mouse_move_callback);

    game_loop_init(g_world);

    _iron_set_update_callback(game_loop_update);
    g_initialized = true;
}

void game_engine_shutdown(void) {
    if (!g_initialized) return;
    printf("Game Engine Shutting Down...\n");

    sys_2d_shutdown();
    sprite_bridge_shutdown();
    sprite_texture_cache_shutdown();
    camera_bridge_shutdown();
    minic_system_unload_all();
    game_loop_shutdown();
    ecs_dynamic_shutdown();
    game_world_destroy(g_world);
    g_world = NULL;
    engine_world_set(NULL);
    g_initialized = false;

    printf("Game Engine Shutdown Complete\n");
}

void game_engine_start(void) {
    if (!g_initialized) {
        fprintf(stderr, "Game engine not initialized\n");
        return;
    }
    iron_start();
}

game_world_t *game_engine_get_world(void) {
    return g_world;
}

float game_engine_get_delta_time(void) {
    return game_loop_get_delta_time();
}

float game_engine_get_time(void) {
    return game_loop_get_time();
}

uint64_t game_engine_get_frame_count(void) {
    return game_loop_get_frame_count();
}

void _kickstart(void) {
    iron_window_options_t *ops = GC_ALLOC_INIT(iron_window_options_t, {
        .title = "Game Engine",
        .x = -1,
        .y = -1,
        .width = 1280,
        .height = 720,
        .features = 0,
        .mode = 0,
        .frequency = 60,
        .vsync = true,
        .display_index = -1,
        .visible = true,
        .color_bits = 32,
        .depth_bits = 24
    });

    sys_start(ops);
    game_engine_init();

    printf("Loading Minic systems...\n");
    minic_system_load("MinicBench", "data/systems/minic_bench.minic");
    minic_system_call_init();

    game_engine_start();
}
