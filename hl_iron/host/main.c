// Host program for HashLink Iron Engine
// Compiles with Iron engine sources + HashLink sources (unity build).
// Iron owns the main loop; Haxe closures are called from Iron callbacks.
// HDLL (iron.hdll) is loaded automatically by HashLink's module system.

#include <hl.h>
#include <hlmodule.h>
#include <iron.h>
#include <stdio.h>
#include <stdlib.h>

// --- Stored Haxe callbacks ---
static vclosure *g_haxe_render_cb = NULL;
static vclosure *g_haxe_update_cb = NULL;
static vdynamic *g_haxe_exc = NULL;
static hl_module *g_hl_module = NULL;
static hl_code   *g_hl_code = NULL;

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

// --- Called from Haxe via DEFINE_PRIM in iron.hdll ---
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

// --- Load .hl bytecode file ---
static hl_code *load_code(const char *file, char **error_msg) {
	hl_code *code;
	FILE *f = fopen(file, "rb");
	int pos, size;
	char *fdata;
	if (f == NULL) {
		*error_msg = "File not found";
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	size = (int)ftell(f);
	fseek(f, 0, SEEK_SET);
	fdata = (char *)malloc(size);
	pos = 0;
	while (pos < size) {
		int r = (int)fread(fdata + pos, 1, size - pos, f);
		if (r <= 0) {
			*error_msg = "Read error";
			free(fdata);
			fclose(f);
			return NULL;
		}
		pos += r;
	}
	fclose(f);
	code = hl_code_read(fdata, size, error_msg);
	free(fdata);
	return code;
}

// --- Entry point ---
void _kickstart() {
	char *error_msg = NULL;
	vclosure cl;

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
	vdynamic *ret;
	hl_register_thread(&ret);

	char *argv[] = {"hl_iron", NULL};
	int   argc   = 1;
	hl_setup.sys_args = argv + 1;
	hl_setup.sys_nargs = argc - 1;
	hl_sys_init();

	// 4. Load Haxe bytecode
	g_hl_code = load_code("hlboot.hl", &error_msg);
	if (g_hl_code == NULL) {
		fprintf(stderr, "Failed to load hlboot.hl: %s\n",
		    error_msg ? error_msg : "unknown error");
		return;
	}

	// 5. Allocate and initialize module (JIT compile)
	//    This resolves @:hlNative("iron") by loading iron.hdll
	g_hl_module = hl_module_alloc(g_hl_code);
	if (g_hl_module == NULL) {
		fprintf(stderr, "Failed to allocate HashLink module\n");
		return;
	}
	if (!hl_module_init(g_hl_module, false)) {
		fprintf(stderr, "Failed to initialize HashLink module\n");
		return;
	}

	// 6. Call Haxe entry point
	cl.t = g_hl_code->functions[g_hl_module->functions_indexes[g_hl_code->entrypoint]].type;
	cl.fun = g_hl_module->functions_ptrs[g_hl_code->entrypoint];
	cl.hasValue = 0;

	bool is_exc = false;
	hl_dyn_call_safe(&cl, NULL, 0, &is_exc);
	if (is_exc) {
		fprintf(stderr, "Haxe entry point exception\n");
		return;
	}

	// 7. Set Iron callbacks — Haxe main() will have registered closures
	//    via iron_bridge_set_render_closure/update_closure
	gc_unroot(render_path_commands);
	render_path_commands = host_render_callback;
	gc_root(render_path_commands);

	_iron_set_update_callback(host_update_callback);

	// 8. Free bytecode (module has what it needs)
	hl_code_free(g_hl_code);

	// 9. Enter Iron main loop (blocks until window closes)
	iron_start();

	// Cleanup
	hl_module_free(g_hl_module);
	hl_global_free();
}
