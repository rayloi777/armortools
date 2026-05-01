let project = new Project("HLIron");
project.add_project("../base");

// Host and bridge sources
project.add_cfiles("host/main.c");
project.add_cfiles("hdll/sources/iron_bridge.c");

// HashLink include path
project.add_include_dir("/usr/local/include");

// No assets or shaders needed for clear-screen MVP
return project;