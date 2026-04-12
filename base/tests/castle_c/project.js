let flags = globalThis.flags;

// Build a standalone test executable (no Iron engine needed)
let project = new Project("CastleCDBTest");
project.add_project("../../", { is_application: false });  // Only include base sources, not as app

// Add castle_c library sources - only the minimal ones needed for testing
// jsmn is the JSON parser, cdb_* files are the CDB library
project.add_include_dir("../../../castle_c/sources/lib");
project.add_cfiles("../../../castle_c/sources/lib/jsmn.c");
project.add_cfiles("../../../castle_c/sources/lib/cdb.c");
project.add_cfiles("../../../castle_c/sources/lib/cdb_reader.c");
project.add_cfiles("../../../castle_c/sources/lib/cdb_writer.c");
project.add_cfiles("../../../castle_c/sources/lib/cdb_types.c");
project.add_cfiles("../../../castle_c/sources/lib/cdb_json.c");

// Test entry point
project.add_cfiles("test_cdb.c");

// Stubs for Iron symbols that are linked but not used in the test
project.add_cfiles("iron_stubs.c");

return project;