
let flags                = globalThis.flags;
flags.name              = "ArmorUI";
flags.with_physics      = false;
flags.with_d3dcompiler  = false;
flags.with_nfd          = false;
flags.with_compress     = false;
flags.with_image_write  = false;
flags.with_video_write  = false;
flags.with_eval         = false;
flags.with_plugins      = false;
flags.with_kong         = false;
flags.with_raytrace     = false;
flags.idle_sleep        = true;
flags.export_version_info = false;

let project = new Project(flags.name);
project.add_project("../base");
project.add_cfiles("sources/main.c");
project.add_assets("assets/*", {destination: "data/{name}"});
return project;
