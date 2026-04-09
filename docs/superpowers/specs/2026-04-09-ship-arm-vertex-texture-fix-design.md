# Fix: ship.arm Vertex Format Mismatch and Texture

**Date:** 2026-04-09
**Status:** Approved

## Problem

Loading `ship.arm` produces "exploding" geometry (sharp triangular faces) and the wrong texture.

## Root Cause

1. **Vertex format mismatch**: ship.arm has 3 vertex arrays (`pos` short4norm + `nor` short2norm + `tex` short2norm = 16 bytes/vertex). The shader only declares 2 vertex elements (`pos` + `tex` = 12 bytes/vertex). The 4-byte stride mismatch causes the GPU to misinterpret vertex data.

2. **Wrong texture**: ship.arm has no embedded material or texture references. The default material uses `"texture.k"` but the correct texture is `"colormap"`.

## Solution

### Change: `engine/sources/core/asset_loader.c`

In `scene_ensure_defaults()`:

1. Add `nor` (short2norm) vertex element between `pos` and `tex` in the shader vertex layout:
   ```c
   // Before:
   vertex_elements = [pos(short4norm), tex(short2norm)]
   // After:
   vertex_elements = [pos(short4norm), nor(short2norm), tex(short2norm)]
   ```

2. Change default texture from `"texture.k"` to `"colormap"`:
   ```c
   // Before:
   {.name = "my_texture", .file = "texture.k"}
   // After:
   {.name = "my_texture", .file = "colormap"}
   ```

## Scope

- Single file: `engine/sources/core/asset_loader.c`
- Two targeted changes in `scene_ensure_defaults()`
- No architectural changes
