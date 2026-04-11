# Flecs Iterator ecs_iter_fini Memory Leak Fix — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `ecs_iter_fini(&it)` calls after every Flecs iterator loop to stop the stack allocator from leaking ~30 MB/min.

**Architecture:** The Flecs ECS iterator creates internal stack pages via `ecs_query_iter()`. These must be freed with `ecs_iter_fini()`. The fix is a surgical addition of `ecs_iter_fini` calls in bridge systems (per-frame C code) and the query API (cached iterators used by Minic scripts).

**Tech Stack:** C, Flecs ECS, Iron engine

---

## File Structure

| File | Change |
|------|--------|
| `engine/sources/ecs/camera_bridge_3d.c` | Add `ecs_iter_fini` in `camera_bridge_3d_update()` |
| `engine/sources/ecs/mesh_bridge_3d.c` | Add `ecs_iter_fini` in `mesh_bridge_3d_sync_transforms()` and `mesh_bridge_3d_shutdown()` |
| `engine/sources/ecs/render2d_bridge.c` | Add `ecs_iter_fini` in `sys_2d_draw()` |
| `engine/sources/core/query_api.c` | Add `ecs_iter_fini` in `query_iter_next()`, `build_filter_and_run()`, `query_next()`, and `query_destroy()` |

---

### Task 1: Fix camera_bridge_3d.c

**Files:**
- Modify: `engine/sources/ecs/camera_bridge_3d.c`

The `camera_bridge_3d_update()` function has a `return` inside the loop (line 170, "single active camera — stop after first match") and a natural loop end at line 172. Both paths must call `ecs_iter_fini`.

- [ ] **Step 1: Add ecs_iter_fini before the early return (line 170)**

Replace:
```c
            // Single active camera — stop after first match
            return;
```
With:
```c
            // Single active camera — stop after first match
            ecs_iter_fini(&it);
            return;
```

- [ ] **Step 2: Add ecs_iter_fini after the while loop (after line 172)**

Replace:
```c
        }
    }
}

void *camera_bridge_3d_get_active(void) {
```
With:
```c
        }
    }
    ecs_iter_fini(&it);
}

void *camera_bridge_3d_get_active(void) {
```

- [ ] **Step 3: Verify the edit compiles**

Run: `cd engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | tail -5`
Expected: `** BUILD SUCCEEDED **`

- [ ] **Step 4: Commit**

```bash
git add engine/sources/ecs/camera_bridge_3d.c
git commit -m "fix(camera_bridge_3d): add ecs_iter_fini to prevent Flecs stack allocator leak"
```

---

### Task 2: Fix mesh_bridge_3d.c

**Files:**
- Modify: `engine/sources/ecs/mesh_bridge_3d.c`

Two functions need `ecs_iter_fini`: `mesh_bridge_3d_sync_transforms()` (per-frame) and `mesh_bridge_3d_shutdown()` (once at cleanup).

- [ ] **Step 1: Add ecs_iter_fini in mesh_bridge_3d_sync_transforms (after line 108)**

Replace:
```c
        }
    }
}

void mesh_bridge_3d_create_mesh(uint64_t entity) {
```
With:
```c
        }
    }
    ecs_iter_fini(&it);
}

void mesh_bridge_3d_create_mesh(uint64_t entity) {
```

- [ ] **Step 2: Add ecs_iter_fini in mesh_bridge_3d_shutdown (after line 61)**

Replace:
```c
                }
            }
        }
    }
    if (g_sync_query) { ecs_query_fini(g_sync_query); g_sync_query = NULL; }
```
With:
```c
                }
            }
            ecs_iter_fini(&it);
        }
    }
    if (g_sync_query) { ecs_query_fini(g_sync_query); g_sync_query = NULL; }
```

- [ ] **Step 3: Verify the edit compiles**

Run: `cd engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | tail -5`
Expected: `** BUILD SUCCEEDED **`

- [ ] **Step 4: Commit**

```bash
git add engine/sources/ecs/mesh_bridge_3d.c
git commit -m "fix(mesh_bridge_3d): add ecs_iter_fini to prevent Flecs stack allocator leak"
```

---

### Task 3: Fix render2d_bridge.c

**Files:**
- Modify: `engine/sources/ecs/render2d_bridge.c`

The `sys_2d_draw()` function creates an iterator at line 178 and exhausts it by line 202. Must call `ecs_iter_fini` after the loop. Note: the function has an early `return` at line 204 when `count == 0` — `ecs_iter_fini` must be called before that return.

- [ ] **Step 1: Add ecs_iter_fini after the while loop and before the early return**

Replace:
```c
        }
    }

    if (count == 0) return;
```
With:
```c
        }
    }
    ecs_iter_fini(&it);

    if (count == 0) return;
```

- [ ] **Step 2: Verify the edit compiles**

Run: `cd engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | tail -5`
Expected: `** BUILD SUCCEEDED **`

- [ ] **Step 3: Commit**

```bash
git add engine/sources/ecs/render2d_bridge.c
git commit -m "fix(render2d_bridge): add ecs_iter_fini to prevent Flecs stack allocator leak"
```

---

### Task 4: Fix query_api.c — query_iter_next

**Files:**
- Modify: `engine/sources/core/query_api.c`

The `query_iter_next()` function uses a cached iterator. When `ecs_query_next` returns false (iteration exhausted), it calls `restore_iter_state()` but never calls `ecs_iter_fini`. Must call `ecs_iter_fini` before restoring.

- [ ] **Step 1: Add ecs_iter_fini when iteration ends in query_iter_next**

Replace:
```c
    if (!ecs_query_next((ecs_iter_t *)q->cached_it)) {
        q->last_count = 0;
        restore_iter_state(q);
        return false;
    }
```
With:
```c
    if (!ecs_query_next((ecs_iter_t *)q->cached_it)) {
        q->last_count = 0;
        ecs_iter_fini((ecs_iter_t *)q->cached_it);
        restore_iter_state(q);
        return false;
    }
```

- [ ] **Step 2: Verify the edit compiles**

Run: `cd engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | tail -5`
Expected: `** BUILD SUCCEEDED **`

- [ ] **Step 3: Commit**

```bash
git add engine/sources/core/query_api.c
git commit -m "fix(query_api): add ecs_iter_fini in query_iter_next to prevent Flecs stack leak"
```

---

### Task 5: Fix query_api.c — build_filter_and_run and query_next

**Files:**
- Modify: `engine/sources/core/query_api.c`

Two more functions that create iterators without cleanup:

1. `build_filter_and_run()` — creates an iterator at line 192. When `ecs_query_next` returns false (line 194), must call `ecs_iter_fini`.
2. `query_next()` — when `iter_started` is false it creates a new iterator at line 292. But if there was a previous iterator still active, it leaks. Must fini the old one before starting new. Also must fini when `ecs_query_next` returns false at line 295.

- [ ] **Step 1: Add ecs_iter_fini in build_filter_and_run when iteration is empty**

Replace:
```c
    *(ecs_iter_t *)q->cached_it = ecs_query_iter(ecs, query);
    q->iter_started = true;
    if (!ecs_query_next((ecs_iter_t *)q->cached_it)) {
        q->last_count = 0;
        q->total_count = 0;
        q->truncated = false;
        memset(q->last_entities, 0, sizeof(q->last_entities));
        q->iter_started = false;
        return false;
    }
```
With:
```c
    *(ecs_iter_t *)q->cached_it = ecs_query_iter(ecs, query);
    q->iter_started = true;
    if (!ecs_query_next((ecs_iter_t *)q->cached_it)) {
        q->last_count = 0;
        q->total_count = 0;
        q->truncated = false;
        memset(q->last_entities, 0, sizeof(q->last_entities));
        ecs_iter_fini((ecs_iter_t *)q->cached_it);
        q->iter_started = false;
        return false;
    }
```

- [ ] **Step 2: Add ecs_iter_fini before creating new iterator in query_next**

Replace:
```c
    if (!q->iter_started) {
        *(ecs_iter_t *)q->cached_it = ecs_query_iter(ecs, query);
        q->iter_started = true;
    }
    if (!ecs_query_next((ecs_iter_t *)q->cached_it)) {
        q->last_count = 0;
        q->total_count = 0;
        q->truncated = false;
        memset(q->last_entities, 0, sizeof(q->last_entities));
        q->iter_started = false;
        return false;
    }
```
With:
```c
    if (!q->iter_started) {
        *(ecs_iter_t *)q->cached_it = ecs_query_iter(ecs, query);
        q->iter_started = true;
    }
    if (!ecs_query_next((ecs_iter_t *)q->cached_it)) {
        q->last_count = 0;
        q->total_count = 0;
        q->truncated = false;
        memset(q->last_entities, 0, sizeof(q->last_entities));
        ecs_iter_fini((ecs_iter_t *)q->cached_it);
        q->iter_started = false;
        return false;
    }
```

- [ ] **Step 3: Add ecs_iter_fini in query_destroy before freeing cached_it**

Replace:
```c
    if (q->cached_it) {
        free(q->cached_it);
        q->cached_it = NULL;
    }
```
With:
```c
    if (q->cached_it) {
        if (q->iter_started) {
            ecs_iter_fini((ecs_iter_t *)q->cached_it);
        }
        free(q->cached_it);
        q->cached_it = NULL;
    }
```

- [ ] **Step 4: Verify the edit compiles**

Run: `cd engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build 2>&1 | tail -5`
Expected: `** BUILD SUCCEEDED **`

- [ ] **Step 5: Commit**

```bash
git add engine/sources/core/query_api.c
git commit -m "fix(query_api): add ecs_iter_fini in build_filter_and_run, query_next, and query_destroy"
```

---

### Task 6: Build and smoke test

**Files:** None (verification only)

- [ ] **Step 1: Full rebuild**

```bash
cd engine && ../base/make macos metal && cd build && xcodebuild -project IronGame.xcodeproj -configuration Release build
```
Expected: `** BUILD SUCCEEDED **`

- [ ] **Step 2: Run the binary for 5+ minutes and monitor RSS**

```bash
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame
```

Watch `[mem]` output every 120 frames. Expected: RSS plateaus instead of growing linearly. Previously grew ~0.5 MB/sec; after fix should stabilize within first few minutes.

- [ ] **Step 3: Commit any remaining changes**

```bash
git add -A
git commit -m "fix: complete Flecs ecs_iter_fini memory leak fix across all bridge systems"
```
(Only if there are remaining uncommitted changes.)
