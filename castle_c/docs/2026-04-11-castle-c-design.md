# CastleDB C Implementation Design

## Overview

CastleDB C is a pure C reimplementation of the CastleDB structured database editor, targeting multi-platform (Windows, Linux, macOS) with a Dear ImGui-based GUI. It integrates with the armortools/base framework.

**Project Location:** `/Users/rayloi/Documents/GitHub/armortools/castle_c/`

## Architecture

### Directory Structure

```
castle_c/
├── project.js          # Build config (integrates with armortools/base)
├── docs/               # Documentation
├── sources/
│   ├── lib/            # Core library (read/write, platform-independent)
│   │   ├── cdb.h       # Public API
│   │   ├── cdb.c       # Database, Sheet, Column management
│   │   ├── cdb_reader.c
│   │   ├── cdb_writer.c
│   │   ├── cdb_json.c  # JSON parse/encode (using iron_json)
│   │   ├── cdb_types.c # Complex types (Gradient, Curve, TileLayer)
│   │   └── cdb_validate.c
│   └── editor/         # Dear ImGui GUI
│       ├── main.c
│       ├── editor_app.c
│       ├── sheet_view.c
│       ├── column_editor.c
│       ├── value_editor.c
│       └── menu.c
├── assets/             # Fonts, icons
└── build/              # Output
```

## Core Library (lib/)

### Dependencies on armortools/base

- `iron_json.c/h` — JSON parsing/encoding
- `iron_lz4.c/h` — LZ4 compression for TileLayer
- `jsmn.h` — JSON tokenizer (libs/jsmn.h)
- `nfd.h/c` — Native file dialogs (via WITH_NFD)

### Data Structures

```c
typedef enum {
    CDB_TID = 0, CDB_TSTRING = 1, CDB_TBOOL = 2, CDB_TINT = 3,
    CDB_TFLOAT = 4, CDB_TENUM = 5, CDB_TREF = 6, CDB_TIMAGE = 7,
    CDB_TLIST = 8, CDB_TCUSTOM = 9, CDB_TFLAGS = 10, CDB_TCOLOR = 11,
    CDB_TLAYER = 12, CDB_TFILE = 13, CDB_TTILEPOS = 14, CDB_TTILELAYER = 15,
    CDB_TDYNAMIC = 16, CDB_TPROPERTIES = 17, CDB_TGRADIENT = 18,
    CDB_TCURVE = 19, CDB_TGUID = 20, CDB_TPOLYMORPH = 21
} CDBColumnType;

typedef enum {
    CDB_OK, CDB_ERR_FILE_NOT_FOUND, CDB_ERR_FILE_WRITE,
    CDB_ERR_JSON_PARSE, CDB_ERR_INVALID_CDB, CDB_ERR_OUT_OF_MEMORY
} CDBError;

// Database -> Sheets -> Columns -> Rows -> Values
struct CDB;
struct CDBSheet;
struct CDBColumn;
struct CDBRow;
```

### Public API

```c
// Lifecycle
CDB *cdb_create(void);
int cdb_load(CDB *db, const char *filepath);
int cdb_save(CDB *db, const char *filepath);
void cdb_close(CDB *db);
CDBError cdb_get_error(CDB *db);

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
int cdb_sheet_add_column(CDBSheet *sheet, CDBColumn *col, int position);
int cdb_sheet_delete_column(CDBSheet *sheet, const char *column_name);
int cdb_sheet_row_count(CDBSheet *sheet);
int cdb_sheet_add_row(CDBSheet *sheet, int position);
int cdb_sheet_delete_row(CDBSheet *sheet, int row_index);
int cdb_sheet_move_row(CDBSheet *sheet, int from, int to);

// Column operations
CDBColumn *cdb_column_create(const char *name, CDBColumnType type, ...);
void cdb_column_destroy(CDBColumn *col);
const char *cdb_column_name(CDBColumn *col);
CDBColumnType cdb_column_type(CDBColumn *col);
const char *cdb_column_type_str(CDBColumn *col);

// Value access
const char *cdb_get_string(CDBSheet *sheet, int row, const char *column);
int cdb_set_string(CDBSheet *sheet, int row, const char *column, const char *value);
int cdb_get_int(CDBSheet *sheet, int row, const char *column);
int cdb_set_int(CDBSheet *sheet, int row, const char *column, int value);
float cdb_get_float(CDBSheet *sheet, int row, const char *column);
int cdb_set_float(CDBSheet *sheet, int row, const char *column, float value);
int cdb_get_bool(CDBSheet *sheet, int row, const char *column);
int cdb_set_bool(CDBSheet *sheet, int row, const char *column, int value);
```

### CDB File Format

JSON with newline-delimited records:
- `sheets[]` — array of sheet definitions
- `columns[]` — per-sheet column definitions (name, typeStr, opt)
- `lines[]` — per-sheet row data (key-value pairs)
- `customTypes[]` — custom type definitions
- `compress` — boolean for LZ4 compression

Column type string format: `"TYPE"` or `"TYPE:extra"` (e.g., `"5:value1,value2"` for TEnum)

## Editor GUI

### Layout

```
+--------------------------------------------------+
| Menu Bar (File | Edit | View | Help)             |
+----------+---------------------------------------+
| Sheet    | Sheet View (Table)                    |
| List     | +-----------------------------------+ |
|          | | Col1 | Col2 | Col3 | ...         | |
| [Sheet1] | |-----|-----|-----|----------------| |
| [Sheet2] | | val | val | val |                | |
| [Sheet3] | | val | val | val |                | |
|          | +-----------------------------------+ |
+----------+---------------------------------------+
```

### Features

- Open/Save CDB files (via NFD)
- Add/Delete/Rename sheets
- Add/Delete/Move/Rename columns
- Column type selection (all 22 types)
- Add/Delete/Move rows
- Edit cell values (context-aware: enum dropdown, color picker, etc.)
- Undo/Redo (command pattern)
- Search/Filter rows

### Value Editors per Type

| Type | Editor |
|------|--------|
| TId | Text input |
| TString | Text input |
| TBool | Checkbox |
| TInt | Integer input |
| TFloat | Float input |
| TEnum | Dropdown |
| TRef | Dropdown (from referenced sheet) |
| TImage | Image picker |
| TFlags | Multi-select checkbox list |
| TColor | Color picker |
| TCustom | Custom editor per type definition |
| TGradient | Gradient editor |
| TCurve | Curve editor |

## Build Integration

In `project.js`, add a new project block:

```javascript
// In armortools/project.js or create castle_c/project.js
let project = new Project("Castle");
project.add_include_dir("castle_c/sources");
project.add_include_dir("base/sources");
project.add_include_dir("base/sources/libs");
project.add_cfiles("castle_c/sources/lib/*.c");
project.add_cfiles("castle_c/sources/editor/*.c");
// Iron libraries
project.add_cfiles("base/sources/iron_json.c");
project.add_cfiles("base/sources/iron_lz4.c");
// NFD
project.add_define("WITH_NFD");
project.add_cfiles("base/sources/libs/nfd.c");
if (platform == "macos") {
    project.add_cfiles("base/sources/libs/nfd.m");
}
project.flatten();
return project;
```

## Implementation Order

1. **Core library (lib/)**
   - cdb.h, cdb.c — Basic structures and API
   - cdb_reader.c — Read CDB JSON files
   - cdb_writer.c — Write CDB JSON files
   - cdb_json.c — JSON parsing/encoding
   - cdb_types.c — Complex types (Gradient, Curve, TileLayer)
   - cdb_validate.c — Type validation and conversion

2. **Editor (editor/)**
   - main.c + editor_app.c — App scaffolding
   - menu.c — Menu bar
   - sheet_view.c — Sheet list and table view
   - column_editor.c — Column operations
   - value_editor.c — Per-type value editors

3. **Integration & Polish**
   - Undo/Redo system
   - Search/Filter
   - Keyboard shortcuts
