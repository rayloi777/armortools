#pragma once

#include "../ecs_world.h"

void decal_bridge_init(game_world_t *world);
void decal_bridge_shutdown(void);

// Render all decals as fullscreen post-process onto the given render target.
// Must be called within a gpu_begin/gpu_end block. Reads depth and scene textures.
void decal_bridge_render(gpu_texture_t *depth_tex, gpu_texture_t *scene_tex, int w, int h);
