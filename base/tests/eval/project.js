let flags = globalThis.flags;

let project = new Project("EvalTest");
project.add_project("../../");
project.add_define("WITH_EVAL");
project.add_cfiles("main.c");
project.add_cfiles("../../sources/libs/quickjs-amalgam.c");
project.add_define("QJS_BUILD_LIBC");
project.add_assets("assets/*.js", {destination: "data/{name}"});

return project;
