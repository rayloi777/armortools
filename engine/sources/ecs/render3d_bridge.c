#include "render3d_bridge.h"
#include "ecs_world.h"
#include "deferred/deferred_gbuffer.h"
#include <iron.h>
#include <stdio.h>
#include <stdbool.h>

// Whether any 3D content has been rendered this frame.
// When true, the 2D overlay should NOT clear the framebuffer.
static bool g_3d_rendered = false;

// Debug visualization mode: 0=Normal, 1=Depth, 2=Albedo, 3=RoughMet
static int g_debug_mode = 0;

static void sys_3d_render_commands(void) {
    int w = sys_w();
    int h = sys_h();
    if (w <= 0 || h <= 0) return;

    gbuffer_resize(w, h);
    gbuffer_t *gb = gbuffer_get();
    if (!gb || !gb->initialized) return;

    // Pass 1: Render mesh to gbuffer0
    gpu_texture_t *targets[1] = { gb->gbuffer0 };
    gpu_begin(targets, 1, gb->depth_target,
              GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH, 0xff000000, 1.0f);
    render_path_submit_draw("mesh");
    gpu_end();

    // Pass 2: Display gbuffer0 to screen
    _gpu_begin(NULL, NULL, NULL, GPU_CLEAR_COLOR, 0xff1a1a2e, 1.0f);
    if (gb->gbuffer0) {
        draw_scaled_image(gb->gbuffer0, 0.0f, 0.0f, (float)w, (float)h);
    }
    gpu_end();

    g_3d_rendered = true;
}

void sys_3d_set_world(game_world_t *world) {
    (void)world;
}

void sys_3d_init(void) {
    int w = sys_w();
    int h = sys_h();
    gbuffer_init(w > 0 ? w : 1280, h > 0 ? h : 720);
    render_path_commands = sys_3d_render_commands;
    render_path_current_w = w;
    render_path_current_h = h;
    printf("3D Render Bridge: initialized (deferred G-buffer pipeline)\n");
}

void sys_3d_shutdown(void) {
    gbuffer_shutdown();
    render_path_commands = NULL;
    g_3d_rendered = false;
    g_debug_mode = 0;
    printf("3D Render Bridge: shutdown\n");
}

void sys_3d_draw(void) {
    // 3D rendering is handled by render_path_commands callback,
    // invoked during sys_render() -> scene_render_frame().
}

bool sys_3d_was_rendered(void) {
    return g_3d_rendered;
}

void sys_3d_reset_frame(void) {
    g_3d_rendered = false;
}

void sys_3d_set_debug_mode(int mode) {
    if (mode < 0) mode = 0;
    if (mode > 3) mode = 3;
    g_debug_mode = mode;
    printf("3D Render Bridge: debug mode set to %d\n", g_debug_mode);
}

int sys_3d_get_debug_mode(void) {
    return g_debug_mode;
}
