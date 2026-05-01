// Host program for HashLink Iron Engine
// Compiles with Iron engine sources (unity build).
// Embeds HashLink VM, registers bridge primitives, loads Haxe module.
// Iron owns the main loop; Haxe closures are called from Iron callbacks.

#include <hl.h>
#include <iron.h>
#include <stdio.h>
#include <stdlib.h>

// --- Bridge registration (defined in iron_bridge.c) ---
extern void iron_register_all_prims(void);

// --- Stored Haxe callbacks ---
static vclosure *g_haxe_render_cb = NULL;
static vclosure *g_haxe_update_cb = NULL;
static vdynamic *g_haxe_exc = NULL;

// --- Iron callback implementations ---
static void host_render_callback(void) {
	if (g_haxe_render_cb != NULL) {
		hl_dyn_call_safe(g_haxe_render_cb, NULL, 0, &g_haxe_exc);
		if (g_haxe_exc != NULL) {
			fprintf(stderr, "Haxe render exception: %s\n",
			    hl_to_string(g_haxe_exc));
			g_haxe_exc = NULL;
		}
	}
}

static void host_update_callback(void) {
	if (g_haxe_update_cb != NULL) {
		hl_dyn_call_safe(g_haxe_update_cb, NULL, 0, &g_haxe_exc);
		if (g_haxe_exc != NULL) {
			fprintf(stderr, "Haxe update exception: %s\n",
			    hl_to_string(g_haxe_exc));
			g_haxe_exc = NULL;
		}
	}
}

// --- Iron bridge functions (called from Haxe via DEFINE_PRIM) ---
// These are exposed to Haxe through hl_register_prim() in iron_bridge.c.
// Storage for callbacks set from Haxe:

void iron_bridge_set_render_closure(vclosure *cb) {
	g_haxe_render_cb = cb;
}

void iron_bridge_set_update_closure(vclosure *cb) {
	g_haxe_update_cb = cb;
}

// --- Required Iron stubs (needed by iron.h internals) ---
any_map_t *ui_children;
any_map_t *ui_nodes_custom_buttons;
i32        pipes_offset;

char *strings_check_internet_connection() { return ""; }
void  console_error(char *s) {}
void  console_info(char *s) {}
char *tr(char *id) { return id; }
i32   pipes_get_constant_location(char *s) { return 0; }

// --- Entry point ---
void _kickstart() {
	// 1. Configure and create Iron window
	iron_window_options_t *ops =
	    GC_ALLOC_INIT(iron_window_options_t, {
	        .title      = "HL Iron",
	        .width      = 1280,
	        .height     = 720,
	        .x          = -1,
	        .y          = -1,
	        .features   = IRON_WINDOW_FEATURES_RESIZABLE | IRON_WINDOW_FEATURES_MINIMIZABLE
	                     | IRON_WINDOW_FEATURES_MAXIMIZABLE,
	        .mode       = IRON_WINDOW_MODE_WINDOW,
	        .frequency  = 60,
	        .vsync      = true,
	        .depth_bits = 32,
	    });
	sys_start(ops);

	// 2. Initialize render dimensions before first frame
	render_path_current_w = sys_w();
	render_path_current_h = sys_h();

	// 3. Initialize HashLink VM
	hl_global_init();
	char *argv[] = {"hl_iron", NULL};
	int   argc   = 1;
	hl_sys_init(&argc, argv);

	// 4. Register all bridge primitives
	iron_register_all_prims();

	// 5. Load Haxe module
	hl_module *m = hl_module_load("hlboot.hl", true);
	if (m == NULL) {
		fprintf(stderr, "Failed to load hlboot.hl\n");
		return;
	}

	// 6. Initialize and JIT-compile the module
	hl_module_init(m, NULL);

	// 7. Set up exception handler and call Haxe main
	hl_code *code = m->code;
	hl_trap(ctx, g_haxe_exc, on_error);

	// Call module init extras (runs Haxe __init__ blocks)
	hl_module_init_extra(m);

	// 8. Set Iron callbacks — Haxe main() will have registered closures
	//    via iron_bridge_set_render_closure/update_closure
	gc_unroot(render_path_commands);
	render_path_commands = host_render_callback;
	gc_root(render_path_commands);

	_iron_set_update_callback(host_update_callback);

	// 9. Enter Iron main loop (blocks until window closes)
	iron_start();

	// Cleanup (reached after iron_start returns on window close)
	hl_global_free();
	return;

on_error:
	fprintf(stderr, "Haxe fatal exception: %s\n",
	    hl_to_string(g_haxe_exc));
	hl_global_free();
}
