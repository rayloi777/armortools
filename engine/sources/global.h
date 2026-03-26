#pragma once

#include <iron.h>
#include <minic.h>
#include <gc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "game_engine.h"
#include "ecs/ecs_world.h"
#include "ecs/ecs_components.h"
#include "ecs/ecs_dynamic.h"
#include "ecs/ecs_bridge.h"
#include "core/runtime_api.h"
#include "core/entity_api.h"
#include "core/component_api.h"
#include "core/system_api.h"
#include "core/query_api.h"
#include "core/game_loop.h"
#include "core/input.h"
#include "core/prefab.h"
