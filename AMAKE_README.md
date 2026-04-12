# ArmorTools Build System

This documents the build system in `base/`, which is based on **amake** (Armor Make).

## Overview

The build system uses a two-layer architecture:

```
make (shell wrapper)          → selects platform toolchain
    ↓
amake (JavaScript)            → parses flags, generates build files, compiles
    ↓
project.js (per-target config) → declares sources, flags, platform-specific options
```

Both `paint/` and `engine/` use this system. The `base/make` script is invoked from each product directory, and `project.js` lives alongside it.

## Step 1 — `make` (Shell Wrapper)

**Path:** `base/make` (Linux/macOS), `base/make.bat` (Windows)

```bash
#!/usr/bin/env bash
. `dirname "$0"`/tools/platform.sh
MAKE="`dirname "$0"`/tools/bin/$IRON_PLATFORM/amake"
exec $MAKE `dirname "$0"`/tools/make.js "$@"
```

`platform.sh` detects the host OS and sets `IRON_PLATFORM`:
- `linux_x64` or `linux_arm64` on Linux
- `macos` on macOS
- Windows uses `.bat` and its own detection

The script delegates entirely to the prebuilt `amake` binary for the target platform, passing `make.js` and all arguments through.

## Step 2 — `make.js` (Build Orchestrator)

**Path:** `base/tools/make.js` (~3300 lines)

`make.js` is a JavaScript program run by the `amake` binary. It:

1. **Parses command-line arguments** into `goptions` (target, graphics backend, debug, etc.)
2. **Sets global flags** (`globalThis.platform`, `globalThis.graphics`, `globalThis.flags`) visible to `project.js`
3. **Loads `project.js`** by evaluating it as JavaScript
4. **Generates platform-specific project files** (Makefile, Xcode, VS, Gradle, etc.)
5. **Runs the compiler** (Make, Xcode, MSBuild, Gradle, etc.)

Key `goptions` fields:

| Option | Default | Description |
|---|---|---|
| `--target` | auto-detected | `linux`, `windows`, `macos`, `ios`, `android`, `wasm` |
| `--graphics` | auto-detected | `metal`, `vulkan`, `direct3d12`, `webgpu` |
| `--debug` | false | Build debug variant |
| `--run` | false | Build and run immediately |
| `--compile` | false | Run compiler after generating files |
| `--ccompiler` | `clang` | C compiler override |
| `--arch` | `default` | Architecture override |

## Step 3 — `project.js` (Per-Target Configuration)

**Path:** `base/project.js`, `paint/project.js`, `engine/project.js`

Each `project.js` is a JavaScript file that calls `Project` API methods to configure the build:

```javascript
let project = new Project("ArmorPaint");
project.add_include_dir("sources");
project.add_shaders("shaders/*.kong");
project.add_assets("assets/*", {destination : "data/{name}"});
project.add_cfiles("sources/*.c");
```

`project.js` reads `globalThis.platform`, `globalThis.graphics`, and `globalThis.flags` (set by `make.js`) to conditionally add platform-specific files and defines.

### Flags passed to project.js

These are set in `load_project()` in `make.js`:

| Flag | Type | Description |
|---|---|---|
| `name` | string | Project name (e.g., `"ArmorPaint"`) |
| `dirname` | string | Project directory path |
| `release` | bool | `true` unless `--debug` passed |
| `embed` | bool | Embed assets into binary (--embed) |
| `with_audio` | bool | Enable audio support |
| `with_gamepad` | bool | Enable gamepad support |
| `with_physics` | bool | Enable physics (ASiM) |
| `with_kong` | bool | Enable Kong shader compiler |
| `with_plugins` | bool | Enable plugin system |
| `with_eval` | bool | Enable Minic evaluator |
| `with_bc7` | bool | Enable BC7 texture compression |
| `with_nfd` | bool | Enable native file dialog |
| `with_compress` | bool | Enable tar decompression |
| `with_image_write` | bool | Enable image export |
| `with_video_write` | bool | Enable video export |
| `with_d3dcompiler` | bool | Enable D3D shader compiler |
| `idle_sleep` | bool | Enable idle sleep |
| `export_version_info` | bool | Export git SHA + date |
| `export_data_list` | bool | Export data directory lists |
| `package` | string | Android package name |

### How platform detection works

In `make.js`:

```javascript
globalThis.platform = goptions.target;  // e.g. "macos", "windows", "linux"
globalThis.graphics = goptions.graphics;  // e.g. "metal", "vulkan"
globalThis.flags    = { name: "Armory", ... };
```

Then in `project.js`:

```javascript
if (platform == "macos") {
    project.add_cfiles("sources/backends/metal_gpu.*");
    project.add_define("IRON_METAL");
}
else if (platform == "windows") {
    project.add_cfiles("sources/backends/direct3d12_gpu.*");
    project.add_define("IRON_DIRECT3D12");
}
// etc.
```

## Build Commands by Platform

### macOS

```bash
# From paint/ or engine/ directory:
cd paint && ../base/make macos metal
cd paint/build && xcodebuild -project IronPaint.xcodeproj -configuration Release build
```

Two steps: `make` exports assets/shaders, `xcodebuild` compiles C code.

### Linux

```bash
cd engine && ../base/make linux vulkan
cd engine/build && make -j$(nproc)
```

On Linux the build system directly invokes `make` after generating Makefiles.

### Windows

```bash
cd paint && ..\base\make windows direct3d12
# Opens generated .sln in Visual Studio
```

On Windows, `make.js` generates a Visual Studio solution.

### Android

```bash
cd paint && ../base/make android vulkan --package com.mypackage
cd paint/build && ./gradlew assembleRelease
```

### iOS

```bash
cd paint && ../base/make ios metal
cd paint/build && xcodebuild -project IronPaint.xcodeproj -configuration Release build
```

### WebAssembly

```bash
cd paint && ../base/make wasm webgpu
cd paint/build && make -j$(nproc)
```

## Project API Reference

`project.js` calls these methods on the `Project` object:

| Method | Description |
|---|---|
| `add_include_dir(path)` | Add `-I` include directory |
| `add_define(name)` | Add `-D` preprocessor define |
| `add_cfiles(pattern)` | Add C source files (glob pattern) |
| `add_shaders(pattern)` | Compile shaders (Kong format) |
| `add_assets(pattern, opts)` | Copy assets to `data/` output |
| `add_lib(name)` | Link against system library |
| `add_project(path)` | Include sub-project |
| `flatten()` | Finalize and generate build files |

## Asset Export

Assets are copied to `build/data/` with directory structure preserved:

```javascript
project.add_assets("assets/*", {destination : "data/{name}"});
// assets/ui.png → build/data/ui.png

project.add_assets("assets/themes/*.json", {destination : "data/themes/{name}"});
// assets/themes/dark.json → build/data/themes/dark.json
```

The `{name}` placeholder is replaced with the original filename.

## Shader Compilation

Shaders (`.kong` format) are compiled by the Kong shader compiler into platform-specific output:

```javascript
project.add_shaders("shaders/*.kong");
```

On metal: `.metal` files
On vulkan: `.spv` (SPIR-V)
On direct3d12: `.cso`
On wasm/webgpu: `.wgsl`

## Including Other project.js Files

Use `project.add_project(path)` to include another project's `project.js`. This recursively loads the referenced project, merging its sources, shaders, asset matchers, and defines into the parent project.

```javascript
// In paint/project.js
if (flags.with_plugins) {
    project.add_project("../paint/plugins");  // Loads paint/plugins/project.js
    project.add_cfiles("sources/plugins/plugin_api.c");
}
```

How it works:

```javascript
add_project(directory) {
    let project = load_project(directory, false);  // is_root_project = false
    this.subProjects.push(project);
    this.asset_matchers  = this.asset_matchers.concat(project.asset_matchers);
    this.sources         = this.sources.concat(project.sources);
    this.shader_matchers = this.shader_matchers.concat(project.shader_matchers);
    this.defines         = this.defines.concat(project.defines);
    return project;
}
```

Key points:
- `load_project()` is called with `is_root_project = false`, so it does **not** reset `globalThis.platform` or `globalThis.flags` — the parent's settings are inherited
- The included project's `project.js` runs with access to the same `platform`, `graphics`, and `flags` globals
- Included projects can themselves include further projects (nested inclusion is supported)
- The included project's `export_iron_project()` is **not** called when loaded via `add_project()` — only when `is_root_project = true`

### Example: Plugin System

In `paint/project.js`:

```javascript
if (flags.with_plugins) {
    project.add_define("WITH_PLUGINS");
    project.add_project("../paint/plugins");  // Merges plugin sources into paint build
}
```

The `paint/plugins/project.js` might look like:

```javascript
let project = new Project("ArmorPaint Plugins");
project.add_cfiles("sources/plugins/*.c");
project.add_define("ARM_PAINT");
project.flatten();
return project;
```

Note: `add_project()` returns the sub-project, but you typically don't need to capture it. The merge is done inline.

## Adding a New Flag

To add a new build flag (e.g., `--with-foo`):

1. **In `make.js`** — add the flag to `goptions` defaults if needed
2. **In `load_project()`** — add `with_foo : false` to the `globalThis.flags` object
3. **In `project.js`** — read `flags.with_foo` and call appropriate `project.*` methods

## Troubleshooting

### "command not found: amake" on macOS
Ensure `base/tools/platform.sh` runs correctly. On macOS it should set `IRON_PLATFORM=macos`. Check that `base/tools/bin/macos/amake` exists and is executable.

### Windows build fails to find include dirs
On Windows, some include paths in `project.js` use `/usr/include/...` style — these are Linux-only and guarded by `if (platform == "linux")`.

### Debug vs Release not applied
`--debug` sets `goptions.debug = true` which sets `globalThis.flags.release = false`. Check that `project.js` uses `flags.release` to control optimization levels.