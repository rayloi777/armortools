let project = new Project("HLIron");
project.add_project("../base");

// Define LIBHL_EXPORTS so DEFINE_PRIM creates hlp_* registration functions
// (needed for std library primitives to be discoverable via dlsym)
project.add_define("LIBHL_EXPORTS");

// Host source
project.add_cfiles("host/main.c");

// HashLink sources (symlinked from hl_src/)
// Core VM (allocator.c is #included by gc.c, not compiled separately)
project.add_cfiles("hl_src/code.c");
project.add_cfiles("hl_src/gc.c");
project.add_cfiles("hl_src/jit.c");
project.add_cfiles("hl_src/module.c");
project.add_cfiles("hl_src/profile.c");
// Standard library (minimal subset — no external deps)
project.add_cfiles("hl_src/std/array.c");
project.add_cfiles("hl_src/std/buffer.c");
project.add_cfiles("hl_src/std/bytes.c");
project.add_cfiles("hl_src/std/cast.c");
project.add_cfiles("hl_src/std/date.c");
project.add_cfiles("hl_src/std/debug.c");
project.add_cfiles("hl_src/std/error.c");
project.add_cfiles("hl_src/std/file.c");
project.add_cfiles("hl_src/std/fun.c");
project.add_cfiles("hl_src/std/maps.c");
project.add_cfiles("hl_src/std/math.c");
project.add_cfiles("hl_src/std/obj.c");
project.add_cfiles("hl_src/std/random.c");
project.add_cfiles("hl_src/std/string.c");
project.add_cfiles("hl_src/std/track.c");
project.add_cfiles("hl_src/std/sys.c");
project.add_cfiles("hl_src/std/thread.c");
project.add_cfiles("hl_src/std/types.c");
project.add_cfiles("hl_src/std/ucs2.c");

// HashLink include paths
project.add_include_dir("hl_src");
project.add_include_dir("/usr/local/include");

// No assets or shaders needed for clear-screen MVP
return project;
