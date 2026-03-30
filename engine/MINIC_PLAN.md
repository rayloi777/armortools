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

Minic is a tree-walking interpreter (~2500 lines). Every loop iteration re-tokenizes + re-parses from source. Every function call re-parses the body from source.

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

#### M1: Ext Function Dispatch Hash

**Before**: `minic_ext_func_get()` did linear `strcmp` over ~1024 registered functions — O(n) per ext call,**After**: Open-addressed hash table (1024 slots). Hash computed at registration time (`name_hash` field in `minic_ext_func_t`). Lookup by hash with `strcmp` collision resolution — O(1) amortized.

**Files**:
- `minic.h`: Added `uint32_t name_hash` to `minic_ext_func_t`
- `minic_ext.c`: Added hash table + `minic_ext_name_hash()`, `minic_ext_hash_insert()`, `minic_ext_func_hash_init()`. Updated `minic_register()`, `minic_register_native()`, `minic_ext_func_get()` to use hash lookup.

#### M2: Variable Lookup Hash

**Before**: `minic_var_get()`, `minic_var_set()`, `minic_var_addr()`, `minic_var_struct()` all did linear `strcmp` over local + global scope — O(n) per variable access.

**After**: Added `uint32_t name_hash` to `minic_var_t` and `minic_vartype_t`. All lookup loops compare hash first (`name_hash == h`), then `strcmp` only on hash match. Eliminates `strcmp` for non-matching entries.

**Files**: `minic.c` — added `minic_name_hash()`, updated `minic_var_get`, `minic_var_set`, `minic_var_decl`, `minic_var_addr`, `minic_var_struct`, `minic_vartype_set`.

#### M3: Arena Save/Restore in `minic_call()`

**Before**: `minic_call()` allocated child env vars/arrs/vartypes from the 8MB bump allocator but never restored the arena pointer on return. Repeated function calls in loops exhausted the arena → SIGSEGV.

**After**: `minic_call()` saves `*minic_active_mem_used` before allocations and restores it on return. Arena memory is properly reclaimed for each function call.

**Files**: `minic.c` — added `saved_mem_used` save/restore in `minic_call()`.

#### M4: Function Body Token Cache

**Before**: `minic_call()` re-lexed + re-parsed function body from source on every call. Every `minic_lex_next()` call scanned source characters, skipped whitespace/comments, matched keywords — all repeated for every call.

**After**: On first call, `minic_lex_body_tokens()` pre-lexes the body `{`..`}` into a malloc'd token array stored in `minic_func_t.body_tokens`. On subsequent calls, `minic_lex_next()` replays from the cached array (token-stream mode). The lexer's `pos` field is repurposed as a token index in stream mode, so existing for-loop/while-loop position-save/restore code works without changes. String literals are stored in `token.text[]` during pre-lex and re-allocated into the current arena on replay.

**Files**: `minic.c` — added `src_pos` to `minic_token_t`, `tokens`/`token_count` to `minic_lexer_t`, `body_tokens`/`body_token_count` to `minic_func_t`. Added `minic_lex_body_tokens()` helper. Modified `minic_lex_next()` (stream-mode fast path), `minic_call()` (lazy cache + stream-mode setup), `minic_current_line()` (use `src_pos`), for-loop temp lexer (inherit token array), `minic_ctx_free()` (free cached tokens).

### MinicBench Baseline (100K iters x 3 runs)

Interpreter micro-benchmarks measuring minic overhead. Located at `engine/assets/systems/minic_bench.minic`.

```
Pre-M1 Baseline (100000 iters x 3 runs, Apple M1 Pro):
  P1  Variable get/set:   150 ms
  P2  Ext dispatch 12/i:  205 ms
  P3  Function call:      83 ms
  P4  Float math:          155 ms
  P5  Loop overhead:       39 ms
  P3-P5 (call overhead):  43 ms
  P2/P5 (dispatch/loop):  5.2 x
  P3/P5 (call/loop):      2.1 x
  P4/P5 (math/loop):      3.9 x

Post-M4 (100000 iters x 3 runs, Apple M1 Pro):
  P1  Variable get/set:   65 ms
  P2  Ext dispatch 12/i:  80 ms
  P3  Function call:      29 ms     (was 83 ms → 2.9x faster)
  P4  Float math:          63 ms
  P5  Loop overhead:       16 ms
  P3-P5 (call overhead):  13 ms     (was 43 ms → 3.3x faster)
  P2/P5 (dispatch/loop):  5.0 x
  P3/P5 (call/loop):      1.9 x    (was 2.1 x)
  P4/P5 (math/loop):      4.0 x
```

---

## Remaining Minic Optimizations (by priority)

### Hot-Path Bottlenecks (remaining)

| Function | File:Line | Problem | Complexity |
|----------|-----------|---------|------------|
| `minic_struct_field_idx()` | `minic.c` | Linear `strcmp` over 16 fields | O(16) |
| Lexer keyword matching | `minic.c` | 15+ consecutive `strcmp()` calls | O(15) |

### Implementation Plan (by priority)

#### ~~Phase M4: Function Body Cache~~ — **Done** (2.9x function call speedup)

#### Phase M5: For-loop Pre-scan (1.5-2x)

**File**: `base/sources/libs/minic.c`

**Current**: For-loop condition is re-lexed + re-parsed every iteration.

**Implementation**: Cache `cond_pos` and `incr_pos` at first parse. On subsequent iterations, jump directly to cached positions.

#### Phase M6: Struct Field Cache

**File**: `base/sources/libs/minic.c`

**Current**: `minic_struct_field_idx()`: Linear `strcmp` over max 16 fields.

**Implementation**: Add `uint32_t field_hash[16]` to `minic_struct_def_t`. Compute hash at struct registration. Compare hash before `strcmp`.

#### Phase M7: Lexer Keyword Switch (1.5-2x)

**File**: `base/sources/libs/minic.c`

**Current**: 15+ consecutive `strcmp()` for keyword matching in `minic_lex_next()`.

**Implementation**: Replace with length + first-letter switch dispatch.

#### Phase M8: String Interning (1.5x)

**Files**: `base/sources/libs/minic.c`, `base/sources/libs/minic_ext.c`

**Current**: All name comparisons use `strcmp()`. Replace with `==` on interned integer IDs.

#### Phase M9-M15: Lower Priority Items

| Phase | Item | Impact |
|-------|------|--------|
| M9 | Enum constant hash | 1.5x |
| M10 | NaN-boxing | cache |
| M11 | Line number pre-compute | minor |
| M12 | Reduce memcpy | minor |
| M13 | Struct native layout | memory |
| M14 | Arena allocator | memory |
| M15 | Bytecode VM | 10-100x (ultimate goal) |

---

## Native Struct `->field` Fix — **Done**

`component_finalize()` calls `minic_register_struct_native()` but registered struct layout was not visible to running contexts. Contexts snapshot global struct array at creation time. Dynamic components register after context creation.

**Fix**: Modified `minic_struct_get()` to fall back to the global registry when a struct isn't found in the context-local array. On first access, copies the global struct into the context for fast future lookups. No new API needed.

---

## Next Steps (Prioritized)

1. ~~**M4**: Function body cache in `minic.c`~~ — **Done** (2.9x speedup)
2. ~~**Native struct fix**~~ — **Done** (fallback in `minic_struct_get()`)
3. **M5**: For-loop pre-scan in `minic.c` — 1.5-2x
4. **M6**: Struct field cache in `minic.c` — quick win
5. **M7-M8**: Lexer keyword switch, string interning (incremental)
6. **M9-M15**: Enum hash, NaN-boxing, arena, etc.
7. **M15**: Bytecode VM — ultimate goal (long-term)

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
