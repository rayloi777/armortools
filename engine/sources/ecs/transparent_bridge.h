#pragma once

#include "ecs_world.h"

void transparent_bridge_init(game_world_t *world);
void transparent_bridge_shutdown(void);

// Set opacity=0 on all transparent meshes so they're invisible in the G-buffer pass.
void transparent_bridge_hide(void);

// Render all transparent entities forward onto the current render target.
// Must be called within a gpu_begin/gpu_end block. Expects depth buffer
// from the deferred pass to be bound.
void transparent_bridge_render(void);

// Update particle lifetimes and positions. Call once per frame.
void particle_bridge_update(float dt);
