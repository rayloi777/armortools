# Ship.arm Vertex/Texture Fix — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix exploding geometry and wrong texture when loading ship.arm by matching the shader's vertex layout to the mesh data format and pointing the default material to colormap.

**Architecture:** Two targeted edits in `scene_ensure_defaults()` — add a `nor` vertex element to match ship.arm's 3-array layout, and change the texture file reference from `texture.k` to `colormap`.

**Tech Stack:** C (engine)

---

### Task 1: Fix vertex format and texture in scene_ensure_defaults()

**Files:**
- Modify: `engine/sources/core/asset_loader.c:91,117-120`

- [ ] **Step 1: Change texture file reference**

In `engine/sources/core/asset_loader.c` line 91, change the default texture from `"texture.k"` to `"colormap"`:

```c
// Before:
GC_ALLOC_INIT(bind_tex_t, {.name = "my_texture", .file = "texture.k"}),
// After:
GC_ALLOC_INIT(bind_tex_t, {.name = "my_texture", .file = "colormap"}),
```

- [ ] **Step 2: Add nor vertex element**

In `engine/sources/core/asset_loader.c` lines 115-120, add `nor` between `pos` and `tex` and update the array count from 2 to 3:

```c
// Before:
.vertex_elements = any_array_create_from_raw(
    (void *[]){
        GC_ALLOC_INIT(vertex_element_t, {.name = "pos", .data = "short4norm"}),
        GC_ALLOC_INIT(vertex_element_t, {.name = "tex", .data = "short2norm"}),
    },
    2),
// After:
.vertex_elements = any_array_create_from_raw(
    (void *[]){
        GC_ALLOC_INIT(vertex_element_t, {.name = "pos", .data = "short4norm"}),
        GC_ALLOC_INIT(vertex_element_t, {.name = "nor", .data = "short2norm"}),
        GC_ALLOC_INIT(vertex_element_t, {.name = "tex", .data = "short2norm"}),
    },
    3),
```

- [ ] **Step 3: Commit**

```bash
git add engine/sources/core/asset_loader.c
git commit -m "fix(asset_loader): match vertex layout to .arm files (add nor element) and use colormap texture"
```

---

### Task 2: Build and verify

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

- [ ] **Step 3: Run and manually test**

```bash
engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame
```

Verify:
1. Ship model renders with correct geometry (no exploding faces)
2. Ship model shows the colormap texture (not a solid color or missing texture)
