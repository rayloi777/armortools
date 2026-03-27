let flags = globalThis.flags;
flags.with_gamepad = true;

let project = new Project("IronGame");
project.add_project("../base");
project.add_include_dir("sources");
project.add_include_dir("sources/ecs/flecs");
project.add_cfiles("sources/main.c");
project.add_cfiles("sources/ecs/flecs/flecs.c");
project.add_assets("assets/scripts/*.minic", {destination: "data/{name}"});
project.add_assets("assets/systems/*.minic", {destination: "data/systems/{name}"});
project.add_assets("assets/prefabs/*.json", {destination: "data/{name}"});
project.add_assets("assets/*.png", {destination: "data/{name}"});
project.add_assets("assets/images/*.png", {destination: "data/{name}"});
return project;
