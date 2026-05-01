#pragma once

// Bridge registration function — called by host before loading Haxe module.
void iron_register_all_prims(void);

// Callback storage — called by host to store Haxe closures.
extern void iron_bridge_set_render_closure(vclosure *cb);
extern void iron_bridge_set_update_closure(vclosure *cb);
