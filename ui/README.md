# ArmorUI - Iron Engine UI Demo

A demonstration project showcasing the Iron Engine's immediate mode UI system.

## Build & Run

```bash
cd /Users/rayloi/Documents/GitHub/armortools/ui
../base/make macos metal          # Build only
../base/make macos metal --run    # Build and run
```

Or use Xcode:
```bash
cd build
xcodebuild -project ArmorUI.xcodeproj -scheme ArmorUI -configuration Release build
open /Users/rayloi/Library/Developer/Xcode/DerivedData/ArmorUI-ewplyaelesnkkafqkekhsdzdhrty/Build/Products/Release/ArmorUI.app
```

## Features

- **Window**: Main demo window with title "Iron Engine UI Demo"
- **Buttons**: 2 buttons (Button 1, Button 2)
- **Checkbox**: Toggle option with persistent state
- **Slider**: Float slider (0.0 - 1.0 range)
- **Text Input**: Editable text field with stored value
- **Combo Box**: Dropdown selection with 3 options

## Project Structure

```
ui/
├── project.js          # Build configuration
├── sources/
│   ├── main.c          # Entry point (includes kickstart.c)
│   ├── global.h        # Master include header
│   ├── kickstart.c    # Initialization (sys_start, ui_create, etc.)
│   ├── demo_ui.c       # UI demonstration code
│   ├── types.h         # demo_ui_t type definition
│   ├── enums.h         # Window IDs, theme enums
│   ├── globals.h       # Global declarations (ui, demo_ui, ui_children)
│   └── functions.h     # Function declarations
└── build/              # Xcode project output
```

## Key Discoveries

### UI Handle Pattern

Each UI widget requires a `ui_handle_t` for state management:
```c
// Checkbox/Slider pattern:
ui_handle_t *h = ui_handle(__ID__);
h->b = stored_bool;  // or h->f for float
stored_bool = ui_check(h, "Label", "");  // or ui_slider()

// Text Input pattern:
ui_handle_t *h_text = ui_handle(__ID__);
h_text->text = text_buffer;  // MUST set text pointer every frame
ui_text_input(h_text, "", UI_ALIGN_LEFT, true, false);

// Combo Box pattern:
ui_handle_t *h_combo = ui_handle(__ID__);
selection = ui_combo(h_combo, &options_array, "Label", true, UI_ALIGN_LEFT, false);
```

**Important**: For text input, you must set `h->text = buffer` BEFORE calling `ui_text_input` every frame. The buffer must be persistent (not a temporary string).

### Required Global Variables

```c
ui_t *ui;                  // Main UI context
any_map_t *ui_children;    // UI handle map (MUST be initialized!)
demo_ui_t *demo_ui;        // Application-specific UI state
```

### Initialization Order

```c
sys_start(ops);                          // 1. Start system
ui_children = any_map_create();          // 2. Initialize handle map
gc_root(ui_children);                    //    (required before ui_* calls)

ui = ui_create(ui_ops);                  // 3. Create UI context
gc_root(ui);                             //    (after ui_children init)

demo_ui = GC_ALLOC_INIT(demo_ui_t, {0}); // 4. Create app UI state
gc_root(demo_ui);
```

### Missing Declarations Added to base/sources/iron_ui.h

```c
ui_handle_t *ui_handle(char *s);           // Line 339
ui_t        *ui_create(ui_options_t *ops); // Line 340
ui_theme_t  *ui_theme_create();            // Line 336
```

### Metal Rendering Note

Do NOT use `draw_begin()` / `draw_end()` when using `sys_start()`. The Iron engine handles rendering internally, and calling these functions causes "command encoder already encoding" errors.

## Troubleshooting

### Crash: any_map_get at iron_map.c:117

Cause: `ui_children` was not initialized.

Fix: Add `ui_children = any_map_create(); gc_root(ui_children);` after `sys_start()`.

### Crash: any_map_get with NULL pointer

Cause: `demo_ui_init()` was not called.

Fix: Call `demo_ui_init();` before registering callbacks.

### LSP shows "iron.h not found"

This is a false positive. The build system correctly finds `iron.h` through include paths. Build succeeds despite LSP errors.

## Reference Files

- Base UI implementation: `base/sources/iron_ui.c`, `base/sources/iron_ui.h`
- Paint UI usage: `paint/sources/base.c`, `paint/sources/ui_*.c`
- Draw test (simple rendering): `base/tests/draw/main.c`
