
let flags = globalThis.flags;
flags.with_eval = true;

let project = new Project("MinicPluginTest");
project.add_project("../../");
project.add_cfiles("main.c");
project.add_assets("assets/*.c", {destination: "data/{name}"});
return project;
