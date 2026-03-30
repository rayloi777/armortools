# Minic + Flecs Optimization Plan

## Architecture

```
Minic Scripts (.minic)
    ↓ minic_register() / minic_register_native()
Core API (engine/sources/core/)
    ↓ operates on
Flecs ECS (engine/sources/ecs/)
    ↓ synced via bridges
Iron Engine (base/)
```

Minic is a tree-walking interpreter (~3100 lines). Every loop iteration re-tokenizes + re-parses from source. Every function call re-parses the body from source.

**Minic source files `base/sources/libs/minic*` are modifiable.**

---

## Completed Work

### Engine-Side Optimizations

| Item | What | Files |
|------|------|-------|
| Field offset cache | djb2 hash `(comp_id, field_hash) -> offset` | `ecs/ecs_dynamic.c` |
| Component ID cache | `flecs_id -> dynamic_component_t*` | `ecs/ecs_dynamic.c` |
| 64-bit ID fix | `(uint64_t)minic_val_to_d()` | `core/runtime_api.c` |
| Unified world pointer | `engine_world_set/get()` | `core/engine_world.c` |
| Streaming query iterator | `query_iter_begin/next/count/entity/comp_ptr` | `core/query_api.c` |
| Batch field APIs | `comp_add_float`, `comp_set/get/add_floats` | `core/runtime_api.c` |
| C-side iteration | `query_foreach`, `query_foreach_batch` | `core/runtime_api.c` |
| Registration consolidation | All APIs via `runtime_api_register()` | `core/runtime_api.c` |
| frog_system migration | `query_foreach + FrogVelocity + bounce physics` | `assets/systems/frog_system.minic` |

### Engine-Side ECS Benchmark (500 entities, 10 iters/frame, 30 frames)

| Phase | Pattern | Trips/eEntity | Speedup |
|-------|---------|-------------|---------|
| P2 | `comp_add_float` | 2 | ~2.0x |
| P3 | `comp_add_floats` batch | 1 | ~2.1x |
| P4 | `query` + `entity_get` | 3 | ~1.3x |
| P5 | `query` + `comp_ptr` | 2 | ~2.4x |
| P6 | `query_foreach` (C iterate + callback) | 1 callback | ~4.8x |

### Minic Core Optimizations (completed)

| Phase | What | Files Changed | Status |
|-------|------|---------------|--------|
| M1 | Ext function dispatch hash | `minic_ext.c`, `minic.h` | **Done** |
| M2 | Variable lookup hash | `minic.c` (`minic_var_t`, `minic_vartype_t`) | **Done** |
| M3 | Arena save/restore in `minic_call()` | `minic.c` | **Done** |
| M4 | Function body token cache | `minic.c` | **Done** |
| M6 | Struct field cache | `minic.c` (`minic_struct_def_t`) | **Done** |
| M7 | Lexer keyword switch | `minic.c` | **Done** |
| Hot-path | INT fast-path in `minic_arith()`, `minic_var_get_ptr()` for in-place ops | `minic.c` | **Done** |

### MinicBench Results (100K iters x 3 runs, Apple M4 Pro)

```
Post-M4 + hot-path optimizations (current baseline):
  P1  Variable get/set:   65 ms
  P2  Ext dispatch 12/i:  91 ms
  P3  Function call:      35 ms
  P4  Float math:          70 ms
  P5  Loop overhead:       18 ms
  P3-P5 (call overhead):  16 ms
  P2/P5 (dispatch/loop):  5.1 x
  P3/P5 (call/loop):      1.9 x
  P4/P5 (math/loop):      3.9 x
```

---

## M15: Bytecode VM — In Progress

### Status: Infrastructure complete, execution disabled

The register-based bytecode VM is implemented but **disabled** — all execution falls back to the stable tree-walking interpreter. The VM fast path in `minic_call()` and the compilation step in `minic_ctx_create()` are commented out.

### What's built

| Component | File | Status |
|-----------|------|--------|
| Opcode enum (55 opcodes) | `minic.c` | **Done** |
| 32-bit instruction encoding | `minic.c` | **Done** |
| VM dispatch loop | `minic.c` (`minic_vm_exec_inner`) | **Done** |
| Register allocator | `minic_vm_compiler.c` (`vc_reg`) | **Done** |
| Expression compiler | `minic_vm_compiler.c` | **Done** |
| Statement compiler | `minic_vm_compiler.c` | **Done** |
| Globals sync | `minic.c` (`vm_globals_load/store`) | **Done** |
| Constant pool | `minic_vm_compiler.c` | **Done** |
| Call frame stack | `minic.c` (`minic_frame_t`) | **Done** |

### Known bugs blocking activation

1. **Ext function dispatch passes NULL pointers**: The compiled ext call (`OP_CALL_EXT`) generates incorrect register assignments. String literal arguments (e.g. `"font.ttf"` passed to `draw_string`) arrive as NULL pointers, causing SIGSEGV in `draw_string`.

2. **For-loop increment not compiled**: The compiler skips over the increment expression tokens but doesn't emit bytecode for it. `i = i + 1` increments are lost.

3. **Argument register allocation**: Args are compiled left-to-right but may not land in consecutive registers as expected by the `OP_CALL_EXT` encoding.

### To re-enable

1. Add a bytecode disassembler (`vm_disasm()`) to debug what the compiler emits
2. Fix ext func argument passing — ensure string literals are loaded into correct registers before the call
3. Implement for-loop increment compilation
4. Uncomment the fast path in `minic_call()` and the compilation step in `minic_ctx_create()`
5. Run MinicBench to measure speedup

### Files

| File | Description |
|------|-------------|
| `base/sources/libs/minic.c` | VM dispatch, opcodes, globals, frame stack |
| `base/sources/libs/minic_vm_compiler.c` | Bytecode compiler (included at end of minic.c) |

### Opcodes

```
Constants:    OP_CONSTANT OP_CONST_INT OP_CONST_ZERO OP_CONST_ONE OP_CONST_FZERO OP_CONST_NULL
Arithmetic:   OP_ADD OP_SUB OP_MUL OP_DIV OP_MOD OP_NEG OP_NOT
Comparison:   OP_EQ OP_NEQ OP_LT OP_GT OP_LE OP_GE
Control:      OP_JMP OP_JMP_FALSE OP_JMP_TRUE OP_LOOP
Globals:      OP_LOAD_GLOBAL OP_STORE_GLOBAL
Calls:        OP_CALL OP_CALL_EXT OP_RETURN OP_RETURN_VOID
Misc:         OP_HALT OP_INC OP_DEC OP_MOV
Compound:     OP_ADD_ASSIGN OP_SUB_ASSIGN OP_MUL_ASSIGN OP_DIV_ASSIGN OP_MOD_ASSIGN
```

---

## Remaining Minic Optimizations (by priority)

| Priority | Phase | Item | Impact | Status |
|----------|-------|------|--------|--------|
| 1 | M15 | Bytecode VM — debug and activate | 15-50x | **In Progress** |
| 2 | M5 | For-loop pre-scan (tree-walker) | 1.5-2x | Pending |
| 3 | M8 | String interning | 1.5x | Pending |
| 4 | M9 | Enum constant hash | 1.5x | Pending |
| 5 | M10 | NaN-boxing | cache | Pending |
| 6 | M11 | Line number pre-compute | minor | Pending |
| 7 | M12 | Reduce memcpy | minor | Pending |
| 8 | M13 | Struct native layout | memory | Pending |
| 9 | M14 | Arena allocator | memory | Pending |

---

## Native Struct `->field` Fix — **Done**

`component_finalize()` calls `minic_register_struct_native()` but registered struct layout was not visible to running contexts. Contexts snapshot global struct array at creation time. Dynamic components register after context creation.

**Fix**: Modified `minic_struct_get()` to fall back to the global registry when a struct isn't found in the context-local array. On first access, copies the global struct into the context for fast future lookups.

---

## Build and Verify

```bash
# Build (two steps on macOS)
cd engine && ../base/make macos metal
cd engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build

# Run
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame

# Copy .minic changes to bundle
cp engine/assets/systems/frog_system.minic engine/build/build/Release/IronGame.app/Contents/Resources/out/data/systems/
cp engine/assets/systems/minic_bench.minic engine/build/build/Release/IronGame.app/Contents/Resources/out/data/systems/
```
