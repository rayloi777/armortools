#pragma once

#include <stdint.h>

struct game_world_t;

void sprite_bridge_set_world(struct game_world_t *world);
void sprite_bridge_init(void);
void sprite_bridge_shutdown(void);

void sprite_bridge_sync_sprite(uint64_t entity);
void sprite_bridge_create_sprite(uint64_t entity);
void sprite_bridge_set_texture(uint64_t entity, const char *path);
void sprite_bridge_destroy_sprite(uint64_t entity);

void sprite_bridge_render_all(void);
