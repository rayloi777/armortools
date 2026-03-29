#include "sprite_bridge.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "ecs_bridge.h"
#include "flecs.h"
#include "../core/sprite_api.h"
#include <stdio.h>
#include <string.h>

void sprite_bridge_set_world(game_world_t *world) {
	ecs_bridge_set_world(world);
}

void sprite_bridge_init(void) {
    printf("Sprite Bridge initialized (no system)\n");
}

void sprite_bridge_shutdown(void) {
    printf("Sprite Bridge shutdown\n");
}
void sprite_bridge_sync_sprite(uint64_t entity) {
    (void)entity;
}
void sprite_bridge_create_sprite(uint64_t entity) {
    game_world_t *world = ecs_bridge_get_world();
    if (!world || !entity) return;
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(world);
    if (!ecs) return;
    comp_2d_sprite *sprite = (comp_2d_sprite *)ecs_get_id(ecs, (ecs_entity_t)entity, ecs_component_comp_2d_sprite());
    if (!sprite || sprite->render_object != NULL) return;
    if (sprite->texture_path != NULL) {
        sprite_renderer_t *sr = sprite_renderer_create();
        sprite_renderer_set_texture(sr, sprite->texture_path);
        if (sprite->src_width > 0 && sprite->src_height > 0) {
            sprite_renderer_set_region(sr, sprite->region_x, sprite->region_y,
                                     sprite->src_width, sprite->src_height);
        }
        sprite->render_object = sr;
        sprite->width = sprite_renderer_get_width(sr);
        sprite->height = sprite_renderer_get_height(sr);
    }
}
void sprite_bridge_destroy_sprite(uint64_t entity) {
    game_world_t *world = ecs_bridge_get_world();
    if (!world || !entity) return;
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(world);
    if (!ecs) return;
    comp_2d_sprite *sprite = (comp_2d_sprite *)ecs_get_id(ecs, (ecs_entity_t)entity, ecs_component_comp_2d_sprite());
    if (sprite && sprite->render_object != NULL) {
        sprite_renderer_destroy((sprite_renderer_t *)sprite->render_object);
        sprite->render_object = NULL;
    }
}
