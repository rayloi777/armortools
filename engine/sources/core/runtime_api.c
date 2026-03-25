#include "runtime_api.h"
#include "component_api.h"
#include "entity_api.h"
#include "system_api.h"
#include "query_api.h"
#include "game_loop.h"
#include <stdio.h>

void runtime_api_register(void) {
    printf("Registering runtime APIs (stub mode)...\n");
    
    component_api_register();
    entity_api_register();
    system_api_register();
    query_api_register();
    
    printf("Runtime APIs registered\n");
}
