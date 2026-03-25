
let flags = globalThis.flags;

let project = new Project("IronGame");

project.add_project("../base");

project.add_include_dir("sources");
project.add_include_dir("sources/ecs/flecs");

project.add_cfiles("sources/ecs/flecs/flecs.c");
project.add_cfiles("sources/game_engine.c");
project.add_cfiles("sources/core/runtime_api.c");
project.add_cfiles("sources/core/entity_api.c");
project.add_cfiles("sources/core/component_api.c");
project.add_cfiles("sources/core/system_api.c");
project.add_cfiles("sources/core/query_api.c");
project.add_cfiles("sources/core/game_loop.c");
project.add_cfiles("sources/core/input.c");
project.add_cfiles("sources/core/prefab.c");
project.add_cfiles("sources/core/prefab_load.c");
project.add_cfiles("sources/core/prefab_save.c");
project.add_cfiles("sources/ecs/ecs_world.c");
project.add_cfiles("sources/ecs/ecs_components.c");
project.add_cfiles("sources/ecs/ecs_dynamic.c");
project.add_cfiles("sources/ecs/ecs_bridge.c");
project.add_cfiles("sources/components/transform.c");
project.add_cfiles("sources/components/mesh_renderer.c");
project.add_cfiles("sources/components/camera.c");

project.add_assets("assets/scripts/*.minic", {destination: "data/{name}"});
project.add_assets("assets/prefabs/*.json", {destination: "data/{name}"});

return project;
