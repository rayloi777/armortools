#include "sprite_bridge.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "ecs_bridge.h"
#include "flecs.h"
#include "../core/sprite_api.h"
#include <stdio.h>

void sprite_bridge_set_world(game_world_t *world) {
    ecs_bridge_set_world(world);
}

void sprite_bridge_init(void) {
    printf("Sprite Bridge initialized\n");
}

void sprite_bridge_shutdown(void) {
    printf("Sprite Bridge shutdown\n");
}

void sprite_bridge_sync_sprite(uint64_t entity) {
    (void)entity;
}

void sprite_bridge_set_texture(uint64_t entity, const char *path) {
    game_world_t *world = ecs_bridge_get_world();
    if (!world || !entity || !path) return;
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(world);
    if (!ecs) return;
    comp_2d_sprite *sprite = (comp_2d_sprite *)ecs_get_id(ecs, (ecs_entity_t)entity, ecs_component_comp_2d_sprite());
    if (sprite) {
        sprite->texture_path = (char *)path;
    }
}

void sprite_bridge_create_sprite(uint64_t entity) {
    game_world_t *world = ecs_bridge_get_world();
    if (!world || !entity) return;
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(world);
    if (!ecs) return;
    comp_2d_sprite *sprite = (comp_2d_sprite *)ecs_get_id(ecs, (ecs_entity_t)entity, ecs_component_comp_2d_sprite());
    if (!sprite) return;
    // Always reload if texture_path is set (render_object may contain garbage)
    if (sprite->texture_path != NULL) {
        gpu_texture_t *tex = data_get_image(sprite->texture_path);
        if (!tex) {
            fprintf(stderr, "Sprite Bridge: failed to load texture '%s'\n", sprite->texture_path);
            return;
        }
        sprite->render_object = (void *)tex;
        // Use >= 1.0f threshold: Flecs doesn't zero-init components,
        // so src_width/src_height may contain small garbage values
        float src_w = sprite->src_width >= 1.0f ? sprite->src_width : (float)tex->width;
        float src_h = sprite->src_height >= 1.0f ? sprite->src_height : (float)tex->height;
        sprite->width = src_w * sprite->scale_x;
        sprite->height = src_h * sprite->scale_y;
    }
}

void sprite_bridge_destroy_sprite(uint64_t entity) {
    game_world_t *world = ecs_bridge_get_world();
    if (!world || !entity) return;
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(world);
    if (!ecs) return;
    comp_2d_sprite *sprite = (comp_2d_sprite *)ecs_get_id(ecs, (ecs_entity_t)entity, ecs_component_comp_2d_sprite());
    if (sprite) {
        sprite->render_object = NULL;
    }
}
