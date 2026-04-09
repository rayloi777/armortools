# Fix: Engine Memory Leak (55MB → 3GB)

**Date:** 2026-04-09
**Status:** Approved

## Problem

Engine RAM grows steadily from 55MB to 3GB over time (~1MB/sec). Steady per-frame allocation not being freed.

## Approach

Quick RSS metric + iterative fix.

## Changes

### 1. Add RSS memory metric and periodic GC trigger

**File:** `engine/sources/game_engine.c`

- Add a function to read current RSS (using `mach_task_info` on macOS)
- Print RSS every 60 frames (~1 second) to console
- Check if Boehm GC `GC_collect()` is called periodically; if not, add a call every 60 frames

### 2. Investigate and fix per-frame allocations

Examine these high-suspect areas and fix leaks found:

- **`engine/sources/core/minic_system.c`**: Check if `step()`/`draw()` invocation creates new allocations each frame
- **`engine/sources/ecs/camera_bridge_3d.c`**: Check `camera_bridge_3d_update()` for per-frame `gc_alloc` calls
- **`engine/sources/core/scene_3d_api.c`**: Check if `camera_3d_look_at` or `camera_3d_set_position` allocates per call
- **`engine/sources/ecs/render2d_bridge.c`**: Check for per-frame buffer reallocation

### 3. Verify

Run engine, confirm RSS stabilizes instead of growing.

## Scope

- Diagnostic: RSS logging + GC trigger in game loop
- Fixes: per-frame allocation elimination (depends on investigation findings)
