#!/bin/bash
# CastleDB C - Standalone Build Script

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
OBJ_DIR="$BUILD_DIR/obj"
FINAL_DIR="$BUILD_DIR/final"

# Create directories
mkdir -p "$OBJ_DIR"
mkdir -p "$FINAL_DIR"

# Compiler settings
CC="clang"
CXX="clang++"
CFLAGS="-c -I$SCRIPT_DIR/sources -I$SCRIPT_DIR/sources/lib -I$SCRIPT_DIR/sources/editor -I$SCRIPT_DIR/sources/editor/imgui -I$SCRIPT_DIR/sources/editor/imgui/backends -mmacosx-version-min=10.15 -O2 -Wall"
CXXFLAGS="$CFLAGS -std=c++17 -fobjc-arc"
LDFLAGS="-framework Metal -framework MetalKit -framework AppKit -framework Foundation -framework GameController -lstdc++ /opt/homebrew/lib/liblz4.a"

echo "=== CastleDB C Build ==="
echo "Build directory: $BUILD_DIR"
echo ""

# Compile CDB library (standalone - no external dependencies)
echo "Compiling CDB library..."
$CC $CFLAGS "$SCRIPT_DIR/sources/lib/jsmn.c" -o "$OBJ_DIR/jsmn.o"
$CC $CFLAGS "$SCRIPT_DIR/sources/lib/cdb.c" -o "$OBJ_DIR/cdb.o"
$CC $CFLAGS "$SCRIPT_DIR/sources/lib/cdb_reader.c" -o "$OBJ_DIR/cdb_reader.o"
$CC $CFLAGS "$SCRIPT_DIR/sources/lib/cdb_writer.c" -o "$OBJ_DIR/cdb_writer.o"
$CC $CFLAGS "$SCRIPT_DIR/sources/lib/cdb_types.c" -o "$OBJ_DIR/cdb_types.o"

# Compile editor (C files)
echo "Compiling editor (C)..."
$CC $CFLAGS "$SCRIPT_DIR/sources/editor/main.c" -o "$OBJ_DIR/main.o"
$CC $CFLAGS "$SCRIPT_DIR/sources/editor/editor_app.c" -o "$OBJ_DIR/editor_app.o"

# Compile editor (Objective-C++ files)
echo "Compiling editor (Objective-C++)..."
$CXX $CXXFLAGS "$SCRIPT_DIR/sources/editor/menu.mm" -o "$OBJ_DIR/menu.o"
$CXX $CXXFLAGS "$SCRIPT_DIR/sources/editor/sheet_view.mm" -o "$OBJ_DIR/sheet_view.o"
$CXX $CXXFLAGS "$SCRIPT_DIR/sources/editor/column_editor.mm" -o "$OBJ_DIR/column_editor.o"
$CXX $CXXFLAGS "$SCRIPT_DIR/sources/editor/value_editor.mm" -o "$OBJ_DIR/value_editor.o"
$CXX $CXXFLAGS "$SCRIPT_DIR/sources/editor/editor_ui.mm" -o "$OBJ_DIR/editor_ui.o"

# Compile Dear ImGui
echo "Compiling Dear ImGui..."
$CXX $CXXFLAGS "$SCRIPT_DIR/sources/editor/imgui/imgui.cpp" -o "$OBJ_DIR/imgui.o"
$CXX $CXXFLAGS "$SCRIPT_DIR/sources/editor/imgui/imgui_draw.cpp" -o "$OBJ_DIR/imgui_draw.o"
$CXX $CXXFLAGS "$SCRIPT_DIR/sources/editor/imgui/imgui_tables.cpp" -o "$OBJ_DIR/imgui_tables.o"
$CXX $CXXFLAGS "$SCRIPT_DIR/sources/editor/imgui/imgui_widgets.cpp" -o "$OBJ_DIR/imgui_widgets.o"

# Compile Dear ImGui backends
echo "Compiling Dear ImGui backends..."
$CXX $CXXFLAGS "$SCRIPT_DIR/sources/editor/imgui/backends/imgui_impl_metal.mm" -o "$OBJ_DIR/imgui_impl_metal.o"
$CXX $CXXFLAGS "$SCRIPT_DIR/sources/editor/imgui/backends/imgui_impl_osx.mm" -o "$OBJ_DIR/imgui_impl_osx.o"

echo ""
echo "=== Linking ==="
# Link all objects
$CXX -o "$FINAL_DIR/CastleEditor" \
    "$OBJ_DIR"/*.o \
    $LDFLAGS

echo "Build complete: $FINAL_DIR/CastleEditor"
