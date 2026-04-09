#include "game_loop.h"
#include "ecs/ecs_world.h"
#include "ecs/camera_bridge.h"
#include "ecs/render2d_bridge.h"
#include "ecs/camera_bridge_3d.h"
#include "ecs/mesh_bridge_3d.h"
#include "ecs/render3d_bridge.h"
#include "minic_system.h"
#include "ui_ext_api.h"
#include <iron.h>
#include <iron_draw.h>
#include <stdio.h>
#include <stdbool.h>
#include <iron_gc.h>
#ifdef IRON_MACOS
#include <mach/mach.h>
#endif

static float get_rss_mb(void) {
#ifdef IRON_MACOS
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) != KERN_SUCCESS) {
        return 0.0f;
    }
    return (float)info.resident_size / (1024.0f * 1024.0f);
#else
    return 0.0f;
#endif
}

static game_world_t *g_loop_world = NULL;
static float g_delta_time = 0.0f;
static float g_time = 0.0f;
static uint64_t g_frame_count = 0;

void game_loop_init(game_world_t *world) {
    g_loop_world = world;
    g_delta_time = 0.0f;
    g_time = 0.0f;
    g_frame_count = 0;
}

void game_loop_shutdown(void) {
    g_loop_world = NULL;
}

void game_loop_update(void) {
    if (!g_loop_world) {
        return;
    }
    g_delta_time = sys_delta();
    g_time += g_delta_time;
    g_frame_count++;

    game_world_progress(g_loop_world, g_delta_time);
    minic_system_call_step();

    // Sync 3D transforms BEFORE rendering so GPU gets up-to-date data
    camera_bridge_3d_update();
    mesh_bridge_3d_sync_transforms();

    sys_render();

    // If 3D was rendered, don't clear framebuffer — 2D draws on top of 3D
    bool has_3d = sys_3d_was_rendered();
    draw_begin(NULL, !has_3d, has_3d ? 0 : 0xff1a1a2e);

    camera2d_update(camera_bridge_get_camera(), g_delta_time);
    sys_2d_draw();
    minic_system_call_draw();
    minic_system_call_draw_ui();
    draw_end();

    // Iron UI widgets (windows, menubars, trees, etc.)
    ui_ext_api_update_input();
    ui_ext_api_begin();
    minic_system_call_draw_ui_ext();
    ui_ext_api_end();
    sys_3d_reset_frame();

    // Periodic GC collection and memory diagnostics (every 120 frames ≈ 2 sec)
    if (g_frame_count % 120 == 0) {
        gc_run();
        printf("[mem] frame %llu  RSS: %.1f MB\n",
            (unsigned long long)g_frame_count, get_rss_mb());
    }
}

float game_loop_get_delta_time(void) {
    return g_delta_time;
}

float game_loop_get_time(void) {
    return g_time;
}
uint64_t game_loop_get_frame_count(void) {
    return g_frame_count;
}
