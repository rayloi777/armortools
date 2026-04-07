#include "global.h"

// Unity Build - 包含所有源文件
#include "game_engine.c"
#include "core/engine_world.c"
#include "core/runtime_api.c"
#include "core/entity_api.c"
#include "core/component_api.c"
#include "core/system_api.c"
#include "core/query_api.c"
#include "core/game_loop.c"
#include "core/minic_system.c"
#include "core/input.c"
#include "core/prefab.c"
#include "core/prefab_load.c"
#include "core/prefab_save.c"
#include "ecs/ecs_world.c"
#include "ecs/ecs_components.c"
#include "ecs/ecs_dynamic.c"
#include "ecs/ecs_bridge.c"
#include "components/transform.c"
#include "components/mesh_renderer.c"
#include "components/camera.c"

// 2D Systems
#include "core/sprite_api.c"
#include "core/camera2d.c"
#include "ecs/sprite_bridge.c"
#include "ecs/camera_bridge.c"
#include "ecs/render2d_bridge.c"

// 3D Systems
#include "ecs/camera_bridge_3d.c"
#include "ecs/mesh_bridge_3d.c"

// UI Extension
#include "core/ui_ext_api.c"

// Base minic library
#include "../../base/sources/libs/minic.c"
#include "../../base/sources/libs/minic_ext.c"
