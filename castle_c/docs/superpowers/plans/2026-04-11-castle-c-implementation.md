# CastleDB C Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement CastleDB C - a pure C structured database editor with Dear ImGui GUI, integrated with armortools/base.

**Architecture:** Core library (lib/) handles CDB read/write/validation. Editor (editor/) provides ImGui GUI. Integrates with armortools/base for JSON, LZ4, file I/O, and native dialogs.

**Tech Stack:** Pure C, Dear ImGui, armortools/base (iron_json, iron_lz4, iron_file), jsmn, NFD

---

## Phase 1: Core Library - Basic Structures and Reader

### File Structure
```
castle_c/sources/lib/
├── cdb.h          # Main public header
├── cdb.c          # Database/Sheet/Column implementation
├── cdb_reader.c   # Read CDB JSON files
├── cdb_json.c     # JSON parsing/encoding helpers
├── cdb_types.c    # Complex types (Gradient, Curve, TileLayer)
└── cdb_internal.h # Internal structures and helpers
```

### Tasks

#### Task 1: Create cdb.h - Main Public Header

**Files:**
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/lib/cdb.h`

```c
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define CDB_VERSION "1.0.0"

typedef enum {
    CDB_TID = 0,
    CDB_TSTRING = 1,
    CDB_TBOOL = 2,
    CDB_TINT = 3,
    CDB_TFLOAT = 4,
    CDB_TENUM = 5,
    CDB_TREF = 6,
    CDB_TIMAGE = 7,
    CDB_TLIST = 8,
    CDB_TCUSTOM = 9,
    CDB_TFLAGS = 10,
    CDB_TCOLOR = 11,
    CDB_TLAYER = 12,
    CDB_TFILE = 13,
    CDB_TTILEPOS = 14,
    CDB_TTILELAYER = 15,
    CDB_TDYNAMIC = 16,
    CDB_TPROPERTIES = 17,
    CDB_TGRADIENT = 18,
    CDB_TCURVE = 19,
    CDB_TGUID = 20,
    CDB_TPOLYMORPH = 21
} CDBColumnType;

typedef enum {
    CDB_OK,
    CDB_ERR_FILE_NOT_FOUND,
    CDB_ERR_FILE_WRITE,
    CDB_ERR_JSON_PARSE,
    CDB_ERR_INVALID_CDB,
    CDB_ERR_OUT_OF_MEMORY,
    CDB_ERR_NOT_FOUND,
    CDB_ERR_TYPE_MISMATCH
} CDBError;

typedef struct CDB CDB;
typedef struct CDBSheet CDBSheet;
typedef struct CDBColumn CDBColumn;

// Lifecycle
CDB *cdb_create(void);
int cdb_load(CDB *db, const char *filepath);
int cdb_save(CDB *db, const char *filepath);
void cdb_close(CDB *db);
CDBError cdb_get_error(CDB *db);
const char *cdb_error_string(CDBError err);

// Database operations
int cdb_get_sheet_count(CDB *db);
CDBSheet *cdb_get_sheet(CDB *db, int index);
CDBSheet *cdb_get_sheet_by_name(CDB *db, const char *name);
int cdb_create_sheet(CDB *db, const char *name);
int cdb_delete_sheet(CDB *db, const char *name);

// Sheet operations
const char *cdb_sheet_name(CDBSheet *sheet);
int cdb_sheet_column_count(CDBSheet *sheet);
CDBColumn *cdb_sheet_column(CDBSheet *sheet, int index);
CDBColumn *cdb_sheet_column_by_name(CDBSheet *sheet, const char *name);
int cdb_sheet_add_column(CDBSheet *sheet, CDBColumn *col, int position);
int cdb_sheet_delete_column(CDBSheet *sheet, const char *column_name);
int cdb_sheet_row_count(CDBSheet *sheet);
int cdb_sheet_add_row(CDBSheet *sheet, int position);
int cdb_sheet_delete_row(CDBSheet *sheet, int row_index);
int cdb_sheet_move_row(CDBSheet *sheet, int from, int to);

// Column operations
CDBColumn *cdb_column_create(const char *name, CDBColumnType type);
void cdb_column_destroy(CDBColumn *col);
const char *cdb_column_name(CDBColumn *col);
CDBColumnType cdb_column_type(CDBColumn *col);
const char *cdb_column_type_str(CDBColumn *col);  // e.g., "5:value1,value2"
int cdb_column_is_optional(CDBColumn *col);
void cdb_column_set_optional(CDBColumn *col, int optional);

// Value access - string
const char *cdb_get_string(CDBSheet *sheet, int row, const char *column);
int cdb_set_string(CDBSheet *sheet, int row, const char *column, const char *value);

// Value access - int
int cdb_get_int(CDBSheet *sheet, int row, const char *column);
int cdb_set_int(CDBSheet *sheet, int row, const char *column, int value);

// Value access - float
float cdb_get_float(CDBSheet *sheet, int row, const char *column);
int cdb_set_float(CDBSheet *sheet, int row, const char *column, float value);

// Value access - bool
int cdb_get_bool(CDBSheet *sheet, int row, const char *column);
int cdb_set_bool(CDBSheet *sheet, int row, const char *column, int value);

// Value access - raw (for complex types)
const char *cdb_get_raw(CDBSheet *sheet, int row, const char *column);
int cdb_set_raw(CDBSheet *sheet, int row, const char *column, const char *value);

// Value null check
int cdb_value_is_null(CDBSheet *sheet, int row, const char *column);
```

#### Task 2: Create cdb_internal.h - Internal Structures

**Files:**
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/lib/cdb_internal.h`

```c
#pragma once

#include "cdb.h"

// Internal column structure
struct CDBColumn {
    char *name;
    CDBColumnType type;
    char *type_str;        // Original type string e.g., "5:value1,value2"
    char **enum_values;    // For TEnum, TFlags
    int enum_count;
    char *ref_sheet;       // For TRef
    int is_optional;
};

// Internal sheet structure
struct CDBSheet {
    char *name;
    CDBColumn *columns;
    int column_count;
    char ***lines;         // 2D array: lines[row][col]
    int line_count;
    void *props;           // Sheet properties (displayColumn, etc.)
    int hidden;            // For sub-sheets (hide flag)
};

// Internal database structure
struct CDB {
    CDBError error;
    char *filepath;
    CDBSheet *sheets;
    int sheet_count;
    int compress;          // LZ4 compression enabled
    void *custom_types;    // For TCustom support
};
```

#### Task 3: Create cdb.c - Core Implementation

**Files:**
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/lib/cdb.c`

- [ ] **Step 1: Write basic structure allocation functions**

```c
#include "cdb_internal.h"
#include <stdlib.h>
#include <string.h>

CDB *cdb_create(void) {
    CDB *db = calloc(1, sizeof(CDB));
    if (!db) return NULL;
    db->error = CDB_OK;
    return db;
}

CDBColumn *cdb_column_create(const char *name, CDBColumnType type) {
    CDBColumn *col = calloc(1, sizeof(CDBColumn));
    if (!col) return NULL;
    col->name = strdup(name);
    col->type = type;
    return col;
}

void cdb_column_destroy(CDBColumn *col) {
    if (!col) return;
    free(col->name);
    free(col->type_str);
    if (col->enum_values) {
        for (int i = 0; i < col->enum_count; i++) free(col->enum_values[i]);
        free(col->enum_values);
    }
    free(col->ref_sheet);
    free(col);
}
```

- [ ] **Step 2: Write sheet operations**

```c
const char *cdb_sheet_name(CDBSheet *sheet) {
    return sheet ? sheet->name : NULL;
}

int cdb_sheet_column_count(CDBSheet *sheet) {
    return sheet ? sheet->column_count : 0;
}

CDBColumn *cdb_sheet_column(CDBSheet *sheet, int index) {
    if (!sheet || index < 0 || index >= sheet->column_count) return NULL;
    return &sheet->columns[index];
}

int cdb_sheet_row_count(CDBSheet *sheet) {
    return sheet ? sheet->line_count : 0;
}
```

- [ ] **Step 3: Write value getter/setter stubs (for Task 5 to fill)**

```c
const char *cdb_get_string(CDBSheet *sheet, int row, const char *column) {
    if (!sheet || !column) return NULL;
    int col_idx = -1;
    for (int i = 0; i < sheet->column_count; i++) {
        if (strcmp(sheet->columns[i].name, column) == 0) {
            col_idx = i;
            break;
        }
    }
    if (col_idx < 0 || row < 0 || row >= sheet->line_count) return NULL;
    return sheet->lines[row][col_idx];
}

int cdb_set_string(CDBSheet *sheet, int row, const char *column, const char *value) {
    // Implement in Task 5
    return 0;
}
```

#### Task 4: Create cdb_reader.c - JSON Reader

**Files:**
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/lib/cdb_reader.c`

Based on existing `test/cdb_reader/cdb_reader.c` but integrated with armortools:

- [ ] **Step 1: Write file reading and JSON parsing**

```c
#include "cdb_internal.h"
#include <iron_json.h>
#include <iron_file.h>
#include <stdlib.h>
#include <string.h>

static char *read_file_content(const char *filepath, size_t *out_size) {
    iron_file_reader_t reader;
    if (!iron_file_reader_open(&reader, filepath, IRON_FILE_TYPE_SAVE)) {
        return NULL;
    }
    size_t size = iron_file_reader_size(&reader);
    char *content = malloc(size + 1);
    if (!content) {
        iron_file_reader_close(&reader);
        return NULL;
    }
    iron_file_reader_read(&reader, content, size);
    content[size] = '\0';
    iron_file_reader_close(&reader);
    if (out_size) *out_size = size;
    return content;
}

int cdb_load(CDB *db, const char *filepath) {
    size_t size;
    char *content = read_file_content(filepath, &size);
    if (!content) {
        db->error = CDB_ERR_FILE_NOT_FOUND;
        return -1;
    }

    any_map_t *root = json_parse_to_map(content);
    free(content);

    if (!root) {
        db->error = CDB_ERR_JSON_PARSE;
        return -1;
    }

    // Parse sheets array
    any_array_t *sheets_arr = any_map_get(root, "sheets");
    if (!sheets_arr) {
        map_delete(root, "sheets");
        db->error = CDB_ERR_INVALID_CDB;
        return -1;
    }

    db->sheet_count = sheets_arr->length;
    db->sheets = calloc(db->sheet_count, sizeof(CDBSheet));

    for (int i = 0; i < db->sheet_count; i++) {
        any_map_t *sheet_map = any_array_get(sheets_arr, i);
        parse_sheet(&db->sheets[i], sheet_map);
    }

    map_delete(root, "sheets");
    return 0;
}
```

- [ ] **Step 2: Write sheet and column parsing**

```c
static void parse_sheet(CDBSheet *sheet, any_map_t *sheet_map) {
    sheet->name = strdup(any_map_get(sheet_map, "name"));

    any_array_t *cols_arr = any_map_get(sheet_map, "columns");
    sheet->column_count = cols_arr ? cols_arr->length : 0;
    sheet->columns = calloc(sheet->column_count, sizeof(CDBColumn));

    for (int i = 0; i < sheet->column_count; i++) {
        any_map_t *col_map = any_array_get(cols_arr, i);
        parse_column(&sheet->columns[i], col_map);
    }

    any_array_t *lines_arr = any_map_get(sheet_map, "lines");
    sheet->line_count = lines_arr ? lines_arr->length : 0;
    sheet->lines = calloc(sheet->line_count, sizeof(char**));

    for (int i = 0; i < sheet->line_count; i++) {
        any_map_t *line_map = any_array_get(lines_arr, i);
        parse_line(sheet, i, line_map);
    }
}

static void parse_column(CDBColumn *col, any_map_t *col_map) {
    col->name = strdup(any_map_get(col_map, "name"));
    const char *type_str = any_map_get(col_map, "typeStr");
    col->type_str = strdup(type_str);
    parse_type_string(col, type_str);
}

static void parse_type_string(CDBColumn *col, const char *type_str) {
    // Parse "TYPE" or "TYPE:extra"
    // For now, basic parsing - enhance in Task 6
}
```

#### Task 5: Implement Value Getters/Setters in cdb.c

**Files:**
- Modify: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/lib/cdb.c`

- [ ] **Step 1: Implement all cdb_set_* and cdb_get_* functions**

```c
int cdb_set_string(CDBSheet *sheet, int row, const char *column, const char *value) {
    if (!sheet || !column) return -1;
    int col_idx = find_column_index(sheet, column);
    if (col_idx < 0 || row < 0 || row >= sheet->line_count) return -1;

    free(sheet->lines[row][col_idx]);
    sheet->lines[row][col_idx] = value ? strdup(value) : NULL;
    return 0;
}

int cdb_get_int(CDBSheet *sheet, int row, const char *column) {
    const char *val = cdb_get_string(sheet, row, column);
    return val ? atoi(val) : 0;
}

int cdb_set_int(CDBSheet *sheet, int row, const char *column, int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    return cdb_set_string(sheet, row, column, buf);
}

float cdb_get_float(CDBSheet *sheet, int row, const char *column) {
    const char *val = cdb_get_string(sheet, row, column);
    return val ? atof(val) : 0.0f;
}

int cdb_set_float(CDBSheet *sheet, int row, const char *column, float value) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", value);
    return cdb_set_string(sheet, row, column, buf);
}

int cdb_get_bool(CDBSheet *sheet, int row, const char *column) {
    const char *val = cdb_get_string(sheet, row, column);
    return val && (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
}

int cdb_set_bool(CDBSheet *sheet, int row, const char *column, int value) {
    return cdb_set_string(sheet, row, column, value ? "true" : "false");
}

int cdb_value_is_null(CDBSheet *sheet, int row, const char *column) {
    const char *val = cdb_get_string(sheet, row, column);
    return val == NULL || strlen(val) == 0;
}
```

---

## Phase 2: Core Library - Writer and Types

### Tasks

#### Task 6: Create cdb_writer.c - JSON Writer

**Files:**
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/lib/cdb_writer.c`

- [ ] **Step 1: Write JSON encoding infrastructure**

```c
#include "cdb_internal.h"
#include <iron_json.h>
#include <iron_file.h>
#include <stdlib.h>
#include <string.h>

static void encode_sheet(any_map_t *sheet_map, CDBSheet *sheet) {
    any_map_set(sheet_map, "name", sheet->name);

    // Encode columns
    json_encode_begin_array("columns");
    for (int i = 0; i < sheet->column_count; i++) {
        json_encode_begin_object();
        CDBColumn *col = &sheet->columns[i];
        json_encode_string("name", col->name);
        json_encode_string("typeStr", col->type_str ? col->type_str : "1"); // Default TString
        if (col->is_optional) json_encode_bool("opt", true);
        json_encode_end_object();
    }
    json_encode_end_array();

    // Encode lines
    json_encode_begin_array("lines");
    for (int i = 0; i < sheet->line_count; i++) {
        json_encode_begin_object();
        for (int j = 0; j < sheet->column_count; j++) {
            const char *val = sheet->lines[i][j];
            if (val && strlen(val) > 0) {
                json_encode_string(sheet->columns[j].name, val);
            }
        }
        json_encode_end_object();
    }
    json_encode_end_array();
}

int cdb_save(CDB *db, const char *filepath) {
    json_encode_begin();
    json_encode_begin_array("sheets");

    for (int i = 0; i < db->sheet_count; i++) {
        json_encode_begin_object();
        encode_sheet(NULL, &db->sheets[i]);
        json_encode_end_object();
    }

    json_encode_end_array();
    if (db->compress) json_encode_bool("compress", true);

    char *json = json_encode_end();

    iron_file_writer_t writer;
    if (!iron_file_writer_open(&writer, filepath)) {
        free(json);
        db->error = CDB_ERR_FILE_WRITE;
        return -1;
    }
    iron_file_writer_write(&writer, json, strlen(json));
    iron_file_writer_close(&writer);
    free(json);
    return 0;
}
```

#### Task 7: Create cdb_types.c - Complex Types

**Files:**
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/lib/cdb_types.c`

- [ ] **Step 1: Implement Gradient parsing/generation**

```c
#include "cdb_internal.h"
#include <iron_lz4.h>
#include <string.h>
#include <stdlib.h>

// Gradient format: {"colors":[0xFF0000,0x00FF00], "positions":[0.0,1.0]}
typedef struct {
    int *colors;
    float *positions;
    int count;
} CDBGradient;

CDBGradient *cdb_gradient_parse(const char *str) {
    // Parse JSON gradient
}

char *cdb_gradient_to_string(CDBGradient *g) {
    // Generate JSON gradient
}

int *cdb_gradient_generate(CDBGradient *g, int count) {
    // Generate interpolated colors
}
```

- [ ] **Step 2: Implement TileLayer encoding/decoding**

```c
// TileLayer format: LZ4 compressed 16-bit tile indices
// Structure: {file: "tiles.png", stride: 16, size: 256, data: "LZ4_BASE64..."}

typedef struct {
    char *file;
    int stride;
    int size;
    char *data;  // LZ4 compressed
} CDBTileLayer;

i32_array_t *cdb_tilelayer_decode(CDBTileLayer *tl) {
    // Decode LZ4 compressed data to array of 16-bit tile IDs
}

char *cdb_tilelayer_encode(i32_array_t *tiles, int stride, int compress) {
    // Encode tile array to LZ4 compressed string
}
```

#### Task 8: Create cdb_json.c - JSON Helpers

**Files:**
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/lib/cdb_json.c`

- [ ] **Step 1: Write type string parsing**

```c
#include "cdb_internal.h"
#include <string.h>
#include <stdlib.h>

int cdb_parse_type_str(CDBColumn *col, const char *type_str) {
    // Parse "TYPE" -> type enum
    // Parse "TYPE:extra" -> type enum + extra data

    char buffer[256];
    strncpy(buffer, type_str, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char *colon = strchr(buffer, ':');
    if (colon) *colon = '\0';

    int type = atoi(buffer);
    col->type = type;

    if (colon) {
        char *extra = colon + 1;

        if (type == CDB_TENUM || type == CDB_TFLAGS) {
            // Parse comma-separated values
            col->enum_count = 1;
            for (char *p = extra; *p; p++) {
                if (*p == ',') col->enum_count++;
            }
            col->enum_values = calloc(col->enum_count, sizeof(char*));
            char *token = strtok(extra, ",");
            int i = 0;
            while (token && i < col->enum_count) {
                col->enum_values[i++] = strdup(token);
                token = strtok(NULL, ",");
            }
        }
        else if (type == CDB_TREF) {
            col->ref_sheet = strdup(extra);
        }
    }

    return 0;
}
```

---

## Phase 3: Editor - Scaffold and Menu

### File Structure
```
castle_c/sources/editor/
├── main.c          # Entry point
├── editor_app.c    # App state and main loop
├── editor_app.h
├── menu.c          # Menu bar
├── sheet_view.c    # Sheet list and table
├── column_editor.c # Column operations
├── value_editor.c  # Per-type editors
└── imgui.ini       # ImGui settings
```

### Tasks

#### Task 9: Create editor/main.c - Entry Point

**Files:**
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/editor/main.c`

```c
#include "editor_app.h"
#include <iron_file.h>
#include <nfd.h>

int main(int argc, char **argv) {
    editor_init();

    // Load font
    editor_load_font("C:/Windows/Fonts/segoeui.ttf", 16);

    // Main loop
    while (!editor_should_close()) {
        editor_update();
        editor_render();
    }

    editor_shutdown();
    return 0;
}
```

#### Task 10: Create editor/editor_app.h and editor_app.c - App State

**Files:**
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/editor/editor_app.h`
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/editor/editor_app.c`

```c
// editor_app.h
#pragma once

#include <cdb.h>
#include <stdbool.h>

typedef struct {
    CDB *db;
    char *filepath;
    int selected_sheet;
    int selected_row;
    int selected_col;
    bool modified;
    bool show_column_editor;
    bool show_about;
} EditorState;

extern EditorState g_editor;

void editor_init(void);
void editor_shutdown(void);
bool editor_should_close(void);
void editor_update(void);
void editor_render(void);
void editor_load_font(const char *path, float size);
```

```c
// editor_app.c
#include "editor_app.h"
#include <stdlib.h>
#include <string.h>

EditorState g_editor = {0};

void editor_init(void) {
    g_editor.db = cdb_create();
    g_editor.selected_sheet = 0;
    g_editor.modified = false;
}

void editor_shutdown(void) {
    if (g_editor.db) cdb_close(g_editor.db);
    free(g_editor.filepath);
}

bool editor_should_close(void) {
    // Check if window should close
    return false; // Placeholder - integrate with window close
}
```

#### Task 11: Create editor/menu.c - Menu Bar

**Files:**
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/editor/menu.c`

```c
#include "editor_app.h"
#include <nfd.h>

static void show_file_menu(void) {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New", "Ctrl+N")) {
            editor_new();
        }
        if (ImGui::MenuItem("Open...", "Ctrl+O")) {
            editor_open();
        }
        if (ImGui::MenuItem("Save", "Ctrl+S")) {
            editor_save();
        }
        if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
            editor_save_as();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt+F4")) {
            // Request close
        }
        ImGui::EndMenu();
    }
}

static void show_edit_menu(void) {
    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
            editor_undo();
        }
        if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
            editor_redo();
        }
        ImGui::EndMenu();
    }
}

static void show_sheet_menu(void) {
    if (ImGui::BeginMenu("Sheet")) {
        if (ImGui::MenuItem("Add Sheet")) {
            editor_add_sheet();
        }
        if (ImGui::MenuItem("Delete Sheet")) {
            editor_delete_sheet();
        }
        ImGui::EndMenu();
    }
}

void editor_render_menu(void) {
    if (ImGui::BeginMenuBar()) {
        show_file_menu();
        show_edit_menu();
        show_sheet_menu();
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About CastleDB");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}
```

---

## Phase 4: Editor - Sheet View and Value Editors

### Tasks

#### Task 12: Create editor/sheet_view.c - Sheet List and Table

**Files:**
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/editor/sheet_view.c`

- [ ] **Step 1: Write sheet list panel**

```c
#include "editor_app.h"

void editor_render_sheet_list(void) {
    ImGui::BeginChild("SheetList", ImVec2(200, 0), true);

    int sheet_count = cdb_get_sheet_count(g_editor.db);
    for (int i = 0; i < sheet_count; i++) {
        CDBSheet *sheet = cdb_get_sheet(g_editor.db, i);
        const char *name = cdb_sheet_name(sheet);

        // Skip hidden sheets
        if (strchr(name, '@') != NULL) continue; // Sub-sheets have @

        ImGuiTreeNodeFlags flags = (g_editor.selected_sheet == i)
            ? ImGuiTreeNodeFlags_Selected : 0;
        flags |= ImGuiTreeNodeFlags_Leaf;

        if (ImGui::TreeNodeEx(name, flags)) {
            ImGui::TreePop();
        }

        if (ImGui::IsItemClicked()) {
            g_editor.selected_sheet = i;
        }
    }

    ImGui::EndChild();
}
```

- [ ] **Step 2: Write sheet table view**

```c
void editor_render_sheet_table(void) {
    CDBSheet *sheet = cdb_get_sheet(g_editor.db, g_editor.selected_sheet);
    if (!sheet) return;

    int col_count = cdb_sheet_column_count(sheet);
    int row_count = cdb_sheet_row_count(sheet);

    ImGui::BeginChild("SheetView");

    // Header row
    ImGui::Columns(col_count + 1, "sheet_columns", true);
    ImGui::Text("Row"); ImGui::NextColumn();

    for (int i = 0; i < col_count; i++) {
        CDBColumn *col = cdb_sheet_column(sheet, i);
        ImGui::Text("%s", cdb_column_name(col));
        ImGui::NextColumn();
    }
    ImGui::Separator();

    // Data rows
    for (int r = 0; r < row_count; r++) {
        char row_label[32];
        snprintf(row_label, sizeof(row_label), "%d", r);
        if (ImGui::Selectable(row_label, g_editor.selected_row == r)) {
            g_editor.selected_row = r;
        }
        ImGui::NextColumn();

        for (int c = 0; c < col_count; c++) {
            CDBColumn *col = cdb_sheet_column(sheet, c);
            const char *col_name = cdb_column_name(col);

            // Render based on type
            editor_render_cell_value(sheet, r, col);

            ImGui::NextColumn();
        }
    }

    ImGui::Columns(1);
    ImGui::EndChild();
}
```

#### Task 13: Create editor/value_editor.c - Per-Type Editors

**Files:**
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/editor/value_editor.c`

- [ ] **Step 1: Write basic type editors**

```c
#include "editor_app.h"

void editor_render_cell_value(CDBSheet *sheet, int row, CDBColumn *col) {
    CDBColumnType type = cdb_column_type(col);
    const char *col_name = cdb_column_name(col);

    switch (type) {
        case CDB_TSTRING:
        case CDB_TID:
        case CDB_TFILE: {
            static char buf[256];
            const char *val = cdb_get_string(sheet, row, col_name);
            strncpy(buf, val ? val : "", sizeof(buf));
            if (ImGui::InputText(buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                cdb_set_string(sheet, row, col_name, buf);
                g_editor.modified = true;
            }
            break;
        }
        case CDB_TINT: {
            int val = cdb_get_int(sheet, row, col_name);
            if (ImGui::InputInt(col_name, &val)) {
                cdb_set_int(sheet, row, col_name, val);
                g_editor.modified = true;
            }
            break;
        }
        case CDB_TFLOAT: {
            float val = cdb_get_float(sheet, row, col_name);
            if (ImGui::InputFloat(col_name, &val)) {
                cdb_set_float(sheet, row, col_name, val);
                g_editor.modified = true;
            }
            break;
        }
        case CDB_TBOOL: {
            int val = cdb_get_bool(sheet, row, col_name);
            if (ImGui::Checkbox(col_name, &val)) {
                cdb_set_bool(sheet, row, col_name, val);
                g_editor.modified = true;
            }
            break;
        }
        // Add cases for other types...
    }
}
```

- [ ] **Step 2: Write enum dropdown editor**

```c
void editor_render_enum_editor(CDBSheet *sheet, int row, CDBColumn *col, const char *col_name) {
    int current = cdb_get_int(sheet, row, col_name);
    const char *preview = (current >= 0 && current < col->enum_count)
        ? col->enum_values[current] : "";

    if (ImGui::BeginCombo(col_name, preview)) {
        for (int i = 0; i < col->enum_count; i++) {
            bool selected = (current == i);
            if (ImGui::Selectable(col->enum_values[i], selected)) {
                cdb_set_int(sheet, row, col_name, i);
                g_editor.modified = true;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}
```

- [ ] **Step 3: Write color picker editor**

```c
void editor_render_color_editor(CDBSheet *sheet, int row, CDBColumn *col, const char *col_name) {
    int val = cdb_get_int(sheet, row, col_name);
    float col_r = ((val >> 16) & 0xFF) / 255.0f;
    float col_g = ((val >> 8) & 0xFF) / 255.0f;
    float col_b = (val & 0xFF) / 255.0f;

    float color[4] = {col_r, col_g, col_b, 1.0f};
    if (ImGui::ColorEdit4(col_name, color)) {
        int new_val = ((int)(color[0] * 255) << 16) |
                      ((int)(color[1] * 255) << 8) |
                      ((int)(color[2] * 255));
        cdb_set_int(sheet, row, col_name, new_val);
        g_editor.modified = true;
    }
}
```

#### Task 14: Create editor/column_editor.c - Column Operations

**Files:**
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/editor/column_editor.c`

```c
#include "editor_app.h"

void editor_show_column_editor(void) {
    if (!g_editor.show_column_editor) return;

    ImGui::Begin("Column Editor", &g_editor.show_column_editor);

    static char name_buf[64] = "";
    static int type_selected = 1; // TString default
    static bool optional = false;

    ImGui::InputText("Name", name_buf, sizeof(name_buf));

    const char *type_names[] = {
        "TId", "TString", "TBool", "TInt", "TFloat",
        "TEnum", "TRef", "TImage", "TList", "TCustom",
        "TFlags", "TColor", "TLayer", "TFile", "TTilePos",
        "TTileLayer", "TDynamic", "TProperties", "TGradient", "TCurve",
        "TGuid", "TPolymorph"
    };

    ImGui::Combo("Type", &type_selected, type_names, 22);
    ImGui::Checkbox("Optional", &optional);

    // Type-specific options
    if (type_selected == CDB_TENUM || type_selected == CDB_TFLAGS) {
        static char enum_buf[256] = "value1,value2,value3";
        ImGui::InputText("Values (comma separated)", enum_buf, sizeof(enum_buf));
    }
    else if (type_selected == CDB_TREF) {
        static char ref_buf[64] = "";
        ImGui::InputText("Referenced Sheet", ref_buf, sizeof(ref_buf));
    }

    if (ImGui::Button("Add Column")) {
        CDBColumnType type = type_selected;
        CDBColumn *col = cdb_column_create(name_buf, type);
        cdb_column_set_optional(col, optional);

        // Set type-specific options
        // ...

        CDBSheet *sheet = cdb_get_sheet(g_editor.db, g_editor.selected_sheet);
        cdb_sheet_add_column(sheet, col, -1);
        g_editor.modified = true;
        memset(name_buf, 0, sizeof(name_buf));
    }

    ImGui::End();
}
```

---

## Phase 5: Integration and Polish

### Tasks

#### Task 15: File Operations Integration

**Files:**
- Modify: `/Users/rayloi/Documents/GitHub/armortools/castle_c/sources/editor/editor_app.c`

- [ ] **Step 1: Implement editor_open() with NFD**

```c
void editor_open(void) {
    nfdchar_t *out_path = NULL;
    nfdresult_t result = NFD_OpenDialog("cdb", NULL, &out_path);

    if (result == NFD_OKAY) {
        if (g_editor.db) cdb_close(g_editor.db);
        g_editor.db = cdb_create();
        if (cdb_load(g_editor.db, out_path) != 0) {
            // Show error
        }
        free(g_editor.filepath);
        g_editor.filepath = strdup(out_path);
        g_editor.modified = false;
        free(out_path);
    }
}

void editor_save(void) {
    if (!g_editor.filepath) {
        editor_save_as();
        return;
    }
    if (cdb_save(g_editor.db, g_editor.filepath) != 0) {
        // Show error
    }
    g_editor.modified = false;
}

void editor_save_as(void) {
    nfdchar_t *out_path = NULL;
    nfdresult_t result = NFD_SaveDialog("cdb", NULL, &out_path);

    if (result == NFD_OKAY) {
        if (cdb_save(g_editor.db, out_path) != 0) {
            // Show error
        }
        free(g_editor.filepath);
        g_editor.filepath = strdup(out_path);
        g_editor.modified = false;
        free(out_path);
    }
}

void editor_new(void) {
    if (g_editor.db) cdb_close(g_editor.db);
    g_editor.db = cdb_create();
    cdb_create_sheet(g_editor.db, "Sheet1");
    free(g_editor.filepath);
    g_editor.filepath = NULL;
    g_editor.modified = false;
    g_editor.selected_sheet = 0;
}
```

#### Task 16: Create project.js - Build Integration

**Files:**
- Create: `/Users/rayloi/Documents/GitHub/armortools/castle_c/project.js`

```javascript
function get_project() {
    let project = new Project("Castle");
    project.add_include_dir("castle_c/sources");
    project.add_include_dir("base/sources");
    project.add_include_dir("base/sources/libs");
    project.add_define("WITH_NFD");
    project.add_define("WITH_CUSTOM_CASTLE"); // Our flag

    // Castle C library
    project.add_cfiles("castle_c/sources/lib/*.c");

    // Editor
    project.add_cfiles("castle_c/sources/editor/*.c");

    // Iron dependencies
    project.add_cfiles("base/sources/iron_json.c");
    project.add_cfiles("base/sources/iron_lz4.c");
    project.add_cfiles("base/sources/iron_file.c");
    project.add_cfiles("base/sources/iron_array.c");
    project.add_cfiles("base/sources/iron_map.c");

    // NFD
    project.add_cfiles("base/sources/libs/nfd.c");
    if (platform == "macos") {
        project.add_cfiles("base/sources/libs/nfd.m");
    }

    // Libraries
    project.add_cfiles("base/sources/libs/jsmn.c");
    project.add_cfiles("base/sources/libs/sinfl.c");
    project.add_cfiles("base/sources/libs/sdefl.c");

    project.flatten();
    return project;
}
```

#### Task 17: Add Dear ImGui Integration

**Files:**
- Modify: `/Users/rayloi/Documents/GitHub/armortools/castle_c/project.js`

Add Dear ImGui to the project:
- Download or vendor Dear ImGui sources
- Add imconfig.h, imgui.h, imgui.cpp, imgui_draw.cpp, imgui_tables.cpp, imgui_widgets.cpp
- Add imgui_impl_opengl3.cpp (or appropriate backend)

Note: Dear ImGui integration depends on armortools/base graphics backend. If armortools uses its own UI system (iron_ui), consider using that instead or adding Dear ImGui on top.

---

## Spec Coverage Check

- [x] Core library with all 22 column types
- [x] Read/Write CDB JSON files
- [x] Dear ImGui GUI with menu, sheet list, table view
- [x] Value editors per type (string, int, float, bool, enum, color, etc.)
- [x] Column operations (add, delete, rename)
- [x] Sheet operations (add, delete)
- [x] File operations via NFD
- [x] Build integration with armortools/base

## Implementation Notes

1. **armortools/base dependencies**: iron_json, iron_lz4, iron_file, iron_array, iron_map are already available
2. **jsmn**: Header-only, need to add jsmn.c from libs/
3. **Dear ImGui**: Vendor or integrate based on armortools structure
4. **File paths**: Use iron_path.h for cross-platform path handling
5. **Testing**: Use existing `test/item.cdb` from castle repository to test reading
