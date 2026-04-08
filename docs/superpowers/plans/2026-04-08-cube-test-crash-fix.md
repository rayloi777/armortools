# Cube Test Crash Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix the cube test crash caused by a NULL material reference on the mesh object loaded from cube.arm.

**Architecture:** Set `material_ref` on the raw obj_t before `scene_create()` processes it, so the Iron scene creation pipeline resolves our "MyMaterial" material data correctly.

**Tech Stack:** C (Iron engine test project, built with `make` + `xcodebuild`)

---

### Task 1: Set material_ref on mesh object

**Files:**
- Modify: `base/tests/cube/sources/main.c:39-102`

- [ ] **Step 1: Add material_ref assignment loop**

Insert this block in `ready()`, after the `any_array_push(scene->objects, ...)` call that adds the Camera (line 39), and before the `scene->camera_datas = ...` line (line 42):

```c
	// cube.arm mesh has null material_ref — point it to our material
	for (int i = 0; i < scene->objects->length; i++) {
		obj_t *o = (obj_t *)scene->objects->buffer[i];
		if (string_equals(o->type, "mesh_object")) {
			o->material_ref = "MyMaterial";
		}
	}
```

- [ ] **Step 2: Build the project**

Run:
```bash
cd /Users/user/Documents/GitHub/armortools/base/tests/cube && ../../make macos metal
cd /Users/user/Documents/GitHub/armortools/base/tests/cube/build && xcodebuild -project IronGame.xcodeproj -configuration Release build
```

Expected: Build succeeds with no errors.

- [ ] **Step 3: Run and verify no crash**

Run:
```bash
/Users/user/Documents/GitHub/armortools/base/tests/cube/build/build/Release/IronGame.app/Contents/MacOS/IronGame
```

Expected: Window opens showing a textured cube. No crash.

- [ ] **Step 4: Commit**

```bash
git add base/tests/cube/sources/main.c
git commit -m "fix(cube-test): set material_ref on mesh object to fix NULL material crash"
```
