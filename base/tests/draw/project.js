
let flags = globalThis.flags;
flags.with_wren = false;
flags.with_eval = false;

let project = new Project("test");
project.add_project("../../");
project.add_cfiles("main.c");
project.add_assets("assets/*", {destination: "data/{name}"});
return project;
