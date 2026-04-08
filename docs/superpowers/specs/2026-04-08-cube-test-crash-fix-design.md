# Cube Test Crash Fix

## Problem

The cube test (`base/tests/cube/`) crashes when rendering. Root cause: the mesh object in `cube.arm` has `material_refs: null`, so `scene_create()` creates the mesh with a NULL material. The render loop then segfaults when `mesh_object_valid_context()` calls `material_data_get_context(NULL, "mesh")`.

## Fix

In `ready()`, before calling `scene_create(scene)`, set `material_ref` on the mesh object to `"MyMaterial"` so the scene creation pipeline resolves it correctly.

**File:** `base/tests/cube/sources/main.c`

**Change:** Add a loop before `scene_create(scene)` that finds the mesh object in `scene->objects` and sets its `material_ref`:

```c
// cube.arm mesh has null material_ref — point it to our material
for (int i = 0; i < scene->objects->length; i++) {
    obj_t *o = (obj_t *)scene->objects->buffer[i];
    if (string_equals(o->type, "mesh_object")) {
        o->material_ref = "MyMaterial";
    }
}
```

**Placement:** After the camera object is pushed to `scene->objects` and before `any_map_set(data_cached_scene_raws, ...)`.

## Verification

The fix ensures `data_get_material("Scene", "MyMaterial")` is called during scene creation, which resolves through:
1. `data_get_scene_raw("Scene")` finds the cached scene
2. `material_data_get_raw_by_name(format->material_datas, "MyMaterial")` finds the material
3. `material_data_create()` processes it with shader and context loading
4. The mesh object is created with a valid material, and rendering succeeds
