# Minic Plugin Test

A test project for the minic C interpreter plugin system.

## Building

```bash
cd base/tests/plugin
../../make
```

## Running

```bash
../../make --run
```

## Minic C Interpreter - Technical Documentation

### Core Architecture

**Execution Flow** (`minic_eval_named()` in `minic.c:2304-2382`):

1. Allocate 8MB memory arena (`minic_ctx_t`)
2. Copy source code to `ctx->src_copy`
3. **Pass 1**: Scan for struct/enum definitions
4. **Pass 2**: Register function signatures (skip bodies)
5. **Pass 3**: Execute top-level code and function bodies

### Function Registration System

| Signature | Type |
|----------|------|
| `i` | MINIC_T_INT |
| `f` | MINIC_T_FLOAT |
| `p` | MINIC_T_PTR |
| `b` | MINIC_T_BOOL |
| `c` | MINIC_T_CHAR |
| `v` | **MINIC_T_INT** (void treated as int, returns 0) |

**Signature Format**: `return_type(param1,param2,...)`

Examples:
- `"v(f,f,f,f)"` - `void func(float, float, float, float)`
- `"i(p)"` - `int func(void*)`
- `"f(p,f,f)"` - `float func(void*, float, float)`

### Why 'v' (void) Returns INT

**Location**: `minic_ext.c:129-143`

```c
static minic_type_t minic_sig_char(char c) {
    switch (c) {
        case 'i': return MINIC_T_INT;
        case 'f': return MINIC_T_FLOAT;
        case 'p': return MINIC_T_PTR;
        case 'b': return MINIC_T_BOOL;
        case 'c': return MINIC_T_CHAR;
        default:  return MINIC_T_INT; // 'v' -> INT/0 for void
    }
}
```

Minic stores all values in `minic_val_t` union - there's no native "void". The dispatch function returns `minic_val_int(0)` for void-returning functions.

### Context Lifecycle

```c
struct minic_ctx_s {
    minic_u8   *mem;      // 8MB memory arena
    int         mem_used;  // Current arena usage
    minic_env_t e;        // Execution environment
    float       result;    // Last evaluation result
    char       *src_copy; // Source code copy
};
```

**Critical**: `minic_func_t` contains a pointer back to its owning context. **If context is freed, function pointers become dangling!**

```c
// CORRECT - Keep ctx alive
g_plugin_ctx = minic_eval_named(...);
minic_ctx_call_fn(g_plugin_ctx, fn_ptr, args, argc);  // ✅

// WRONG - Free immediately
minic_ctx_t *ctx = minic_eval_named(...);
minic_ctx_free(ctx);  // ❌ fn_ptr is now dangling!
```

### Calling Callbacks

```c
// Correct way to call plugin callbacks from C
minic_ctx_call_fn(ctx, fn_ptr, args, argc);

// When function is stored as void*
minic_ext_func_t *ext = minic_ext_func_get("on_ui_callback");
if (ext != NULL) {
    minic_call_fn(ext, args, argc);  // Uses fn->ctx internally
}
```

### Native Functions

```c
// minic_register: Register normal C function
minic_register("draw_string", "v(p,f,f)", (minic_ext_fn_raw_t)draw_string);

// minic_register_native: Register native function (bypasses type dispatch)
minic_register_native("printf", minic_printf_native);

// printf signature: v(p,...) - void return, NOT SUPPORTED in plugins!
```

### Paint Plugin Lifecycle

```
1. plugin_start() → minic_eval_named() → ctx stored in plugin_t
2. Plugin main() → Registers on_ui, on_update callbacks (stores function pointers)
3. Per frame: minic_ctx_call_fn(ctx, on_ui, ...) → callbacks invoked
4. plugin_stop() → Call on_delete → minic_ctx_free(ctx)
```

### minic_ctx_t Memory Layout

```
┌─────────────────────────────────────────────────────────────┐
│                     minic_ctx_t                              │
│  ┌──────────────┐  ┌─────────────────────────────────────┐ │
│  │ mem[8MB]     │  │ minic_env_t                          │ │
│  │ (arena)      │  │  ├── vars[] - local variables        │ │
│  │              │  │  ├── arrs[] - array descriptors      │ │
│  │              │  │  ├── arr_data[] - array element data  │ │
│  │              │  │  ├── funcs[] - function defs           │ │
│  │              │  │  ├── structs[] - struct definitions    │ │
│  │              │  │  └── vartypes[] - var->struct mapping   │ │
│  └──────────────┘  └─────────────────────────────────────┘ │
│  ├── result       └── lex.src ──────────────────────────────┤
│  └── src_copy ──────────────────────────────────────────────┘
└─────────────────────────────────────────────────────────────┘
```

### minic_ext_funcs Registry

```
┌─────────────────────────────────────────────────────────────┐
│ minic_ext_funcs[] (global registry)                        │
│  ├── name: "printf"     sig: "v(p,...)"  fn: raw_ptr      │
│  ├── name: "ui_button"  sig: "b(p,p,i)"  fn: raw_ptr      │
│  └── name: "vec4_create" sig: "p(f,f,f,f)" native_fn      │
└─────────────────────────────────────────────────────────────┘
                              ▲
                              │
┌─────────────────────────────────────────────────────────────┐
│ minic_dispatch(ext_func_t*, args[], argc)                   │
│  1. If native_fn set: call native_fn(args, argc)            │
│  2. Else: coerce args to C types, call fn via function ptr  │
└─────────────────────────────────────────────────────────────┘
```

## Limitations

### Void-Returning Functions Not Supported

Minic does not support void-returning native functions. The signature `'v'` is treated as `MINIC_T_INT`. This means:

- `draw_*` functions (e.g., `draw_set_color`, `draw_filled_rect`, `draw_string`) **cannot** be used directly in minic plugins
- These functions return `void` but minic expects `int`

### Workaround

Use only value-returning functions in minic plugins:

```c
// NOT WORKING - void return
void on_ui() {
    draw_set_color(0xffffffff);  // ❌
    draw_filled_rect(50, 50, 100, 100);  // ❌
}

// WORKING - float return (even if unused)
float on_ui() {
    // Just do calculations, let C side handle rendering
    return 1.0;
}
```

For full UI plugins with actual rendering, use the **Paint application** which has complete UI initialization.

## Files

- `main.c` - Test harness that loads and executes minic plugins
- `assets/test_plugin.c` - Example minic plugin (simplified, non-rendering)
