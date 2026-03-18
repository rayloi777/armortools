let flags = globalThis.flags;

let project = new Project("WrenTest");
project.add_project("../../");
project.add_define("WITH_WREN");
project.add_cfiles("main.c");
project.add_cfiles("../../sources/wren/iron_wren.c");
project.add_cfiles("../../sources/wren/wren_iron.c");
project.add_cfiles("../../sources/libs/wren/wren_compiler.c");
project.add_cfiles("../../sources/libs/wren/wren_core.c");
project.add_cfiles("../../sources/libs/wren/wren_debug.c");
project.add_cfiles("../../sources/libs/wren/wren_primitive.c");
project.add_cfiles("../../sources/libs/wren/wren_utils.c");
project.add_cfiles("../../sources/libs/wren/wren_value.c");
project.add_cfiles("../../sources/libs/wren/wren_vm.c");
project.add_cfiles("../../sources/libs/wren/wren_opt_meta.c");
project.add_cfiles("../../sources/libs/wren/wren_opt_random.c");
project.add_include_dir("../../sources/libs/wren/include");

if (fs_exists("assets")) {
	project.add_assets("assets/*.wren", {destination: "data/wren/{name}"});
}

return project;
