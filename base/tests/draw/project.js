
let flags = globalThis.flags;

let project = new Project("test");
project.add_project("../../");
project.add_cfiles("sources/main.c");
return project;
