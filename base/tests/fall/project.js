
let flags = globalThis.flags;
flags.with_physics = true;

let project = new Project("test");
flags.with_wren = false;
project.add_project("../../");

project.add_cfiles("sources/main.c");
project.add_shaders("shaders/*.kong");
project.add_assets("assets/*", {destination : "data/{name}"});

return project;
