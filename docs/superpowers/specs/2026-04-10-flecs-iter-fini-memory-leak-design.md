# Fix: Flecs Iterator ecs_iter_fini Memory Leak

Date: 2026-04-10

## Problem

RSS grows ~30 MB/min in 3D scenes, never recovers. Activity Monitor confirms linear growth from ~50 MB to 200+ MB in 5 minutes.

## Root Cause

Flecs ECS query iterators created via `ecs_query_iter()` allocate internal stack pages. These pages must be released by calling `ecs_iter_fini(&it)` after iteration completes. The Flecs source contains an explicit warning:

> `"a stack allocator leak is most likely due to an unterminated iteration: call ecs_iter_fini to fix"`

Every frame, the engine creates iterators in bridge systems and query API calls but never calls `ecs_iter_fini`. The Flecs stack allocator keeps allocating new pages because old ones are never freed.

### Affected Code Paths

| File | Function | Calls per frame | Missing fini |
|------|----------|----------------|-------------|
| `ecs/camera_bridge_3d.c` | `camera_bridge_3d_update()` | 1 | Yes |
| `ecs/mesh_bridge_3d.c` | `mesh_bridge_3d_sync_transforms()` | 1 | Yes |
| `ecs/render2d_bridge.c` | `sys_2d_draw()` | 1 | Yes |
| `ecs/mesh_bridge_3d.c` | `mesh_bridge_3d_shutdown()` | once | Yes |
| `core/query_api.c` | `query_iter_begin/next` cycle | per script query | Yes |
| `core/query_api.c` | `build_filter_and_run()` | per query_find | Yes |
| `core/query_api.c` | `query_next()` when returning false | per failed next | Yes |

## Fix

Add `ecs_iter_fini(&it)` after every iterator loop completes (whether exhausted naturally or exited early via `return`).

### Changes

1. **`ecs/camera_bridge_3d.c`** — `camera_bridge_3d_update()`:
   - Add `ecs_iter_fini(&it)` after the `while(ecs_query_next)` loop
   - Also before the early `return` inside the loop (first-match camera exit)

2. **`ecs/mesh_bridge_3d.c`** — `mesh_bridge_3d_sync_transforms()`:
   - Add `ecs_iter_fini(&it)` after the `while(ecs_query_next)` loop

3. **`ecs/mesh_bridge_3d.c`** — `mesh_bridge_3d_shutdown()`:
   - Add `ecs_iter_fini(&it)` after the cleanup iteration loop

4. **`ecs/render2d_bridge.c`** — `sys_2d_draw()`:
   - Add `ecs_iter_fini(&it)` after the `while(ecs_query_next)` loop

5. **`core/query_api.c`** — `query_iter_next()`:
   - Call `ecs_iter_fini()` on the cached iterator before `restore_iter_state()` when iteration ends (returns false)

6. **`core/query_api.c`** — `build_filter_and_run()`:
   - Call `ecs_iter_fini()` on the cached iterator before creating a new one in `query_iter_begin` or `query_next` restarts

7. **`core/query_api.c`** — `query_destroy()`:
   - Call `ecs_iter_fini()` on the cached iterator if `iter_started` is true, before freeing the cached_it buffer

## Verification

After fix, run the 3D scene for 5+ minutes and confirm:
- RSS stays stable (no linear growth)
- `[mem]` log shows RSS plateau
- GC `gc_run()` output shows stable or declining memory
