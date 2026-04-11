// CastleDB Editor - Project Configuration

let flags                 = globalThis.flags;
flags.name                = "CastleEditor";
flags.with_nfd            = true;
flags.with_compress       = true;

let project = new Project(flags.name);
project.add_project("../base");

// Castle C library (minimal - just what we need)
project.add_cfiles("sources/lib/*.c");

// Editor UI (Objective-C++ for Metal/ImGui)
project.add_cfiles("sources/editor/*.mm");

// Custom Dear ImGui sources
project.add_cfiles("sources/editor/imgui/imgui.cpp");
project.add_cfiles("sources/editor/imgui/imgui_draw.cpp");
project.add_cfiles("sources/editor/imgui/imgui_tables.cpp");
project.add_cfiles("sources/editor/imgui/imgui_widgets.cpp");
project.add_cfiles("sources/editor/imgui/backends/imgui_impl_metal.mm");
project.add_cfiles("sources/editor/imgui/backends/imgui_impl_osx.mm");

// No shaders needed for CDB editor
// No assets needed for CDB editor

return project;
