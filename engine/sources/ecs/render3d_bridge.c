#include "render3d_bridge.h"
#include "ecs_world.h"
#include <iron.h>
#include <stdio.h>
#include <stdbool.h>

// Whether any 3D content has been rendered this frame.
// When true, the 2D overlay should NOT clear the framebuffer.
static bool g_3d_rendered = false;

static void sys_3d_render_commands(void) {
    // Render to framebuffer with depth+color clear
    render_path_set_target("", NULL, NULL, GPU_CLEAR_COLOR | GPU_CLEAR_DEPTH, 0xff1a1a2e, 1.0f);

    // Draw all scene meshes using their "mesh" material context.
    // render_path_draw_meshes internally calls render_path_end().
    render_path_draw_meshes("mesh");

    g_3d_rendered = true;
}

void sys_3d_set_world(game_world_t *world) {
    (void)world;
}

void sys_3d_init(void) {
    render_path_commands = sys_3d_render_commands;
    printf("3D Render Bridge: initialized (forward rendering)\n");
}

void sys_3d_shutdown(void) {
    render_path_commands = NULL;
    g_3d_rendered = false;
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
