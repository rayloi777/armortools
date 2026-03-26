#include "game_engine.h"
#include "core/game_loop.h"
#include "core/runtime_api.h"
#include "core/system_api.h"
#include "ecs/ecs_world.h"
#include "ecs/ecs_components.h"
#include "ecs/ecs_dynamic.h"
#include "ecs/ecs_bridge.h"
#include <iron.h>
#include <minic.h>
#include <stdio.h>
#include <stdbool.h>
#include <gc.h>

static game_world_t *g_world = NULL;
static bool g_initialized = false;

static void load_and_run_script(const char *path) {
    iron_file_reader_t reader;
    if (iron_file_reader_open(&reader, path, IRON_FILE_TYPE_ASSET)) {
        size_t size = iron_file_reader_size(&reader);
        char *script = malloc(size + 1);
        if (script) {
            iron_file_reader_read(&reader, script, size);
            script[size] = '\0';
            minic_ctx_t *ctx = minic_eval(script);
            minic_ctx_free(ctx);
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
    ecs_dynamic_init();
    runtime_api_set_world(g_world);
    runtime_api_register();
    
    game_loop_init(g_world);
    
    _iron_set_update_callback(game_loop_update);
    g_initialized = true;
}

void game_engine_shutdown(void) {
    if (!g_initialized) return;
    printf("Game Engine Shutting Down...\n");
    
    game_loop_shutdown();
    ecs_dynamic_shutdown();
    game_world_destroy(g_world);
    g_world = NULL;
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
        .title = "Iron Game Engine",
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
    load_and_run_script("data/game.minic");
    _iron_set_update_callback(game_loop_update);
    game_engine_start();
}
