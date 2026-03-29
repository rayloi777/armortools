#include "sprite_bridge.h"
#include "ecs_world.h"
#include "ecs_components.h"
#include "ecs_bridge.h"
#include "flecs.h"
#include "../core/sprite_api.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static FILE* s_debug_log = NULL;

static void sb_debug_log_init(void) {
    if (!s_debug_log) {
        s_debug_log = fopen("/tmp/sprite_bridge_debug.log", "w");
    }
}

static void sb_debug_log(const char *fmt, ...) {
    sb_debug_log_init();
    if (s_debug_log) {
        va_list args;
        va_start(args, fmt);
        vfprintf(s_debug_log, fmt, args);
        va_end(args);
        fprintf(s_debug_log, "\n");
        fflush(s_debug_log);
    }
}

static ecs_entity_t g_sprite_bridge_system = 0;
static ecs_query_t *g_sprite_query = NULL;

static void sprite_bridge_system(ecs_iter_t *it) {
    TransformPosition *pos = ecs_field(it, TransformPosition, 0);
    RenderSprite *sprite = ecs_field(it, RenderSprite, 1);
    
    for (int i = 0; i < it->count; i++) {
        ecs_entity_t e = it->entities[i];
        if (!e) continue;
        
        ecs_entity_t pos_src = ecs_field_src(it, 0);
        ecs_entity_t sprite_src = ecs_field_src(it, 1);
        
        if (pos_src != e || sprite_src != e) continue;
        
        if (sprite[i].render_object == NULL && sprite[i].texture_path != NULL) {
            sprite_renderer_t *sr = sprite_renderer_create();
            sprite_renderer_set_texture(sr, sprite[i].texture_path);
            
            if (sprite[i].src_width > 0 && sprite[i].src_height > 0) {
                sprite_renderer_set_region(sr, sprite[i].region_x, sprite[i].region_y, 
                                         sprite[i].src_width, sprite[i].src_height);
            }
            
            sprite[i].render_object = sr;
            sprite[i].width = sprite_renderer_get_width(sr);
            sprite[i].height = sprite_renderer_get_height(sr);
        }
        
        if (sprite[i].render_object != NULL) {
            sprite_renderer_t *sr = (sprite_renderer_t *)sprite[i].render_object;
            if (!sr) continue;
            
            sprite_renderer_set_position(sr, pos[i].x, pos[i].y);
            sprite_renderer_set_scale(sr, sprite[i].scale_x, sprite[i].scale_y);
            sprite_renderer_set_flip(sr, sprite[i].flip_x, sprite[i].flip_y);
            sprite_renderer_set_layer(sr, sprite[i].layer);
            sprite_renderer_set_visible(sr, sprite[i].visible);
            sprite_renderer_set_pivot(sr, sprite[i].pivot_x, sprite[i].pivot_y);
            
            if (sprite[i].src_width > 0 && sprite[i].src_height > 0) {
                sprite_renderer_set_region(sr, sprite[i].region_x, sprite[i].region_y,
                                         sprite[i].src_width, sprite[i].src_height);
            }
            
            sprite[i].width = sprite_renderer_get_width(sr);
            sprite[i].height = sprite_renderer_get_height(sr);
        }
    }
}

void sprite_bridge_set_world(game_world_t *world) {
    ecs_bridge_set_world(world);
}

void sprite_bridge_init(void) {
    game_world_t *world = ecs_bridge_get_world();
    if (!world) {
        sb_debug_log("Sprite Bridge: game world not set");
        return;
    }
    
    ecs_world_t *ecs = (ecs_world_t *)game_world_get_ecs(world);
    if (!ecs) {
        sb_debug_log("Failed to get ECS world for sprite bridge");
        return;
    }
    
    ecs_system_desc_t sys_desc = {0};
    sys_desc.entity = 0;
    sys_desc.query.terms[0].id = ecs_component_TransformPosition();
    sys_desc.query.terms[1].id = ecs_component_RenderSprite();
    sys_desc.callback = sprite_bridge_system;
    
    g_sprite_bridge_system = ecs_system_init(ecs, &sys_desc);
    
    if (g_sprite_bridge_system == 0) {
        sb_debug_log("Failed to create sprite bridge system");
        return;
    }
    
    ecs_add_pair(ecs, g_sprite_bridge_system, EcsDependsOn, EcsPreUpdate);
    
    ecs_query_desc_t qdesc = {0};
    qdesc.terms[0].id = ecs_component_TransformPosition();
    qdesc.terms[1].id = ecs_component_RenderSprite();
    g_sprite_query = ecs_query_init(ecs, &qdesc);
    
    printf("Sprite Bridge: system entity=%llu\n", (unsigned long long)g_sprite_bridge_system);
    fflush(stdout);
    
    printf("Sprite Bridge initialized\n");
    printf("Sprite Bridge: TransformPosition id=%llu, RenderSprite id=%llu\n", 
           (unsigned long long)ecs_component_TransformPosition(),
           (unsigned long long)ecs_component_RenderSprite());
    fflush(stdout);
}

void sprite_bridge_shutdown(void) {
    if (g_sprite_query) {
        ecs_query_fini(g_sprite_query);
        g_sprite_query = NULL;
    }
    g_sprite_bridge_system = 0;
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
    
    RenderSprite *sprite = (RenderSprite *)ecs_get_id(ecs, (ecs_entity_t)entity, ecs_component_RenderSprite());
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
    
    RenderSprite *sprite = (RenderSprite *)ecs_get_id(ecs, (ecs_entity_t)entity, ecs_component_RenderSprite());
    if (sprite && sprite->render_object != NULL) {
        sprite_renderer_destroy((sprite_renderer_t *)sprite->render_object);
        sprite->render_object = NULL;
    }
}

void sprite_bridge_render_all(void) {
    static int debug_call = 0;
    sb_debug_log("[sprite_bridge_render_all] called #%d", debug_call++);
    
    game_world_t *world = ecs_bridge_get_world();
    if (!world) {
        sb_debug_log("[sprite_bridge_render_all] world is NULL");
        return;
    }
    if (!world->world) {
        sb_debug_log("[sprite_bridge_render_all] world->world is NULL");
        return;
    }
    
    ecs_world_t *ecs = (ecs_world_t *)world->world;
    
    static int debug_frame = 0;
    
    ecs_query_desc_t qdesc = {0};
    qdesc.terms[0].id = ecs_component_TransformPosition();
    qdesc.terms[1].id = ecs_component_RenderSprite();
    ecs_query_t *q = ecs_query_init(ecs, &qdesc);
    if (!q) return;
    
    int total_entities = 0;
    int created_sprites = 0;
    int updated_sprites = 0;
    
    ecs_iter_t it = ecs_query_iter(ecs, q);
    
    while (ecs_query_next(&it)) {
        TransformPosition *pos = ecs_field(&it, TransformPosition, 0);
        RenderSprite *sprite = ecs_field(&it, RenderSprite, 1);
        
        total_entities += it.count;
        
        for (int i = 0; i < it.count; i++) {
            if (sprite[i].render_object == NULL && sprite[i].texture_path != NULL) {
                sprite_renderer_t *sr = sprite_renderer_create();
                sprite_renderer_set_texture(sr, sprite[i].texture_path);
                if (sprite[i].src_width > 0 && sprite[i].src_height > 0) {
                    sprite_renderer_set_region(sr, sprite[i].region_x, sprite[i].region_y, 
                                             sprite[i].src_width, sprite[i].src_height);
                }
                sprite[i].render_object = sr;
                created_sprites++;
                
                if (debug_frame < 3) {
                    sb_debug_log("  [sprite_bridge] Created sprite for entity %llu, pos=(%.1f, %.1f), tex=%s",
                            (unsigned long long)it.entities[i], pos[i].x, pos[i].y, sprite[i].texture_path);
                }
            }
            else if (sprite[i].render_object != NULL && sprite_renderer_is_valid(sprite[i].render_object)) {
                sprite_renderer_t *sr = (sprite_renderer_t *)sprite[i].render_object;
                if (sr) {
                    sprite_renderer_set_position(sr, pos[i].x, pos[i].y);
                    sprite_renderer_set_scale(sr, sprite[i].scale_x, sprite[i].scale_y);
                    sprite_renderer_set_flip(sr, sprite[i].flip_x, sprite[i].flip_y);
                    sprite_renderer_set_layer(sr, sprite[i].layer);
                    sprite_renderer_set_visible(sr, sprite[i].visible);
                    sprite_renderer_set_pivot(sr, sprite[i].pivot_x, sprite[i].pivot_y);
                    if (sprite[i].src_width > 0 && sprite[i].src_height > 0) {
                        sprite_renderer_set_region(sr, sprite[i].region_x, sprite[i].region_y,
                                                 sprite[i].src_width, sprite[i].src_height);
                    }
                    updated_sprites++;
                    
                    if (debug_frame < 3 && i < 3) {
                        sb_debug_log("  [sprite_bridge] Updated sprite[%d] pos=(%.1f, %.1f)",
                                i, pos[i].x, pos[i].y);
                    }
                }
            }
        }
    }
    
    ecs_query_fini(q);
    
    if (debug_frame < 3) {
        sb_debug_log("[sprite_bridge_render_all] frame=%d entities=%d created=%d updated=%d total_sprites=%d",
                debug_frame, total_entities, created_sprites, updated_sprites, sprite_renderer_count);
    }
    debug_frame++;
    
    sprite_renderer_sort_by_layer();
    
    for (int i = 0; i < sprite_renderer_count; i++) {
        if (sprite_renderers[i] && sprite_renderer_is_visible(sprite_renderers[i])) {
            sprite_renderer_draw(sprite_renderers[i]);
        }
    }
}
