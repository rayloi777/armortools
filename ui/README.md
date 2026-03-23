# ArmorUI - Iron Engine UI Demo

A demonstration project showcasing the Iron Engine's immediate mode UI system with all available widgets.

## Build & Run

```bash
cd armortools/ui
../base/make macos metal          # Build only
../base/make macos metal --run    # Build and run
```

Or use Xcode:
```bash
cd build
xcodebuild -project ArmorUI.xcodeproj -scheme ArmorUI -configuration Release build
open build/build/Release/ArmorUI.app
```

## Features

- **Chinese Text Support** - Full UTF-8 text rendering with dynamic glyph loading
- **Combo Box** - Dropdown selection with optional search bar
- **All Standard Widgets** - Buttons, Checkboxes, Radio, Sliders, Inputs, etc.

## Demo Tabs

| Tab | Widgets |
|-----|---------|
| **繁體中文** | Chinese text display test with all widgets |
| **Buttons** | Button, Checkbox, Radio, Panel |
| **Inputs** | Slider, Float Input, Text Input, Text Area, Combo, Inline Radio |
| **Layout** | Row layouts (2-7), Indent/Unindent, Separator |
| **Display** | Image, Tooltip |
| **Color** | Color Wheel |

## Project Structure

```
ui/
├── project.js          # Build configuration
├── README.md           # This file
└── sources/
    ├── global.h        # Master include header
    ├── kickstart.c     # Initialization
    ├── demo_ui.c       # UI demonstration code
    ├── types.h         # demo_ui_t type definition
    ├── enums.h         # Tab and window enums
    ├── globals.h       # Global declarations
    └── functions.h     # Function declarations
```

---

# Complete Iron UI API Reference

## Core Types

### `ui_t` - UI Context
Main UI context structure containing all state.

### `ui_handle_t` - Widget State Handle
```c
typedef struct ui_handle {
    union { float f; int i; bool b; };  // Value storage
    union {
        struct { int layout; int drag_x; int drag_y; float last_max_x; float last_max_y; float scroll_offset; }; // window
        struct { float hue; float sat; float val; float red; float green; float blue; }; // color_wheel
    };
    uint32_t color;
    int redraws;
    char *text;
    bool scroll_enabled;
    bool drag_enabled;
    bool changed;
    bool init;
    gpu_texture_t texture;
    ui_handle_array_t *children;
} ui_handle_t;
```

### `ui_theme_t` - Theme Configuration
```c
typedef struct ui_theme {
    int WINDOW_BG_COL, HOVER_COL, BUTTON_COL, PRESSED_COL;
    int TEXT_COL, LABEL_COL, SEPARATOR_COL, HIGHLIGHT_COL;
    int FONT_SIZE, ELEMENT_W, ELEMENT_H, ELEMENT_OFFSET;
    int ARROW_SIZE, BUTTON_H, CHECK_SIZE, CHECK_SELECT_SIZE;
    int SCROLL_W, SCROLL_MINI_W, TEXT_OFFSET, TAB_W;
    int FILL_WINDOW_BG, FILL_BUTTON_BG, LINK_STYLE;
    int FULL_TABS, ROUND_CORNERS, SHADOWS, VIEWPORT_COL;
} ui_theme_t;
```

### Enums

```c
typedef enum { UI_LAYOUT_VERTICAL, UI_LAYOUT_HORIZONTAL } ui_layout_t;
typedef enum { UI_ALIGN_LEFT, UI_ALIGN_CENTER, UI_ALIGN_RIGHT } ui_align_t;
typedef enum { UI_STATE_IDLE, UI_STATE_STARTED, UI_STATE_DOWN, UI_STATE_RELEASED, UI_STATE_HOVERED } ui_state_t;
typedef enum { UI_LINK_STYLE_LINE, UI_LINK_STYLE_CUBIC_BEZIER } ui_link_style_t;
```

---

## Window System

```c
bool ui_window(ui_handle_t *handle, int x, int y, int w, int h, bool drag);
void ui_end_window();
void ui_begin_sticky();
void ui_end_sticky();
void ui_begin_region(ui_t *ui, int x, int y, int w);
void ui_end_region();
```

---

## Basic Widgets

### Button
```c
bool ui_button(char *text, int align, char *label);
// Returns true when clicked
```

### Checkbox
```c
bool ui_check(ui_handle_t *handle, char *text, char *label);
// Returns current checked state
```

### Radio Button
```c
bool ui_radio(ui_handle_t *handle, int position, char *text, char *label);
// Returns true when this option is selected
```

### Tab
```c
bool ui_tab(ui_handle_t *handle, char *text, bool vertical, uint32_t color, bool align_right);
// Returns true when clicked
```

### Panel (Collapsible)
```c
bool ui_panel(ui_handle_t *handle, char *text, bool is_tree, bool filled, bool align_right);
// Returns expanded state
```

### Text Display
```c
int ui_text(char *text, int align, int bg);
// Returns UI state
```

---

## Input Widgets

### Slider
```c
float ui_slider(ui_handle_t *handle, char *text, float from, float to, 
                bool filled, float precision, bool display_value, int align, bool text_edit);
```

### Float Input
```c
float ui_float_input(ui_handle_t *handle, char *label, int align, float precision);
```

### Text Input (Single Line)
```c
char *ui_text_input(ui_handle_t *handle, char *label, int align, bool editable, bool live_update);
```

### Text Area (Multi Line)
```c
char *ui_text_area(ui_handle_t *handle, int align, bool editable, char *label, bool word_wrap);

// Global options:
extern bool ui_text_area_line_numbers;      // Show line numbers
extern bool ui_text_area_scroll_past_end;   // Allow scrolling past content
extern ui_text_coloring_t *ui_text_area_coloring;  // Syntax coloring
```

### Combo Box
```c
int ui_combo(ui_handle_t *handle, char_ptr_array_t *texts, char *label, 
             bool show_label, int align, bool search_bar);
// Returns selected index
```

### Inline Radio (Segmented Control)
```c
int ui_inline_radio(ui_handle_t *handle, char_ptr_array_t *texts, int align);
// Returns selected index
```

### Color Wheel
```c
uint32_t ui_color_wheel(ui_handle_t *handle, bool alpha, float w, float h, 
                        bool color_preview, void (*picker)(void *), void *data);
// Returns ARGB color
```

---

## Layout

### Row Layouts
```c
void ui_row(f32_array_t *ratios);   // Custom ratios
void ui_row2();  // 2 columns (50/50)
void ui_row3();  // 3 columns (33/33/33)
void ui_row4();  // 4 columns
void ui_row5();  // 5 columns
void ui_row6();  // 6 columns
void ui_row7();  // 7 columns
```

### Indentation
```c
void ui_indent();
void ui_unindent();
```

### Separator
```c
void ui_separator(int h, bool fill);
// h = height (0 = default), fill = filled bar vs line
```

### End Element
```c
void ui_end_element();
void ui_end_element_of_size(float element_size);
```

---

## Image & Drawing

```c
int ui_image(gpu_texture_t *image, uint32_t tint, int h);
int ui_sub_image(gpu_texture_t *image, uint32_t tint, int h, int sx, int sy, int sw, int sh);
void ui_fill(float x, float y, float w, float h, uint32_t color);
void ui_rect(float x, float y, float w, float h, uint32_t color, float strength);
```

---

## Tooltip & Menu

```c
void ui_tooltip(char *text);
void ui_tooltip_image(gpu_texture_t *image, int max_width);
void ui_begin_menu();
void ui_end_menu();
bool ui_menubar_button(char *text);
```

---

## Color Utilities

```c
uint8_t  ui_color_r(uint32_t color);
uint8_t  ui_color_g(uint32_t color);
uint8_t  ui_color_b(uint32_t color);
uint8_t  ui_color_a(uint32_t color);
uint32_t ui_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void     ui_hsv_to_rgb(float h, float s, float v, float *out);
void     ui_rgb_to_hsv(float r, float g, float b, float *out);
```

---

## Input Events

```c
void ui_mouse_down(ui_t *ui, int button, int x, int y);
void ui_mouse_up(ui_t *ui, int button, int x, int y);
void ui_mouse_move(ui_t *ui, int x, int y, int movement_x, int movement_y);
void ui_mouse_wheel(ui_t *ui, float delta);
void ui_pen_down(ui_t *ui, int x, int y, float pressure);
void ui_pen_up(ui_t *ui, int x, int y, float pressure);
void ui_pen_move(ui_t *ui, int x, int y, float pressure);
void ui_key_down(ui_t *ui, int key_code);
void ui_key_up(ui_t *ui, int key_code);
void ui_key_press(ui_t *ui, unsigned character);

#if defined(IRON_ANDROID) || defined(IRON_IOS)
void ui_touch_down(ui_t *ui, int index, int x, int y);
void ui_touch_up(ui_t *ui, int index, int x, int y);
void ui_touch_move(ui_t *ui, int index, int x, int y);
#endif
```

---

## Clipboard

```c
char *ui_copy();
char *ui_cut();
void  ui_paste(char *s);
```

---

## Core Functions

```c
void         ui_init(ui_t *ui, ui_options_t *ops);
void         ui_begin(ui_t *ui);
void         ui_end();
ui_t        *ui_create(ui_options_t *ops);
ui_t        *ui_get_current();
void         ui_set_current(ui_t *current);
ui_handle_t *ui_handle_create();
ui_handle_t *ui_handle(char *s);           // Use with __ID__ macro
ui_theme_t  *ui_theme_create();
void         ui_theme_default(ui_theme_t *t);
void         ui_set_scale(float factor);
ui_handle_t *ui_nest(ui_handle_t *handle, int pos);
```

---

## Utility Functions

```c
bool  ui_get_hover(float elem_h);
bool  ui_get_released(float elem_h);
bool  ui_input_in_rect(float x, float y, float w, float h);
void  ui_fade_color(float alpha);
void  ui_draw_string(char *text, float x_offset, float y_offset, int align, bool truncation);
void  ui_draw_shadow(float x, float y, float w, float h);
void  ui_draw_rect(bool fill, float x, float y, float w, float h);
void  ui_draw_round_bottom(float x, float y, float w);
bool  ui_is_visible(float elem_h);
int   ui_line_count(char *str);
char *ui_extract_line(char *str, int line);
char *ui_hovered_tab_name();
void  ui_set_hovered_tab_name(char *name);
```

---

## Text Editing Utilities

```c
void ui_start_text_edit(ui_handle_t *handle, int align);
void ui_remove_char_at(char *str, int at);
void ui_remove_chars_at(char *str, int at, int count);
void ui_insert_char_at(char *str, int at, char c);
void ui_insert_chars_at(char *str, int at, char *cs);
```

---

## Scale Macros

```c
float UI_SCALE();
float UI_ELEMENT_W();
float UI_ELEMENT_H();
float UI_ELEMENT_OFFSET();
float UI_ARROW_SIZE();
float UI_BUTTON_H();
float UI_CHECK_SIZE();
float UI_CHECK_SELECT_SIZE();
float UI_FONT_SIZE();
float UI_SCROLL_W();
float UI_TEXT_OFFSET();
float UI_TAB_W();
float UI_HEADER_DRAG_H();
float UI_FLASH_SPEED();
float UI_TOOLTIP_DELAY();
```

---

## Node Editor (`iron_ui_nodes.h`)

For creating visual node-based editors (shader graphs, material editors):

### Types
```c
typedef struct ui_node_socket {
    int id, node_id;
    char *name, *type;
    uint32_t color;
    f32_array_t *default_value;
    float min, max, precision;
    int display;
} ui_node_socket_t;

typedef struct ui_node {
    int id;
    char *name, *type;
    float x, y;
    uint32_t color;
    ui_node_socket_array_t *inputs, *outputs;
    ui_node_button_array_t *buttons;
    float width;
    int flags;  // UI_NODE_FLAG_COLLAPSED, UI_NODE_FLAG_PREVIEW
} ui_node_t;

typedef struct ui_node_link {
    int id, from_id, from_socket, to_id, to_socket;
} ui_node_link_t;

typedef struct ui_node_canvas {
    char *name;
    ui_node_array_t *nodes;
    ui_node_link_array_t *links;
} ui_node_canvas_t;
```

### Functions
```c
void  ui_nodes_init(ui_nodes_t *nodes);
void  ui_node_canvas(ui_nodes_t *nodes, ui_node_canvas_t *canvas);
void  ui_nodes_rgba_popup(ui_handle_t *handle, float *val, int x, int y);
void  ui_remove_node(ui_node_t *n, ui_node_canvas_t *canvas);

void     ui_node_canvas_encode(ui_node_canvas_t *canvas);
uint32_t ui_node_canvas_encoded_size(ui_node_canvas_t *canvas);
char    *ui_node_canvas_to_json(ui_node_canvas_t *canvas);

int             ui_get_socket_id(ui_node_array_t *nodes);
ui_node_link_t *ui_get_link(ui_node_link_array_t *links, int id);
int             ui_next_link_id(ui_node_link_array_t *links);
ui_node_t      *ui_get_node(ui_node_array_t *nodes, int id);
int             ui_next_node_id(ui_node_array_t *nodes);
```

### Node Positioning Macros
```c
float UI_NODE_X(ui_node_t *node);
float UI_NODE_Y(ui_node_t *node);
float UI_NODE_W(ui_node_t *node);
float UI_NODE_H(ui_node_canvas_t *canvas, ui_node_t *node);
float UI_OUTPUT_Y(ui_node_t *node, int pos);
float UI_INPUT_Y(ui_node_canvas_t *canvas, ui_node_t *node, int pos);
float UI_OUTPUTS_H(ui_node_t *node, int length);
float UI_BUTTONS_H(ui_node_t *node);
float UI_LINE_H();
float UI_NODES_PAN_X();
float UI_NODES_PAN_Y();
float UI_NODES_SCALE();
float ui_p(float f);
```

---

## Global Variables

```c
extern bool ui_touch_control;
extern bool ui_touch_speed;
extern bool ui_is_cut;
extern bool ui_is_copy;
extern bool ui_is_paste;
extern bool ui_text_area_line_numbers;
extern bool ui_text_area_scroll_past_end;
extern bool ui_nodes_grid_snap;
extern bool ui_nodes_socket_released;
extern char *ui_clipboard;
extern char_ptr_array_t *ui_nodes_exclude_remove;

extern void (*ui_on_border_hover)(ui_handle_t *, int);
extern void (*ui_on_tab_drop)(ui_handle_t *, int, ui_handle_t *, int);
extern char_ptr_array_t *(*ui_nodes_enum_texts)(char *);
extern gpu_texture_t *(*ui_nodes_preview_image)(ui_node_t *);
extern void (*ui_nodes_on_custom_button)(int, char *);
extern ui_canvas_control_t *(*ui_nodes_on_canvas_control)(void);
extern void (*ui_nodes_on_canvas_released)(void);
extern void (*ui_nodes_on_socket_released)(int);
extern void (*ui_nodes_on_link_drag)(int, bool);
extern void (*ui_nodes_on_node_remove)(ui_node_t *);
extern void (*ui_nodes_on_node_changed)(ui_node_t *);
```

---

## Usage Patterns

### Handle Pattern
```c
ui_handle_t *h = ui_handle(__ID__);
h->b = stored_bool;       // Set before widget call
stored_bool = ui_check(h, "Label", "");
if (h->changed) { }       // Check if modified
```

### Text Input Pattern
```c
static char buffer[256] = "Initial text";
ui_handle_t *h = ui_handle(__ID__);
h->text = buffer;  // MUST set before EVERY call
ui_text_input(h, "", UI_ALIGN_LEFT, true, false);
```

### Initialization Order
```c
sys_start(ops);
ui_children = any_map_create();
gc_root(ui_children);

ui = ui_create(ui_ops);
gc_root(ui);

demo_ui = GC_ALLOC_INIT(demo_ui_t, {0});
gc_root(demo_ui);

ui_begin(ui);
ui_end();
```

### Framebuffer Clear
```c
void render(void *_) {
    draw_begin(NULL, true, 0xff1a1a2e);
    draw_end();
    
    ui_begin(ui);
    // ... UI code ...
    ui_end();
}
```

---

## Reference Files

- **UI Implementation**: `base/sources/iron_ui.c`, `base/sources/iron_ui.h`
- **Node Editor**: `base/sources/iron_ui_nodes.h`
- **Paint UI Usage**: `paint/sources/base.c`, `paint/sources/ui_*.c`
- **Draw Test**: `base/tests/draw/main.c`
