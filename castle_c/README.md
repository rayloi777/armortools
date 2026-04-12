# CastleDB C Editor

A pure C implementation of CastleDB structured database editor with Dear ImGui GUI.

## Status

This is a work-in-progress implementation. The core library (`lib/`) is largely complete, but the Dear ImGui GUI requires additional setup.

## Project Structure

```
castle_c/
├── sources/
│   ├── lib/               # Core CDB library (C)
│   │   ├── cdb.h         # Public API
│   │   ├── cdb.c         # Database/Sheet/Column operations
│   │   ├── cdb_reader.c   # JSON reading
│   │   ├── cdb_writer.c   # JSON writing
│   │   ├── cdb_json.c     # Type string parsing
│   │   └── cdb_types.c    # Complex types (Gradient, TileLayer)
│   └── editor/           # Dear ImGui GUI
│       ├── main.c         # Entry point
│       ├── editor_app.c   # App state
│       ├── menu.c         # Menu bar
│       ├── sheet_view.c   # Sheet list/table
│       ├── value_editor.c # Cell editors
│       ├── column_editor.c # Column dialog
│       └── imgui/         # Dear ImGui integration
│           └── imgui.h    # Stub header
└── project.js            # Build configuration
```

## Building

### Prerequisites

1. **Dear ImGui sources** must be added manually:

```bash
cd castle_c/sources/editor/imgui
git clone https://github.com/ocornut/imgui.git
mv imgui/* .
rm -rf imgui
```

Required files:
- `imgui.h`, `imgui.cpp`
- `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`
- `imgui_impl_metal.h`, `imgui_impl_metal.mm` (or other platform backend)
- `imconfig.h` (optional configuration)

2. **Build with armortools**:

```bash
cd armortools
./make castle
```

## Dear ImGui Backends

### macOS (Metal)
Uses `imgui_impl_metal.mm`. Metal backend is partially implemented in `imgui_impl_metal.mm`.

### Other Platforms
For Linux/Windows, add appropriate backends:
- Linux: `imgui_impl_opengl3.cpp` + GLFW
- Windows: `imgui_impl_dx11.cpp` or `imgui_impl_dx12.cpp` or `imgui_impl_vulkan.cpp`

## Architecture

### Core Library (lib/)

The `lib/` directory contains a complete CDB read/write library:

- **cdb.h/c** - Database, Sheet, Column management
- **cdb_reader.c** - Parse CDB JSON files
- **cdb_writer.c** - Write CDB JSON files
- **cdb_json.c** - Type string parsing (TEnum, TFlags, TRef)
- **cdb_types.c** - Complex types (Gradient, TileLayer with LZ4)

### Editor (editor/)

Dear ImGui-based GUI components:
- Menu bar (File, Edit, Sheet, Help)
- Sheet list panel
- Spreadsheet-style table view
- Per-type value editors (string, int, float, bool, enum, color, etc.)

## Dependencies

- armortools/base: iron_json, iron_lz4, iron_file, iron_array, iron_map
- NFD (Native File Dialog)
- Dear ImGui v1.90+

## Testing

The CDB library is tested using a dedicated test suite located in `base/tests/castle_c/`.

```bash
# Build and run tests
cd base/tests/castle_c
make -f Makefile
./CastleCDBTest
```

Expected: `=== 25 tests, 0 failures ===`
