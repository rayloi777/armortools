# ArmorUI - Iron Engine UI Demo

A demonstration project showcasing the Iron Engine's immediate mode UI system with all available widgets.

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

The demo is organized into 5 tabs:

### Tab 1: Buttons
- **Button** (`ui_button`) - Clickable button
- **Checkbox** (`ui_check`) - Toggle option with boolean state
- **Radio Button** (`ui_radio`) - Mutually exclusive options (3 choices)
- **Panel** (`ui_panel`) - Collapsible/expandable section

### Tab 2: Inputs
- **Slider** (`ui_slider`) - Float slider with min/max range
- **Float Input** (`ui_float_input`) - Numeric text input field
- **Text Input** (`ui_text_input`) - Single-line text editor
- **Text Area** (`ui_text_area`) - Multi-line text editor
- **Combo Box** (`ui_combo`) - Dropdown selection list
- **Inline Radio** (`ui_inline_radio`) - Segmented button control

### Tab 3: Layout
- **Row Layouts** (`ui_row2` - `ui_row7`) - Split widgets into columns
- **Indent/Unindent** (`ui_indent`, `ui_unindent`) - Nested indentation
- **Separator** (`ui_separator`) - Horizontal divider line

### Tab 4: Display
- **Image** (`ui_image`) - Display textures with optional color tint
- **Tooltip** (`ui_tooltip`) - Hover help text

### Tab 5: Color
- **Color Wheel** (`ui_color_wheel`) - HSV color picker with RGB output

## Project Structure

```
ui/
├── project.js          # Build configuration
├── sources/
│   ├── main.c          # Entry point
│   ├── global.h        # Master include header
│   ├── kickstart.c     # Initialization
│   ├── demo_ui.c       # UI demonstration code
│   ├── types.h         # demo_ui_t type definition
│   ├── enums.h         # Tab and window enums
│   ├── globals.h       # Global declarations
│   └── functions.h     # Function declarations
└── build/              # Xcode project output
```

## Complete UI Widget Reference

### Button
```c
if (ui_button("Click Me", UI_ALIGN_CENTER, "")) {
    // Button was clicked
}
```

### Checkbox
```c
ui_handle_t *h = ui_handle(__ID__);
h->b = stored_bool;
stored_bool = ui_check(h, "Enable Option", "");
```

### Radio Button
```c
ui_handle_t *h = ui_handle(__ID__);
h->i = stored_selection;
if (ui_radio(h, 0, "Option A", "")) stored_selection = 0;
if (ui_radio(h, 1, "Option B", "")) stored_selection = 1;
```

### Panel (Collapsible)
```c
ui_handle_t *h = ui_handle(__ID__);
h->b = is_expanded;
is_expanded = ui_panel(h, "Settings", false, true, false);
if (is_expanded) {
    // Draw panel contents
}
```

### Slider
```c
ui_handle_t *h = ui_handle(__ID__);
h->f = stored_value;
stored_value = ui_slider(h, "Value", 0.0, 1.0, true, 100.0, true, UI_ALIGN_RIGHT, true);
```

### Float Input
```c
ui_handle_t *h = ui_handle(__ID__);
stored_value = ui_float_input(h, "Number:", UI_ALIGN_LEFT, 100.0);
```

### Text Input
```c
ui_handle_t *h = ui_handle(__ID__);
h->text = text_buffer;  // Set pointer every frame
ui_text_input(h, "", UI_ALIGN_LEFT, true, false);
```

### Text Area
```c
ui_text_area_line_numbers = false;
ui_handle_t *h = ui_handle(__ID__);
h->text = text_buffer;
ui_text_area(h, UI_ALIGN_LEFT, true, "", false);
```

### Combo Box
```c
ui_handle_t *h = ui_handle(__ID__);
stored_index = ui_combo(h, &options_array, "Select:", true, UI_ALIGN_LEFT, false);
```

### Inline Radio (Segmented Control)
```c
ui_handle_t *h = ui_handle(__ID__);
stored_index = ui_inline_radio(h, &options_array, UI_ALIGN_LEFT);
```

### Tab
```c
ui_handle_t *h = ui_handle(__ID__);
if (ui_tab(h, "Tab Name", false, -1, false)) {
    // Tab was clicked - use for selection
}
```

### Image
```c
int state = ui_image(texture, 0xFFFFFFFF, 64);  // tint, height
if (state == UI_STATE_HOVERED) {
    ui_tooltip("Image tooltip");
}
```

### Color Wheel
```c
ui_handle_t *h = ui_handle(__ID__);
h->color = stored_color;
stored_color = ui_color_wheel(h, true, -1, -1, true, NULL, NULL);
```

### Row Layouts
```c
ui_row2();  // 2 columns (50/50)
ui_button("A", UI_ALIGN_CENTER, "");
ui_button("B", UI_ALIGN_CENTER, "");

ui_row3();  // 3 columns (33/33/33)
```

### Indent/Unindent
```c
ui_button("Normal", UI_ALIGN_CENTER, "");
ui_indent();
ui_button("Indented", UI_ALIGN_CENTER, "");
ui_indent();
ui_button("More Indented", UI_ALIGN_CENTER, "");
ui_unindent();
ui_button("Back to Indented", UI_ALIGN_CENTER, "");
ui_unindent();
```

### Separator
```c
ui_separator(1, false);   // Thin line
ui_separator(4, true);    // Filled bar
```

## Key Discoveries

### UI Handle Pattern

Each UI widget requires a `ui_handle_t` for state management:
```c
// Get/create handle - must use ui_handle(__ID__) not ui_handle_create()
ui_handle_t *h = ui_handle(__ID__);

// Initialize value on first call (h->init == true)
if (h->init) {
    h->f = stored_value;  // float value
    h->b = stored_bool;   // boolean value
    h->i = stored_int;    // integer value
    h->text = buffer;     // text buffer pointer
}

// Use widget - returns current value
stored_value = ui_slider(h, "Label", ...);

// State check
if (h->changed) { }  // Value was modified
```

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
ui_handle_t *ui_handle(char *s);           // Line 340
ui_t        *ui_create(ui_options_t *ops); // Line 341
ui_theme_t  *ui_theme_create();            // Line 336
void          ui_indent();                   // Added
void          ui_unindent();                 // Added
```

### Metal Rendering Note

Do NOT use `draw_begin()` / `draw_end()` when using `sys_start()`. The Iron engine handles rendering internally.

## Troubleshooting

### Crash: any_map_get at iron_map.c:117

Cause: `ui_children` was not initialized.

Fix: Add `ui_children = any_map_create(); gc_root(ui_children);` after `sys_start()`.

### Text Input Not Working

Cause: Text buffer pointer not set.

Fix: Set `h->text = buffer` BEFORE calling `ui_text_input()` every frame.

### LSP shows "iron.h not found"

This is a false positive. The build system correctly finds `iron.h` through include paths.

## Reference Files

- Base UI implementation: `base/sources/iron_ui.c`, `base/sources/iron_ui.h`
- Paint UI usage: `paint/sources/base.c`, `paint/sources/ui_*.c`
- Draw test (simple rendering): `base/tests/draw/main.c`
