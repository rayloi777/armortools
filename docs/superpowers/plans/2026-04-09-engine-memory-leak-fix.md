# Engine Memory Leak Fix — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add RSS memory tracking and periodic GC collection to the game loop to stop the 55MB → 3GB leak.

**Architecture:** Add a `get_rss_mb()` helper using macOS `mach_task_info`, call `gc_run()` every 120 frames in the game loop, and log RSS every 120 frames. Fix the camera bridge ortho buffer to allocate once instead of per-frame.

**Tech Stack:** C, macOS mach API, Boehm GC

---

### Task 1: Add RSS metric and periodic GC trigger to game loop

**Files:**
- Modify: `engine/sources/core/game_loop.c`

- [ ] **Step 1: Add includes and RSS helper**

Add these includes and helper function at the top of `game_loop.c`, after the existing includes:

```c
#include <iron_gc.h>
#ifdef IRON_MACOS
#include <mach/mach.h>
#endif

static float get_rss_mb(void) {
#ifdef IRON_MACOS
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) != KERN_SUCCESS) {
        return 0.0f;
    }
    return (float)info.resident_size / (1024.0f * 1024.0f);
#else
    return 0.0f;
#endif
}
```

- [ ] **Step 2: Add GC run and RSS logging to game_loop_update**

Add the following block at the **end** of `game_loop_update()`, just before the closing `}`:

```c
    // Periodic GC collection and memory diagnostics (every 120 frames ≈ 2 sec)
    if (g_frame_count % 120 == 0) {
        gc_run();
        printf("[mem] frame %llu  RSS: %.1f MB\n",
            (unsigned long long)g_frame_count, get_rss_mb());
    }
```

- [ ] **Step 3: Commit**

```bash
cd /Users/rayloi/Documents/GitHub/armortools && git add engine/sources/core/game_loop.c && git commit -m "feat(game_loop): add periodic GC collection and RSS memory logging"
```

---

### Task 2: Fix camera bridge ortho buffer per-frame allocation

**Files:**
- Modify: `engine/sources/ecs/camera_bridge_3d.c`

- [ ] **Step 1: Allocate ortho buffer once in init, reuse in update**

In `camera_bridge_3d_init()` (after line 70, before the printf), add the ortho buffer allocation:

```c
    // Pre-allocate ortho buffer to avoid per-frame gc_alloc
    g_camera_3d_data->ortho = GC_ALLOC_INIT(f32_array_t, {
        .buffer = gc_alloc(sizeof(float) * 4),
        .length = 4,
        .capacity = 4
    });
```

Then in `camera_bridge_3d_update()`, replace the per-frame allocation block (lines 135-148):

```c
            // Sync perspective/ortho mode
            if (cam_fresh->perspective) {
                g_camera_3d_data->ortho = NULL;
            } else {
                if (g_camera_3d_data->ortho == NULL || g_camera_3d_data->ortho->length < 4) {
                    g_camera_3d_data->ortho = gc_alloc(sizeof(f32_array_t));
                    g_camera_3d_data->ortho->buffer = gc_alloc(sizeof(float) * 4);
                    g_camera_3d_data->ortho->length = 4;
                    g_camera_3d_data->ortho->capacity = 4;
                }
```

Replace it with:

```c
            // Sync perspective/ortho mode
            if (cam_fresh->perspective) {
                g_camera_3d_data->ortho = NULL;
            } else {
                // Reuse pre-allocated ortho buffer
                if (g_camera_3d_data->ortho == NULL) {
                    g_camera_3d_data->ortho = GC_ALLOC_INIT(f32_array_t, {
                        .buffer = gc_alloc(sizeof(float) * 4),
                        .length = 4,
                        .capacity = 4
                    });
                }
```

This eliminates the per-frame `gc_alloc` by only allocating once when switching to ortho mode.

- [ ] **Step 2: Commit**

```bash
cd /Users/rayloi/Documents/GitHub/armortools && git add engine/sources/ecs/camera_bridge_3d.c && git commit -m "fix(camera_bridge_3d): eliminate per-frame ortho buffer allocation"
```

---

### Task 3: Build and verify

- [ ] **Step 1: Export assets**

```bash
cd /Users/rayloi/Documents/GitHub/armortools/engine && ../base/make macos metal
```

Expected: completes without errors.

- [ ] **Step 2: Compile**

```bash
cd /Users/rayloi/Documents/GitHub/armortools/engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build
```

Expected: `** BUILD SUCCEEDED **`

- [ ] **Step 3: Run and observe memory**

```bash
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame
```

Verify:
1. Console prints `[mem] frame 120  RSS: XX.X MB` every ~2 seconds
2. RSS stays roughly stable (should not grow beyond ~100MB during normal idle use)
3. If RSS still grows, the log shows the growth rate for further diagnosis
